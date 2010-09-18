/*****
 *       swinput 
 *                                                                   
 *  swmouse.c, fakes a mouse using Linux Input System
 *     heavily based on
 *         idiom.c 
 *     which is heavily based on on usbmouse.c
 *
 *  Copyright (c) 2003-2007 Henrik Sandklef <hesa@gnu.org> 
 *                2008 Henrik Sandklef <hesa@gnu.org>, 
 *                     Daniel Hiepler <rigid@boogiepalace.hopto.org>
 *                2010 Henrik Sandklef <hesa@gnu.org>
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

#include "swinput.h"

MODULE_DESCRIPTION ( "Fake-mouse input device" );
MODULE_AUTHOR ( "Henrik Sandklef  <hesa@gnu.org>" );
MODULE_LICENSE ( "GPL" );

#define MODULE_NAME "swmouse"
#define XMAX 1280
#define YMAX 800

/* debugging macro */
//#ifdef SWINPUT_DEBUG
#define debug(...) if(log > 1) logger(1,MODULE_NAME, __func__, __LINE__, __VA_ARGS__)
#define verbose(...) if(log > 0) logger(0,MODULE_NAME, __func__, __LINE__, __VA_ARGS__)
//#endif

/* screen-resolutions */
static int xmax, ymax, devs, nrofmice;
static int log = 1;

/* module parameters */
module_param ( xmax, int, 0444 );
module_param ( ymax, int, 0444 );
module_param ( devs, int, 0444 );
module_param ( log, int, 0444 );
module_param ( nrofmice, int, 0444 );
MODULE_PARM_DESC ( xmax, "nominal screen-width (default 1280)" );
MODULE_PARM_DESC ( ymax, "nominal screen-height (default 800)" );
MODULE_PARM_DESC ( devs, "how many mice to emulate (maximum 16, default 1)" );
MODULE_PARM_DESC ( log, "0=quiet, 1=verbose, 2=debug (default 1)" );
MODULE_PARM_DESC ( nrofmice, "Define how many mice you want to use (defaults to 10)" );

struct file_operations swmouse_file_operations;
int swm_read_procmem ( char *buf, char **start, off_t offset,
                       int count, int *eof, void *data );
void cleanup_devices ( int dev );


struct swmouse_device
{
        /* input device, to push out input  data */
        struct input_dev *idev;
        int misc_reg, input_reg;

        /* statistic counters */
        int fixed_x;
        int fixed_y;
        int ups;
        int downs;
        int lefts;
        int rights;
        int buttons;
} swmouse[MAX_DEVNUM];

struct miscdevice swmouse_misc[MAX_DEVNUM];

/**
 * Name:        init_module
 *
 * Description: invoked when inserting the module
 *
 */
