#include "PageCache.h"

#define INVALID_FID    0xff
#define INVALID_OFFSET 0xffffffff

/*
typedef struct Page
{
    char data[PAGE_BYTES];
    uint16_t num_bytes;
    bool modified;
    
    uint32_t file_offset;
} Page;
*/

#define NUM_PAGES 3

uint8_t active_fid = INVALID_FID;
uint32_t active_fid_disk_size = 0;
//Page page[4];
Page page[NUM_PAGES];

Page saveTemp;

Page *prevPage;
Page *currentPage;
Page *editOverflowPage;
//Page *nextPage;

static bool fill_buffer (Page *buffer)
{
    if (buffer->num_bytes >= PAGE_BYTES)
        return true;  // already full
    
    // seek to where the buffer's data runs out
    if (!m_sd_seek (active_fid, buffer->file_offset + buffer->num_bytes))
        return false;
    
    // fill the buffer line by line so we don't
    // hit any size limits in the transfer code
    while (buffer->num_bytes < PAGE_BYTES &&
           buffer->file_offset + buffer->num_bytes < active_fid_disk_size)
    {
        uint32_t increment = COL_PER_LINE;
        
        // avoid reading past the end of the file
        if (active_fid_disk_size < buffer->file_offset + buffer->num_bytes + increment)
            increment = active_fid_disk_size -
                           (buffer->file_offset + (uint32_t)buffer->num_bytes);
       
        // avoid reading more than the buffer can hold
        if (buffer->num_bytes + increment > PAGE_BYTES)
            increment = PAGE_BYTES - buffer->num_bytes;
        
        if (!m_sd_read_file (file_id, increment, &(buffer->data[buffer->num_bytes]) ))
            return false;
    }
}

bool init_pages (uint8_t file_id)
{
    save_pages();
    
    for (uint8_t i = 0; i < 4; i++)
    {
        page[i].num_bytes = 0;
        page[i].modified = false;
        page[i].file_offset = INVALID_OFFSET;
    }
    
    // get the file's size on disk
    if (!m_sd_seek (file_id, FILE_END_POS))
        return false;
    
    if (!m_sd_get_seek_pos (file_id, &active_fid_disk_size))
    {
        m_sd_seek (file_id, 0);
        return false;
    }
    
    // seek to the beginning of the file
    if (!m_sd_seek (file_id, 0))
        return false;
    
    // set up the page pointers
    prevPage = &page[0];
    currentPage = &page[1];
    editOverflowPage = &page[2];
    
    // initialize the "previous page" buffer
    prevPage->num_bytes = 0;
    prevPage->file_offset = 0;
    prevPage->modified = false;
    
    // initialize and read into the "current page" buffer
    currentPage->num_bytes = 0;
    currentPage->file_offset = 0;
    currentPage->modified = false;
    if (!fill_buffer (currentPage))
        return false;
    
    // initialize the "edit overflow" buffer
    editOverflowPage->num_bytes = 0;
    editOverflowPage->file_offset = current_page->num_bytes;
    editOverflowPage->modified = false;
    
    active_fid = file_id;
    return true;
}

bool insert_char (char c, int pos)
{
    if (currentPage->num_bytes < PAGE_BYTES)
    {  // there's room for another character in our current buffer
        // shift everything beyond pos forward
        for (int i = PAGE_BYTES - 1; i > pos; i--)
            currentPage->data[i] = currentPage->data[i - 1];
        
        currentPage->num_bytes++;
        editOverflowPage->file_offset++;
        
        currentPage->data[i] = c;
        return true;
    }
    else if (editOverflowPage->num_bytes < PAGE_BYTES)
    {  // there's room in the overflow buffer
        // shift everything in the overflow buffer forward
        for (int i = PAGE_BYTES - 1; i > 0; i--)
            editOverflowPage->data[i] = editOverflowPage->data[i - 1];
        
        // copy the last byte of the current page into the first byte of overflow
        editOverflowPage->data[0] = currentPage->data[PAGE_BYTES - 1];
        editOverflowPage->num_bytes++;
        
        // shift everything beyond pos forward in the current buffer
        for (int i = PAGE_BYTES - 1; i > pos; i--)
            currentPage->data[i] = currentPage->data[i - 1];
        
        currentPage->data[i] = c;
        return true;
    }
    else
    {  // both the current and edit overflow buffers are full
        // we need to save first, to clear out the overflow buffer
        if (!save_pages())
            return false;
        
        // copy the last byte of the current page into the first byte of overflow
        editOverflowPage->data[0] = currentPage->data[PAGE_BYTES - 1];
        editOverflowPage->num_bytes++;
        
        // shift everything beyond pos forward in the current buffer
        for (int i = PAGE_BYTES - 1; i > pos; i--)
            currentPage->data[i] = currentPage->data[i - 1];
        
        currentPage->data[i] = c;
        return true;
    }
}

