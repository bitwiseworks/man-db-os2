/* Preserve bytes on OS/2 streams without changing the process default.
 * Distributed under the GNU General Public License, version 2 or later.
 */

#ifndef MAN_BINARY_IO_H
#define MAN_BINARY_IO_H

#ifdef __OS2__
#include <fcntl.h>
#include <io.h>
#include <errno.h>
#include "error.h"

static void binary_io (int fd)
{
	if (setmode (fd, O_BINARY) == -1)
		error (FATAL, errno, "can't set binary mode on descriptor %d", fd);
}
#else
#define binary_io(fd) ((void) 0)
#endif

#endif /* MAN_BINARY_IO_H */
