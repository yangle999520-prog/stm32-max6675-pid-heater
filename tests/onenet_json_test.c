#include <assert.h>
#include <string.h>

#include "../onenet/property_json.h"

static void check_value(const char *json, const char *key, int expected)
{
    jsmn_parser parser;
    jsmntok_t tokens[32];
    int count, params, item, value = -1;

    jsmn_init(&parser);
    count = jsmn_parse(&parser, json, strlen(json), tokens, 32);
    assert(count > 0 && tokens[0].type == JSMN_OBJECT);
    params = OneNET_ObjectItem(json, tokens, count, 0, "params");
    if (params < 0) params = 0;
    item = OneNET_GetValueNode(json, tokens, count, params, key);
    assert(item >= 0);
    assert(OneNET_ParseBoolValue(json, &tokens[item], &value));
    assert(value == expected);
}

int main(void)
{
    const char *invalid = "{\"params\":{\"mode\":\"maybe\"}}";
    const char *many = "{\"a\":0,\"b\":0,\"c\":0,\"d\":0,\"e\":0,\"f\":0,\"g\":0,\"h\":0,\"i\":0,\"j\":0,\"k\":0,\"l\":0,\"m\":0,\"n\":0,\"o\":0,\"p\":0,\"q\":0}";
    jsmn_parser parser;
    jsmntok_t tokens[32];
    int count, params, item, value = -1;
    float temperature = -1.0f;
    const char *setpoint = "{\"params\":{\"set_watertemp\":{\"value\":40.25}}}";

    check_value("{\"id\":\"42\",\"params\":{\"mode\":{\"value\":true}}}", "mode", 1);
    check_value("{\"mode\":\"1\"}", "mode", 1);
    check_value("{\"params\":{\"mode\":\"false\"}}", "mode", 0);
    check_value("{\"params\":{\"mode\":-0.0e5}}", "mode", 0);
    check_value("{\"params\":{\"mode\":-2.5e-3}}", "mode", 1);

    jsmn_init(&parser);
    count = jsmn_parse(&parser, invalid, strlen(invalid), tokens, 32);
    params = OneNET_ObjectItem(invalid, tokens, count, 0, "params");
    item = OneNET_GetValueNode(invalid, tokens, count, params, "mode");
    assert(item >= 0 && !OneNET_ParseBoolValue(invalid, &tokens[item], &value));

    jsmn_init(&parser);
    assert(jsmn_parse(&parser, many, strlen(many), tokens, 32) == JSMN_ERROR_NOMEM);

    jsmn_init(&parser);
    count = jsmn_parse(&parser, setpoint, strlen(setpoint), tokens, 32);
    assert(count > 0);
    params = OneNET_ObjectItem(setpoint, tokens, count, 0, "params");
    item = OneNET_GetValueNode(setpoint, tokens, count, params, "set_watertemp");
    assert(item >= 0 && OneNET_ParseTemperatureValue(setpoint, &tokens[item], &temperature));
    assert(temperature == 40.25f);
    {
        const char *limit = "{\"set_watertemp\":1023.75}";
        jsmn_init(&parser);
        count = jsmn_parse(&parser, limit, strlen(limit), tokens, 32);
        assert(count > 0);
        item = OneNET_GetValueNode(limit, tokens, count, 0, "set_watertemp");
        assert(item >= 0 && OneNET_ParseTemperatureValue(limit, &tokens[item], &temperature));
        assert(temperature == 1023.75f);
    }
    {
        const char *invalid_values[] = {
            "{\"set_watertemp\":-1}", "{\"set_watertemp\":1024}",
            "{\"set_watertemp\":true}", "{\"set_watertemp\":\"40\"}",
            "{\"set_watertemp\":40e1}", "{\"set_watertemp\":1.2.3}"
        };
        unsigned int i;
        for(i = 0; i < sizeof(invalid_values) / sizeof(invalid_values[0]); ++i)
        {
            jsmn_init(&parser);
            count = jsmn_parse(&parser, invalid_values[i], strlen(invalid_values[i]), tokens, 32);
            if(count > 0)
            {
                item = OneNET_GetValueNode(invalid_values[i], tokens, count, 0, "set_watertemp");
                assert(item >= 0 && !OneNET_ParseTemperatureValue(invalid_values[i], &tokens[item], &temperature));
            }
        }
    }
    return 0;
}
