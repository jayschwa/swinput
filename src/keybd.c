/*****
 *       swinput 
 *                                                                   
 *  swkeybd.c, fakes a keyboard using Linux Input System
 *     heavily based on
 *         idiom.c 
 *     which is heavily based on on usbmouse.c
 *
 *  Copyright (c) 2003,2004,2005,2006 Henrik Sandklef <hesa@gnu.org>
 *                2007 Henrik Sandklef <hesa@gnu.org>, 
 *                     Daniel Hiepler <rigid@boogiepalace.hopto.org>
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

#define BUF_SIZE 1024

#include <linux/module.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/input.h>        /* usb stuff */
#include <linux/poll.h>         /* miscdevice stuff */
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "swinput.h"



MODULE_DESCRIPTION ( "Fake-keyboard input device" );
MODULE_AUTHOR ( "Henrik Sandklef  <hesa@gnu.org>" );
MODULE_LICENSE ( "GPL" );

static char keycodes[512];
void init_keycodes ( void );

/*
 * We need a local data structure, as it must be allocated for each new
 * mouse device plugged in the USB bus
 */
struct swkeybd_device
{
  struct input_dev *idev; /* input device, to push out input  data */

  int shift_press;
  int shift_release;

  int key_press;
  int key_release;
} swkeybd;

/* forward declaration */
struct file_operations swkeybd_file_operations;

struct miscdevice swkeybd_misc = {
 minor:MISC_DYNAMIC_MINOR,
 name:"swkeybd",
 fops:&swkeybd_file_operations,
};

/*
 * Name:        swkeybd_read_procmem
 *
 * Description: invoked when reading from /proc/swkeybd
 *
 */
int swkeybd_read_procmem ( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
  static char internal_buf[128];
  static int len;

  len = sprintf ( internal_buf,
		  "swkeybd:%d;%d;%d;%d\n",
		  swkeybd.key_press,
		  swkeybd.key_release,
		  swkeybd.shift_press, swkeybd.shift_release );

  if ( len <= count )
    {
      strcpy ( buf, internal_buf );
      return len;
    }
  else
    {
      return 0;
    }
  return 0;
}

/*
 * Name:        init_module
 *
 * Description: invoked when module is loaded
 *
 */
int init_module ( void )
{
  int retval = -1;

  printk ( KERN_INFO "swkeybd: Initializing...\n" );

  /* initialize local structure */
  memset ( &swkeybd, 0, sizeof ( struct swkeybd_device ) );
  swkeybd.idev = input_allocate_device (  );

  if ( swkeybd.idev )
    {
      retval = misc_register ( &swkeybd_misc );

      if ( retval != 0 )
	{
	  /* return if failure ... */
	  printk ( KERN_INFO
		   "swkeybd: failed to register the swkeybd as a misc device\n" );

	  input_free_device ( swkeybd.idev );
	  return retval;
	}

      /* set the name */
      swkeybd.idev->name = "swkeybd";
      swkeybd.idev->id.vendor = 0x00;
      swkeybd.idev->id.product = 0x00;
      swkeybd.idev->id.version = 0x00;

      /* tell the features of this input device: fake only keys */
      swkeybd.idev->evbit[0] = BIT ( EV_KEY );

      init_keycodes (  );

      if ( input_register_device ( swkeybd.idev ) )
	{
	  printk ( KERN_INFO
		   "swkeybd: Unable to register input device!\n" );
	  input_free_device ( swkeybd.idev );
	  return -1;
	}

      create_proc_read_entry ( "swkeybd", 0 /* default mode */ ,
			       NULL /* parent dir */ ,
			       swkeybd_read_procmem, NULL /* client data */  );       /* announce yourself */
    }

  return retval;
}

/*
 * Name:        cleanup_module
 *
 * Description: invoked when module is loaded
 *
 */
void cleanup_module ( void )
{
  printk ( KERN_INFO "swkeybd: Cleaning up.\n" );
  input_unregister_device ( swkeybd.idev );
  input_free_device ( swkeybd.idev );
  printk ( KERN_INFO "swkeybd: closing misc device\n" );
  misc_deregister ( &swkeybd_misc );
  remove_proc_entry ( "swkeybd", NULL /* parent dir */  );
  printk ( KERN_INFO "swkeybd: module unregistered\n" );
}

