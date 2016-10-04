/*
 * xchown.c: chown replacement function
 *
 * Copyright (C) 2016 Mihail Konev.
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
 * */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>

#include "gettext.h"
#define _(String) gettext (String)

#include "error.h"
#include "manconfig.h"

void xchown (const char *path, uid_t owner, gid_t group)
{
	int rc;
	rc = chown (path, owner, group);
	if (rc)
		error (FATAL, 0, _("can't chown %s"), path);
}
