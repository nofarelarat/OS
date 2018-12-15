#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");


struct node {
    char data[4][128];
    struct node *next;
    int minor;
    int isMsg[4];
    int lengthMsg[4];
};

static struct node* head = NULL;

// device major number
static int major;

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    int minor_num,i;
    struct node* tmp;
    struct node *slot;
    printk( "opening device.\n");
    minor_num = iminor(inode);

    tmp = head;
    while(tmp != NULL) {
        if (tmp->minor == minor_num){
            file->private_data = (void *) (-1);
            return  0;
        }
        tmp = tmp-> next;
    }

    slot = (struct node*) kmalloc(sizeof(struct node),GFP_KERNEL);
    if(slot < 0){
        return -ENOMEM;
    }

    for ( i = 0; i < 4; i++) {
        slot->isMsg[i] = 0;
        slot->lengthMsg[i] = 0;
    }

    slot->next = head;
    slot->minor = minor_num;
    file->private_data = (void *) (-1);

    //point first to new first node
    head = slot;

    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
    printk("Invoking device_release(%p,%p)\n", inode, file);

    return SUCCESS;
}

static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    printk("Invoking ioctl: setting channel id "
           "to %ld\n", ioctl_param);

    if (MSG_SLOT_CHANNEL != ioctl_command_id)
    {
        return -EINVAL;
    }

    if (ioctl_param != 1 && ioctl_param != 2 && ioctl_param != 3 && ioctl_param != 0)
    {
        return -EINVAL;
    }

    file->private_data = (void *) ioctl_param;

    return SUCCESS;
}

static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t length,
                            loff_t* offset )
{
    int i, minor_num, res;
    long channel;
    struct node *tmp;

    printk("Invoking device_read(%p,%d)\n", file, (int)length);

    channel = (long) file->private_data;

    if(channel < 0){
        return -EINVAL;
    }

    minor_num = iminor(file_inode(file));
    tmp = head;
    while(tmp != NULL) {
        if (tmp->minor == minor_num){
            break;
        }
        tmp = tmp->next;
    }
    if(tmp->isMsg[channel] == 0){
        return -EWOULDBLOCK;
    }

    if(tmp->lengthMsg[channel] > length){
        return -ENOSPC;
    }

    for(i = 0; i< tmp->lengthMsg[channel] && i< length; ++i )
    {
        res = put_user(tmp->data[channel][i], &buffer[i]);
        if(res < 0){
            return -EFAULT;
        }
    }

    // return the number of input characters used
    return i;
}

static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                            size_t length, loff_t* offset)
{
    int minor_num,i,sizeofinput, res;
    long channel;
    struct node* tmp;
    printk("Invoking device_write(%p,%d)\n", file, (int)length);

    sizeofinput = 0;

    channel = (long) file->private_data;
    if(channel < 0){
        return -EINVAL;
    }

    minor_num = iminor(file_inode(file));

    if(length > BUF_LEN){
        return -EINVAL;
    }

    tmp = head;
    while(tmp != NULL) {
        if (tmp->minor == minor_num)
        {
            for( i = 0; i < length && i < BUF_LEN; ++i )
            {
                get_user(tmp->data[channel][i], &buffer[i]);
                if(res < 0){
                    return -EFAULT;
                }
            }

            tmp->lengthMsg[channel] = i;
            tmp->isMsg[channel] = 1;
            sizeofinput = i;
        }
        tmp = tmp-> next;
    }

// return the number of input characters used
return sizeofinput;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .release        = device_release,
        };

// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc;
    printk("Invoking device registration.\n");

    rc= -1;

    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 )
    {
        printk( KERN_ALERT "%s registraion failed for  %d\n",
                DEVICE_FILE_NAME, MAJOR_NUM );
        return -EBUSY;
    }

    printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);
    return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    printk("Invoking cleanup\n");

    // Unregister the device
    unregister_chrdev(major, DEVICE_RANGE_NAME);
    while (head != NULL) {
        kfree(head);
        //mark next to first link as first
        head = head->next;
    }
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
