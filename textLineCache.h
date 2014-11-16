#ifndef TEXTLINECACHE_H
#define TEXTLINECACHE_H

#include <stdbool.h>

#define CACHED_LINES  10

// screen resolution: 80 characters wide
#define COLS_PER_LINE 80


typedef struct
{
    char data[COLS];
    uint8_t num_chars;
    uint32_t line_number;
    bool modified;
    uint32_t file_offset;
    
    uint8_t last_used_index;  // for determining which line to remove from cache first
} linebuffer;

#define INVALID_LINE ((uint32_t)(0xffffffff))
#define INVALID_OFFSET INVALID_LINE


void init_line_cache();

// returns false if the line doesn't exist (data will be set to NULL)
bool get_line (uint8_t file_id, uint32_t line_number, char[COLS] data);

// if the line number exceeds the current number of lines in the file, lines
// will be added to the file until it reaches the line number
bool set_line (uint8_t file_id, uint32_t line_number, char[COLS] data);

// save all cached changes to the SD card
void flush_cached_lines();


#endif