/*
 * Name:        swkeybd_open
 *
 * Description: the open method does the same as swkeybd_probe does
 *
 */
int swkeybd_open ( struct inode *inode, struct file *filp )
{
  return 0;               /* Ok */
}

/*
 * Name:        swkeybd_release
 *
 * Description: close releases the device, like swkeybd_disconnect
 *
 */
int swkeybd_release ( struct inode *inode, struct file *filp )
{
  return 0;
}

/*
 * Name:        swkeybd_key
 *
 * Description: simulate keypress and keyrelease
 *
 */
#define swkeybd_key(key) swkeybd_keyPress(key); swkeybd_keyRelease(key);

/*
 * Name:        swkeybd_keyPress
 *
 * Description: simulate keypress
 *
 */
int swkeybd_keyPress ( int key )
{
  input_report_key ( swkeybd.idev, key, 1 );      /* keypress */
  return 0;
}

/*
 * Name:        swkeybd_keyRelease
 *
 * Description: simulate key-release
 *
 */
int swkeybd_keyRelease ( int key )
{
  input_report_key ( swkeybd.idev, key, 0 );      /* release */
  return 0;
}

/*
 * Name:        swkeybd_pressRelease
 *
 * Description: close releases the device, like swkeybd_disconnect
 *
 */
int swkeybd_keyHit ( int key )
{
  swkeybd_keyPress ( key );       /* keypress */
  swkeybd_keyRelease ( key );     /* release */
  return 0;
}

/*
 * Name:        swkeybd_fakeKey
 *
 * Description: Handler for data sent in by the device. 
 *              The function is called by the USB kernel subsystem 
 *              whenever the device spits out new data
 *
 * @param c - char to send
 */
static void swkeybd_fakeKey ( char c )
{

  int do_shift = 0;
  swinput_debug ( "swkeybd: printing char '%c'\n", c );

  if ( ( c >= 'A' ) && ( c <= 'Z' ) )
    {
      do_shift = 1;
      c = c - 'A' + 'a';
    }

  if ( do_shift )
    {
      swkeybd_keyPress ( KEY_LEFTSHIFT );     /* keypress */
      swkeybd.shift_press++;
    }

  swkeybd_keyPress ( keycodes[( int ) c] );
  swkeybd.key_press++;
  swinput_debug ( "swkeybd: key_press = %d\n", swkeybd.key_press );

  swkeybd_keyRelease ( keycodes[( int ) c] );     /* release */
  swkeybd.key_release++;

  if ( do_shift )
    {
      swkeybd_keyRelease ( KEY_LEFTSHIFT );   /* keyrelease */
      swkeybd.shift_release++;
    }
}

/**
 * fake keypress from Key-identifier String 
 * @param str - Identifier string
 * @param str_length - length of string
 * @result 0 upon success
 */
