/* tempfile.c: handle temporary directory creation (formerly also temporary
 * files but this is no longer used).
 *
 * Copyright (C) 2001, 2003, 2007 Colin Watson <cjwatson@debian.org>.
 *
 * This file is part of man-db.
 *
 * man-db is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * man-db is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with man-db; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#ifdef STDC_HEADERS
#  include <stdlib.h>
#endif /* STDC_HEADERS */
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "manconfig.h"

/* Other library functions used in man-db. */
extern int mkstemp (char *template);
extern char *mkdtemp (char *template);

static const char *path_search (void)
{
	const char *dir = NULL;

	if (getuid () == geteuid () && getgid () == getegid ()) {
		dir = getenv ("TMPDIR");
		if (!dir || access (dir, W_OK) == -1)
			dir = NULL;
		if (!dir) {
			dir = getenv ("TMP");
			if (!dir || access (dir, W_OK) == -1)
				dir = NULL;
		}
	}
#ifdef P_tmpdir
	if (!dir) {
		dir = P_tmpdir;
		if (!dir || access (dir, W_OK) == -1)
			dir = NULL;
	}
#endif
	if (!dir) {
		dir = "/tmp";
		if (access (dir, W_OK) == -1)
			dir = NULL;
	}

	return dir;
}

/* Get a sane temporary directory, looking in $TMPDIR, P_tmpdir, and finally
 * /tmp.
 */
char *create_tempdir (const char *template)
{
	char *dir = xstrdup (path_search ());
	char *created_dirname;

	if (!dir)
		return NULL;
	created_dirname = appendstr (dir, "/", template, "XXXXXX", NULL);
	mkdtemp (created_dirname);
	return created_dirname;
}
