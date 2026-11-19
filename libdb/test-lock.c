/* Exercise advisory database locking, including the OS/2 fcntl path.
 * Distributed under the GNU Library General Public License, version 2 or
 * (at your option) any later version.  See db_btree.c for license details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* Exercise the OS/2 implementation on other build hosts as well. */
#ifndef __OS2__
#define __OS2__
#endif
#include "db_flock.h"

static void check (int success, const char *message)
{
	if (!success) {
		fprintf (stderr, "%s (errno %d)\n", message, errno);
		exit (1);
	}
}

static void contender (const char *name, int operation, int allowed)
{
	pid_t pid = fork ();
	int status;

	check (pid >= 0, "fork");
	if (pid == 0) {
		int fd = open (name, O_RDWR);
		int result;

		check (fd >= 0, "open contender");
		result = db_flock (fd, operation | LOCK_NB);
		if (allowed)
			check (result == 0, "compatible lock rejected");
		else
			check (result == -1 &&
			       (errno == EACCES || errno == EAGAIN ||
				errno == EWOULDBLOCK),
			       "conflicting lock accepted");
		close (fd);
		_exit (0);
	}
	check (waitpid (pid, &status, 0) == pid, "waitpid");
	check (WIFEXITED (status) && WEXITSTATUS (status) == 0,
	       "contender failed");
}

int main (void)
{
	char name[] = "test-lock-XXXXXX";
	char buffer[512] = { 0 };
	int fd = mkstemp (name);
	int other;

	check (fd >= 0, "mkstemp");
	check (write (fd, buffer, sizeof buffer) == sizeof buffer, "write");
	check (db_flock (fd, LOCK_EX | LOCK_NB) == 0, "exclusive lock");

	/* Match the lock-before-dbopen sequence: the second handle must be
	 * able to read the database header despite the existing lock.
	 */
	other = open (name, O_RDWR);
	check (other >= 0, "reopen locked database");
	check (read (other, buffer, sizeof buffer) == sizeof buffer,
	       "read locked database header");
	check (write (other, buffer, sizeof buffer) == sizeof buffer,
	       "write locked database");
	contender (name, LOCK_EX, 0);
	contender (name, LOCK_SH, 0);
	check (db_flock (fd, LOCK_UN) == 0, "unlock exclusive lock");
	contender (name, LOCK_EX, 1);
	check (db_flock (fd, LOCK_SH | LOCK_NB) == 0, "shared lock");
	contender (name, LOCK_SH, 1);
	contender (name, LOCK_EX, 0);
	check (db_flock (fd, LOCK_UN) == 0, "unlock shared lock");
	close (other);
	close (fd);
	check (unlink (name) == 0, "unlink");
	return 0;
}
