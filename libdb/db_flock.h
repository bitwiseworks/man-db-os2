/* Advisory locking for the Berkeley DB and ndbm interfaces.
 * Distributed under the GNU Library General Public License, version 2 or
 * (at your option) any later version.  See db_btree.c for license details.
 */

#ifndef DB_FLOCK_H
#define DB_FLOCK_H

#include <sys/file.h>

#ifdef __OS2__
#include <fcntl.h>
#include <unistd.h>

/* kLIBC's flock calls its own fcntl implementation, which uses mandatory
 * OS/2 byte-range locks.  In particular, locking before dbopen prevents
 * Berkeley DB from reading through its separately opened handle.  Call
 * fcntl directly so that libcx (required when linking on OS/2) supplies
 * advisory locks instead.
 */
static int db_flock (int fd, int operation)
{
	struct flock lock = { 0 };

	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;		/* Lock through EOF, including future growth. */
	if (operation & LOCK_UN)
		lock.l_type = F_UNLCK;
	else if (operation & LOCK_EX)
		lock.l_type = F_WRLCK;
	else
		lock.l_type = F_RDLCK;

	return fcntl (fd, (operation & LOCK_NB) ? F_SETLK : F_SETLKW,
		      &lock);
}
#else
#define db_flock flock
#endif

#endif /* DB_FLOCK_H */
