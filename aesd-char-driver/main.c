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
#include "aesd_ioctl.h"
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

    dev = container_of(inode->i_cdev , struct aesd_dev , cdev);

    filp->private_data = dev;

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


// Custom llseek function to modify the file pointer position
// It uses the fixed_size_llseek() function to make the change in the kernel
static long aesd_adjust_file_offset(struct file *filp , unsigned int write_cmd , unsigned int write_cmd_offset)
{
    
    long ret_val = 0;

    // Iterator
    int i = 0;

    int ret_status = 0;

    // Store the temporary value of the f_pos before writing to the filp member
    uint32_t temp_fpos = 0;
    
    // Structure to reference the circular buffer
    struct aesd_dev *dev_offset = filp->private_data;

    PDEBUG("adjust 1\n");

    // Error check 1: If the write command is greater than the allowed number of entries aka 9
    if (write_cmd > (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1)){
        ret_val = -EINVAL;
        return ret_val;
    }

    // Error check 2: If the write command offset is greater than allowed offset
    if (write_cmd_offset >= dev_offset->rw_circular_buffer.entry[write_cmd].size){
        ret_val = -EINVAL;
        return ret_val;
    }

    PDEBUG("adjust 2\n");

    // Locking is necessary since you don't want to operate on a stale copy of the circular buffer
    ret_status = mutex_lock_interruptible(&dev_offset->rw_mutex_lock);

    // Error check
    if (ret_status != 0){
        ret_val = -ERESTARTSYS;
        return ret_val;
    }

    PDEBUG("adjust 3\n");

    for (i = 0; i < write_cmd; i++){
        // Increment the temp file position 
        temp_fpos += dev_offset->rw_circular_buffer.entry[i].size;
    }

    PDEBUG("adjust 4\n");

    // Finally, increment it by the required offset in the given command
    temp_fpos += write_cmd_offset;

    filp->f_pos = temp_fpos;

    PDEBUG("adjust 5\n");

    mutex_unlock(&dev_offset->rw_mutex_lock);

    PDEBUG("adjust 6\n");

    return ret_val;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t retval = 0;

    // Mutex return values
    int ret_status = 0;

    // Store filp structure private data in a local data variable
    struct aesd_dev *llseek_dev =  filp->private_data;

    ret_status = mutex_lock_interruptible(&aesd_device.rw_mutex_lock);

    // Error check
    if (ret_status != 0){
        retval = -ERESTARTSYS;
        return retval;
    }

    // Reference: Lecture video
    // Wrapper and supporting function to implement own llseek function
    retval = fixed_size_llseek(filp , off , whence , llseek_dev->rw_circular_buffer.total_buff_size);

    // Unlock after making the changes to the kernel
    mutex_unlock(&aesd_device.rw_mutex_lock);

    // To return any error with the wrapper function fixed_size_llseek()
    return retval;
}

// Ioctl support
// Reference: Linux device drivers book
long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) 
{
    long ret = 0;

    // Reference: Lecture video
    struct aesd_seekto seekto;

    // Reference: Linux Device drivers book
    /*
    * extract the type and number bitfields, and don't decode
    * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok(  )
    */
    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;

    switch(cmd){
        case AESDCHAR_IOCSEEKTO:
        if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)) != 0 ) {
            ret = -EFAULT;
        } else {
            ret = aesd_adjust_file_offset(filp, seekto.write_cmd, seekto.write_cmd_offset);

            if (ret != 0){
                PDEBUG("Error in adjust function: %ld\n", ret);
            }
        }
        break; 

        // By default if wrong command is issued, ENOTTY should be returned
        // Can also return EINVAL
        default:
            PDEBUG("Did you come here\n");
            ret = -ENOTTY;
            break;
    }  

    return ret; 
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

    // Use the kernel malloc function to allocate memory for data to be written (of count bytes)
    write_buffer = (char *)kmalloc(count, GFP_KERNEL);
    // Error check
    if(write_buffer == NULL) {
        retval = -ENOMEM;
        goto clean;
    }
        
    // Copy the data from the userspace into the kernel space
    if(copy_from_user(write_buffer, buf, count)) {
        retval = -EFAULT;
		goto free;
	}


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
    .llseek = aesd_llseek,
    .unlocked_ioctl = aesd_ioctl
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
    uint8_t del_idx;
    dev_t devno;

    devno = MKDEV(aesd_major, aesd_minor);

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
