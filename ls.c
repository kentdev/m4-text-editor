#include "main.h"

void ls (void)
{
    uint32_t size;
    bool directory;
    char name[13];
    
    bool result = m_sd_get_dir_entry_first (name, &size, &directory);
    
    while (result)
    {
        if (directory)
            printf ("[DIR] ");
        else
            printf ("      ");
        
        uint8_t i = 0;
        for (char *c = name; *c != '\0'; c++, i++)
            putchar (*c);
        
        for (; i < 14; i++)
            putchar (' ');
        
        if (!directory)
            printf ("%lu", size);
        
        printf ("\r\n");
        
        result = m_sd_get_dir_entry_next (name, &size, &directory);
        
        if (m_sd_error_code != ERROR_NONE)
        {
            fatal_error = true;
            return;
        }
    }
}

