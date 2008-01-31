/*****
 *       swinput 
 *                                                                   
 *  swmouse.c, fakes a mouse using Linux Input System
 *     heavily based on
 *         idiom.c 
 *     which is heavily based on on usbmouse.c
 *
 *  Copyright (c) 2003,2004,2005,2006 Henrik Sandklef <hesa@gnu.org>                    
 *                                                                   
 * This program is free software; you can redistribute it and/or     
 * modify it under the terms of the GNU General Public License       
 * as published by the Free Software Foundation; either version 2    
 * of the License, or any later version.                             
 *                                                                   
 *                                                                   
 * This program is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     
 * GNU General Public License for more details.                      
 *                                                                   
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software       
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,            
 * MA  02111-1307, USA.                                              
 ****/


#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>


MODULE_DESCRIPTION("Fake-mouse input device");
MODULE_AUTHOR("Henrik Sandklef  <hesa@gnu.org>");
MODULE_LICENSE ("GPL");

#define XMAX 1280
#define YMAX 800

/* screen-resolutions */
static int xmax, ymax;

/* module parameters */
module_param(xmax, int, 0444);
module_param(ymax, int, 0444);
MODULE_PARM_DESC(xmax, "nominal screen-width (default 1280)");
MODULE_PARM_DESC(ymax, "nominal screen-height (default 800)");

struct swmouse_device {
  /* input device, to push out input  data */
  struct input_dev *idev;

  /* statistic counters */
  int fixed_x;
  int fixed_y;
  int ups;
  int downs;
  int lefts;
  int rights;
};

struct swmouse_device swmouse ;


struct file_operations swmouse_file_operations; 

struct miscdevice swmouse_misc = {
    minor:      MISC_DYNAMIC_MINOR,
    name:       "swmouse",
    fops:       &swmouse_file_operations,
};



int swmouse_open_simple(struct input_dev *dev);

void swmouse_release_simple(struct input_dev *dev);

/*
 *   Name:        swmouse_read_procmem
 *
 *   Description: invoked when reading from /proc/swmouse
 *
 */ 
int swmouse_read_procmem(char *buf, char **start, off_t offset,
			 int count, int *eof, void *data)
{
  static char internal_buf[128];
  static int len;

  len=sprintf (internal_buf,"swmouse:%d;%d;%d;%d,%d,%d\n", 
	       swmouse.ups,swmouse.downs,
	       swmouse.lefts,swmouse.rights,
	       swmouse.fixed_x,swmouse.fixed_y);
  
  if(len<=count)
    {
      strcpy(buf, internal_buf);
      return len;
    }
  else
    {
      return 0;
    }
  return 0;
}



/*
 *   Name:        init_module
 *
 *   Description: invoked when inserting the module
 *
 */ 
int init_module(void)
{
    int retval = -1;
    
    printk (KERN_INFO "swmouse: Initializing...\n");
    
    if(xmax == 0)
        xmax = XMAX;
    if(ymax == 0)
        ymax = YMAX;
    
    if(xmax < 0)
    {
        printk(KERN_INFO "swmouse: xmax must be > 0. Using default.\n");
        xmax = XMAX;
    }
    
    if(ymax < 0)
    {
        printk(KERN_INFO "swmouse: ymax must be > 0. Using default.\n");
        ymax = YMAX;
    }
    
    memset(&swmouse, 0, sizeof(struct swmouse_device));
      
    swmouse.idev = input_allocate_device();
    
    if(swmouse.idev)
    {
        /* 
         * register our faker as a misc device
         */ 
        retval = misc_register(&swmouse_misc);
        if (retval!=0)
        {
          /* return if failure ... */
          printk(KERN_INFO "swmouse: failed to register the swmouse as a misc device\n");
          input_free_device(swmouse.idev);
          return retval;
        }
        
        
        /* set the name */
        swmouse.idev->name    = "swmouse";
        swmouse.idev->id.vendor  = 0x00;
        swmouse.idev->id.product = 0x00;
        swmouse.idev->id.version = 0x00;
        
        swmouse.idev->open  = swmouse_open_simple;
        swmouse.idev->close = swmouse_release_simple;
        
        swmouse.idev->evbit[0]  = BIT(EV_KEY) | BIT(EV_REL) | BIT(EV_ABS) ;
        swmouse.idev->relbit[0] = BIT(REL_Y) | BIT(REL_X) ;
        swmouse.idev->keybit[LONG(BTN_LEFT)] = BIT(BTN_LEFT) | BIT(BTN_MIDDLE) | BIT(BTN_RIGHT);
        swmouse.idev->keybit[LONG(BTN_TOUCH)] = BIT(BTN_TOUCH);
        
        input_set_abs_params(swmouse.idev, ABS_X, 0, xmax, 0, 0);
        input_set_abs_params(swmouse.idev, ABS_Y, 0, ymax, 0, 0);
        
        /* register the device to the input system */
        if(input_register_device(swmouse.idev))
        {
            printk(KERN_INFO "swmouse: Unable to register input device!\n");
            input_free_device(swmouse.idev);
            return -1;
        }
        
        printk(KERN_INFO "swmouse: module registered (xmax: %d, ymax: %d)\n", 
                    xmax, ymax);
        
        /* create the /proc entry */
        create_proc_read_entry("swmouse", 
                 0    /* default mode */,
                 NULL /* parent dir */, 
                 swmouse_read_procmem, 
                 NULL /* client data */);
        printk(KERN_INFO "swmouse: proc entry created\n");

    }
    
    return retval;
}






/*
 *   Name:        cleanup_module
 *
 *   Description: invoked when removing the module
 *
 */ 
