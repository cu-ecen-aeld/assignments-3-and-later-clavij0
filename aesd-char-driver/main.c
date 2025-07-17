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
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("clavij0"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev *aesd_device;

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
    loff_t current_pos = filp->f_pos;
    PDEBUG ("Current Post %lld\n",current_pos);
    struct aesd_dev *dev = filp->private_data;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    if(mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    PDEBUG("EOF1");

    struct aesd_buffer_entry *entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->cir_buff, *f_pos, &entry_offset);

   	//mutex_unlock(&dev->lock);
    if (entry != NULL){
		// unread_bytes = entry->size - entry_offset;
        // printk(KERN_INFO "unread_bytes: %zu\n", unread_bytes);
		// read_size = (unread_bytes > count) ? count : unread_bytes;
        // printk(KERN_INFO "entry->size: %zu\n", entry->size);
        // printk(KERN_INFO "entry_offset: %zu\n", entry_offset);

        if(!entry->buffptr){
            PDEBUG("entry->buffptr is NULL");
            return -EFAULT;
        }
        bytes_to_copy = min(entry->size - entry_offset, count);
        
        PDEBUG ("Current 2 Post %lld, entry_offset %zu \n",current_pos, entry_offset);
        printk(KERN_INFO "bytes_to_copy: %zu\n", bytes_to_copy);
        printk(KERN_INFO "count: %zu\n", count);


		PDEBUG("Reading message %.*s of size %zu", bytes_to_copy, entry->buffptr + entry_offset, bytes_to_copy);
		if (copy_to_user(buf, entry->buffptr + entry_offset, bytes_to_copy)){
            mutex_unlock(&dev->lock);
			return -EINTR;
		}
        // *f_pos += bytes_to_copy; update the pointer position for the next reading
		*f_pos += bytes_to_copy;
		retval = bytes_to_copy;
        mutex_unlock(&dev->lock);
	}else{
        PDEBUG("NO DATA TO READ");
        //mutex_unlock(&dev->lock);
    }


    //retval = count;
   // out:
    mutex_unlock(&dev->lock);
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
    //dev->cir_buff.entry_count = 1;
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

	// dev->buffer_entry.buffptr = krealloc(dev->buffer_entry.buffptr,dev->buffer_entry.size + count,GFP_KERNEL);


    // if(!dev->buffer_entry.buffptr){
    //     PDEBUG("No longer needed");
    //     kfree(dev->buffer_entry.buffptr);
    //     dev->buffer_entry.buffptr = NULL;
    //     dev->buffer_entry.size = 0;
    //     goto out;
    // }
    
    char *tmp = krealloc(dev->buffer_entry.buffptr,dev->buffer_entry.size + count,GFP_KERNEL);
    if(!tmp){
        PDEBUG("No longer needed");
        kfree(dev->buffer_entry.buffptr);
        dev->buffer_entry.buffptr = NULL;
        dev->buffer_entry.size = 0;
        retval = -ENOMEM;
        goto out;
    }
    dev->buffer_entry.buffptr = tmp;

    if (copy_from_user(dev->buffer_entry.buffptr + dev->buffer_entry.size,buf,count)){
        PDEBUG("Failed allocating ");
        kfree(dev->buffer_entry.buffptr); //Free dev->char if copy fails.
        return -EFAULT; //return Error if copy fails
    }
    
    //Update dev->buffer_entry.size ya que una vez tengamos una parte del texto recibido como "write" teemos que aumentarle a size ese tamaño de texto hasta que llegue el /n , cuando enviemos el 
    // echo "5" > dev/aesdchar nuestro pasará directamente a la función (strchr(dev->buffer_entry.buffptr,'\n') 
    dev->buffer_entry.size += count;
    //PDEBUG("OUT  dev->buffer_entry.size %zu",dev->buffer_entry.size );
    //PDEBUG("OUT  BEFORE add entry %.*s",dev->buffer_entry.size, dev->buffer_entry.buffptr);
    
    size_t size = dev->buffer_entry.size;
    if (size > 0 && dev->buffer_entry.buffptr[size - 1] == '\n') {
        PDEBUG("Newline detected without adding null terminator");
    }
    /*BE AWARE: This chunk of code allows me to be sure that at the end of each echo we have '\0' null-terminated,
      and avoid error when is not sent the '\n'.
      Issue: The driver might be detecting \n in the write operation, even when it doesn't exist
      , due to uninitialized memory or incorrect handling of strchr.
      ==> Ensure that the buffer_entry.buffptr is null-terminated before calling strchr, as this 
      function depends on a null-terminated string. Otherwise, it might misinterpret memory contents 
      and lead to unexpected newline detections.

    */
    dev->buffer_entry.buffptr[dev->buffer_entry.size] = '\0';
    PDEBUG("Buffer content before strchr: %.*s", (int)dev->buffer_entry.size, dev->buffer_entry.buffptr);

    if (strchr(dev->buffer_entry.buffptr,'\n') != NULL){
        PDEBUG("Newline character detected");
        
        const char *remove_strchr = aesd_circular_buffer_add_entry(&dev->cir_buff,&dev->buffer_entry);
        //PDEBUG("nEWLINE COUNTER %d\n",newl_counter);
        //PDEBUG("Added entry  %.*s",dev->buffer_entry.size, dev->buffer_entry.buffptr);
        //aesd_circular_buffer_add_entry(dev->cir_buff,dev->tmp_entry);

        // Assignment 9 track the buffer
        // ========== ENTRY COUNT UPDATE START ========== //
        if (!dev->cir_buff.full) {
            dev->cir_buff.entry_count++;
            PDEBUG("DENTRO if(!dev->cir_buff.entry_count) %u",  dev->cir_buff.entry_count);
            if(dev->cir_buff.entry_count >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
                PDEBUG("FULL buffer %u",  dev->cir_buff.entry_count);
                dev->cir_buff.entry_count = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
                dev->cir_buff.full = true;
            }
         }
        PDEBUG("dev->cir_buff.entry_count %u",  dev->cir_buff.entry_count);
        
        // ========== ENTRY COUNT UPDATE END ========== //

        if (remove_strchr  != NULL){
            
            PDEBUG("Remove entry : %.*s",dev->buffer_entry.size,remove_strchr );
            PDEBUG("Data Removed size : %zu", dev->buffer_entry.size);
            
			kfree(remove_strchr);
		}	
        //once we detect the /n we reset the value of buffptr and size to get the next text.
        //kfree(dev->buffer_entry.buffptr);
        dev->buffer_entry.buffptr = NULL;
        dev->buffer_entry.size = 0;

    }else{
        PDEBUG("Partial data remains uncommitted, size: %zu", dev->buffer_entry.size);
    }
        
    
    //*f_pos +=count;
    
    retval = count;
    //mutex_unlock(&dev->lock);
    //--------------//
    out:
        mutex_unlock(&dev->lock);
        return retval;
}

