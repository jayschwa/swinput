

#include <linux/kernel.h>
#include "swinput.h"

#define STATE_INVALID           -1
#define STATE_IDLE              0
#define STATE_PARSE_TOKEN       1
#define STATE_PARSE_ARG         2

static struct swi_parser
{
        int state;
} parser;

/**
 * Name:        stateSet
 *
 * Description: save current state
 * 
 * @param state - state to set
 * @result 0 if state was set, <0 on invalid state-transition
 *
 */
void parser_stateSet ( int state )
{
        /* set state */
        parser.state = state;
}

/**
 * Name:        stateGet
 *
 * Description: returns the current state
 *
 */
int parser_stateGet ( void )
{
        return parser.state;
}


/**
 * Name:        logger
 *
 * Description: wrapper-function for printk (max. 1024 chars at all)
 *
 */
void logger(int detail, char *progname, const char *func, int line, char *logmsg, ... )
{
	static char buf[1024] = { 0 };
	va_list ap = NULL;
	
	
        /* build logmsg */
        va_start(ap, logmsg);
        vsnprintf(buf, 1024, logmsg, ap);
        va_end(ap);
                        
        if(detail)
                printk(KERN_INFO "%s: %s():%u: %s", progname, func, line, (char *) &buf);
        else
                printk(KERN_INFO "%s: %s", progname, (char *) &buf);
        
}
