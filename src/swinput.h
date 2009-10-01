#ifndef _SWINPUT_H
#define _SWINPUT_H


void logger(int detail, char *progname, const char *func, int line, char *logmsg, ... );

/*
 * Simple debug macros, switch them on by defining SWINPUT_DEBUG
 *
 */
 #define SWINPUT_DEBUG 

#ifdef SWINPUT_DEBUG
#define swinput_debugs(str) printk("swinput: %s():%u: %s", __func__, __LINE__, str)
#define swinput_debug(fmt, x) printk("swinput: %s():%u: %s=" fmt, __func__, __LINE__, #x, x)
#else
#define swinput_debugs(str)
#define swinput_debug(fmt, x)
#endif

/* maximum amount of devices */
#define MAX_DEVNUM 16

#endif /* _SWINPUT_H */