int init_module ( void )
{
        int retval = -1;
        int dev;
        char *name;

        verbose ( "initializing...\n" );

        /* amount of devices to emulate */
        if ( devs == 0 )
                devs = 1;

        if ( devs > MAX_DEVNUM )
        {
                verbose ( "a maximum of %d devices are supported -\n"
                          "swmouse: recompile to increase that.\n",
                          MAX_DEVNUM );
                devs = MAX_DEVNUM;
        }

        /* boundaries */
        if ( xmax == 0 )
                xmax = XMAX;
        if ( ymax == 0 )
                ymax = YMAX;

        if ( xmax < 0 )
        {
                verbose ( "xmax must be > 0. Using default.\n" );
                xmax = XMAX;
        }

        if ( ymax < 0 )
        {
                verbose ( "ymax must be > 0. Using default.\n" );
                ymax = YMAX;
        }

        /* initialize every device */
        for ( dev = 0; dev < devs; dev++ )
        {

                /* clear memory of local structure */
                memset ( &swmouse[dev], 0, sizeof ( struct swmouse_device ) );
                memset ( &swmouse_misc[dev], 0, sizeof ( struct miscdevice ) );

                ( &swmouse[dev] )->idev = input_allocate_device (  );

                if ( swmouse[dev].idev == NULL )
                {
                        debug ( "failed to allocate input device\n" );
                        retval = -EFAULT;
                        goto im_error;
                }

                /* initialize misc-device structure */
                ( &swmouse_misc[dev] )->minor = MISC_DYNAMIC_MINOR;
                ( &swmouse_misc[dev] )->fops = &swmouse_file_operations;

                /* build device name */
                if ( ( name =
                       kmalloc ( GFP_KERNEL,
                                 sizeof ( MODULE_NAME ) + 2 ) ) == NULL )
                {
                        debug ( "failed to allocate memory\n" );
                        retval = -ENOMEM;
                        goto im_error;
                }

                ( &swmouse_misc[dev] )->name = name;

                snprintf ( name, sizeof ( MODULE_NAME ) + 2, "%s%d",
                           MODULE_NAME, dev );

                /* 
                 * register our faker as a misc device
                 */
                retval = misc_register ( &swmouse_misc[dev] );
                if ( retval != 0 )
                {
                        /* return if failure ... */
                        debug ( "failed to register the swmouse as a misc device\n" );
                        goto im_error;
                }

                ( &swmouse[dev] )->misc_reg = 1;

                /* set the name */
                ( &swmouse[dev] )->idev->name = "swinput faked mouse device";
                ( &swmouse[dev] )->idev->id.vendor = 0x00;
                ( &swmouse[dev] )->idev->id.product = 0x00;
                ( &swmouse[dev] )->idev->id.version = 0x00;

                /*( &swmouse[dev] )->idev->open = swm_open_simple;
                ( &swmouse[dev] )->idev->close = swm_release_simple;*/

                /* set event-bits */
		if (0)
		  {
                set_bit ( EV_KEY, ( &swmouse[dev] )->idev->evbit );
                set_bit ( EV_REL, ( &swmouse[dev] )->idev->evbit );
                set_bit ( EV_ABS, ( &swmouse[dev] )->idev->evbit );
		  }
		( &swmouse[dev] )->idev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;

                set_bit ( REL_X, ( &swmouse[dev] )->idev->relbit );
                set_bit ( REL_Y, ( &swmouse[dev] )->idev->relbit );

		if (0)
		  {
                set_bit ( ABS_X, ( &swmouse[dev] )->idev->relbit );
                set_bit ( ABS_Y, ( &swmouse[dev] )->idev->relbit );
		  }


                /* set bits for mouse-buttons */
                set_bit ( BTN_LEFT, ( &swmouse[dev] )->idev->keybit );
                set_bit ( BTN_MIDDLE, ( &swmouse[dev] )->idev->keybit );
                set_bit ( BTN_RIGHT, ( &swmouse[dev] )->idev->keybit );
                
                
		printk ("Using abs %d %d %d %d %d\n",ABS_X, 0,
                                       xmax, 0, 0 ); 
                input_set_abs_params ( (&swmouse[dev])->idev, ABS_X, 0,
                                       xmax, 4, 8 );

                input_set_abs_params ( (&swmouse[dev])->idev, ABS_Y, 0,
                                       ymax, 4, 8 );


                /* register the device to the input system */
                if ( input_register_device ( ( &swmouse[dev] )->idev ) )
                {
                        debug ( "Unable to register input device!\n" );
                        retval = -EFAULT;
                        goto im_error;
                }

                ( &swmouse[dev] )->input_reg = 1;

        }

        /* create the /proc entry */
        if ( create_proc_read_entry ( MODULE_NAME, 0 /* default mode */ ,
                                      NULL /* parent dir */ ,
                                      swm_read_procmem,
                                      NULL /* client data */  ) ==
             NULL )
        {
                debug ( "failed to create proc entry\n" );
        }

        verbose ( "module loaded (xmax: %d, ymax: %d, devs: %d)\n",
                  xmax, ymax, devs );

        return retval;

      im_error:
        cleanup_devices ( dev );
        return retval;
}