static long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){

    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seek_cmd_paramts;
    struct aesd_buffer_entry *entry; // Create a local copy to know and validate the offset of the command
    int retval=0;
    size_t new_pos = 0; //Calcula la nueva posición
    PDEBUG("Dentro de aesd_ioctl\n");
    switch (cmd)
    {
    case AESDCHAR_IOCSEEKTO:
        /* code */
                          //&seek_cmd the address of the struct we are gonna copy the arguments
                          // (struct seek_cmd *)arg Source address in user space the arguments recieved from the userspace, and must be cast with *arg since are "unsined long arg"
                          // sizeof(seek_cmd_params, size of the struct to be copied.
        PDEBUG("Dentro del CASE aesd_ioctl\n");
        if (copy_from_user(&seek_cmd_paramts,(struct seek_cmd *)arg,sizeof(seek_cmd_paramts)))
            return -EFAULT;
        
        mutex_lock(&dev->lock);

        /**
         * @brief 
         * Check the write_cmd is in the boundeirs of the already commands writed if it is not send to goto
         * I'm assuming the cir_buffer is not always full with 10 -> "AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED=10" so thats the reason to create
         * the  entry_count
         */
        // 1st Validate write_cmd is in the cir_buff AESDMAX_BUFFER < 10
        if(seek_cmd_paramts.write_cmd >= dev->cir_buff.entry_count ){
            PDEBUG("EL valor de cmd es más grande que el cir_buff");
            retval = -EINVAL;
            goto out;
        }
        //printf("dev->cir_buff.entry_count = dev->cir_buff.entry_count  %u = %lu\n", seek_cmd_paramts.write_cmd ,dev->cir_buff.entry_count );

        // Get the buffer entry locally to check if offset exist inside the entry
        entry = &dev->cir_buff.entry[(dev->cir_buff.out_offs + seek_cmd_paramts.write_cmd ) 
                                        % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];

        // 2nd Validate write_cmd_offset with the specific entry->size of the write_cmd
        if (seek_cmd_paramts.write_cmd_offset >= entry->size) { 
            PDEBUG("Error tmaño del offset es mayor a la entrada\n");
            retval = -EINVAL;
            goto out;
        }
        PDEBUG("seek_cmd_paramts.write_cmd_offset: %u  y entry->size: %lu, entry_count %u\n",seek_cmd_paramts.write_cmd_offset,entry->size,dev->cir_buff.entry_count);
        // for (int i = 0; i < dev->cir_buff.entry_count; i++) {
        //      PDEBUG("vERIFICACIÓN DE PASOS");
        //     size_t index =(dev->cir_buff.out_offs+i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
           
        //     PDEBUG("Entry[%d] = %.*s", i, dev->cir_buff.entry[index].size , dev->cir_buff.entry[index].buffptr);  
        // } 

        //3rd Calculate new file position
        for(int i =0; i < seek_cmd_paramts.write_cmd;i++){
            struct aesd_buffer_entry *get_size = &dev->cir_buff.entry[(dev->cir_buff.out_offs+i)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];
            new_pos += get_size->size;
            if(get_size->buffptr == NULL){
                PDEBUG("goto ERROR get_size->buffptr == NULL\n");
                retval = -EINVAL;
                goto out;
            }
            PDEBUG("new position FOR %lu \n", new_pos);

        }

        //Here we add the get_size->size of each cmd and sume the offset "write_cmd_offset"
        new_pos += seek_cmd_paramts.write_cmd_offset;
        PDEBUG("new position + write_cmd_offset %lu \n", new_pos);
        
        // Update file position
        filp->f_pos = new_pos;
        PDEBUG("filp->f_pos %lu \n", filp->f_pos);
        PDEBUG("retval interno %lu \n", retval);
        break;
    
    default:
        PDEBUG("No ingreso a aesd_ioctl Default Error\n");
        return -EINVAL;
        break;
    }

    PDEBUG("retval externo %lu \n", retval);
    mutex_unlock(&dev->lock);
    return retval;
    
    out:
        mutex_unlock(&dev->lock);
        return retval;

}

