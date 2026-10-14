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
    item = OneNET_GetBoolNode(json, tokens, count, params, key);
    assert(item >= 0);
    assert(OneNET_ParseBoolValue(json, &tokens[item], &value));
    assert(value == expected);
}

int main(void)
{
    const char *invalid = "{\"params\":{\"led\":\"maybe\"}}";
    const char *many = "{\"a\":0,\"b\":0,\"c\":0,\"d\":0,\"e\":0,\"f\":0,\"g\":0,\"h\":0,\"i\":0,\"j\":0,\"k\":0,\"l\":0,\"m\":0,\"n\":0,\"o\":0,\"p\":0,\"q\":0}";
    jsmn_parser parser;
    jsmntok_t tokens[32];
    int count, params, item, value = -1;

    check_value("{\"id\":\"42\",\"params\":{\"mode\":{\"value\":true},\"led\":{\"value\":false}}}", "mode", 1);
    check_value("{\"id\":\"42\",\"params\":{\"mode\":{\"value\":true},\"led\":{\"value\":false}}}", "led", 0);
    check_value("{\"mode\":\"1\",\"led\":0}", "mode", 1);
    check_value("{\"mode\":\"1\",\"led\":0}", "led", 0);
    check_value("{\"params\":{\"mode\":\"false\",\"led\":\"true\"}}", "mode", 0);
    check_value("{\"params\":{\"mode\":\"false\",\"led\":\"true\"}}", "led", 1);
    check_value("{\"params\":{\"led\":-0.0e5}}", "led", 0);
    check_value("{\"params\":{\"led\":-2.5e-3}}", "led", 1);

    jsmn_init(&parser);
    count = jsmn_parse(&parser, invalid, strlen(invalid), tokens, 32);
    params = OneNET_ObjectItem(invalid, tokens, count, 0, "params");
    item = OneNET_GetBoolNode(invalid, tokens, count, params, "led");
    assert(item >= 0 && !OneNET_ParseBoolValue(invalid, &tokens[item], &value));

    jsmn_init(&parser);
    assert(jsmn_parse(&parser, many, strlen(many), tokens, 32) == JSMN_ERROR_NOMEM);
    return 0;
}
