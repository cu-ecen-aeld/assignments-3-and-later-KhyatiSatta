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
#include <linux/fs.h> // file_operation
#include "aesdchar.h"
#include "linux/slab.h"
#include "linux/string.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Khyati Satta"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    // Reference: Linux Device Drivers book

    // Device information
    struct aesd_dev *dev;

    PDEBUG("open");
    /**
     * TODO: handle open
     */

    printk(KERN_INFO "Open 1\n");

    dev = container_of(inode->i_cdev , struct aesd_dev , cdev);

    filp->private_data = dev;

    printk(KERN_INFO "Open 2\n");

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */

    // Reference: Linux Device Drivers book
    // No code required as device has no hardware to shut down
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;

    // Reference: Linux Device Drivers book
    // Store the filp paramter in the user structure
    struct aesd_dev *dev = filp->private_data;
    size_t entry_byte_offs = 0;
    ssize_t res_bytes = 0;
    struct aesd_buffer_entry * ret_entry_buffer;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */

    // Acquire the mutex lock
    mutex_lock(&aesd_device.rw_mutex_lock);

    ret_entry_buffer = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->rw_circular_buffer , *f_pos , &entry_byte_offs);

    // Error check
    if (ret_entry_buffer == NULL){
        *f_pos = 0;
        goto clean;
    }

    // If the difference between size of the data entry is positive but less than count, part of data has been transferred
    if ((ret_entry_buffer->size - entry_byte_offs) < count){
        *f_pos += (ret_entry_buffer->size - entry_byte_offs);
        res_bytes = ret_entry_buffer->size - entry_byte_offs;

    } 
    // Optimal case: All bytes can be transferred
    else{
        *f_pos += count;
        res_bytes = count;
    }

    if (copy_to_user(buf, (ret_entry_buffer->buffptr + entry_byte_offs), res_bytes)) {
		retval = -EFAULT;
		goto clean;
	}

    retval = res_bytes;

    clean : mutex_unlock(&aesd_device.rw_mutex_lock);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    // Write buffer
    char *write_buffer; 

    // Flag indicating complete packet
    bool is_packet_complete = false;

    // Stores the total length of the packet
    int total_packet_len = 0;

    // Holds the value to be added to the circular buffer
    struct aesd_buffer_entry write_cb_entrys; 
    struct aesd_dev *dev;

    // Iterates over incoming data while looking for '\n'
    int i; 

    int realloc_size = 0; 

    char *ret_ptr;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */


    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    dev = filp->private_data;

    mutex_lock(&aesd_device.rw_mutex_lock);

    printk(KERN_INFO "Write 1\n");

    // Use the kernel malloc function to allocate memory for data to be written (of count bytes)
    write_buffer = (char *)kmalloc(count, GFP_KERNEL);
    // Error check
    if(write_buffer == NULL) {
        retval = -ENOMEM;
        goto clean;
    }

    printk(KERN_INFO "Write 2\n");
        
    // Copy the data from the userspace into the kernel space
    if(copy_from_user(write_buffer, buf, count)) {
        retval = -EFAULT;
		goto free;
	}

    printk(KERN_INFO "Write 2\n");

    // Check if any of the incoming bytes has '\n' (packet complete)
    for (i = 0; i < count; i++) {
        if (write_buffer[i] == '\n') {
            is_packet_complete = true;
            total_packet_len = (i + 1);
            break;
        }
    }

    // Copy the buffer into the user structure buffer
    if (dev->rw_buff_size == 0) {
        dev->rw_buff_ptr = (char *)kmalloc(count, GFP_KERNEL);
        // Error check
        if (dev->rw_buff_ptr == NULL) {
            retval = -ENOMEM;
            goto free;
        }
        memcpy(dev->rw_buff_ptr, write_buffer, count);
        dev->rw_buff_size += count;
    } 
    else {
        // If no complete packet was encountered
        if (is_packet_complete)
            realloc_size = total_packet_len;
        else
            realloc_size = count;

        // Realloc copy buffer size based on temporary size increment variable
        dev->rw_buff_ptr = (char *)krealloc(dev->rw_buff_ptr, dev->rw_buff_size + realloc_size, GFP_KERNEL);
        // Error check
        if (dev->rw_buff_ptr == NULL) {
            retval = -ENOMEM;
            goto free;
        }

        // Copying temp_buffer contents into copy_buffer
        memcpy(dev->rw_buff_ptr + dev->rw_buff_size, write_buffer, realloc_size);
        dev->rw_buff_size += realloc_size;        
    }
    
    // Push the entry in the circular buffer if packet is complete
    if (is_packet_complete) {

        // Fill in the structure entries
        write_cb_entrys.buffptr = dev->rw_buff_ptr;
        write_cb_entrys.size = dev->rw_buff_size;
        ret_ptr = aesd_circular_buffer_add_entry(&dev->rw_circular_buffer, &write_cb_entrys);
    
        // Freeing return_pointer if buffer is full 
        if (ret_ptr != NULL)
            kfree(ret_ptr);
        
        dev->rw_buff_size = 0;
    } 

    retval = count;

    free: kfree(write_buffer);

    clean: mutex_unlock(&aesd_device.rw_mutex_lock);

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
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    // Initialize the mutex
    mutex_init(&aesd_device.rw_mutex_lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    struct aesd_buffer_entry *del_entry;
    int del_idx = 0;

    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    AESD_CIRCULAR_BUFFER_FOREACH(del_entry, &aesd_device.rw_circular_buffer, del_idx) {
      kfree(del_entry->buffptr);
    }
    mutex_destroy(&aesd_device.rw_mutex_lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