void cleanup_module(void)
{
    input_unregister_device(swmouse.idev);
    input_free_device(swmouse.idev);
    /*   input_free_device(swmouse.idev); */
    misc_deregister(&swmouse_misc);
    remove_proc_entry("swmouse", NULL /* parent dir */);
    printk(KERN_INFO "swmouse: module unregistered\n");
}


int swmouse_open(struct inode *inode, struct file *filp)
{
/*   printk(KERN_INFO "swmouse: open\n"); */
  return 0; /* Ok */
}    



int swmouse_release(struct inode *inode, struct file *filp)
{
  /*   printk(KERN_INFO "swmouse: releas\n"); */
    return 0;
}


int swmouse_open_simple(struct input_dev *dev)
{
/*   printk(KERN_INFO "swmouse: open_simple\n"); */
  return 0; /* Ok */
}    



void swmouse_release_simple(struct input_dev *dev)
{
/*   printk(KERN_INFO "swmouse: release_simple\n"); */
  return ;
}



/* write accepts data and converts it to mouse movement */
ssize_t swmouse_write(struct file *filp, const char *buf, size_t count,
		    loff_t *offp)
{
    #define BUF_SIZE 32
    static char localbuf[BUF_SIZE];
    char letter;
    char *tmp;
    int  nrs;
    int direction=-1;
    int is_abs = 0;
    int pix=0;

    if (count==0)
    {
        return 0;
    }

    /* accept BUF_SIZE bytes at a time, at most */
    if (count >BUF_SIZE) count=BUF_SIZE;
    if(copy_from_user(localbuf, buf, count) != 0)
	{
		/* copy_from_user() failed */
		printk(KERN_INFO "swmouse: copy_from_user() failed!\n");
		
		/* silently ignore */
		return count;
	}
	
    tmp=&localbuf[0];
    if (tmp==NULL)
    {
        /* Strange case, silently ignore it :(   */
        return count;
    }
    
    /* Remove leading blanks ...*/
    while ( (tmp!=NULL) && (tmp[0]==' ') ) 
    {
        tmp++;
    }

    /* Save char to get direction later on */
    letter=tmp[0];

    /* Go to next character */
    if (tmp!=NULL)
    {
        tmp++;
    }
    
    /* Remove leading blanks ...*/
    while ( (tmp!=NULL) && (tmp[0]==' ')) 
    {
        tmp++;
    }
 
    /* Remove "=" if any */
    if ( (tmp!=NULL) && (tmp[0]=='=') )
    {
        tmp++;
    }


    if (!sscanf (tmp, "%d",&nrs)<0)
    {
        printk(KERN_INFO "swmouse: problems converting %s (tmp=%s   nrs=%d)\n",localbuf, tmp, nrs);
        return count;
    }
 
    
    switch (letter) 
    {
        case 'u': case 'U': 
            if ( (nrs<=0) || (nrs > ymax) ) nrs=1;
            swmouse.ups+=nrs;
            direction = REL_Y;
            pix = 0-nrs;
            break;
        
        case 'd': case 'D': 
            if ( (nrs<=0) || (nrs > ymax) ) nrs=1;
            swmouse.downs+=nrs;
            direction = REL_Y;
            pix = nrs;
            break;
        
        case 'l': case 'L': 
            if ( (nrs<=0) || (nrs > xmax) ) nrs=1;
            direction = REL_X;
            pix = 0-nrs;
            swmouse.lefts+=nrs;
            break;
        
        case 'r': case 'R': 
            if ( (nrs<=0) || (nrs > xmax) ) nrs=1;
            swmouse.rights+=nrs;
            direction = REL_X;
            pix = nrs;
            break;
        
        case 'x': case 'X': 
            if ( (nrs<=0) || (nrs > xmax) ) nrs=1;
            swmouse.fixed_x++;
            is_abs=1;
            direction = ABS_X;
            pix = nrs;
            break;
        
        case 'y': case 'Y': 
            if ( (nrs<=0) || (nrs > ymax) ) nrs=1;
            swmouse.fixed_y++;
            is_abs=1;
            direction = ABS_Y;
            pix = nrs;
            break;
        
        case '0': 
            swmouse.fixed_x=0;
            swmouse.fixed_y=0;
            swmouse.rights=0;
            swmouse.lefts=0;
            swmouse.downs=0;
            swmouse.ups=0;
            pix=0;
            break;
        
        default:
            ;
    }
    
    if ( direction!=-1 )
    {
        if (is_abs)
        {
            printk(KERN_INFO "input_report_abs(%p,%d,%d)\n",
                    swmouse.idev, direction, pix);
            input_report_abs(swmouse.idev, direction, 0);
            input_sync(swmouse.idev);
            input_sync(swmouse.idev);
            input_report_abs(swmouse.idev, direction, pix);
        }
        else
        {
            printk(KERN_INFO "input_report_rel(%p,%d,%d)\n",
            swmouse.idev, direction, pix);
            input_report_rel(swmouse.idev, direction, pix); 
        }
        input_sync(swmouse.idev); 
    }

    return count;
}



struct file_operations swmouse_file_operations = {
	.owner   =    THIS_MODULE,
	.write   =    swmouse_write,
	.open    =    swmouse_open,
	.release =    swmouse_release,
};


/* Test function:
     xy() {   echo "x $1" > /dev/swmouse ; echo "y $2" > /dev/swmouse ; sleep 2 ; XPOS=$(/home/hesa/gnu/xnee/cnee/test/src/xgetter  -mprx) ; YPOS=$( /home/hesa/gnu/xnee/cnee/test/src/xgetter  -mpry) ; echo "${1}x${2} ==> ${XPOS}x${YPOS}" ; }
   

   xy 1 2 
*/
