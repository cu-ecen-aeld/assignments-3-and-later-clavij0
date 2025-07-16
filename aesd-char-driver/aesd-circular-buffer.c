/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/kernel.h>
#else
#include <string.h>
#endif
//#include <stdio.h>


#include "aesd-circular-buffer.h"


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
    //Set starting point of read=buffer->out_offs
    uint8_t out_offset = buffer->out_offs;
    //printk("out_offset %u", out_offset);
    //printk("char_offset %lu",char_offset);
    //Set bounderi of the circle buffer AESDCHAR_MAX.....
    //printk("aesd_circular_buffer_find buffer->entry_count %u",  buffer->entry_count);
    for (int i =0 ; i < buffer->entry_count; i++){
        //printk("buffer %s e i++ %d\n",buffer->entry[out_offset].buffptr, i);

        if(char_offset < buffer->entry[out_offset].size){
            *entry_offset_byte_rtn = char_offset;
            //printk("unread_bytes 3 %lu", entry_offset_byte_rtn); //control
            return &buffer->entry[out_offset];

        }else{
            //With this opt off_Set I can track the char offset current position and move throught it buffer
            char_offset = char_offset - buffer->entry[out_offset].size;
             //printk("Char_offset INSIDE %lu\n",char_offset);

            //With this modulus I can move the out_offset index circular buffer
            out_offset = (out_offset+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        }
    }
        //printf("Entradas");
    return NULL;
}

/**
* Adds entry @param add_ent ry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char *aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
   //Check if in_offs and out_offs are in the same location to change the status of buffer->full

   const char* retval = NULL;
   
   
   //printk("aesd_circular_buffer %d",buffer->entry_count);
   if(buffer->full){
    retval = buffer->entry[buffer->out_offs].buffptr; // Save the pointer to the overwritten data, it is returned TO aesd_write to check the NULL pointer
    buffer->out_offs = (buffer->out_offs+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    //printk("Entre a mi destino buffer %d", buffer->full);
   }
   
   //Add add new entry to the circular buffer untill full "in_offs=rear=write"
   buffer->entry[buffer->in_offs]= *add_entry;
    
   if((buffer->in_offs +1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED == buffer->out_offs){
        //printk("Bandera activa de buffer->full");
        buffer->full=true;
        //printk("Antes Bandera activa buffer->full %u", buffer->entry_count);
        buffer->entry_count++;
        //printk("Después Bandera activa buffer->full %u", buffer->entry_count);
        //printk("Bandera activa de buffer->full");
   }
   //printk("aesd_circular_buffer Buffer->stated %d",buffer->full);

//   Update position of in_offs
   buffer->in_offs = (buffer->in_offs + 1 ) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
   //printk("aesd_circular_buffer_add buffer->entry_count %u",  buffer->entry_count);
//    if (!buffer->full) {
//             buffer->entry_count++;
//             if(buffer->entry_count >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
//                 printk("FULL buffer %u",  buffer->entry_count);
//                 buffer->entry_count = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
//                 buffer->full = true;
//             }
//    }

   return retval;

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
