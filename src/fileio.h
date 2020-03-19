/* fileio.h: Simple file/directory access functions with error-handling.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_fileio_h_
#define	HEADER_fileio_h_

/* enum for different directories
 */
enum {
	RESDIR,
	SERIESDIR,
	USER_SERIESDATDIR,
	GLOBAL_SERIESDATDIR,
	SOLUTIONDIR,
	SETTINGSDIR,
	NUMBER_OF_DIRS
};

/* Initialise the directories using Qt standard paths
 */
extern void initdirs();

/* function to access app dir
 */
extern const char *getdir(int t);

/* Return TRUE if name contains a path but is not a directory itself.
 */
extern int haspathname(char const *name);

/* Return the pathname for a directory and/or filename, using the same
 * algorithm to construct the path as openfileindir(). The caller must
 * free the returned buffer.
 */
extern char *getpathforfileindir(int dirInt, char const *filename);


/* Call filecallback once for every file in dir. The first argument to
 * the callback function is an allocated buffer containing the
 * filename. data is passed as the second argument to the callback. If
 * the callback's return value is zero, the buffer is deallocated
 * normally; if the return value is positive, the callback function
 * inherits the buffer and the responsibility of freeing it. If the
 * return value is negative, findfiles() stops scanning the directory
 * and returns. FALSE is returned if the directory could not be
 * examined.
 */
extern int findfiles(int dirInt, void *data,
			 int (*filecallback)(char const*, void*));

class fileinfo
{
public:
	char       *name;		/* the name of the file */
    int        dir;         /* the directory the file is in */
	FILE       *fp;		/* the real file handle */
	char	alloc;		/* TRUE if name was allocated internally */

	/* Reset a fileinfo structure to indicate no file.
	 */
	void clearfileinfo();

	/* The following functions correspond directly to C's standard I/O
	 * functions. If msg is NULL, no error will be displayed if
	 * the operation fails. If msg points to a string, an error will
	 * be displayed. The text of msg will be used only if errno is
	 * zero; otherwise a message appropriate to the error will be used.
	 */
	int filerewind(char const *msg);
	int fileread(void *data, unsigned long size, char const *msg);
	int filewrite(void const *data, unsigned long size, char const *msg);
	void fileclose(char const *msg);

	/* filetestend() forces a check for EOF by attempting to read a byte
	 * from the file, and ungetting the byte if one is successfully read.
	 */
	int filetestend();

	/* The following functions read and write an unsigned integer value
	 * from the current position in the given file. For the multi-byte
	 * values, the value is assumed to be stored in little-endian.
	 */
	int filereadint8(unsigned char *val8, char const *msg);
	int filewriteint8(unsigned char val8, char const *msg);
	int filereadint16(unsigned short *val16, char const *msg);
	int filewriteint16(unsigned short val16, char const *msg);
	int filereadint32(unsigned long *val32, char const *msg);
	int filewriteint32(unsigned long val32, char const *msg);

	/* Read size bytes from the given file and return the bytes in a
	 * newly allocated buffer.
	 */
	unsigned char *filereadbuf(unsigned long size, char const *msg);

	/* Read one full line from fp and store the first len characters,
	 * including any trailing newline. len receives the length of the line
	 * stored in buf, minus any trailing newline, upon return.
	 */
	int filegetline(char *buf, int *len, char const *msg);

	/* Open a file, using dir as the directory if filename is not already
	 * a complete pathname. FALSE is returned if the directory could not
	 * be created.
	 */
	int openfileindir(int dirInt, char const *filename,
				 char const *mode, char const *msg);

	/* Display a simple error message prefixed by the name of the given
	 * file. If errno is set, a message appropriate to the value is used;
	 * otherwise the text pointed to by msg is used. If msg is NULL, the
	 * function does nothing. The return value is always FALSE.
	 */
	int fileerr_(char const *cfile, unsigned long lineno, char const *msg);
};

/* Display a simple error message prefixed by the name of the given
 * file. If errno is set, a message appropriate to the value is used;
 * otherwise the text pointed to by msg is used. If msg is NULL, the
 * function does nothing. The return value is always FALSE.
 */
//extern int fileerr_(fileinfo *file, char const *cfile, unsigned long lineno, char const *msg);
#define	fileerr(file, msg)	((file)->fileerr_(__FILE__, __LINE__, (msg)))

#endif
