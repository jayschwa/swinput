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
 *  swkeybd.c, sample code for USB drivers, 
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
#include <linux/usb.h>

/* miscdevice stuff */
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include <linux/proc_fs.h>

MODULE_LICENSE ("GPL");

static char keycodes[512] ;

struct swkeybd_device *swkeybd ;




/*
 * We need a local data structure, as it must be allocated for each new
 * mouse device plugged in the USB bus
 */
struct swkeybd_device {
  struct input_dev idev;   /* input device, to push out input  data */

  int    shift_press;
  int    shift_release;

  int    key_press;
  int    key_release;
};





/* forward declaration */
struct file_operations swkeybd_file_operations;


struct miscdevice swkeybd_misc = {
    minor:      MISC_DYNAMIC_MINOR,
    name:       "swkeybd",
    fops:       &swkeybd_file_operations,
};

/*
 *   Name:        swkeybd_read_procmem
 *
 *   Description: invoked when reading from /proc/swkeybd
 *
 */ 
int 
swkeybd_read_procmem(char *buf, char **start, off_t offset,
			 int count, int *eof, void *data)
{
  static char internal_buf[128];
  static int len;

  len=sprintf (internal_buf,
	       "swkeybd:%d;%d;%d;%d\n", 
	       swkeybd->key_press,
	       swkeybd->key_release,
	       swkeybd->shift_press,
	       swkeybd->shift_release);
  
  
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




int 
init_module(void)
{
    int retval;

    printk(KERN_INFO "swkeybd: 1\n");
    retval = misc_register(&swkeybd_misc);
    if (retval!=0)
      {
	/* return if failure ... */
	printk(KERN_INFO "swkeybd: failed to register the swkeybd as a misc device\n");
	return retval;
      }



    swkeybd = kmalloc(sizeof(struct swkeybd_device), GFP_KERNEL);
    if (!swkeybd) 
      return -ENOMEM;
    memset(swkeybd, 0, sizeof(*swkeybd));

    /* tell the features of this input device: fake only keys */
    swkeybd->idev.evbit[0] = BIT(EV_KEY);

    swkeybd->shift_press=0;
    swkeybd->shift_release=0;
    swkeybd->key_press=0;
    swkeybd->key_release=0;



    init_keycodes();

    input_register_device(&swkeybd->idev);

    create_proc_read_entry("swkeybd", 
			   0    /* default mode */,
			   NULL /* parent dir */, 
			   swkeybd_read_procmem, 
			   NULL /* client data */);    /* announce yourself */


    return retval;
}

void cleanup_module(void)
{

    input_unregister_device(&swkeybd->idev);
    kfree(swkeybd);
    printk(KERN_INFO "swkeybd: closing misc device\n");


    misc_deregister(&swkeybd_misc);
}


/*
 * Handler for data sent in by the device. The function is called by
 * the USB kernel subsystem whenever the device spits out new data
 */
static void 
fake_key(char c)
{
  
  int do_shift=0;
  printk(KERN_INFO "swkeybd: printing char '%c' (%d)  ---> %d\n", c, c, keycodes[c]);
  
  if (( c>='A' ) && ( c<='Z'))
    {
      do_shift=1;
      c=c-'A'+'a';
    }
  
  if (do_shift) 
    {
      input_report_key(&swkeybd->idev, KEY_LEFTSHIFT, 1); /* keypress */
      swkeybd->shift_press++;
    }
  
  input_report_key(&swkeybd->idev, keycodes[c], 1); /* keypress */
  swkeybd->key_press++;
  printk(KERN_INFO "swkeybd: key_press = %d\n", swkeybd->key_press);
  
  input_report_key(&swkeybd->idev, keycodes[c], 0); /* release */
  swkeybd->key_release++;
  
  if (do_shift) 
    {
      input_report_key(&swkeybd->idev, KEY_LEFTSHIFT, 0); /* keyrelease */
      swkeybd->shift_release++;
    }
}

/*
 * Finally, offer an entry point to write data using the misc device
 */


/* the open method does the same as swkeybd_probe does */
int swkeybd_open(struct inode *inode, struct file *filp)
{
    return 0; /* Ok */
}    

/* close releases the device, like swkeybd_disconnect */
int swkeybd_release(struct inode *inode, struct file *filp)
{
    return 0;
}

int 
press_rel (int key)
{
  input_report_key(&swkeybd->idev, key, 1); /* keypress */
  input_report_key(&swkeybd->idev, key, 0); /* release */
}


int
fake_esc (char *str)
{
  printk ("fake_esc \"%s\"\n", str);
  if (str[0]=='F')
    {
      int key;
      if (strcmp(str,"F1")==0)
	key=KEY_F1;
      if (strcmp(str,"F2")==0)
	key=KEY_F2;
      if (strcmp(str,"F3")==0)
	key=KEY_F3;
      if (strcmp(str,"F4")==0)
	key=KEY_F4;
      if (strcmp(str,"F5")==0)
	key=KEY_F5;
      if (strcmp(str,"F6")==0)
	key=KEY_F6;
      if (strcmp(str,"F7")==0)
	key=KEY_F7;
      if (strcmp(str,"F8")==0)
	key=KEY_F8;
      if (strcmp(str,"F9")==0)
	key=KEY_F9;
      else if (strcmp(str,"F10")==0)
	key=KEY_F10;
      else if (strcmp(str,"F11")==0)
	key=KEY_F11;
      else if (strcmp(str,"F12")==0)
	key=KEY_F12;
      else if (strcmp(str,"F13")==0)
	key=KEY_F13;
      else if (strcmp(str,"F14")==0)
	key=KEY_F14;
      else if (strcmp(str,"F15")==0)
	key=KEY_F15;
      else if (strcmp(str,"F16")==0)
	key=KEY_F16;
      else if (strcmp(str,"F17")==0)
	key=KEY_F17;
      else if (strcmp(str,"F18")==0)
	key=KEY_F18;
      else if (strcmp(str,"F19")==0)
	key=KEY_F19;
      else if (strcmp(str,"F20")==0)
	key=KEY_F20;
      else 
	key=0;
      if (key!=0)
	press_rel (key);
    }
  else if (strcmp (str, "BACKSPACE")==0)
    {
      press_rel(KEY_BACKSPACE); 
    }
  else if (strcmp (str, "SPACE")==0)
    {
      press_rel(KEY_SPACE); 
    }
  else if (strcmp (str, "SPACE")==0)
    {
      press_rel(KEY_SPACE); 
    }
  else if (strcmp (str, "COMMA")==0)
    {
      press_rel(KEY_COMMA); 
    }
  else if (strcmp (str, "DOT")==0)
    {
      press_rel(KEY_DOT); 
    }
  else if (strcmp (str, "KEY_LEFT")==0)
    {
      press_rel(KEY_LEFT); 
    }
  else if (strcmp (str, "KEY_RIGHT")==0)
    {
      press_rel(KEY_RIGHT); 
    }
  else if (strcmp (str, "KEY_DOWN")==0)
    {
      press_rel(KEY_DOWN); 
    }
  else if (strcmp (str, "KEY_UP")==0)
    {
      press_rel(KEY_UP); 
    }
  else if (strcmp (str, "clear")==0)
    {
      swkeybd->shift_press=0;
      swkeybd->shift_release=0;
      swkeybd->key_press=0;
      swkeybd->key_release=0;
    }
  else
    ;
}

/* poll reports the device as writeable */
unsigned int swkeybd_poll(struct file *filp, struct poll_table_struct *table)
{
    return POLLWRNORM | POLLOUT;
}


/* 
 * write accepts data and converts it to mouse movement 
 */
ssize_t 
swkeybd_write(struct file *filp, const char *buf, size_t count,
		    loff_t *offp)
{
    struct swkeybd_device *swkeybd = filp->private_data;
    static char localbuf[16];
    static char escapebuf[16];
    struct urb urb;
    int i;
    int idx=0;
    int found=0;
    int toggle=0;
    
    if (count >16) count=16;
    copy_from_user(localbuf, buf, count);
    
    /* scan written data */
    for (i=0; i<count; i++)
      {

	if ( localbuf[i] == '\\')
	  {
	    i++;
	    printk ("escape sequence : %c\n", localbuf[i]);
	    fake_key(localbuf[i]);
	  }
	    
	if ( localbuf[i] == '[')
	  {
	    printk (" [ found\n");
	    found=1;
	  } 	
	else if ( localbuf[i] == ']')
	  {
	    found=0;
	    escapebuf[idx]='\0';
	    escapebuf[idx++]='\0';
	    printk (" ]  found\n");
	    printk ("calling fake on string idx=%d   %c%c%c\n", idx, 
		    escapebuf[0], escapebuf[1], escapebuf[2]);
	    fake_esc (escapebuf);
	    idx=0;
	  }
	else
	  {
	    if (found)
	      {
		printk ("adding %c at %d\n", localbuf[i], idx);
		escapebuf[idx++]=localbuf[i];
	      }
	    else
	      {
		fake_key(localbuf[i]);
	      }
	  }
      }
    escapebuf[0]='\0';
    localbuf[0]='\0';
    return count;
}


struct file_operations swkeybd_file_operations = {
	write:    swkeybd_write,
	poll:     swkeybd_poll,
	open:     swkeybd_open,
	release:  swkeybd_release,
};





int 
set_keycodes(int in, int out)
{
  keycodes[in]=out ;
  set_bit (out, &swkeybd->idev.keybit);
}


int init_keycodes()
{
  memset (keycodes,0, 512);

  /* letters */
  set_keycodes('a',KEY_A );
  set_keycodes('b',KEY_B );
  set_keycodes('c',KEY_C );
  set_keycodes('d',KEY_D );
  set_keycodes('e',KEY_E );
  set_keycodes('f',KEY_F );
  set_keycodes('g',KEY_G );
  set_keycodes('h',KEY_H );
  set_keycodes('i',KEY_I );
  set_keycodes('j',KEY_J );
  set_keycodes('k',KEY_K );
  set_keycodes('l',KEY_L );
  set_keycodes('o',KEY_O );
  set_keycodes('m',KEY_M );
  set_keycodes('n',KEY_N );
  set_keycodes('p',KEY_P );
  set_keycodes('q',KEY_Q );
  set_keycodes('r',KEY_R );
  set_keycodes('s',KEY_S );
  set_keycodes('t',KEY_T );
  set_keycodes('u',KEY_U );
  set_keycodes('v',KEY_V );
  set_keycodes('x',KEY_X );
  set_keycodes('y',KEY_Y );
  set_keycodes('z',KEY_Z );

  /* digits */
  set_keycodes('0',KEY_0 );
  set_keycodes('1',KEY_1 );
  set_keycodes('2',KEY_2 );
  set_keycodes('3',KEY_3 );
  set_keycodes('4',KEY_4 );
  set_keycodes('5',KEY_5 );
  set_keycodes('6',KEY_6 );
  set_keycodes('7',KEY_7 );
  set_keycodes('8',KEY_8 );
  set_keycodes('9',KEY_9 );

  /* specials ..... */
  set_keycodes(':', KEY_SEMICOLON);
  set_keycodes(' ', KEY_SPACE);
  set_keycodes('-', KEY_KPMINUS);
  set_keycodes('+', KEY_KPPLUS);
  set_keycodes('=', KEY_EQUAL);
  set_keycodes(',', KEY_COMMA);
  set_keycodes('.', KEY_DOT);
  


  /* just set them....*/
  set_bit (KEY_SEMICOLON,  &swkeybd->idev.keybit);
  set_bit (KEY_SPACE,      &swkeybd->idev.keybit);
  set_bit (KEY_LEFTBRACE,  &swkeybd->idev.keybit);
  set_bit (KEY_RIGHTBRACE, &swkeybd->idev.keybit);
  set_bit (KEY_BACKSPACE,  &swkeybd->idev.keybit);

  set_bit (KEY_ENTER,  &swkeybd->idev.keybit);
  set_bit (KEY_ESC,  &swkeybd->idev.keybit);
  set_bit (KEY_TAB,  &swkeybd->idev.keybit);

  set_bit (KEY_LEFTSHIFT, &swkeybd->idev.keybit);
  set_bit (KEY_RIGHTSHIFT, &swkeybd->idev.keybit);
  set_bit (KEY_LEFTCTRL, &swkeybd->idev.keybit);
  set_bit (KEY_RIGHTCTRL, &swkeybd->idev.keybit);
  set_bit (KEY_LEFTALT, &swkeybd->idev.keybit);

  set_bit (KEY_CAPSLOCK, &swkeybd->idev.keybit);
  set_bit (KEY_APOSTROPHE, &swkeybd->idev.keybit);
  set_bit (KEY_GRAVE, &swkeybd->idev.keybit);
  set_bit (KEY_SLASH, &swkeybd->idev.keybit);

  set_bit (KEY_F1 , &swkeybd->idev.keybit);
  set_bit (KEY_F2 , &swkeybd->idev.keybit);
  set_bit (KEY_F3 , &swkeybd->idev.keybit);
  set_bit (KEY_F4 , &swkeybd->idev.keybit);
  set_bit (KEY_F5 , &swkeybd->idev.keybit);
  set_bit (KEY_F6 , &swkeybd->idev.keybit);
  set_bit (KEY_F7 , &swkeybd->idev.keybit);
  set_bit (KEY_F8 , &swkeybd->idev.keybit);
  set_bit (KEY_F9 , &swkeybd->idev.keybit);
  set_bit (KEY_F10 , &swkeybd->idev.keybit);
  set_bit (KEY_F11 , &swkeybd->idev.keybit);
  set_bit (KEY_F12 , &swkeybd->idev.keybit);
  set_bit (KEY_F13 , &swkeybd->idev.keybit);
  set_bit (KEY_F14 , &swkeybd->idev.keybit);
  set_bit (KEY_F15 , &swkeybd->idev.keybit);
  set_bit (KEY_F16 , &swkeybd->idev.keybit);
  set_bit (KEY_F17 , &swkeybd->idev.keybit);
  set_bit (KEY_F18 , &swkeybd->idev.keybit);
  set_bit (KEY_F19 , &swkeybd->idev.keybit);
  set_bit (KEY_F20 , &swkeybd->idev.keybit);

  set_bit ( KEY_LEFT,	&swkeybd->idev.keybit);
  set_bit ( KEY_RIGHT,	&swkeybd->idev.keybit);
  set_bit ( KEY_UP,	&swkeybd->idev.keybit);
  set_bit ( KEY_DOWN ,&swkeybd->idev.keybit);

set_bit ( KEY_LEFTBRACE,&swkeybd->idev.keybit);
set_bit ( KEY_RIGHTBRACE,&swkeybd->idev.keybit);
}