/**
 * Name:        cleanup_devices
 *
 * Description: clean up all devices in swmouse[] and swmouse_misc[]
 *
 */
void cleanup_devices ( int dev )
{
        int c;

        for ( c = 0; c < dev; c++ )
        {

                if ( ( &swmouse[c] )->input_reg )
                {
                        debug ( "unregistering input-device\n" );
                        input_unregister_device ( ( &swmouse[c] )->idev );
                }
                if ( ( &swmouse[c] )->idev )
                {
                        debug ( "freeing input-device\n" );
                        input_free_device ( ( &swmouse[c] )->idev );
                }
                if ( ( &swmouse[c] )->misc_reg )
                {
                        debug ( "deregistering misc-device\n" );
                        misc_deregister ( ( &swmouse_misc[c] ) );
                }
                if ( swmouse_misc[c].name )
                        kfree ( ( &swmouse_misc[c] )->name );
        }
}

/**
 * Name:        cleanup_module
 *
 * Description: invoked when removing the module
 *
 */
void cleanup_module ( void )
{

        cleanup_devices ( devs );
        remove_proc_entry ( MODULE_NAME, NULL /* parent dir */  );
        printk ( KERN_INFO "swmouse: module unregistered\n" );

}


/**
 * Name:        read_procmem
 *
 * Description: invoked when reading from /proc/swmouse
 *
 */
int swm_read_procmem ( char *buf, char **start, off_t offset,
                       int count, int *eof, void *data )
{
        static char internal_buf[128 * MAX_DEVNUM];
        char *tmp = ( char * ) &internal_buf;
        int len, all = 0;
        int dev;

        if ( offset > 0 )
        {
                /* we have finished to read, return 0 */
                return 0;
        }

        /* build statistics for all devices */
        for ( dev = 0; dev < devs; dev++ )
        {

                len = snprintf ( tmp, ( 128 * MAX_DEVNUM ) - all,
                                 "swmouse%d:%d;%d;%d;%d,%d,%d\n", dev,
                                 swmouse[dev].ups, swmouse[dev].downs,
                                 swmouse[dev].lefts, swmouse[dev].rights,
                                 swmouse[dev].fixed_x, swmouse[dev].fixed_y );
                tmp += len;
                all += len;
        }

        /* more than we can take? */
        if ( all > sizeof ( internal_buf ) )
	  {
	    all = sizeof ( internal_buf );
	  }

        /* less requested than we got? */
        if ( count < all )
	  {
	    all = count;
	  }

        /* copy buffer */
        memcpy ( buf, internal_buf, all );

        return all;
}

/**
 * Name:        devFromName
 *
 * Description: get current device-number from filp
 *
 */
int swm_devFromName(struct file * filp)
{
        int dev;
        const char *name;
        
        name = ( const char * ) ( filp->f_path.dentry->d_name.name );
        
        /* guess device from name */
        if ( sscanf ( name, MODULE_NAME "%d", &dev ) == 0 )
        {
                debug ( "unknown device: \"%s\"\n", name );
                return -1;
        }
        
        return dev;
}
/**
 * Name:        open
 *
 * Description: invoked when fake-device is opened
 *
 */
int swm_open ( struct inode *inode, struct file *filp )
{
        /*printk ( KERN_INFO "swmouse: open\n" ); */
        /* Ok */
        return 0;
}

/**
 * Name:        release
 *
 * Description: invoked when fake-device is released
 *
 */
int swm_release ( struct inode *inode, struct file *filp )
{
        input_sync ( ( &swmouse[swm_devFromName(filp)] )->idev );
        /*printk ( KERN_INFO "swmouse: releas\n" ); */
        return 0;
}


/**
 * Name:        write
 *
 * Description: write accepts data and converts it to mouse movement
 *
 */