size_t aesd_circular_buffer_full_size(struct aesd_circular_buffer *buffer_size){
    size_t total_size = 0;
    uint8_t index = buffer_size->out_offs; //Comienza desde el elemento más antiguo
                                               //No siempre empieza en la posición 0 del buffer.

    for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
    {
        if(buffer_size->entry[index].buffptr != NULL)
            total_size += buffer_size->entry[index].size;
        
        PDEBUG("I     CIRCULAR BUFFER %u \n",i);
        PDEBUG("INDEX CIRCULAR BUFFER %u \n",index);
        index = (index +1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; //this % ensures the Wrap-Around 
        
    }
    PDEBUG("RETURN CIRCULAR BUFFER %u \n",total_size);
    return total_size;
}

static loff_t aesd_llseek(struct file *filp, loff_t offset, int whence) {
    struct aesd_dev *dev = filp->private_data;
    loff_t newpos;
    size_t file_size;

    mutex_lock(&dev->lock);
    file_size = aesd_circular_buffer_full_size(&dev->cir_buff); // Obtiene tamaño actual bajo mutex
                                                            //Dentro del circular Buffer
    PDEBUG("sIZE OF THE CIRCULAR BUFFER %u \n", file_size);
    //Avoid race condition
    mutex_unlock(&dev->lock);

    switch (whence) {
    case SEEK_SET: // Desde inicio del archivo
        newpos = offset; //siempre arranca en cero este archivo
        PDEBUG("Newpost SEEK_SET %u",newpos);
        break;
    case SEEK_CUR: // Desde posición actual
        newpos = filp->f_pos + offset;
        PDEBUG("Newpost SEEK_CUR %u",newpos);
        break;
    case SEEK_END: // Desde final del archivo
        newpos = file_size + offset;
        PDEBUG("Newpost SEEK_END %u",newpos);
        break;
    default:
        return -EINVAL; // Tipo de seek inválido
    }
    
    // No permitir posiciones negativas
    if (newpos < 0) 
        return -EINVAL;
    
    //Avoid EOF to increse more beyon the total size of the buffer
    if (newpos > file_size)
        newpos=file_size;


    filp->f_pos = newpos; // Actualiza posición en el archivo
    PDEBUG();
    return newpos;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek =   aesd_llseek, // Assigment 9
    .unlocked_ioctl = aesd_ioctl, // Assigment 9
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
