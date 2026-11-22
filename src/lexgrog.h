#ifndef MAN_LEXGROG_H
#define MAN_LEXGROG_H

#ifdef ENABLE_INPROCESS_MANPAGES
/* Buffered input entry point for the whatis scanner. */
struct decompressed_page;
struct lexgrog;
int find_name_from_page (struct decompressed_page *page, const char *file,
			 const char *filename, struct lexgrog *lg,
			 const char *encoding);

#endif /* ENABLE_INPROCESS_MANPAGES */
#endif /* MAN_LEXGROG_H */
