/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	onenet.c
	*
	*	作者： 		张继瑞
    *   移植：      STM32F0xx HAL库，OLED替代串口打印
	*
	*	日期： 		2017-05-08
    *   移植日期：  2026-10-13
	*
	*	版本： 		V1.1_F0_HAL_OLED
	*
	*	说明： 		与onenet平台的数据交互接口层，STM32F0 HAL库，OLED屏幕打印日志
	*
	*	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
	*				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
    *               V1.1_F0_HAL：移植到STM32F0xx HAL库，HAL_Delay替代延时
    *               V1.1_F0_HAL_OLED：移除串口UsartPrintf，改用I2C OLED显示日志
	************************************************************
	************************************************************
	************************************************************
**/
//单片机头文件 STM32F0 HAL库
#include "stm32f0xx_hal.h"
#include "OLED.h"
//网络设备
#include "esp8266.h"
//协议文件
#include "onenet.h"
#include "mqttkit.h"
#include "property_json.h"
//算法
#include "base64.h"
#include "hmac_sha1.h"
//硬件驱动(HAL版本)
#include "led.h"
//C库
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PROID			"6x3n4Y0FuX"
#define ACCESS_KEY		"YWRpSTh5V3IzVU1vZHd0Vkxyenl4ajVtQ3dWdTVqVVg="
#define DEVICE_NAME		"D1"

char devid[16];
char key[48];
extern unsigned char esp8266_buf[512];
extern uint8_t ledMode;
extern uint8_t ledState;
extern uint16_t light;
extern float temp;

void OneNET_Publish(const char *topic, const char *msg);

static void OneNET_ReplyPropertySet(const char *request_id, int code, const char *msg)
{
	char topic_buf[64];
	char reply_buf[128];
	
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set_reply", PROID, DEVICE_NAME);
	snprintf(reply_buf, sizeof(reply_buf), "{\"id\":\"%s\",\"code\":%d,\"msg\":\"%s\"}",
			 request_id ? request_id : "0", code, msg ? msg : "");
	
	OneNET_Publish(topic_buf, reply_buf);
}

/*
************************************************************
*	函数名称：	OTA_UrlEncode
*
*	函数功能：	sign需要进行URL编码
*
*	入口参数：	sign：加密结果
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		+			%2B
*				空格		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
/* A SHA1 Base64 signature contains only '+' '/' '=' needing escaping. */
static unsigned char OTA_UrlEncode(char *sign)
{
    char original[29];
    unsigned int i, j = 0;
    if(sign == NULL || strlen(sign) != 28) return 1;
    memcpy(original, sign, sizeof(original));
    for(i = 0; i < 28; i++) {
        if(original[i] == '+' || original[i] == '/' || original[i] == '=') {
            sign[j++] = '%';
            sign[j++] = original[i] == '+' ? '2' : (original[i] == '/' ? '2' : '3');
            sign[j++] = original[i] == '+' ? 'B' : (original[i] == '/' ? 'F' : 'D');
        } else sign[j++] = original[i];
    }
    sign[j] = 0;
    return 0;
}