int fake_esc ( char *str, int str_length )
{
  swinput_debug ( "fake: %s ", str );
  swinput_debug ( " %c", str[1] );
  swinput_debug ( " %c \n", str[2] );

  if ( str[0] == 'F' )
    {
      int key;

      if ( strncmp ( str, "F1", str_length ) == 0 )
	key = KEY_F1;
      if ( strncmp ( str, "F2", str_length ) == 0 )
	key = KEY_F2;
      if ( strncmp ( str, "F3", str_length ) == 0 )
	key = KEY_F3;
      if ( strncmp ( str, "F4", str_length ) == 0 )
	key = KEY_F4;
      if ( strncmp ( str, "F5", str_length ) == 0 )
	key = KEY_F5;
      if ( strncmp ( str, "F6", str_length ) == 0 )
	key = KEY_F6;
      if ( strncmp ( str, "F7", str_length ) == 0 )
	key = KEY_F7;
      if ( strncmp ( str, "F8", str_length ) == 0 )
	key = KEY_F8;
      if ( strncmp ( str, "F9", str_length ) == 0 )
	key = KEY_F9;
      else if ( strncmp ( str, "F10", str_length ) == 0 )
	key = KEY_F10;
      else if ( strncmp ( str, "F11", str_length ) == 0 )
	key = KEY_F11;
      else if ( strncmp ( str, "F12", str_length ) == 0 )
	key = KEY_F12;
      else if ( strncmp ( str, "F13", str_length ) == 0 )
	key = KEY_F13;
      else if ( strncmp ( str, "F14", str_length ) == 0 )
	key = KEY_F14;
      else if ( strncmp ( str, "F15", str_length ) == 0 )
	key = KEY_F15;
      else if ( strncmp ( str, "F16", str_length ) == 0 )
	key = KEY_F16;
      else if ( strncmp ( str, "F17", str_length ) == 0 )
	key = KEY_F17;
      else if ( strncmp ( str, "F18", str_length ) == 0 )
	key = KEY_F18;
      else if ( strncmp ( str, "F19", str_length ) == 0 )
	key = KEY_F19;
      else if ( strncmp ( str, "F20", str_length ) == 0 )
	key = KEY_F20;
      else
	key = 0;
      if ( key != 0 )
	{
	  swkeybd_key ( key );
	}
    }
  else if ( strncmp ( str, "BACKSPACE", str_length ) == 0 )
    {
      swinput_debugs ( " Fakeing backspace " );
      swkeybd_key ( KEY_BACKSPACE );
    }
  else if ( strncmp ( str, "ENTER", str_length ) == 0 )
    {
      swkeybd_key ( KEY_ENTER );
    }
  else if ( strncmp ( str, "SPACE", str_length ) == 0 )
    {
      swkeybd_key ( KEY_SPACE );
    }
  else if ( strncmp ( str, "COMMA", str_length ) == 0 )
    {
      swkeybd_key ( KEY_COMMA );
    }
  else if ( strncmp ( str, "DOT", str_length ) == 0 )
    {
      swkeybd_key ( KEY_DOT );
    }
  else if ( strncmp ( str, "KEY_LEFT", str_length ) == 0 )
    {
      swkeybd_key ( KEY_LEFT );
    }
  else if ( strncmp ( str, "KEY_RIGHT", str_length ) == 0 )
    {
      swkeybd_key ( KEY_RIGHT );
    }
  else if ( strncmp ( str, "KEY_DOWN", str_length ) == 0 )
    {
      swkeybd_key ( KEY_DOWN );
    }
  else if ( strncmp ( str, "KEY_UP", str_length ) == 0 )
    {
      swkeybd_key ( KEY_UP );
    }
  else if ( strncmp ( str, "CONTROL_DOWN", str_length ) == 0 )
    {
      swkeybd_key ( KEY_LEFTCTRL );
    }
  else if ( strncmp ( str, "CONTROL_UP", str_length ) == 0 )
    {
      swkeybd_key ( KEY_LEFTCTRL );
    }
  else if ( strncmp ( str, "CONTROL", str_length ) == 0 )
    {
      swkeybd_key ( KEY_LEFTCTRL );
    }
  else if ( strncmp ( str, "SHIFT_DOWN", str_length ) == 0 )
    {
      swkeybd_key ( KEY_LEFTSHIFT );
    }
  else if ( strncmp ( str, "SHIFT_UP", str_length ) == 0 )
    {
      swkeybd_key ( KEY_LEFTSHIFT );
    }
  else if ( strncmp ( str, "SHIFT", str_length ) == 0 )
    {
      swkeybd_key ( KEY_LEFTSHIFT );
    }
  else if ( strncmp ( str, "clear", str_length ) == 0 )
    {
      swkeybd.shift_press = 0;
      swkeybd.shift_release = 0;
      swkeybd.key_press = 0;
      swkeybd.key_release = 0;
    }

  return 0;
}

/*
 * Name:        swkeybd_poll
 *
 * Description: poll reports the device as writeable
 *
 */
