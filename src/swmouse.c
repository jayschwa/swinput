/*****
 *       swinput 
 *                                                                   
 * swkeybd fakes a keyboard using the Linux Input Subsystem by reading
 * from a device file
 *                                                                   
 *        Copyright (C) 1999, 2000, 2001, 2002, 2003 Henrik Sandklef                    
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

/*
 *  swmouse.c, fakes a mouse using Linux Input System
 *     heavily based on
 *         idiom.c 
 *     which is heavily based on on usbmouse.c
 *
 *  Copyright (c) 2003 Henrik Sandklef <hesa@gnu.org>
 *  Copyright (c) 2000 Alessandro Rubini <rubini@gnu.org>
 *  Copyright (c) 1999 Vojtech Pavlik <vojtech@suse.cz>
 *
 */


#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */

/* usb stuff */
#include <linux/input.h>

/* miscdevice stuff */
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include <linux/proc_fs.h>


MODULE_LICENSE ("GPL");


struct swmouse_device {
  /* input device, to push out input  data */
  struct input_dev idev;

  /* statistic counters */
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



/*
 *   Name:        swmouse_read_procmem
 *
 *   Description: invoked when reading from /proc/swmouse
 *
 */ 
int 
swmouse_read_procmem(char *buf, char **start, off_t offset,
			 int count, int *eof, void *data)
{
  static char internal_buf[128];
  static int len;

  len=sprintf (internal_buf,"swmouse:%d;%d;%d;%d\n", swmouse.ups,swmouse.downs,swmouse.lefts,swmouse.rights);
  
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
int 
init_module(void)
{
  int retval;
    
  /* 
   * register the our faker as a misc device
   */ 
  retval = misc_register(&swmouse_misc);
  if (retval!=0)
    {
      /* return if failure ... */
      printk(KERN_INFO "swmouse: failed to register the swmouse as a misc device\n");
      return retval;
    }

  /* zero the mem for the input device struct
   */ 
  memset(&swmouse.idev, 0, sizeof(struct swmouse_device));
  
  /* set the name */
  swmouse.idev.name = "swmouse";
  
  /* tell which keys: only the arrows */
  set_bit(EV_REL,    swmouse.idev.evbit); /*  */
  set_bit(REL_X,     swmouse.idev.relbit); /*  */
  set_bit(REL_Y,     swmouse.idev.relbit); /*  */
  
  set_bit(EV_KEY,    swmouse.idev.evbit); /*  */
  set_bit(BTN_LEFT,  swmouse.idev.keybit); /*  */
  set_bit(BTN_RIGHT, swmouse.idev.keybit); /*  */
  
  swmouse.idev.keybit[0]=BIT(EV_KEY) | BIT(EV_REL) | BIT(REL_Y) | BIT(REL_X) ;

  /* register the device to the input system */
  input_register_device(&swmouse.idev);

  /* create the /proc entry */
  create_proc_read_entry("swmouse", 
			 0    /* default mode */,
			 NULL /* parent dir */, 
			 swmouse_read_procmem, 
			 NULL /* client data */);
  printk(KERN_INFO "swmouse: module regsitred\n");

  return retval;
}






/*
 *   Name:        cleanup_module
 *
 *   Description: invoked when removing the module
 *
 */ 
void 
cleanup_module(void)
{
  input_unregister_device(&swmouse.idev);
  misc_deregister(&swmouse_misc);
  remove_proc_entry("swmouse", NULL /* parent dir */);
  printk(KERN_INFO "swmouse: module unregsitred\n");
}


int swmouse_open(struct inode *inode, struct file *filp)
{
    return 0; /* Ok */
}    



int swmouse_release(struct inode *inode, struct file *filp)
{
    return 0;
}



/* write accepts data and converts it to mouse movement */
ssize_t swmouse_write(struct file *filp, const char *buf, size_t count,
		    loff_t *offp)
{
    static char localbuf[10];
    int  i;
    char letter;
    char *tmp;
    int  nrs;
    
    /* accept 10 bytes at a time, at most */
    if (count >10) count=10;
    copy_from_user(localbuf, buf, count);
    
	
    letter=localbuf[0];
    tmp=&localbuf[1];
    if (!sscanf (tmp, "%d",&nrs)<0)
      {
	printk(KERN_INFO "swmouse: problems converting %s (tmp=%s   nrs=%d)\n",localbuf, tmp, nrs);
	return count;
      }
 
    if ( (nrs<=0) || (nrs>1024) ) nrs=1;
    
    switch (letter) {
    case 'u': case 'U': 
      swmouse.ups+=nrs;
      input_report_rel(&swmouse.idev, REL_Y, 0-nrs);
      break;
    case 'd': case 'D': 
      swmouse.downs+=nrs;
      input_report_rel(&swmouse.idev, REL_Y, nrs);
      break;
    case 'l': case 'L': 
      input_report_rel(&swmouse.idev, REL_X, 0-nrs);
      swmouse.lefts+=nrs;
      break;
    case 'r': case 'R': 
      swmouse.rights+=nrs;
      input_report_rel(&swmouse.idev, REL_X, nrs);
      break;
    case '0': 
      swmouse.rights=0;
      swmouse.lefts=0;
      swmouse.downs=0;
      swmouse.ups=0;
      break;
    default:
      ;
    }
    return count;
}



struct file_operations swmouse_file_operations = {
	write:    swmouse_write,
	open:     swmouse_open,
	release:  swmouse_release,
};