/*
************************************************************
*	函数名称：	OTA_Authorization
*
*	函数功能：	计算Authorization
*
*	入口参数：	ver：参数组版本号，日期格式，目前仅支持格式"2018-10-31"
*				res：产品id
*				et：过期时间，UTC秒值
*				access_key：访问密钥
*				dev_name：设备名
*				authorization_buf：缓存token的指针
*				authorization_buf_len：缓存区长度(字节)
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		当前仅支持sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[88];								//保存签名的Base64编码结果 和 URL编码结果
	char hmac_sha1_buf[20];							//保存签名
	char access_key_base64[64];						//保存access_key的Base64编码结合
	char string_for_signature[72];					//保存string_for_signature，这个是加密的key
//----------------------------------------------------参数合法性--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------将access_key进行Base64解码----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	if(BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key)) != 0)
        return 2;
	
//----------------------------------------------------计算string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
	
//----------------------------------------------------加密-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, (int)olen,
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
//----------------------------------------------------将加密结果进行Base64编码------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, sizeof(hmac_sha1_buf));
//----------------------------------------------------将Base64编码结果进行URL编码---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	
//----------------------------------------------------计算Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
	
	return 0;
}

//==========================================================
//	函数名称：	OneNET_RegisterDevice
//
//	函数功能：	在产品中注册一个设备
//
//	入口参数：	access_key：访问密钥
//				pro_id：产品ID
//				serial：唯一设备号
//				devid：保存返回的devid
//				key：保存返回的key
//
//	返回参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
_Bool OneNET_RegisterDevice(void)
{
	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//加密的key
	
	send_ptr = malloc(send_len + 240);
	if(send_ptr == NULL)
		return result;
	
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n", "CONNECT"))
		HAL_Delay(500);		//替换原来DelayXms(500)
	
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, NULL,
							authorization_buf, sizeof(authorization_buf), 1);
	
	snprintf(send_ptr, 240 + send_len, "POST /mqtt/v1/devices/reg HTTP/1.1\r\n"
					"Authorization:%s\r\n"
					"Host:ota.heclouds.com\r\n"
					"Content-Type:application/json\r\n"
					"Content-Length:%d\r\n\r\n"
					"{\"name\":\"%s\"}",
	
					authorization_buf, 11 + strlen(DEVICE_NAME), DEVICE_NAME);
	
	ESP8266_SendData((unsigned char *)send_ptr, strlen(send_ptr));
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//等待平台响应
	
	if(data_ptr)
	{
		data_ptr = strstr(data_ptr, "device_id");
	}
	
	if(data_ptr)
	{
		char name[16];
		int pid = 0;
		
		if(sscanf(data_ptr, "device_id\" : \"%[^\"]\",\r\n\"name\" : \"%[^\"]\",\r\n\r\n\"pid\" : %d,\r\n\"key\" : \"%[^\"]\"", devid, name, &pid, key) == 4)
		{
			result = 0;
		}
	}
	
	free(send_ptr);
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");
	
	return result;
}

//==========================================================
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包
	unsigned char *dataPtr;
	
	char authorization_buf[192];
	
	_Bool status = 1;

    OLED_ShowString(2, 1, "TCP CONNECT     ");
    ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");
    if(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n", "OK"))
    {
        OLED_ShowString(2, 1, "TCP FAIL        ");
        return 1;
    }

	
	if(OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY , DEVICE_NAME,
								authorization_buf, sizeof(authorization_buf), 0) != 0)
    {
        OLED_ShowString(2, 1, "TOKEN FAIL      ");
        return 1;
    }
    OLED_ShowString(2, 1, "MQTT LOGIN      ");
	
	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 30, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)	// Keep Alive 30秒, OneNET约45秒判定离线
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//上传平台
		
		dataPtr = ESP8266_GetIPD(1000);									//等待平台响应
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				OLED_ShowString(3, 1, "CONNACK:        ");
                OLED_ShowNum(3, 9, MQTT_UnPacketConnectAck(dataPtr), 1);
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:status = 0;break;
					
					case 1:break;
					case 2:break;
					case 3:break;
					case 4:break;
					case 5:break;
					
					default:break;
				}
			}
		}
		
        if(dataPtr == NULL)
            OLED_ShowString(3, 1, "NO CONNACK      ");
		MQTT_DeleteBuffer(&mqttPacket);								//删包
	}
	
    if(status == 0)
        OLED_ShowString(2, 1, "ONENET ONLINE   ");
	return status;
	
}

extern  uint16_t light;
extern float temp;
// 外部全局变量，你需要在main或者其他地方定义
extern uint8_t ledState;
extern uint8_t ledMode;
#define MODE_AUTO 0
#define MODE_MANUAL 1


unsigned char OneNet_FillBuf(char *buf)
{
	int tenths;
	char text[48];
	float scaled = temp * 10.0f;
	float fraction;
	tenths = (int)scaled;
	fraction = scaled - tenths;
	if(fraction > 0.5f || (fraction == 0.5f && (tenths & 1))) tenths++;
	if(fraction < -0.5f || (fraction == -0.5f && (tenths & 1))) tenths--;
	
	memset(text, 0, sizeof(text));
	
	strcpy(buf, "{\"id\":\"123\",\"params\":{");
	
	memset(text, 0, sizeof(text));
	snprintf(text, sizeof(text), "\"temp\":{\"value\":%s%d.%d},",
	         tenths < 0 ? "-" : "", abs(tenths / 10), abs(tenths % 10));
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"light\":{\"value\":%d},",light);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"led\":{\"value\":%s},",led_info.LED_Status?"true":"false");
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"mode\":{\"value\":%s}",ledMode==MODE_MANUAL?"true":"false");
	strcat(buf, text);
	
	strcat(buf, "}}");
	
	return strlen(buf);
}

//==========================================================
//	函数名称：	OneNet_SendData
//
//	函数功能：	上传数据到平台
//
//	入口参数：	type：发送数据的格式
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//协议包
	
	char buf[256];
	
	short body_len = 0, i = 0;
	
	memset(buf, 0, sizeof(buf));
	
	body_len = OneNet_FillBuf(buf);																	//获取当前需要发送的数据流的总长度
	
	if(body_len)
	{
		
		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//封包
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];
			
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//上传数据到平台
			
			MQTT_DeleteBuffer(&mqttPacket);															//删包
		}
	}
	
}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}
}

//==========================================================
//	函数名称：	OneNET_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	char topic_buf[56];
	const char *topic = topic_buf;
	
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);
	
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}
}

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	static jsmntok_t tokens[32];
	jsmn_parser parser;
	char request_id[32] = "0";
	int token_count, params_json, led_json, mode_json, id_json;
	int id_length;
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	unsigned char type = 0;
	short result = 0;
	int led_value = 0;
	int mode_value = 0;
	int reply_code = 200;
	const char *reply_msg = "success";
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:																//接收的Publish消息
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				
				jsmn_init(&parser);
				token_count = jsmn_parse(&parser, req_payload, req_len, tokens,
				                         sizeof(tokens) / sizeof(tokens[0]));
				if(token_count < 1 || tokens[0].type != JSMN_OBJECT)
				{
					MQTT_FreeBuffer(cmdid_topic);
					MQTT_FreeBuffer(req_payload);
					result = -1;
					break;
				}
				params_json = OneNET_ObjectItem(req_payload, tokens, token_count, 0, "params");
				
				/* 如果params不存在，直接从根节点取 */
				if(params_json < 0)
					params_json = 0;
				
				id_json = OneNET_ObjectItem(req_payload, tokens, token_count, 0, "id");
				if(id_json >= 0 && tokens[id_json].type == JSMN_STRING)
				{
					id_length = tokens[id_json].end - tokens[id_json].start;
					if(id_length >= 0 && id_length < sizeof(request_id))
					{
						memcpy(request_id, req_payload + tokens[id_json].start, id_length);
						request_id[id_length] = '\0';
					}
				}
				
				/* mode命令：始终有效，任意模式下均可切换自动/手动 */
				mode_json = OneNET_GetBoolNode(req_payload, tokens, token_count, params_json, "mode");
				if(mode_json >= 0)
				{
					if(OneNET_ParseBoolValue(req_payload, &tokens[mode_json], &mode_value))
					{
						ledMode = mode_value ? MODE_MANUAL : MODE_AUTO;
					}
					else
					{
						reply_code = 400;
						reply_msg = "invalid mode value";
					}
				}
				
				/* led命令：仅在手动模式下有效，自动模式下忽略 */
				led_json = OneNET_GetBoolNode(req_payload, tokens, token_count, params_json, "led");
				if(led_json >= 0)
				{
					/* 收到led命令但没有同时下发mode时，自动切到手动模式 */
					if(mode_json < 0)
						ledMode = MODE_MANUAL;
					
					if(ledMode == MODE_MANUAL)
					{
						if(OneNET_ParseBoolValue(req_payload, &tokens[led_json], &led_value) && led_value)
						{
							LED_Set(LED_ON);
							ledState = 1;
						}
						else
						{
							if(!OneNET_ParseBoolValue(req_payload, &tokens[led_json], &led_value))
							{
								reply_code = 400;
								reply_msg = "invalid led value";
							}
							else
							{
								LED_Set(LED_OFF);
								ledState = 0;
							}
						}
					}
				}
				
				OneNET_ReplyPropertySet(request_id, reply_code, reply_msg);
			}
			
			break;	/* 关键：防止穿透到下面的 PUBACK */
			
		case MQTT_PKT_PUBACK:														//发送Publish消息，平台回复的Ack
		
			MQTT_UnPacketPublishAck(cmd);
			
		break;
		
		case MQTT_PKT_SUBACK:																//发送Subscribe消息的Ack
		
			MQTT_UnPacketSubscribe(cmd);
		
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//清空缓存
	
	if(result == -1)
		return;
		
	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}
}