unsigned int swkeybd_poll ( struct file *filp, struct poll_table_struct *table )
{
  return POLLWRNORM | POLLOUT;
}

/*
 * Name:        swkeybd_write
 *
 * Description: write accepts data and converts it to mouse movement
 *
 */
ssize_t swkeybd_write ( struct file * filp, const char *buf, size_t count,
                        loff_t * offp )
{

  static char localbuf[BUF_SIZE];
  static char escapebuf[BUF_SIZE];
  int i;
  int idx = 0;
  int found = 0;

  if ( count > BUF_SIZE )
    count = BUF_SIZE;
  if ( copy_from_user ( localbuf, buf, count ) != 0 )
    {
      printk ( KERN_INFO "swkeybd: copy_from_user() failed!\n" );

      /* silently ignore */
      return count;
    }

  /* scan written data */
  for ( i = 0; i < count; i++ )
    {

      if ( localbuf[i] == '\\' )
	{
	  i++;
	  swinput_debug ( "escape sequence : %c\n", localbuf[i] );
	  swkeybd_fakeKey ( localbuf[i] );
	}

      if ( localbuf[i] == '[' )
	{
	  swinput_debugs ( " [ found\n" );
	  found = 1;
	}
      else if ( localbuf[i] == ']' )
	{
	  found = 0;
	  escapebuf[idx] = '\0';
	  escapebuf[idx++] = '\0';
	  swinput_debugs ( " ]  found\n" );
	  swinput_debug ( "%s\n", escapebuf );
	  fake_esc ( escapebuf, BUF_SIZE );
	  idx = 0;
	}
      else
	{
	  if ( found )
	    {
	      swinput_debug ( "adding %c \n", localbuf[i] );
	      swinput_debug ( "%d\n", idx );
	      escapebuf[idx++] = localbuf[i];
	    }
	  else
	    {
	      swkeybd_fakeKey ( localbuf[i] );
	    }
	}
    }

  escapebuf[0] = '\0';
  localbuf[0] = '\0';
  return count;
}

/*
 * Name:        set_keycodes
 *
 * Description: 
 *
 */
void set_keycodes ( int in, int out )
{
  keycodes[in] = out;
  set_bit ( out, swkeybd.idev->keybit );
}

/*
 * Name:        init_keycodes
 *
 * Description: 
 *
 */
