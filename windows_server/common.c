#include "common.h"
#include <stdio.h>

int parse_message(const char *buffer, parsed_message_t *msg)
{
    if (!buffer || !msg) return -1;

    // Expecting format: "K,<code>,<value>\n" or "M,<code>,<value>\n"
    char type;
    int code, value;
    if (sscanf(buffer, "%c,%d,%d", &type, &code, &value) != 3) {
        return -1; // Parsing error
    }

    if (type != 'K' && type != 'M') {
        return -1; // Invalid type
    }

    msg->type = type;
    msg->code = code;
    msg->value = value;

    return 0; // Success
}
