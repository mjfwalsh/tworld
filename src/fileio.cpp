/* fileio.cpp: Simple file/directory access functions with error-handling.
 *
 * Copyright (C) 2001-2020 by Brian Raiter, Eric Schmidt, and Michael Walsh. Licensed under the
 * GNU General Public License. No warranty. See COPYING for details.
 */

#include	<QDir>
#include	<QApplication>
#include	<QStandardPaths>

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	"defs.h"
#include	"err.h"
#include	"fileio.h"

static char *dirs[NUMBER_OF_DIRS];

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
	if (file->alloc) {
		free(file->name);
		clearfileinfo(file);
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
	file->dir = -1;
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
		if (fd != -1) file = fdopen(fd, "w");
	} else {
		file = fopen(name, mode);
	}

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
		clearfileinfo(file);
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
unsigned char *filereadbuf(fileinfo *file, unsigned long size, char const *msg)
{
	unsigned char       *buf;

	if (!(buf = (unsigned char *)malloc(size))) {
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
	if (!*len) {
		*buf = '\0';
		return TRUE;
	}
	errno = 0;
	if (!fgets(buf, *len, file->fp))
		return fileerr(file, msg);
	int n = strlen(buf);
	if (n == *len - 1 && buf[n] != '\n') {
		int ch;
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

/* Access a dir path.
 */
const char *getdir(int t)
{
	return dirs[t];
}

/* Return TRUE if name contains a path but is not a directory itself.
 */
int haspathname(char const *name)
{
	struct stat	st;

	if (!strchr(name, '/'))
		return FALSE;
	if (!strchr(name, '\\'))
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
	char const *dir;
	dir = getdir(dirInt);

	m = strlen(filename);
	n = strlen(dir);

	x_cmalloc(path, m + n + 2);
	memcpy(path, dir, n);
	path[n++] = '/';
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
	char *name = getpathforfileindir(dirInt, filename);

	if (!file->name) {
		file->alloc = TRUE;
		x_cmalloc(file->name, strlen(filename) + 1);
		strcpy(file->name, filename);
		file->dir = dirInt;
	}

	errno = 0;
	file->fp = FOPEN(name, mode);
	free(name);
	if (file->fp) return TRUE;
	return fileerr(file, msg);
}

/* Read the given directory and call filecallback once for each file
 * contained in it.
 */
int findfiles(int dirInt, void *data, int (*filecallback)(char const*, void*))
{
	const char *dir;
	dir = getdir(dirInt);

	DIR		       *dp;
	struct dirent      *dent;
	int			r;

	if (!(dp = opendir(dir))) {
		fileinfo tmp;
		tmp.name = (char*)dir;
		tmp.alloc = TRUE;
		return fileerr(&tmp, "couldn't access directory");
	}

	while ((dent = readdir(dp))) {
		if (dent->d_name[0] == '.')
			continue;

		char *filename;
		x_cmalloc(filename, strlen(dent->d_name) + 1);
		strcpy(filename, dent->d_name);

		r = (*filecallback)(filename, data);
		free(filename);
		if (r < 0) break;
	}

	closedir(dp);
	return TRUE;
}

/* Save a dir path.
 */
static void savedir(int dirInt, const char *dirPath, int dirPathLength)
{
	x_cmalloc(dirs[dirInt], dirPathLength);
	strcpy(dirs[dirInt], dirPath);
}

/* free stuff
 */
static void shutdown()
{
	for(int i = 0; i < NUMBER_OF_DIRS; i++) {
		free(dirs[i]);
	}
}

/* Initialise the directories using Qt standard paths
 */
void initdirs()
{
	atexit(shutdown);

	auto checkDir = [](QString d)
	{
		QDir dir(d);
		if (!dir.exists() && !dir.mkpath(".")) {
			die("Unable to create folder %s", d.toUtf8().constData());
		}
	};

	// Get the app resources
	QString appRootDir = QApplication::applicationDirPath();
	#if defined Q_OS_OSX
	QDir appShareDir(appRootDir + "/../Resources");
	#elif defined Q_OS_UNIX
	QDir appShareDir(appRootDir + "/../share/tworld");
	#endif

	#if defined Q_OS_UNIX
	if (appShareDir.exists()) appRootDir = appShareDir.path();
	#endif

	// change pwd to appRootDir
	QDir::setCurrent(appRootDir);

	// these folders should already exist
	QString appResDirQ =  QString(appRootDir + "/res");
	QString appDataDirQ =  QString(appRootDir + "/data");

	// set user directory
	QString userDirQ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	checkDir(userDirQ);

	// ~/Library/Application Support/Tile World/sets
	QString userSetsDirQ = QString(userDirQ + "/sets");
	checkDir(userSetsDirQ);

	// ~/Library/Application Support/Tile World/data
	QString userDataDirQ = QString(userDirQ + "/data");
	checkDir(userDataDirQ);

	// ~/Library/Application Support/Tile World/solutions
	QString userSolDirQ = QString(userDirQ + "/solutions");
	checkDir(userSolDirQ);

	savedir(RESDIR, appResDirQ.toUtf8().constData(), appResDirQ.length() + 1);
	savedir(SERIESDIR, userSetsDirQ.toUtf8().constData(), userSetsDirQ.length() + 1);
	savedir(USER_SERIESDATDIR, userDataDirQ.toUtf8().constData(), userDataDirQ.length() + 1);
	savedir(GLOBAL_SERIESDATDIR, appDataDirQ.toUtf8().constData(), appDataDirQ.length() + 1);
	savedir(SOLUTIONDIR, userSolDirQ.toUtf8().constData(), userSolDirQ.length() + 1);
	savedir(SETTINGSDIR, userDirQ.toUtf8().constData(), userDirQ.length() + 1);
}
