#ifndef COMMON_H
#define COMMON_H

// Shared constants
#define BUFFER_SIZE 200
#define DEFAULT_PORT 50037

// Message parsing structures
typedef struct {
    char type;          // 'K' or 'M'
    int code;           // Key/mouse code
    int value;          // Action value
} parsed_message_t;

// Function declarations
int parse_message(const char* buffer, parsed_message_t* msg);

#endif // COMMON_H