void init_keycodes ( void )
{
  memset ( keycodes, 0, 512 );

  /* letters */
  set_keycodes ( 'a', KEY_A );
  set_keycodes ( 'b', KEY_B );
  set_keycodes ( 'c', KEY_C );
  set_keycodes ( 'd', KEY_D );
  set_keycodes ( 'e', KEY_E );
  set_keycodes ( 'f', KEY_F );
  set_keycodes ( 'g', KEY_G );
  set_keycodes ( 'h', KEY_H );
  set_keycodes ( 'i', KEY_I );
  set_keycodes ( 'j', KEY_J );
  set_keycodes ( 'k', KEY_K );
  set_keycodes ( 'l', KEY_L );
  set_keycodes ( 'o', KEY_O );
  set_keycodes ( 'm', KEY_M );
  set_keycodes ( 'n', KEY_N );
  set_keycodes ( 'p', KEY_P );
  set_keycodes ( 'q', KEY_Q );
  set_keycodes ( 'r', KEY_R );
  set_keycodes ( 's', KEY_S );
  set_keycodes ( 't', KEY_T );
  set_keycodes ( 'u', KEY_U );
  set_keycodes ( 'v', KEY_V );
  set_keycodes ( 'x', KEY_X );
  set_keycodes ( 'y', KEY_Y );
  set_keycodes ( 'z', KEY_Z );

  /* digits */
  set_keycodes ( '0', KEY_0 );
  set_keycodes ( '1', KEY_1 );
  set_keycodes ( '2', KEY_2 );
  set_keycodes ( '3', KEY_3 );
  set_keycodes ( '4', KEY_4 );
  set_keycodes ( '5', KEY_5 );
  set_keycodes ( '6', KEY_6 );
  set_keycodes ( '7', KEY_7 );
  set_keycodes ( '8', KEY_8 );
  set_keycodes ( '9', KEY_9 );

  /* specials ..... */
  set_keycodes ( ':', KEY_SEMICOLON );
  set_keycodes ( ' ', KEY_SPACE );
  set_keycodes ( '-', KEY_KPMINUS );
  set_keycodes ( '+', KEY_KPPLUS );
  set_keycodes ( '=', KEY_EQUAL );
  set_keycodes ( ',', KEY_COMMA );
  set_keycodes ( '.', KEY_DOT );

  /* just set them.... */
  set_bit ( KEY_SEMICOLON, swkeybd.idev->keybit );
  set_bit ( KEY_SPACE, swkeybd.idev->keybit );
  set_bit ( KEY_LEFTBRACE, swkeybd.idev->keybit );
  set_bit ( KEY_RIGHTBRACE, swkeybd.idev->keybit );
  set_bit ( KEY_BACKSPACE, swkeybd.idev->keybit );

  set_bit ( KEY_ENTER, swkeybd.idev->keybit );
  set_bit ( KEY_ESC, swkeybd.idev->keybit );
  set_bit ( KEY_TAB, swkeybd.idev->keybit );

  set_bit ( KEY_LEFTSHIFT, swkeybd.idev->keybit );
  set_bit ( KEY_RIGHTSHIFT, swkeybd.idev->keybit );
  set_bit ( KEY_LEFTCTRL, swkeybd.idev->keybit );
  set_bit ( KEY_RIGHTCTRL, swkeybd.idev->keybit );
  set_bit ( KEY_LEFTALT, swkeybd.idev->keybit );

  set_bit ( KEY_CAPSLOCK, swkeybd.idev->keybit );
  set_bit ( KEY_APOSTROPHE, swkeybd.idev->keybit );
  set_bit ( KEY_GRAVE, swkeybd.idev->keybit );
  set_bit ( KEY_SLASH, swkeybd.idev->keybit );

  set_bit ( KEY_F1, swkeybd.idev->keybit );
  set_bit ( KEY_F2, swkeybd.idev->keybit );
  set_bit ( KEY_F3, swkeybd.idev->keybit );
  set_bit ( KEY_F4, swkeybd.idev->keybit );
  set_bit ( KEY_F5, swkeybd.idev->keybit );
  set_bit ( KEY_F6, swkeybd.idev->keybit );
  set_bit ( KEY_F7, swkeybd.idev->keybit );
  set_bit ( KEY_F8, swkeybd.idev->keybit );
  set_bit ( KEY_F9, swkeybd.idev->keybit );
  set_bit ( KEY_F10, swkeybd.idev->keybit );
  set_bit ( KEY_F11, swkeybd.idev->keybit );
  set_bit ( KEY_F12, swkeybd.idev->keybit );
  set_bit ( KEY_F13, swkeybd.idev->keybit );
  set_bit ( KEY_F14, swkeybd.idev->keybit );
  set_bit ( KEY_F15, swkeybd.idev->keybit );
  set_bit ( KEY_F16, swkeybd.idev->keybit );
  set_bit ( KEY_F17, swkeybd.idev->keybit );
  set_bit ( KEY_F18, swkeybd.idev->keybit );
  set_bit ( KEY_F19, swkeybd.idev->keybit );
  set_bit ( KEY_F20, swkeybd.idev->keybit );

  set_bit ( KEY_LEFT, swkeybd.idev->keybit );
  set_bit ( KEY_RIGHT, swkeybd.idev->keybit );
  set_bit ( KEY_UP, swkeybd.idev->keybit );
  set_bit ( KEY_DOWN, swkeybd.idev->keybit );

  set_bit ( KEY_LEFTBRACE, swkeybd.idev->keybit );
  set_bit ( KEY_RIGHTBRACE, swkeybd.idev->keybit );
}

/* file operation-handlers for /dev/swkeybd */
struct file_operations swkeybd_file_operations = {
 write:swkeybd_write,
 poll:swkeybd_poll,
 open:swkeybd_open,
 release:swkeybd_release,
};
