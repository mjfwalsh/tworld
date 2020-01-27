/* fileio.c: Simple file/directory access functions with error-handling.
 *
 * Copyright (C) 2001-2017 by Brian Raiter and Eric Schmidt, under the
 * GNU General Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<dirent.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"err.h"
#include	"fileio.h"
#include	"res.h"

/* Determine the proper directory delimiter and mkdir() arguments.
 */
#ifdef WIN32
#define	DIRSEP_CHAR	'\\'
#else
#define	DIRSEP_CHAR	'/'
#endif

/* Determine a compile-time number to use as the maximum length of a
 * path. Use a value of 1023 if we can't get anything usable from the
 * header files.
 */
#include <limits.h>
#if !defined(PATH_MAX) || PATH_MAX <= 0
#  if defined(MAXPATHLEN) && MAXPATHLEN > 0
#    define PATH_MAX MAXPATHLEN
#  else
#    include <sys/param.h>
#    if !defined(PATH_MAX) || PATH_MAX <= 0
#      if defined(MAXPATHLEN) && MAXPATHLEN > 0
#        define PATH_MAX MAXPATHLEN
#      else
#        define PATH_MAX 1023
#      endif
#    endif
#  endif
#endif

/* The function used to display error messages relating to file I/O.
 */
int fileerr_(char const *cfile, unsigned long lineno,
	     fileinfo *file, char const *msg)
{
    if (msg) {
	err_cfile_ = cfile;
	err_lineno_ = lineno;
	errmsg_(file->name ? file->name : "file error",
		errno ? strerror(errno) : msg);
    }
    return FALSE;
}

/*
 * File-handling functions.
 */

/* Clear the fields of the fileinfo struct.
 */
void clearfileinfo(fileinfo *file)
{
    file->name = NULL;
    file->fp = NULL;
    file->alloc = FALSE;
}

/* Hack to get around MinGW (really msvcrt.dll) not supporting 'x' modifier
 * for fopen.
 */
#if defined __MINGW32__
#include <fcntl.h>
static FILE *FOPEN(char const *name, char const *mode)
{
    FILE * file = NULL;
    if (!strcmp(mode, "wx")) {
        int fd = open(name, O_WRONLY | O_CREAT | O_EXCL);
	if (fd != -1)
	    file = fdopen(fd, "w");
    }
    else
        file = fopen(name, mode);

    return file;
}
#else
#define FOPEN fopen
#endif

/* Close the file, clear the file pointer, and free the name buffer if
 * necessary.
 */
void fileclose(fileinfo *file, char const *msg)
{
    errno = 0;
    if (file->fp) {
	if (fclose(file->fp) == EOF)
	    fileerr(file, msg);
	file->fp = NULL;
    }
    if (file->alloc) {
	free(file->name);
	file->name = NULL;
	file->alloc = FALSE;
    }
}

/* rewind().
 */
int filerewind(fileinfo *file, char const *msg)
{
    (void)msg;
    rewind(file->fp);
    return TRUE;
}

/* fseek().
 */
int fileskip(fileinfo *file, int offset, char const *msg)
{
    errno = 0;
    if (!fseek(file->fp, offset, SEEK_CUR))
	return TRUE;
    return fileerr(file, msg);
}

/* feof().
 */
int filetestend(fileinfo *file)
{
    int	ch;

    if (feof(file->fp))
	return TRUE;
    ch = fgetc(file->fp);
    if (ch == EOF)
	return TRUE;
    ungetc(ch, file->fp);
    return FALSE;
}

/* read().
 */
int fileread(fileinfo *file, void *data, unsigned long size, char const *msg)
{
    if (!size)
	return TRUE;
    errno = 0;
    if (fread(data, size, 1, file->fp) == 1)
	return TRUE;
    return fileerr(file, msg);
}

/* Read size bytes from the given file into a newly allocated buffer.
 */
void *filereadbuf(fileinfo *file, unsigned long size, char const *msg)
{
    void       *buf;

    if (!(buf = malloc(size))) {
	fileerr(file, msg);
	return NULL;
    }
    if (!size)
	return buf;
    errno = 0;
    if (fread(buf, size, 1, file->fp) != 1) {
	fileerr(file, msg);
	free(buf);
	return NULL;
    }
    return buf;
}

/* Read one full line from fp and store the first len characters,
 * including any trailing newline.
 */
int filegetline(fileinfo *file, char *buf, int *len, char const *msg)
{
    int	n, ch;

    if (!*len) {
	*buf = '\0';
	return TRUE;
    }
    errno = 0;
    if (!fgets(buf, *len, file->fp))
	return fileerr(file, msg);
    n = strlen(buf);
    if (n == *len - 1 && buf[n] != '\n') {
	do
	    ch = fgetc(file->fp);
	while (ch != EOF && ch != '\n');
    } else
	buf[n--] = '\0';
    *len = n;
    return TRUE;
}

/* write().
 */
int filewrite(fileinfo *file, void const *data, unsigned long size,
	      char const *msg)
{
    if (!size)
	return TRUE;
    errno = 0;
    if (fwrite(data, size, 1, file->fp) == 1)
	return TRUE;
    return fileerr(file, msg);
}

/* Read one byte as an unsigned integer value.
 */
