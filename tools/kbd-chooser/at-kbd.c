/* @file  at-kbd.c
 * @brief Report keyboards present on PC
 *
 * Copyright (C) 2002,2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: at-kbd.c,v 1.14 2004/03/13 09:17:19 mckinstry Exp $
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"


/**
 * @brief list of keyboards present
 */
kbd_t *at_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = NULL;

#if defined(__m68k__)
	// on m68k only mvme and bvme have PC-style keyboards
	if (strstr(subarch, "vme") != NULL)
		return keyboards;
#endif
#if defined(__powerpc__)
	// On powerpc, prep and chrp machines have pc keyboards
	if ((strstr(subarch, "prep") == NULL) &&
	    (strstr(subarch, "chrp") == NULL))
		return keyboards;
#endif

	k =  xmalloc (sizeof(kbd_t));	
	k->name = "at"; // This must match the name "at" in console-keymaps-at
	k->deflt = NULL;
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;


#if defined (KERNEL_2_6)
	// /proc must be mounted by this point
	assert (check_dir ("/proc") == 1);

	/* In 2.6 series, we can detect keyboard via /proc/bus/input
	 *
	 */
	if (di_check_dir ("/proc/bus/input") >= 0) {
	        // this dir only present in 2.6
		res_at = grep ("/proc/bus/input/devices","AT Set ");
		if (res < 0) {
			di_warning ("at-kbd: Failed to open /proc/bus/input/devices");
			return keyboards;
		}
		k->present = ( res == 0) ? TRUE : FALSE;
		return keyboards;
	}

#endif // KERNEL_2_6

	/* ***  Only reached if KERNEL_2_6 not present ***  */

	/* For 2.6, assume a PC keyboard is present
	 */
	return keyboards;
}
