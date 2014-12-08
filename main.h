#ifndef MAIN_H
#define MAIN_H

#include "mGeneral.h"
#include "mBus.h"
#include "mUSB.h"
#include "m_microsd.h"
#include <stdbool.h>
#include <stdio.h>

// fatal_error indicates whether there was an error so
// bad that we should reset the SD card
extern bool fatal_error;

// ls.c:
void ls (void);  // lists directory entries

// printFile.c:
void printFile (const char *fileName);  // prints the contents of a file

// write.c:
void writeToFile  (const char *fileName,
                   open_option writeMode,
                   uint32_t dataLength,
                   uint8_t *data);  // write or append text to a file

// edit.c:
void edit (uint8_t file_id, const char *name);  // open the text editor for this file

// keycodes.c:
void keycodes (void);  // print the ASCII code of the pressed key

#endif