int filereadint8(fileinfo *file, unsigned char *val8, char const *msg)
{
    int	byte;

    errno = 0;
    if ((byte = fgetc(file->fp)) == EOF)
	return fileerr(file, msg);
    *val8 = (unsigned char)byte;
    return TRUE;
}

/* Write one byte as an unsigned integer value.
 */
int filewriteint8(fileinfo *file, unsigned char val8, char const *msg)
{
    errno = 0;
    if (fputc(val8, file->fp) != EOF)
	return TRUE;
    return fileerr(file, msg);
}

/* Read two bytes as an unsigned integer value stored in little-endian.
 */
int filereadint16(fileinfo *file, unsigned short *val16, char const *msg)
{
    int	byte;

    errno = 0;
    if ((byte = fgetc(file->fp)) != EOF) {
	*val16 = (unsigned char)byte;
	if ((byte = fgetc(file->fp)) != EOF) {
	    *val16 |= (unsigned char)byte << 8;
	    return TRUE;
	}
    }
    return fileerr(file, msg);
}

/* Write two bytes as an unsigned integer value in little-endian.
 */
int filewriteint16(fileinfo *file, unsigned short val16, char const *msg)
{
    errno = 0;
    if (fputc(val16 & 0xFF, file->fp) != EOF
			&& fputc((val16 >> 8) & 0xFF, file->fp) != EOF)
	return TRUE;
    return fileerr(file, msg);
}

/* Read four bytes as an unsigned integer value stored in little-endian.
 */
int filereadint32(fileinfo *file, unsigned long *val32, char const *msg)
{
    int	byte;

    errno = 0;
    if ((byte = fgetc(file->fp)) != EOF) {
	*val32 = (unsigned int)byte;
	if ((byte = fgetc(file->fp)) != EOF) {
	    *val32 |= (unsigned int)byte << 8;
	    if ((byte = fgetc(file->fp)) != EOF) {
		*val32 |= (unsigned int)byte << 16;
		if ((byte = fgetc(file->fp)) != EOF) {
		    *val32 |= (unsigned int)byte << 24;
		    return TRUE;
		}
	    }
	}
    }
    return fileerr(file, msg);
}

/* Write four bytes as an unsigned integer value in little-endian.
 */
int filewriteint32(fileinfo *file, unsigned long val32, char const *msg)
{
    errno = 0;
    if (fputc(val32 & 0xFF, file->fp) != EOF
			&& fputc((val32 >> 8) & 0xFF, file->fp) != EOF
			&& fputc((val32 >> 16) & 0xFF, file->fp) != EOF
			&& fputc((val32 >> 24) & 0xFF, file->fp) != EOF)
	return TRUE;
    return fileerr(file, msg);
}

/*
 * Directory-handling functions.
 */

/* Return TRUE if name contains a path but is not a directory itself.
 */
int haspathname(char const *name)
{
    struct stat	st;

    if (!strchr(name, DIRSEP_CHAR))
	return FALSE;
    if (stat(name, &st) || S_ISDIR(st.st_mode))
	return FALSE;
    return TRUE;
}

/* Return the pathname for a directory and/or filename, using the
 * same algorithm to construct the path as openfileindir().
 */
char *getpathforfileindir(int dirInt, char const *filename)
{
    char       *path;
    int		m, n;

    char const *dir = getdir(dirInt);

    if (strchr(filename, DIRSEP_CHAR))
	die("getpathforfileindir: path passed as filename %s", filename);

	m = strlen(filename);
	n = strlen(dir);
	if (m + n + 1 > PATH_MAX) {
	    errno = ENAMETOOLONG;
	    return NULL;
	}
	path = (char *)malloc(m + n + 1);
	memcpy(path, dir, n);
	path[n++] = DIRSEP_CHAR;
	memcpy(path + n, filename, m + 1);
	return path;
}


/* Open a file from of the directories RESDIR, SERIESDIR, USER_SERIESDATDIR,
 * GLOBAL_SERIESDATDIR, SOLUTIONDIR, or SETTINGSDIR. If the fileinfo structure
 * does not already have a filename assigned to it, use name (after making an
 * independent copy).
 */
int openfileindir(fileinfo *file, int dirInt, char const *filename,
		  char const *mode, char const *msg)
{
	char const *name = getpathforfileindir(dirInt, filename);

	if (!file->name) {
		file->name = malloc(strlen(name) + 1);
		strcpy(file->name, name);
	}

	errno = 0;
	file->fp = FOPEN(name, mode);
	if (file->fp) return TRUE;
	return fileerr(file, msg);
}

/* Read the given directory and call filecallback once for each file
 * contained in it.
 */
int findfiles(int dirInt, void *data,
	      int (*filecallback)(char const*, void*))
{
    const char *dir = getdir(dirInt);

    char	       *filename = NULL;
    DIR		       *dp;
    struct dirent      *dent;
    int			r;

    if (!(dp = opendir(dir))) {
	fileinfo tmp;
	tmp.name = (char*)dir;
	return fileerr(&tmp, "couldn't access directory");
    }

    while ((dent = readdir(dp))) {
	if (dent->d_name[0] == '.')
	    continue;
	x_alloc(filename, strlen(dent->d_name) + 1);
	strcpy(filename, dent->d_name);
	r = (*filecallback)(filename, data);
	if (r < 0)
	    break;
	else if (r > 0)
	    filename = NULL;
    }

    if (filename)
	free(filename);
    closedir(dp);
    return TRUE;
}
