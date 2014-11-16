#include "main.h"

void printFile (const char *fileName)
{
    uint32_t size;
    uint8_t fid;
    
    if (!m_sd_get_size (fileName, &size) || !m_sd_open_file (fileName, READ_FILE, &fid))
    {
        printf ("error opening ");
        for (const char *c = fileName; *c != '\0'; c++)
            putchar (*c);
        printf ("\r\n");
    }
    else
    {
        while (size > 0)
        {
            uint8_t buffer[64];
            uint8_t length_to_read = (size > 64) ? 64 : size;
            
            if (!m_sd_read_file (fid, length_to_read, buffer))
            {
                printf ("<error while reading ");
                for (const char *c = fileName; *c != '\0'; c++)
                    putchar (*c);
                printf (">\r\n");
            }
            else
            {
                for (uint8_t i = 0; i < length_to_read; i++)
                    putchar (buffer[i]);
            }
            
            size -= length_to_read;
        }
        printf ("\r\n");
    }
    
    m_sd_close_file (fid);
}

