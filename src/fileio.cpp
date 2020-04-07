/* fileio.cpp: Simple file/directory access functions with error-handling.
 *
 * Copyright (C) 2001-2020 by Brian Raiter, Eric Schmidt, and Michael Walsh. Licensed under the
 * GNU General Public License. No warranty. See COPYING for details.
 */

#include	<QDir>
#include	<QApplication>
#include	<QStandardPaths>

#include	<cstdio>
#include	<cstdlib>
#include	<cstring>
#include	<cerrno>

#include	<dirent.h>

#include	"defs.h"
#include	"fileio.h"
#include	"err.h"

static char *dirs[NUMBER_OF_DIRS];

/* The function used to display error messages relating to file I/O.
 */
bool fileinfo::fileerr_(char const *cfile, unsigned long lineno, char const *msg)
{
	if (msg) {
		err_cfile_ = cfile;
		err_lineno_ = lineno;
		warn_("%s: %s", this->filename.c_str(),
			errno ? strerror(errno) : msg);
	}
	return false;
}

/*
 * File-handling functions.
 */

/* Clear the fields of the fileinfo struct.
 */
fileinfo::~fileinfo()
{
	if(this->fp) close();
}

/* Hack to get around MinGW (really msvcrt.dll) not supporting 'x' modifier
 * for fopen.
 */
#if defined __MINGW32__
#include <fcntl.h>
static FILE *FOPEN(char const *n, char const *mode)
{
	FILE * file = NULL;
	if (!strcmp(mode, "wx")) {
		int fd = open(n, O_WRONLY | O_CREAT | O_EXCL);
		if (fd != -1) file = fdopen(fd, "w");
	} else {
		file = fopen(n, mode);
	}

	return file;
}
#else
#define FOPEN fopen
#endif

/* Close the file, clear the file pointer, and free the name buffer if
 * necessary.
 */
void fileinfo::close()
{
	errno = 0;
	if (this->fp) {
		if (fclose(this->fp) == EOF)
			fileerr(this, NULL);
		this->fp = NULL;
	}
}

/* rewind().
 */
void fileinfo::rewind()
{
	::rewind(this->fp);
}

/* feof().
 */
bool fileinfo::testend()
{
	int	ch;

	if (feof(this->fp))
		return true;
	ch = fgetc(this->fp);
	if (ch == EOF)
		return true;
	ungetc(ch, this->fp);
	return false;
}

/* read().
 */
bool fileinfo::read(void *data, unsigned long size, char const *msg)
{
	if (!size)
		return true;
	errno = 0;
	if (fread(data, size, 1, this->fp) == 1)
		return true;
	return fileerr(this, msg);
}

/* Read size bytes from the given file into a newly allocated buffer.
 */
unsigned char *fileinfo::readbuf(unsigned long size, char const *msg)
{
	unsigned char       *buf;

	if (!(buf = (unsigned char *)malloc(size))) {
		fileerr(this, msg);
		return NULL;
	}
	if (!size)
		return buf;
	errno = 0;
	if (fread(buf, size, 1, this->fp) != 1) {
		fileerr(this, msg);
		free(buf);
		return NULL;
	}
	return buf;
}

/* Read one full line from fp and store the first len characters,
 * including any trailing newline.
 */
bool fileinfo::getline(char *buf, int *len, char const *msg)
{
	if (!*len) {
		*buf = '\0';
		return true;
	}
	errno = 0;
	if (!fgets(buf, *len, this->fp))
		return fileerr(this, msg);
	int n = strlen(buf);
	if (n == *len - 1 && buf[n] != '\n') {
		int ch;
		do
			ch = fgetc(this->fp);
		while (ch != EOF && ch != '\n');
	} else
		buf[n--] = '\0';
	*len = n;
	return true;
}

/* write().
 */
bool fileinfo::write(void const *data, unsigned long size, char const *msg)
{
	if (!size)
		return true;
	errno = 0;
	if (fwrite(data, size, 1, this->fp) == 1)
		return true;
	return fileerr(this, msg);
}

/* Read one byte as an unsigned integer value.
 */
bool fileinfo::readint8(unsigned char *val8, char const *msg)
{
	int	byte;

	errno = 0;
	if ((byte = fgetc(this->fp)) == EOF)
		return fileerr(this, msg);
	*val8 = (unsigned char)byte;
	return true;
}

/* Write one byte as an unsigned integer value.
 */
bool fileinfo::writeint8(unsigned char val8, char const *msg)
{
	errno = 0;
	if (fputc(val8, this->fp) != EOF)
		return true;
	return fileerr(this, msg);
}

