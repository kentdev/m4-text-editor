/*

Very simple text editor.

Unfortunately, there's no good way to read arrow key presses over serial,
so we have to resort to more arcane methods of moving the cursor around the
screen.

In this case, we'll try going with a vi-ish approach (where you move around
in one mode, press A to enter edit mode, and ESC to go back into the movement
mode), only with WASD to move the cursor instead of the arrow keys.

*/

#include "mGeneral.h"
#include "m_microsd.h"
#include "pageCache.h"
#include <stdbool.h>
#include <stdio.h>

#define INVALID ((uint32_t)0xffffffff)

char tempbuffer[COLS_PER_LINE];

uint32_t FILE_SIZE;

enum EditState
{
    NAVIGATE,
    INSERT
} editState = NAVIGATE;


char prevPrintedChar = 0;
inline void printChar (const char c)
{
    // putchar printable characters and use background colors for the rest
    if (c >= 32 && c <= 126)
        putchar (c);
    else if ( (c == '\r' && prevPrintedChar != '\n') ||
              (c == '\n' && prevPrintedChar != '\r') )
    {  // make sure we don't double-print a CR-LF
        printf ("\033[44m \033[0m");
    }
    else if (c == '\t')
        printf ("\033[43m \033[0m");
    else
        printf ("\033[41m \033[0m");
    
    prevPrintedChar = c;
}


inline void position_cursor (uint8_t line_index, uint8_t column)
{
    printf ("\033[%u;%uH", 3 + line_index, column);
}

void draw_state_line (void)
{
    switch (editState)
    {
        case NAVIGATE:
            // "WASD" is green
            printf ("NAV mode: use \033[32mWASD\033[0m to move around the document, CTRL-P to insert\r\n");
            break;
        case INSERT:
            printf ("INSERT mode: type to insert characters, CTRL-P to navigate\r\n");
            break;
    }
}

void draw_error_line (const char *errorText)
{
    printf ("\033[41;30%s\0330m\r\n", errorText);
}

void draw_info_line (void)
{
    // blue for line breaks, yellow for tabs, red for other unprintables
    printf ("\033[44;30mLine Break\033[0m \033[43;30mTab\033[0m \033[41;30mUnknown Character\033[0m\r\n");
}

void redraw_screen (const char *name)
{
    // save the cursor position
    printf ("\033[s");
    
    // clear the screen
    printf ("\033[2J");
    
    // move the cursor to the top-left corner
    printf ("\033[H");
    
    // highlighted menu on the top
    printf ("\033[47;30m");  // background color white, foreground color black
    printf ("EDITING ");
    uint8_t i = 0;
    for (i = 0; i < 12 && name[i] != ' ' && name[i] != '\0'; i++)
        putchar (name[i]);
    
    // fill the line to right-justify this next bit
    const char *msg = "CTRL-C to exit";
    const uint8_t fillerChars = COLS_PER_LINE - 8 - strlen(msg) - strlen(name);
    for (uint8_t q = 0; q < fillerChars; q++)
        putchar (' ');
    printf ("%s\r\n", msg);
    
    // restore normal text colors
    printf ("\033[0m");
    
    draw_state_line();
    draw_info_line();
    draw_error_line ("");
    
    for (uint8_t q = 0; q < COLS_PER_LINE; q++)
        putchar ('_');
    
    printf ("\033[0m");  // reset text color
    
    // restore the cursor position
    printf ("\033[u");
}

bool save (uint8_t file_id)
{
    // TODO: SAVE!
}

void edit (uint8_t file_id, const char *name)
{
    uint16_t cursor_pos = 0;
    uint8_t  cursor_column = 0;
    uint8_t  cursor_window_line = 0;
    
    //init_line_window();
    
    // get the size of the file
    if (!m_sd_seek (file_id, FILE_END_POS) ||
        !m_sd_get_seek_pos (file_id, &FILE_SIZE))
    {
        printf ("Error getting file size, can't edit (error %d)\r\n> ", m_sd_error_code);
        return;
    }
    
    editState = NAVIGATE;
    
    // move the cursor to the beginning of the first line
    position_cursor (4, 1);
    
    // draw the initial view
    redraw_screen (name);
    
    for (;;)
    {
        const int intch = getchar();
        if (intch < 0)
            continue;
        
        const char c = (char)intch;
        
        if (c == 'C' - 64)  // ctrl-c
        {  // clear screen, return to prompt
            putchar (27);
            printf ("[2J");
            
            // move the cursor to the top-left corner
            putchar (27);
            printf ("[H");
            
            // print a "done" message and return to the command prompt
            printf ("Finished editing ");
            for (int i = 0; i < 12 && name[i] != ' ' && name[i] != '\0'; i++)
                putchar (name[i]);
            printf ("\r\n");
            return;
        }
        else if (c == 'P' - 64) // ctrl-p
        {
            if (editState == NAVIGATE)
                editState = INSERT;
            else
                editState = NAVIGATE;
            
            redraw_screen (name);
            continue;
        }
        
        if (editState == NAVIGATE)
        {
            /*
            if (c == 'w' || c == 'W')
            {  // move cursor up
                
                // if the cursor is below the median, or there are
                // no more previous lines to load, move the cursor up
                if (cursor_window_line > NUM_LINES / 2)
                {
                    if (cursor_window_line > 0)
                    {
                        cursor_window_line--;
                        
                        // move the cursor up one line
                        putchar (27);
                        printf ("[A");
                        
                        // if the new line is invalid, initialize it
                        if (orderedLines[cursor_window_line]->line_number == INVALID)
                        {
                            // TODO
                        }
                    }
                }
                else
                {  // keep the cursor on the same line, but move the whole window up
                    line_window_up();
                    
                    draw_screen (name, "MOVE");
                }
                
                cursor_column = 0;
            }
            else if (c == 's' || c == 'S')
            {  // move cursor down
                
            }
            else if (c == 'a' || c == 'A')
            {  // move cursor left
                
            }
            else if (c == 'd' || c == 'D')
            {  // move cursor right
                
            }
            */
        }
        else if (editState == INSERT)
        {  // in edit mode
            if (c == 8 || c == 127) // backspace or delete
            {
                // need to handle backspacing
            }
            else
            {
                if (insert_char (c, cursor_pos))
                {
                    
                }
                else
                {
                    
                }
                
                printChar (c);
            }
        }
    }
}

