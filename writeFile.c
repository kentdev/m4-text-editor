#include "main.h"

void writeToFile (const char *fileName,
                  open_option writeMode,
                  uint32_t dataLength,
                  uint8_t *data)
{
    if (writeMode != CREATE_FILE && writeMode != APPEND_FILE)
    {
        printf ("Invalid mode given to writeToFile\r\n");
        return;
    }
    
    uint8_t fid;
    if (!m_sd_open_file (fileName, writeMode, &fid))
    {
        printf ("error creating ");
        for (const char *c = fileName; *c != '\0'; c++)
            putchar (*c);
        printf ("\r\n");
        return;
    }
    else if (!m_sd_write_file (fid, dataLength, data))
    {
        printf ("error writing to ");
        for (const char *c = fileName; *c != '\0'; c++)
            putchar (*c);
        printf ("\r\n");
    }
    
    m_sd_close_file (fid);
    m_sd_commit();
}

