/*
 * decompress.c: decompression abstraction layer
 *
 * Copyright (C) 2007, 2008 Colin Watson.
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_LIBZ
#  include "zlib.h"
#endif /* HAVE_LIBZ */

#include "xvasprintf.h"

#include "manconfig.h"
#include "binary_io.h"
#include "comp_src.h"
#include "pipeline.h"
#include "decompress.h"

#ifdef HAVE_LIBZ

static void decompress_zlib (void *data ATTRIBUTE_UNUSED)
{
	gzFile zlibfile;

	zlibfile = gzdopen (dup (STDIN_FILENO), "rb");
	if (!zlibfile)
		return;

	for (;;) {
		char buffer[4096];
		int r = gzread (zlibfile, buffer, 4096);
		if (r <= 0)
			break;
		if (fwrite (buffer, 1, (size_t) r, stdout) < (size_t) r)
			break;
	}

	gzclose (zlibfile);
	return;
}

#endif /* HAVE_LIBZ */

pipeline *decompress_open (const char *filename)
{
	pipecmd *cmd;
	pipeline *p;
	struct stat st;
#ifdef HAVE_LIBZ
	size_t filename_len;
#endif /* HAVE_LIBZ */
	char *ext;
	struct compression *comp;

	if (stat (filename, &st) < 0 || S_ISDIR (st.st_mode))
		return NULL;

#ifdef HAVE_LIBZ
	filename_len = strlen (filename);
	if (filename_len > 3 && STREQ (filename + filename_len - 3, ".gz")) {
		/* informational only; no shell quoting concerns */
		char *name = xasprintf ("zcat < %s", filename);
		cmd = pipecmd_new_function (name, &decompress_zlib, NULL,
					    NULL);
		free (name);
		p = pipeline_new_commands (cmd, NULL);
		goto got_pipeline;
	}
#endif /* HAVE_LIBZ */

	ext = strrchr (filename, '.');
	if (ext) {
		++ext;

		for (comp = comp_list; comp->ext; ++comp) {
			if (!STREQ (comp->ext, ext))
				continue;

			cmd = pipecmd_new_argstr (comp->prog);
			pipecmd_arg (cmd, filename);
			p = pipeline_new_commands (cmd, NULL);
			goto got_pipeline;
		}
	}

#ifdef HAVE_GZIP
	/* HP-UX */
	ext = strstr (filename, ".Z/");
	if (ext) {
		cmd = pipecmd_new_argstr (GUNZIP " -S \"\"");
		pipecmd_arg (cmd, filename);
		p = pipeline_new_commands (cmd, NULL);
		goto got_pipeline;
	}
#endif

	p = pipeline_new ();

got_pipeline:
	pipeline_want_infile (p, filename);
	pipeline_want_out (p, -1);
	return p;
}

pipeline *decompress_fdopen (int fd)
{
	pipeline *p;
#ifdef HAVE_LIBZ
	pipecmd *cmd;
#endif /* HAVE_LIBZ */

	binary_io (fd);

#ifdef HAVE_LIBZ
	cmd = pipecmd_new_function ("zcat", &decompress_zlib, NULL, NULL);
	p = pipeline_new_commands (cmd, NULL);
#else /* HAVE_LIBZ */
	p = pipeline_new ();
#endif /* HAVE_LIBZ */

	pipeline_want_in (p, fd);
	pipeline_want_out (p, -1);
	return p;
}

#ifdef ENABLE_INPROCESS_MANPAGES
/* Read once into a heap buffer so source resolution and NAME parsing can
 * share the same bytes, without a decompressor child or a second open.
 */
int decompressed_page_open (const char *filename,
			    struct decompressed_page **page)
{
	const char *ext = strrchr (filename, '.');
	struct compression *comp;
	struct stat st;
	struct decompressed_page *p;
	FILE *fp = NULL;
#ifdef HAVE_LIBZ
	gzFile gz = NULL;
#endif
	size_t capacity = 65536;
	int saved_errno = 0;

	*page = NULL;
	if (ext) {
		for (comp = comp_list; comp->ext; ++comp) {
			if (STREQ (ext + 1, comp->ext)) {
#ifdef HAVE_LIBZ
				if (STREQ (ext, ".gz"))
					break;
#endif
				return 0;
			}
		}
	}
	if (strstr (filename, ".Z/"))
		return 0;
	if (stat (filename, &st) < 0)
		return -1;
	if (!S_ISREG (st.st_mode))
		return 0;
#ifdef HAVE_LIBZ
	if (ext && STREQ (ext, ".gz")) {
		gz = gzopen (filename, "rb");
		if (!gz)
			return -1;
	} else
#endif
	{
		fp = fopen (filename, "rb");
		if (!fp)
			return -1;
	}
	p = XMALLOC (struct decompressed_page);
	p->data = xmalloc (capacity);
	p->len = 0;
	for (;;) {
		size_t n;
		/* Keep reads bounded for gzread's int return type. */
		if (capacity - p->len < 32769) {
			if (capacity > (size_t) -1 / 2)
				xalloc_die ();
			capacity *= 2;
			p->data = xrealloc (p->data, capacity);
		}
#ifdef HAVE_LIBZ
		if (gz) {
			int got = gzread (gz, p->data + p->len, 32768);
			if (got <= 0) {
				int zerr;
				(void) gzerror (gz, &zerr);
				if (zerr != Z_OK && zerr != Z_STREAM_END)
					saved_errno = zerr == Z_ERRNO ? errno : EIO;
				break;
			}
			n = got;
		} else
#endif
		{
			n = fread (p->data + p->len, 1, 32768, fp);
			if (!n) {
				if (ferror (fp))
					saved_errno = errno ? errno : EIO;
				break;
			}
		}
		p->len += n;
	}
#ifdef HAVE_LIBZ
	if (gz && gzclose (gz) != Z_OK && !saved_errno)
		saved_errno = EIO;
#endif
	if (fp && fclose (fp) != 0 && !saved_errno)
		saved_errno = errno;
	if (saved_errno) {
		decompressed_page_free (p);
		errno = saved_errno;
		return -1;
	}
	p->data[p->len] = '\0';
	*page = p;
	return 1;
}

void decompressed_page_free (struct decompressed_page *page)
{
	if (page) {
		free (page->data);
		free (page);
	}
}
#endif /* ENABLE_INPROCESS_MANPAGES */
