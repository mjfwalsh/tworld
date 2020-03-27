/* fileio.h: Simple file/directory access functions with error-handling.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_fileio_h_
#define	HEADER_fileio_h_

#include <cstdio>

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
extern bool haspathname(char const *name);

/* Return the pathname for a directory and/or filename, using the same
 * algorithm to construct the path as open(). The caller must
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
extern bool findfiles(int dirInt, void *data,
			 int (*filecallback)(char const*, void*));

class fileinfo
{
public:
	/* The following functions correspond directly to C's standard I/O
	 * functions. If msg is NULL, no error will be displayed if
	 * the operation fails. If msg points to a string, an error will
	 * be displayed. The text of msg will be used only if errno is
	 * zero; otherwise a message appropriate to the error will be used.
	 */
	void rewind();
	bool read(void *data, unsigned long size, char const *msg = NULL);
	bool write(void const *data, unsigned long size, char const *msg = NULL);
	void close();

	/* testend() forces a check for EOF by attempting to read a byte
	 * from the file, and ungetting the byte if one is successfully read.
	 */
	bool testend();

	/* The following functions read and write an unsigned integer value
	 * from the current position in the given file. For the multi-byte
	 * values, the value is assumed to be stored in little-endian.
	 */
	bool readint8(unsigned char *val8, char const *msg = NULL);
	bool writeint8(unsigned char val8, char const *msg = NULL);
	bool readint16(unsigned short *val16, char const *msg = NULL);
	bool writeint16(unsigned short val16, char const *msg = NULL);
	bool readint32(unsigned long *val32, char const *msg = NULL);
	bool writeint32(unsigned long val32, char const *msg = NULL);

	/* Read size bytes from the given file and return the bytes in a
	 * newly allocated buffer.
	 */
	unsigned char *readbuf(unsigned long size, char const *msg);

	/* Read one full line from fp and store the first len characters,
	 * including any trailing newline. len receives the length of the line
	 * stored in buf, minus any trailing newline, upon return.
	 */
	bool getline(char *buf, int *len, char const *msg);

	/* Open a file, using dir as the directory if filename is not already
	 * a complete pathname. FALSE is returned if the directory could not
	 * be created.
	 */
	bool open(int dirInt, char const *filename,
				 char const *mode, char const *msg);

	/* Test if the filehandle is open
	 */
	bool isopen();

	/* Alias for printf
	 */
	bool writef(const char *format, ...);

	/* Display a simple error message prefixed by the name of the given
	 * file. If errno is set, a message appropriate to the value is used;
	 * otherwise the text pointed to by msg is used. If msg is NULL, the
	 * function does nothing. The return value is always FALSE.
	 */
	bool fileerr_(char const *cfile, unsigned long lineno, char const *msg);

	/* Access the name var
	 */
	inline char *getName() const
		{return name;}

private:
	/* Reset a fileinfo structure to indicate no file.
	 */
	void clearfileinfo();

	char	*name = NULL;		/* the name of the file */
    int		dir = -1;			/* the directory the file is in */
	FILE	*fp  = NULL;		/* the real file handle */
	bool	alloc = false;		/* true if name was allocated internally */
};

#define	fileerr(file, msg)	((file)->fileerr_(__FILE__, __LINE__, (msg)))

#endif
