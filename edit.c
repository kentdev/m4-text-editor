/*

Very simple text editor.

Unfortunately, there's no good way to read arrow key presses over serial,
so we have to resort to more arcane methods of moving the cursor around the
screen.

In this case, we'll try going with a vi-ish approach (where you move around
in one mode, press A to enter edit mode, and ESC to go back into the movement
mode), only with WASD to move the cursor instead of the arrow keys.

*/

enum linepos
{
    TITLE_LINE = 1,
    MODE_LINE,
    INFO_LINE,
    STATUS_LINE,
    ERROR_LINE,
    SEPARATOR_LINE,
    BLANK_LINE,
    PAGE_START_LINE
};

#include "mGeneral.h"
#include "m_microsd.h"
#include "pageCache.h"
#include <stdbool.h>
#include <stdio.h>

#define INVALID ((uint32_t)0xffffffff)

uint32_t FILE_SIZE;

uint8_t cursor_row = 0;
uint8_t cursor_col = 0;
uint16_t cursor_page_pos = 0;

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
    printf ("\033[%u;%uH", line_index, column);
}

void print_current_page (void)
{
    if (!currentPage)
        return;
    
    position_cursor (PAGE_START_LINE, 0);
    
    int i = 0;
    for (int y = 0; y < LINES_PER_PAGE; y++)
    {
        for (int x = 0; x < COLS_PER_LINE; x++)
        {
            printChar (currentPage->data[i]);
            i++;
            
            if (i >= currentPage->num_bytes)
                return;
        }
        
        printf ("\033[%dD", COLS_PER_LINE);  // move back to the beginning of the column
        printf ("\033[B");  // move down one line
    }
        
}

void draw_mode_line (void)
{
    position_cursor (MODE_LINE, 1);
    
    switch (editState)
    {
        case NAVIGATE:
            // "WASD" is green
            printf ("NAV mode: use \033[32mWASD\033[0m to move around the document, CTRL-P to insert");
            break;
        case INSERT:
            printf ("INSERT mode: type to insert characters, CTRL-P to navigate");
            break;
    }
}

void draw_error_line (const char *errorText)
{
    position_cursor (ERROR_LINE, 1);
    printf ("\033[41;30m%s\033[0m", errorText);
}

void draw_info_line (void)
{
    position_cursor (INFO_LINE, 1);
    
    // blue for line breaks, yellow for tabs, red for other unprintables
    printf ("\033[44;30mLine Break\033[0m \033[43;30mTab\033[0m \033[41;30mUnknown Character\033[0m");
}

void draw_status_line (void)
{
    position_cursor (STATUS_LINE, 1);
    printf ("Cursor position: %d (row %d, col %d)                    \r\n", cursor_page_pos, cursor_row, cursor_col);
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
    
    draw_mode_line();
    draw_info_line();
    draw_status_line();
    draw_error_line ("");
    
    position_cursor (SEPARATOR_LINE, 1);
    for (uint8_t q = 0; q < COLS_PER_LINE; q++)
        putchar ('_');
    
    print_current_page();
    
    printf ("\033[0m");  // make sure the text color is the default
    
    // restore the cursor position
    printf ("\033[u");
}

bool save (uint8_t file_id)
{
    // TODO: SAVE!
}