/* Read two bytes as an unsigned integer value stored in little-endian.
 */
bool fileinfo::readint16(unsigned short *val16, char const *msg)
{
	int	byte;

	errno = 0;
	if ((byte = fgetc(this->fp)) != EOF) {
		*val16 = (unsigned char)byte;
		if ((byte = fgetc(this->fp)) != EOF) {
			*val16 |= (unsigned char)byte << 8;
			return true;
		}
	}
	return fileerr(this, msg);
}

/* Write two bytes as an unsigned integer value in little-endian.
 */
bool fileinfo::writeint16(unsigned short val16, char const *msg)
{
	errno = 0;
	if (fputc(val16 & 0xFF, this->fp) != EOF
		&& fputc((val16 >> 8) & 0xFF, this->fp) != EOF)
		return true;
	return fileerr(this, msg);
}

/* Read four bytes as an unsigned integer value stored in little-endian.
 */
bool fileinfo::readint32(unsigned long *val32, char const *msg)
{
	int	byte;

	errno = 0;
	if ((byte = fgetc(this->fp)) != EOF) {
		*val32 = (unsigned int)byte;
		if ((byte = fgetc(this->fp)) != EOF) {
			*val32 |= (unsigned int)byte << 8;
			if ((byte = fgetc(this->fp)) != EOF) {
				*val32 |= (unsigned int)byte << 16;
				if ((byte = fgetc(this->fp)) != EOF) {
					*val32 |= (unsigned int)byte << 24;
					return true;
				}
			}
		}
	}
	return fileerr(this, msg);
}

/* Write four bytes as an unsigned integer value in little-endian.
 */
bool fileinfo::writeint32(unsigned long val32, char const *msg)
{
	errno = 0;
	if (fputc(val32 & 0xFF, this->fp) != EOF
			&& fputc((val32 >> 8) & 0xFF, this->fp) != EOF
			&& fputc((val32 >> 16) & 0xFF, this->fp) != EOF
			&& fputc((val32 >> 24) & 0xFF, this->fp) != EOF)
		return true;
	return fileerr(this, msg);
}

/* Check if filehandle is open
 */
bool fileinfo::isopen()
{
	return (bool)this->fp;
}

/* Write a formatted line
 */
bool fileinfo::writef(const char *format, ...)
{
    va_list argp;
    va_start(argp, format);
	int wchars = vfprintf(this->fp, format, argp);
	va_end(argp);
	return wchars > 0;
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
bool haspathname(char const *n)
{
	if (!strchr(n, '/') || !strchr(n, '\\'))
		return false;
	return true;
}

/* Return the pathname for a directory and/or filename, using the
 * same algorithm to construct the path as open().
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

fileinfo::fileinfo(int d, char const *fn) : filename(fn), dir(d)
{
	//dir = dirInt;
	//filename = fn;
}

/* Open a file from of the directories RESDIR, SERIESDIR, USER_SERIESDATDIR,
 * GLOBAL_SERIESDATDIR, SOLUTIONDIR, or SETTINGSDIR. If the fileinfo structure
 * does not already have a filename assigned to it, use name (after making an
 * independent copy).
 */
bool fileinfo::open(char const *mode, char const *msg)
{
	errno = 0;

	char *fullpath = getpathforfileindir(dir, filename.c_str().);
	this->fp = FOPEN(fullpath, mode);
	free(fullpath);
	if (this->fp) return true;
	return fileerr(this, msg);
}

bool fileinfo::seek(long int bytes)
{
	return fseek(this->fp, bytes, SEEK_SET);
}


/* Mimic fileerr for folders.
 */
bool direrr_(char const *cfile, unsigned long lineno, char const *dirname, char const *msg)
{
	if (msg) {
		err_cfile_ = cfile;
		err_lineno_ = lineno;
		if(errno) warn_("%s: %s", dirname, strerror(errno));
		else warn_(msg, dirname);
	}
	return false;
}
#define	direrr(dirname, msg)	(direrr_(__FILE__, __LINE__, (dirname), (msg)))

/* Read the given directory and call filecallback once for each file
 * contained in it.
 */
bool findfiles(int dirInt, void *data, bool (*filecallback)(char const*, int, void*))
{
	const char *dir;
	dir = getdir(dirInt);

	DIR		       *dp;
	struct dirent      *dent;
	bool			r;

	if (!(dp = opendir(dir))) {
		return direrr(dir, "couldn't access directory");
	}

	while ((dent = readdir(dp))) {
		if (dent->d_name[0] == '.')
			continue;
		r = (*filecallback)(dent->d_name, dirInt, data);
		if (!r) break;
	}

	closedir(dp);
	return true;
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
