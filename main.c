/*

mMicroSD example, also serial terminal example

This program creates a DOS-like terminal from which you can read
and modify the files on a FAT32-formatted microSD card.

*/


#include "main.h"

void process_command (void);


void welcome_screen (void)
{
    printf ("=====================\r\n");
    printf ("mMicroSD Test Console\r\n");
    printf ("\r\nType 'help' for available commands\r\n");
    printf ("\r\n");
}

#define COMMAND_SIZE 64
char command_buffer[COMMAND_SIZE + 1];
char *command_ptr = command_buffer;

char *command_tokens[(COMMAND_SIZE + 1) / 2];


extern volatile uint32_t Receive_length;
extern __IO uint32_t packet_sent;
extern __IO uint32_t packet_receive;
extern uint8_t Receive_Buffer[64];

bool fatal_error = false;

int main (void)
{
    mInit();
    mBusInit();
    mUSBInit();
    
    mWhiteOFF;
    mRedOFF;
    mGreenON;
    
    for (uint8_t i = 0; i < COMMAND_SIZE + 1; i++)
        command_buffer[i] = '\0';  // initialize command buffer
    
    bool sd_initialized = false;
    
reconnect:
    while (bDeviceState != CONFIGURED);
    
    sd_initialized = m_sd_init();
    
    mWaitms (1000);
    welcome_screen();
    
    if (!sd_initialized)
        printf ("ERROR: Could not initialize microSD card\r\n");
    
    printf ("> ");
    
    for (;;)
    {
        if (bDeviceState != CONFIGURED)
        {
            goto reconnect;
        }
        
        const int rchar = getchar();
        if (rchar < 0)
            continue;
        
        const char c = (char)rchar;
        
        if (c == '\n' || c == '\r')
        {  // received 'enter'
            printf ("\r\n");
            
            if (sd_initialized)
            {
                process_command();
                
                if (fatal_error)
                    sd_initialized = false;
            }
            else
            {
                if (m_sd_init())
                {
                    fatal_error = false;
                    sd_initialized = true;
                    
                    process_command();
                    
                    if (fatal_error)
                        sd_initialized = false;
                }
                else
                {
                    printf ("ERROR: Could not initialize microSD card\r\n");
                }
            }
            
            command_ptr = command_buffer;  // reset the current command length to 0
            
            printf ("> ");
        }
        else if (c == 8 || c == 127)
        {  // received backspace (or delete)
            if (command_ptr > command_buffer)
            {
                command_ptr--;
                
                putchar (27);  // escape
                printf ("[1D");  // move the cursor back 1 character
                putchar (27);
                printf ("[K");  // erase from the cursor position to the end of the line
            }
        }
        else if (c >= 32 && c < 127)
        {  // received a printable character or escape
            if (command_ptr < command_buffer + COMMAND_SIZE)
            {
                *command_ptr = c;
                command_ptr++;
                
                putchar (c);
            }
            else
            {  // out of command space: send bell
                putchar (7);
            }
        }
    }
    
    return 0;
}

uint8_t tokenize_command (void)
{  // split the command buffer into separate words
    uint8_t num_tokens = 0;
    
    char *current_token_start = NULL;
    
    for (char *ptr = command_buffer; ptr != command_ptr; ptr++)
    {
        if (*ptr == ' ' || *ptr == '\t')
        {  // encountered a separator
            if (current_token_start == NULL)
                continue;  // ignore it if we don't have a token start yet
            
            *ptr = '\0';  // change the space to a NULL
            
            if (*(ptr - 1) != '\0')
            {  // if the previous character was not also a separator
                command_tokens[num_tokens] = current_token_start;
                current_token_start = NULL;
                num_tokens++;
            }
        }
        else if (current_token_start == NULL)
        {  // encountered a valid character, and we don't have a token start yet
            current_token_start = ptr;
            
            // special case: don't tokenize data that will be fed to 'write' or 'append'
            if (num_tokens == 2)
            {
                if (strcmp (command_tokens[0], "write") == 0 || strcmp (command_tokens[0], "append") == 0)
                {
                    command_tokens[num_tokens] = ptr;
                    num_tokens++;
                    return num_tokens;
                }
            }
        }
    }
    
    // end of the command string, convert what was left into the final token
    if (current_token_start != NULL)
    {
        *command_ptr = '\0';
        command_tokens[num_tokens] = current_token_start;
        num_tokens++;
    }
    
    return num_tokens;
}