void edit (uint8_t file_id, const char *name)
{
    // get the size of the file
    if (!m_sd_seek (file_id, FILE_END_POS) ||
        !m_sd_get_seek_pos (file_id, &FILE_SIZE))
    {
        printf ("Error getting file size, can't edit (error %d)\r\n", m_sd_error_code);
        return;
    }
    
    editState = NAVIGATE;
    
    // load the initial data from the document
    if (!init_pages (file_id))
    {
        printf ("Error reading file (error %d)\r\n", m_sd_error_code);
        return;
    }
    
    position_cursor (PAGE_START_LINE, 1);
    
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
            {
                editState = INSERT;
                
                if (cursor_page_pos > currentPage->num_bytes)
                {
                    cursor_page_pos = currentPage->num_bytes - 1;
                    cursor_row = cursor_page_pos / COLS_PER_LINE;
                    cursor_col = cursor_page_pos % COLS_PER_LINE;
                    position_cursor (PAGE_START_LINE + cursor_row, cursor_col + 1);
                }
            }
            else
                editState = NAVIGATE;
            
            redraw_screen (name);
            continue;
        }
        
        if (editState == NAVIGATE)
        {
            if (c == 'w' || c == 'W')
            {  // move cursor up
                if (cursor_row > 0)
                {
                    cursor_row--;
                    printf ("\033[A");
                }
                else if (!is_first_page())
                {  // move up a page and set the cursor to the bottom of the new page
                    if (!page_up())
                    {
                        printf ("\033[2J\033[HError reading file while scrolling up (error %d)\r\n", m_sd_error_code);
                        return;
                    }
                    cursor_row = LINES_PER_PAGE - 1;
                    printf ("\033[%dB", LINES_PER_PAGE - 1);
                }
            }
            else if (c == 's' || c == 'S')
            {  // move cursor down
                if (cursor_row < LINES_PER_PAGE - 1)
                {
                    cursor_row++;
                    printf ("\033[B");
                }
                else if (!is_last_page())
                {  // move down a page and set the cursor to the top of the new page
                    if (!page_down())
                    {
                        printf ("\033[2J\033[HError reading file while scrolling down (error %d)\r\n", m_sd_error_code);
                        return;
                    }
                    cursor_row = 0;
                    printf ("\033[%dA", LINES_PER_PAGE - 1);
                }
            }
            else if (c == 'a' || c == 'A')
            {  // move cursor left
                if (cursor_col > 0)
                {
                    cursor_col--;
                    printf ("\033[D");
                }
                else if (cursor_row > 0)
                {  // move to the rightmost position of the previous row
                    cursor_col = COLS_PER_LINE - 1;
                    cursor_row--;
                    printf ("\033[A\033[%dC", COLS_PER_LINE - 1);
                }
                else if (!is_first_page())
                {  // we were on the first column of the first line, move up a page
                    if (!page_up())
                    {
                        printf ("\033[2J\033[HError reading file while scrolling up (error %d)\r\n", m_sd_error_code);
                        return;
                    }
                    cursor_row = LINES_PER_PAGE - 1;
                    cursor_col = COLS_PER_LINE - 1;
                    printf ("\033[%dB\033[%dC", LINES_PER_PAGE - 1, COLS_PER_LINE - 1);
                }
            }
            else if (c == 'd' || c == 'D')
            {  // move cursor right
                if (cursor_col < COLS_PER_LINE - 1)
                {
                    cursor_col++;
                    printf ("\033[C");
                }
                else if (cursor_row < LINES_PER_PAGE - 1)
                {  // if we were on the last column, move down a row
                    cursor_col = 0;
                    cursor_row++;
                    printf ("\033[%dD\033[B", COLS_PER_LINE - 1);
                }
                else if (!is_last_page())
                {  // we were on the last column of the last line, move down a page
                    if (!page_down())
                    {
                        printf ("\033[2J\033[HError reading file while scrolling down (error %d)\r\n", m_sd_error_code);
                        return;
                    }
                    cursor_row = 0;
                    cursor_col = 0;
                    printf ("\033[%dA\033%dD", LINES_PER_PAGE - 1, COLS_PER_LINE - 1);
                }
            }
            
            // update our offset in the page
            cursor_page_pos = cursor_row * COLS_PER_LINE + cursor_col;
            
            printf ("\033[s");  // save the cursor position
            draw_status_line();
            printf ("\033[u");  // restore the cursor position
        }
        else if (editState == INSERT)
        {  // in edit mode
            if (c == 127) // backspace
            {
                backspace_char (cursor_page_pos);
                
                print_current_page();
                
                if (cursor_page_pos > 0)
                    cursor_page_pos--;
            }
            else
            {
                if (insert_char (c, cursor_page_pos))
                {
                    printChar (c);
                    cursor_page_pos++;
                    
                    if (cursor_page_pos >= PAGE_BYTES)
                        cursor_page_pos = 0;
                }
                else
                {
                    draw_error_line ("Error when inserting character!");
                }
            }
        }
    }
}

