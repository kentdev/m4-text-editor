#ifndef PAGECACHE_H
#define PAGECACHE_H

#define COLS_PER_LINE   80
#define LINES_PER_PAGE  10
#define PAGE_BYTES      (COLS_PER_LINE * LINES_PER_PAGE)

typedef struct Page
{
    char data[PAGE_BYTES];
    uint16_t num_bytes;
    bool modified;
    int16_t modified_bytes;
    
    uint32_t file_offset;
} Page;

extern Page *prevPage;
extern Page *currentPage;
extern Page *editOverflowPage;
//extern Page *nextPage;

bool init_pages (uint8_t file_id);

bool insert_char (

bool page_down (void);
bool page_up (void);

bool save_pages (void);


#endif