bool backspace_char (int pos)
{
    
}

bool delete_char (int pos)
{
    
}

bool page_down (void)
{
    // save the document if the prev page buffer has been modified
    if (prevPage->modified && prevPage->file_offset != INVALID_OFFSET)
        save_pages();
    
    // dump the prev page buffer
    prevPage->num_bytes = 0;
    prevPage->modified = false;
    prevPage->file_offset = INVALID_OFFSET;
    
    // shift the buffers
    Page *formerPrevPage = prevPage;
    prevPage = currentPage;
    currentPage = editOverflowPage;
    editOverflowPage = formerPrevPage;
    
    // fill the current page to its limit
    fill_buffer (currentPage);
    
    // initialize the new edit overflow page
    editOverflowPage->file_offset = currentPage->file_offset + currentPage->num_bytes;
    editOverflowPage->num_bytes = 0;
    editOverflowPage->modified = false;
}

bool page_up (void)
{
    // save the edit overflow page if it's been modified
    if (editOverflowPage->modified && editOverflowPage->file_offset != INVALID_OFFSET)
        save_pages();
    
    // shift the buffers
    Page *swap = editOverflowPage;
    editOverflowPage = currentPage;
    currentPage = prevPage;
    prevPage = swap;
    
    // initialize the new prev page
    prevPage->file_offset = (currentPage->file_offset >= PAGE_BYTES) ?
                            currentPage->file_offset - PAGE_BYTES :
                            INVALID_OFFSET;
    prevPage->num_bytes = 0;
    prevPage->modified = false;
    
    if (prevPage->file_offset != INVALID_OFFSET)
    {
        if (!fill_buffer (prevPage))
            return false;
    }
    
    // initialize the new edit overflow page
    editOverflowPage->file_offset = currentPage->file_offset + currentPage->num_bytes;
    editOverflowPage->num_bytes = 0;
    editOverflowPage->modified = false;
}

bool save_pages (void)
{
    if (active_fid == INVALID_FID)
        return false;
    
    // seek to the start of the first modified buffer and write it
    if (prevPage->modified && prevPage->file_offset != INVALID_OFFSET)
    {
        if (!m_sd_seek (active_fid, prevPage->file_offset))
            return false;
        if (!m_sd_write_file (active_fid, prevPage->modified_num_bytes, prevPage->data))
            return false;
    }
    
    // continue writing modified buffers until we reach the edit overflow buffer
    if (currentPage->modified && currentPage->file_offset != INVALID_OFFSET)
    {
        if (!m_sd_seek (active_fid, currentPage->file_offset))
            return false;
        if (!m_sd_write_file (active_fid, currentPage->modified_num_bytes, currentPage->data))
            return false;
    }
    
    if (editOverflowPage->modified && editOverflowPage->file_offset != INVALID_OFFSET)
    {
        // before writing the edit overflow buffer, read from the file to see
        // what it will be overwriting on disk
        
        Page *tempRead = saveTemp;
        Page *tempWrite = editOverflowPage;
        
        bool finished = false;
        while (!finished)
        {
            // set the parameters of the read buffer to begin reading
            // where the write buffer will overwrite
            tempRead->file_offset = tempWrite->file_offset;
            tempRead->num_bytes = 0;
            
            if (!fill_buffer (tempRead))
                return false;
            
            // If, after filling the temp page buffer, the buffer still
            // has room left, then we've reached the end of the file
            if (tempRead->num_bytes != PAGE_BYTES)
                finished = true;
            
            // write the previously-buffered data to its new position
            if (!m_sd_seek (active_fid, tempWrite->file_offset))
                return false;
            if (!m_sd_write_file (active_fid, tempWrite->num_bytes, tempWrite->data))
                return false;
            
            const uint32_t endOfWrite = tempWrite->file_offset + tempWrite->num_bytes;
            
            // swap the read and write buffers
            Page *swap = tempRead;
            tempRead = tempWrite;
            tempWrite = swap;
            
            // the next write will start at the end of the last write
            tempWrite->file_offset = endOfWrite;
        }
    }
    
    prevPage->modified = false;
    currentPage->modified = false;
    
    // clear the edit overflow page
    editOverflowPage->num_bytes = 0;
    editOverflowPage->modified = false;
}

