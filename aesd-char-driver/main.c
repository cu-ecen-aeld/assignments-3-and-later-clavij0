/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include <linux/uaccess.h>	/* copy_*_user */

#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("clavij0"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev *aesd_device;
    int newl_counter;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev; /*Device Information*/
    PDEBUG("open");

    /**
     * TODO: handle open
     */
    //-------------//
    
    dev = container_of(inode->i_cdev,struct aesd_dev, cdev);
    filp->private_data = dev; /*For other methods */
    //-------------//
    return 0; 
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    //const char *read_ptr;
    size_t entry_offset;
    size_t unread_bytes;
    size_t read_size;
    size_t bytes_to_copy;

    struct aesd_dev *dev = filp->private_data;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    if(mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    PDEBUG("EOF1");

    struct aesd_buffer_entry *entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->cir_buff, *f_pos, &entry_offset);

   	mutex_unlock(&dev->lock);

    if (entry != NULL){
		// unread_bytes = entry->size - entry_offset;
        // printk(KERN_INFO "unread_bytes: %zu\n", unread_bytes);
		// read_size = (unread_bytes > count) ? count : unread_bytes;
        // printk(KERN_INFO "entry->size: %zu\n", entry->size);
        // printk(KERN_INFO "entry_offset: %zu\n", entry_offset);

        bytes_to_copy = min(entry->size - entry_offset, count);

        //printk(KERN_INFO "bytes_to_copy: %zu\n", bytes_to_copy);
        //printk(KERN_INFO "count: %zu\n", count);


		//PDEBUG("Reading message %.*s of size %zu", bytes_to_copy, entry->buffptr + entry_offset, bytes_to_copy);
		if (copy_to_user(buf, entry->buffptr + entry_offset, bytes_to_copy)){
			return -EINTR;
		}
        // *f_pos += bytes_to_copy; update the pointer position for the next reading
		*f_pos += bytes_to_copy;
		retval = bytes_to_copy;
	}else{
        PDEBUG("NO DATA TO READ");
    }


    //retval = count;
   // out:
   //     mutex_unlock(&dev->lock);
        return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    //char *buff;
  
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = filp->private_data;
    //char *p;
    //const char *new_entry;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     * size_t count = Specifies the number of bytes to write.
     * loff_t of_pos = "The current position in the file." is a pointer to a “long offset type” object that indicates the file position the user is accessing. 
     *                  The return value is a “signed size type”; its use is discussed late
     */
    //---------------//

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    PDEBUG("dev->buffer_entry.size %zu",dev->buffer_entry.size );
    // Allocate a kernel buffer to store the data
    //dev->buffer_entry.buffptr=kmalloc(count*sizeof(char *),GFP_KERNEL);

	dev->buffer_entry.buffptr = krealloc(dev->buffer_entry.buffptr,dev->buffer_entry.size + count,GFP_KERNEL);


    if(!dev->buffer_entry.buffptr){
        PDEBUG("No longer needed");
        kfree(dev->buffer_entry.buffptr);
        dev->buffer_entry.buffptr = NULL;
        dev->buffer_entry.size = 0;
       // goto out;
    }
 

    if (copy_from_user(dev->buffer_entry.buffptr + dev->buffer_entry.size,buf,count)){
        PDEBUG("Failed allocating ");
        kfree(dev->buffer_entry.buffptr); //Free dev->char if copy fails.
        return -EFAULT; //return Error if copy fails
    }
    
    //Update dev->buffer_entry.size ya que una vez tengamos una parte del texto recibido como "write" teemos que aumentarle a size ese tamaño de texto hasta que llegue el /n , cuando enviemos el 
    // echo "5" > dev/aesdchar nuestro pasará directamente a la función (strchr(dev->buffer_entry.buffptr,'\n') 
    dev->buffer_entry.size += count;
    PDEBUG("OUT \n dev->buffer_entry.size %zu",dev->buffer_entry.size );
    PDEBUG("OUT \n BEFORE add entry %.*s",dev->buffer_entry.size, dev->buffer_entry.buffptr);

    if (strchr(dev->buffer_entry.buffptr,'\n') != NULL){
    //if ((dev->buffer_entry.size) == '\n'){
        //*p = '\0';
        newl_counter++;
        PDEBUG("Newline character detected");
        
        const char *delete_item = aesd_circular_buffer_add_entry(&dev->cir_buff,&dev->buffer_entry);
        PDEBUG("nEWLINE COUNTER %d\n",newl_counter);
        PDEBUG("Added entry  %.*s",dev->buffer_entry.size, dev->buffer_entry.buffptr);
        //aesd_circular_buffer_add_entry(dev->cir_buff,dev->tmp_entry);
        if (delete_item != NULL){
            //char temp[dev->buffer_entry.size+1];
			PDEBUG("Deleted entry %.*s",sizeof(delete_item),delete_item);
            PDEBUG("Data Deleted size: %zu", dev->buffer_entry.size);
			kfree(delete_item);
		}	
        //once we detect the /n we reset the value of buffptr and size to get the next text.
        kfree(dev->buffer_entry.buffptr);
        //dev->buffer_entry.buffptr = NULL;
        //dev->buffer_entry.size = 0;
        

    }else{
        PDEBUG("Partial data remains uncommitted, size: %zu", dev->buffer_entry.size);
    }
        
    
    //*f_pos +=count;
    
    retval = count;
    //mutex_unlock(&dev->lock);
    //--------------//
    
        mutex_unlock(&dev->lock);
        return retval;
}


struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add  (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device->cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}

int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    //struct aesd_dev *aesd_device;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,"aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }

    /**
     * TODO: initialize the AESD specific portion of the device
     * Here Initialize the code and locking primitive
     */
    //----------------//
    aesd_device = kmalloc(sizeof(struct aesd_dev), GFP_KERNEL); /*We need to allocate the aesd_dev struct*/
    if (!aesd_device){
        result = -ENOMEM;
        goto fail;
    }
    memset(aesd_device,0,1*sizeof(struct aesd_dev)); /*Set the memory allocation*/


    mutex_init(&aesd_device->lock);

    //----------------//    

    result = aesd_setup_cdev(aesd_device); 

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return 0;

    fail:
        aesd_cleanup_module();
        return result;

}





module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
