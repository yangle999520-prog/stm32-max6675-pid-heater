#ifndef ONENET_PROPERTY_JSON_H
#define ONENET_PROPERTY_JSON_H

#include <string.h>

#define JSMN_STATIC
#define JSMN_STRICT
#include "jsmn.h"

static int OneNET_TokenEquals(const char *json, const jsmntok_t *token, const char *text)
{
	size_t length = strlen(text);
	return token->end - token->start == (int)length &&
	       memcmp(json + token->start, text, length) == 0;
}

static int OneNET_NextToken(const jsmntok_t *tokens, int count, int index)
{
	int end = tokens[index].end;
	index++;
	while(index < count && tokens[index].start < end)
		index++;
	return index;
}

static int OneNET_ObjectItem(const char *json, const jsmntok_t *tokens,
				   int count, int object, const char *key)
{
	int i, value;
	if(object < 0 || tokens[object].type != JSMN_OBJECT)
		return -1;
	for(i = object + 1; i < count && tokens[i].start < tokens[object].end; )
	{
		value = i + 1;
		if(value >= count || tokens[i].type != JSMN_STRING)
			return -1;
		if(OneNET_TokenEquals(json, &tokens[i], key))
			return value;
		i = OneNET_NextToken(tokens, count, value);
	}
	return -1;
}

static int OneNET_GetBoolNode(const char *json, const jsmntok_t *tokens,
				    int count, int object, const char *key)
{
	int node = OneNET_ObjectItem(json, tokens, count, object, key);
	int value;
	if(node >= 0 && tokens[node].type == JSMN_OBJECT)
	{
		value = OneNET_ObjectItem(json, tokens, count, node, "value");
		if(value >= 0)
			return value;
	}
	return node;
}

static int OneNET_ParseBoolValue(const char *json, const jsmntok_t *token, int *value)
{
	int nonzero = 0;
	const char *p = json + token->start;
	const char *end = json + token->end;
	if(token->type == JSMN_STRING || token->type == JSMN_PRIMITIVE)
	{
		if(OneNET_TokenEquals(json, token, "true") ||
		   (token->type == JSMN_STRING && OneNET_TokenEquals(json, token, "1")))
		{
			*value = 1;
			return 1;
		}
		if(OneNET_TokenEquals(json, token, "false") ||
		   (token->type == JSMN_STRING && OneNET_TokenEquals(json, token, "0")))
		{
			*value = 0;
			return 1;
		}
	}
	if(token->type != JSMN_PRIMITIVE)
		return 0;
	if(p < end && *p == '-') p++;
	if(p == end) return 0;
	if(*p == '0') p++;
	else if(*p >= '1' && *p <= '9')
	{
		do { nonzero |= (*p++ != '0'); } while(p < end && *p >= '0' && *p <= '9');
	}
	else return 0;
	if(p < end && *p == '.')
	{
		p++;
		if(p == end || *p < '0' || *p > '9') return 0;
		do { nonzero |= (*p++ != '0'); } while(p < end && *p >= '0' && *p <= '9');
	}
	if(p < end && (*p == 'e' || *p == 'E'))
	{
		p++;
		if(p < end && (*p == '+' || *p == '-')) p++;
		if(p == end || *p < '0' || *p > '9') return 0;
		do { p++; } while(p < end && *p >= '0' && *p <= '9');
	}
	if(p != end) return 0;
	*value = nonzero;
	return 1;
}

#endif
