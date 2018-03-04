/* src/port/strerror.c */

/*
 * strerror - map error number to descriptive string
 *
 * This version is obviously somewhat Unix-specific.
 *
 * based on code by Henry Spencer
 * modified for ANSI by D'Arcy J.M. Cain
 */

#include "c.h"


extern THREAD_LOCAL const char *const sys_errlist[];
extern THREAD_LOCAL int	sys_nerr;

const char *
strerror(int errnum)
{
	static THREAD_LOCAL char buf[24];

	if (errnum < 0 || errnum > sys_nerr)
	{
		sprintf(buf, _("unrecognized error %d"), errnum);
		return buf;
	}

	return sys_errlist[errnum];
}
