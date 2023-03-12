/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @author Modified by - Khyati Satta
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"
#include <stdio.h>

/**
 * Helper function to advance the in_offs or out_offs pointers
 * @param pointer: The 8-bit pointer to increment for read and write operations
 * @return The incremented pointer. The pointer is wrapped around to the front of the circular buffer if the pointer has reached the end of the 
 *         buffer
 */
uint8_t move_pointer(uint8_t cb_ptr)
{
    // If the pointer is at the last spot in the circular buffer
    if (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED  == (cb_ptr + 1)){
        return 0;
    }
    // Increment the pointer and return the updated value
    else{
        return (cb_ptr + 1);
    }
}

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */

   // Error check 1: If the circular buffer is empty
   if ((buffer->in_offs == buffer->out_offs) && (!buffer->full)){
    return NULL;
   }

   // Local variable to store the resultant bytes travelled in the cirular buffer
   size_t res_size_buffer = 0;
   uint8_t max_writes_buffer = 0;

   // Calculate the total number of bytes in the circular buffer
   while(max_writes_buffer < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
    res_size_buffer += buffer->entry[max_writes_buffer].size;
    max_writes_buffer++;
   }

   // Error check 2: If the requested offset is greater than the actual number of bytes in the buffer
   if (res_size_buffer < char_offset){
    return NULL;
   }

    // Temporary variable to hold the current outoffs value
    // First place to start reading from
    uint8_t rptr = buffer->out_offs;

    // Variable to hold the bytes in the circular buffer at the beginning of each iteration and at the end as well
    // The second variable is to hold the number of bytes till the previous iteration to calculate the entry_offset_byte_rtn value
    size_t curr_cb_count = 0 , prev_cb_count = 0;
    

    for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
        curr_cb_count += buffer->entry[rptr].size;
        // If the requested offset fits in the range of the entry's size
        if (curr_cb_count > char_offset){
            *entry_offset_byte_rtn = char_offset - prev_cb_count;
            return &buffer->entry[rptr];
        }
        // Advance the rptr
        rptr = move_pointer(rptr);
        // Store the previous value in another variable
        prev_cb_count = curr_cb_count;
    }
    // If the value was not found
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */

   // Check for full condition before writing to the buffer; if full, increment the read pointer
   if (buffer->full == true) {
    buffer->out_offs = move_pointer(buffer->out_offs);
   }

    // Write the entry (data and the number of bytes) to the buffer
    buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
    buffer->entry[buffer->in_offs].size = add_entry->size;

    //Increment the write pointer
    buffer->in_offs = move_pointer(buffer->in_offs);

    // Check if the buffer is full; if yes, set the full flag
    if (buffer->in_offs == buffer->out_offs){
        buffer->full = true;
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
