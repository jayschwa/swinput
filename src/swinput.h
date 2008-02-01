#ifndef _SWINPUT_H
#define _SWINPUT_H

/*
 * Simple debug macros, switch them on by defining SWINPUT_DEBUG
 *
 */
/* #define SWINPUT_DEBUG */

#ifdef SWINPUT_DEBUG
#define swinput_debugs(str) printk("%s:%u: %s", __FILE__, __LINE__, str)
#define swinput_debug(fmt, x) printk("%s:%u: %s=" fmt, __FILE__, __LINE__, #x, x)
#else
#define swinput_debugs(str) 
#define swinput_debug(fmt, x) 
#endif

#endif /* _SWINPUT_H */