ssize_t swm_write ( struct file * filp, const char *buf, size_t count,
                    loff_t * offp )
{
#define BUF_SIZE 32
        static char localbuf[BUF_SIZE];
        char letter;
        char *tmp;
        const char *name;
        int nrs;
        int nrs_2;
        int direction = -1;
        int is_abs = 0;
        int pix = 0;
        int pix_x = 0;
        int pix_y = 0;
        int dev = 0;
        int button = 0;
        int button_state = 0;

        if ( count == 0 )
        {
                return 0;
        }

        name = ( const char * ) ( filp->f_path.dentry->d_name.name );

        if((dev = swm_devFromName(filp)) < 0)
                return count;

        /* accept BUF_SIZE bytes at a time, at most */
        if ( count > BUF_SIZE )
                count = BUF_SIZE;

        if ( copy_from_user ( localbuf, buf, count ) != 0 )
        {
                /* copy_from_user() failed */
                debug ( "swmouse%d - copy_from_user() failed!\n", dev );

                /* silently ignore */
                return count;
        }

        tmp = &localbuf[0];
        if ( tmp == NULL )
        {
                /* Strange case, silently ignore it :(   */
                return count;
        }

        input_sync ( ( &swmouse[dev] )->idev );
        
        /* Remove leading blanks ... */
        while ( ( tmp != NULL ) && ( tmp[0] == ' ' ) )
        {
                tmp++;
        }

        /* Save char to get direction later on */
        letter = tmp[0];

        /* Go to next character */
        if ( tmp != NULL )
        {
                tmp++;
        }

        /* Remove leading blanks ... */
        while ( ( tmp != NULL ) && ( tmp[0] == ' ' ) )
        {
                tmp++;
        }

        /* Remove "=" if any */
        if ( ( tmp != NULL ) && ( tmp[0] == '=' ) )
        {
                tmp++;
        }

        if ( !sscanf ( tmp, "%d %d", &nrs, &nrs_2 ) < 0 )
        {
                debug ( "swmouse%d - problems converting %s (tmp=%s   nrs=%d nrs-2=%d)\n", 
			dev, localbuf, tmp, nrs , nrs_2 );
                return count;
        }

        switch ( letter )
        {
                /* button press */
        case 'b':
                if ( ( nrs < 0 ) || ( nrs > 3 ) )
                        break;
                button = nrs;
                button_state = 1;
                break;

                /* button release */
        case 'B':
                if ( ( nrs < 0 ) || ( nrs > 3 ) )
                        break;
                button = nrs;
                button_state = 0;
                break;

                /* up */
        case 'u':
        case 'U':
                if ( ( nrs <= 0 ) || ( nrs > ymax ) )
                        nrs = 1;
                ( &swmouse[dev] )->ups += nrs;
                direction = REL_Y;
                pix = 0 - nrs;
                break;

                /* down */
        case 'd':
        case 'D':
                if ( ( nrs <= 0 ) || ( nrs > ymax ) )
                        nrs = 1;
                ( &swmouse[dev] )->downs += nrs;
                direction = REL_Y;
                pix = nrs;
                break;

                /* left */
        case 'l':
        case 'L':
                if ( ( nrs <= 0 ) || ( nrs > xmax ) )
                        nrs = 1;
                ( &swmouse[dev] )->lefts += nrs;
                direction = REL_X;
                pix = 0 - nrs;
                break;

                /* right */
        case 'r':
        case 'R':
                if ( ( nrs <= 0 ) || ( nrs > xmax ) )
                        nrs = 1;
                ( &swmouse[dev] )->rights += nrs;
                direction = REL_X;
                pix = nrs;
                break;

                /* absolute x position */
        case 'x':
        case 'X':
	  
                if ( ( nrs <= 0 ) || ( nrs > xmax ) )
                        nrs = 1;
                ( &swmouse[dev] )->fixed_x++;
                is_abs = 1;
                direction = ABS_X;
                pix_x = nrs;
		printk ( KERN_INFO "swmouse: abs pos (X) %d\n", pix );
                break;

                /* absolute y position */
        case 'y':
        case 'Y':
                if ( ( nrs <= 0 ) || ( nrs > ymax ) )
                        nrs = 1;
                ( &swmouse[dev] )->fixed_y++;
                is_abs = 1;
                direction = ABS_Y;
                pix_y = nrs;
                break;

                /* home */
        case 'a':
        case 'A':
                if ( ( nrs <= 0 ) || ( nrs > ymax ) )
                        nrs = 1;
                ( &swmouse[dev] )->fixed_y++;
                is_abs = 1;
                direction = ABS_Y;
                pix_x = nrs;
                pix_y = nrs_2;
                break;

                /* home */
        case '0':
                ( &swmouse[dev] )->fixed_x = 0;
                ( &swmouse[dev] )->fixed_y = 0;
                ( &swmouse[dev] )->rights = 0;
                ( &swmouse[dev] )->lefts = 0;
                ( &swmouse[dev] )->downs = 0;
                ( &swmouse[dev] )->ups = 0;
                pix = 0;
                break;

        default:
                debug ( "unknown token\n" );
                ;
        }

        if ( button )
        {

                /* button press? */
                switch ( button )
                {
                case 0:
                        break;

                        /* left mousebutton */
                case 1:
                        button = BTN_LEFT;
                        break;
                        /* middle mousebutton */
                case 2:
                        button = BTN_MIDDLE;
                        break;
                        /* right mousebutton */
                case 3:
                        button = BTN_RIGHT;
                        break;
                default:
                        break;
                }

                verbose ( "swmouse%d - input_report_key(%d,%d)\n",
                          dev, button, button_state );

                input_report_key ( ( &swmouse[dev] )->idev,
                                   button, button_state );
                input_sync ( ( &swmouse[dev] )->idev );

        }

        /* done a valid movement? */
        if ( direction >= 0 )
        {
	  printk ( KERN_INFO "swmouse: about to move ... %d\n", is_abs );
                
                /* absolute movement ... */
                if ( is_abs !=0 )
                {
                        verbose ( "swmouse%d - input_report_abs(%d,%d  ) \n", dev,
                                  pix_x,pix_y );

                        verbose ( "swmouse%d: %d - %d ) \n", 
				  dev, 
				  ( &swmouse[dev] )->idev->absmin[ABS_X] ,
				  ( &swmouse[dev] )->idev->absmax[ABS_X]);
			/*                        input_report_abs ( ( &swmouse[dev] )->idev, 
					   direction,
                                           0 );
			*/

                        input_report_abs ( ( &swmouse[dev] )->idev, 
					   ABS_X,
                                           pix_x );
                        input_report_abs ( ( &swmouse[dev] )->idev, 
					   ABS_Y,
                                           pix_y );
			input_sync ( ( &swmouse[dev] )->idev );
                }
        
                /* relative movement ... */
                else
                {
                        verbose ( "swmouse%d - input_report_rel(%d,%d)\n", dev,
                                  direction, pix );
                        input_report_rel ( ( &swmouse[dev] )->idev, direction,
                                           pix );
                }
        
        }

        input_sync ( ( &swmouse[dev] )->idev );
        return count;
}

/* functions to define how to perform 
   various operations on the device */
struct file_operations swmouse_file_operations = {
        .owner = THIS_MODULE,
        .write = swm_write,
        .open = swm_open,
        .release = swm_release,
};

/* Test function:
     xy() {   echo "x $1" > /dev/swmouse ; echo "y $2" > /dev/swmouse ; sleep 2 ; XPOS=$(/home/hesa/gnu/xnee/cnee/test/src/xgetter  -mprx) ; YPOS=$( /home/hesa/gnu/xnee/cnee/test/src/xgetter  -mpry) ; echo "${1}x${2} ==> ${XPOS}x${YPOS}" ; }
   

   xy 1 2 
*/
