#include "main.h"

void keycodes (void)
{
    for (;;)
    {
        const int intch = getchar();
        if (intch < 0)
            continue;
        
        const char c = (char)intch;
        
        if (c == 'C' - 64)  // ctrl-c
        {
            printf ("\r\n");
            return;
        }
        else
            printf ("%d\r\n", c);
    }
}