void process_command (void)
{
    uint8_t num_tokens = tokenize_command();
    
    if (num_tokens == 0)  // no commands
        return;
    
    /*
    for (uint8_t i = 0; i < num_tokens; i++)
    {
        m_usb_tx_char ('\t');
        
        for (char *ptr = command_tokens[i]; *ptr != '\0'; ptr++)
            m_usb_tx_char (*ptr);
        
        printf ("\n");
    }
    */
    
    if (strcmp (command_tokens[0], "help") == 0)
        printf ("Commands: ls, cd, print, mkdir, rmdir, write, append\r\n");
    else if (strcmp (command_tokens[0], "ls") == 0)
    {
        if (num_tokens > 1)
            printf ("ls does not take any arguments\r\n");
        else
            ls();
    }
    else if (strcmp (command_tokens[0], "cd") == 0)
    {
        if (num_tokens != 2)
        {
            printf ("cd requires one argument (the destination directory)\r\n");
            return;
        }
        
        if (!m_sd_push (command_tokens[1]))
            printf ("error entering ");
        else
            printf ("now in ");
        
        for (char *c = command_tokens[1]; *c != '\0'; c++)
            putchar (*c);
        printf ("\r\n");
    }
    else if (strcmp (command_tokens[0], "print") == 0)
    {
        if (num_tokens != 2)
        {
            printf ("print requires one argument (the file to print)\r\n");
            return;
        }
        
        printFile (command_tokens[1]);
    }
    else if (strcmp (command_tokens[0], "mkdir") == 0)
    {
        if (num_tokens != 2)
        {
            printf ("mkdir requires one argument (the directory to create)\r\n");
            return;
        }
        
        if (!m_sd_mkdir (command_tokens[1]))
        {
            printf ("error creating directory ");
        }
        else
        {
            printf ("successfully created directory ");
            m_sd_commit();
        }
        
        for (char *c = command_tokens[1]; *c != '\0'; c++)
            putchar (*c);
        printf ("\r\n");
    }
    else if (strcmp (command_tokens[0], "rmdir") == 0)
    {
        if (num_tokens != 2)
        {
            printf ("rmdir requires one argument (the directory to delete)\n");
            return;
        }
        
        if (!m_sd_rmdir (command_tokens[1]))
        {
            printf ("error deleting directory ");
        }
        else
        {
            printf ("successfully deleted directory ");
            m_sd_commit();
        }
        
        for (char *c = command_tokens[1]; *c != '\0'; c++)
            putchar (*c);
        printf ("\r\n");
    }
    else if (strcmp (command_tokens[0], "write") == 0)
    {
        if (num_tokens < 3)
        {
            printf ("write requires a filename, followed by the data to write\r\n");
            return;
        }
        
        writeToFile (command_tokens[1],
                     CREATE_FILE,
                     command_ptr - command_tokens[2],
                     (uint8_t*)command_tokens[2]);
    }
    else if (strcmp (command_tokens[0], "append") == 0)
    {
        if (num_tokens < 3)
        {
            printf ("append requires a filename, followed by the data to write\r\n");
            return;
        }
        
        writeToFile (command_tokens[1],
                     APPEND_FILE,
                     command_ptr - command_tokens[2],
                     (uint8_t*)command_tokens[2]);
    }
    else if (strcmp (command_tokens[0], "edit") == 0)
    {
        if (num_tokens < 2)
        {
            printf ("edit requires a filename\r\n");
            return;
        }
        
        uint8_t fid = 255;
        if (!m_sd_open_file (command_tokens[1], APPEND_FILE, &fid))
        {  // if opening the file in r/w mode failed
            if (m_sd_error_code == ERROR_FAT32_NOT_FOUND)
            {  // if the reason was because the file doesn't exist, try creating it
                m_sd_open_file (command_tokens[1], CREATE_FILE, &fid);
            }
        }
        
        if (fid == 255)
        {  // the file is still not open
            printf ("Couldn't open/create file ");
            for (char *c = command_tokens[1]; *c != '\0'; c++)
                putchar (*c);
            printf ("\r\n");
            return;
        }
        
        edit (fid, command_tokens[1]);
        
        if (!m_sd_close_file (fid))
        {
            printf ("Error closing file!  Error code %d\r\n", m_sd_error_code);
        }
    }
    else if (strcmp (command_tokens[0], "keycode") == 0)
    {  // print the ASCII number of the pressed keys, CTRL-C exits
        keycodes();
    }
    else
    {
        printf ("unknown command\r\n");
    }
}

