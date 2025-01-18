/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#define AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED 10
#include "aesd-circular-buffer.h"

struct aesd_dev
{
    /**
     * TODO: Add structure(s) and locks needed to complete assignment requirements
     */
    size_t buffer;           /* The current array size*/
    unsigned long size;   /* Amount of data stored here*/
    char *charbuff;        /* Character*/
    // char charbuff[BUFFER_SIZE];
    // size_t read_offset = 0;
    // size_t write_offset = 0;
    // size_t buffer_size = 0;
    
    struct aesd_buffer_entry buffer_entry;
    struct aesd_circular_buffer cir_buff; 
    
    struct mutex lock;    /* Struct Mutex*/
    struct cdev cdev;     /* Char device structure*/
};

#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
