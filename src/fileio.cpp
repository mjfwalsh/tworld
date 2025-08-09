/* fileio.cpp: Simple file/directory access functions with error-handling.
 *
 * Copyright (C) 2001-2020 by Brian Raiter, Eric Schmidt, and Michael Walsh. Licensed under the
 * GNU General Public License. No warranty. See COPYING for details.
 */

#include    <QtCore/QDir>
#include    <QtWidgets/QApplication>
#include    <QtCore/QStandardPaths>

#include    <cstdio>
#include    <cstdlib>
#include    <cstring>
#include    <string>
#include    <cerrno>

#include    "fileio.h"
#include    "err.h"

static std::string dirs[NUMBER_OF_DIRS];

/* The function used to display error messages relating to file I/O.
 */
bool fileinfo::fileerr_(char const *msg, char const *cfile, unsigned long lineno)
{
    if(msg)
        warn_("error: %s: %s [%s:%lu]\n",
              m_filename.c_str(),
              errno ? strerror(errno) : msg,
              cfile,
              lineno);
    return false;
}

/*
 * File-handling functions.
 */

/* Clear the fields of the fileinfo struct.
 */
fileinfo::~fileinfo()
{
    if(m_fp) close();
}

/* Hack to get around MinGW (really msvcrt.dll) not supporting 'x' modifier
 * for fopen.
 */
#if defined __MINGW32__
#include <fcntl.h>
static FILE *FOPEN(char const *n, char const *mode)
{
    FILE * file = nullptr;
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
    if (this->m_fp) {
        if (fclose(this->m_fp) == EOF)
            fileerr(this, nullptr);
        this->m_fp = nullptr;
    }
}

/* rewind().
 */
void fileinfo::rewind()
{
    ::rewind(this->m_fp);
}

/* feof().
 */
bool fileinfo::testend()
{
    int ch;

    if (feof(this->m_fp))
        return true;
    ch = fgetc(this->m_fp);
    if (ch == EOF)
        return true;
    ungetc(ch, this->m_fp);
    return false;
}

/* read().
 */
bool fileinfo::read(void *data, unsigned long size, char const *msg)
{
    if (!size)
        return true;
    errno = 0;
    if (fread(data, size, 1, this->m_fp) == 1)
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
        return nullptr;
    }
    if (!size)
        return buf;
    errno = 0;
    if (fread(buf, size, 1, this->m_fp) != 1) {
        fileerr(this, msg);
        free(buf);
        return nullptr;
    }
    return buf;
}

/* Read one full line from m_fp and store the first len characters,
 * including any trailing newline.
 */
bool fileinfo::getline(char *buf, const int len)
{
    errno = 0;
    if (!fgets(buf, len, this->m_fp))
        return fileerr(this, nullptr);
    int n = strlen(buf);
    if (n == len - 1 && buf[n] != '\n') {
        int ch;
        do
            ch = fgetc(this->m_fp);
        while (ch != EOF && ch != '\n');
    } else
        buf[n--] = '\0';
    return true;
}

/* write().
 */
bool fileinfo::write(void const *data, unsigned long size, char const *msg)
{
    if (!size)
        return true;
    errno = 0;
    if (fwrite(data, size, 1, this->m_fp) == 1)
        return true;
    return fileerr(this, msg);
}

/* Read one byte as an unsigned integer value.
 */
bool fileinfo::readint8(uint8_t &val8, char const *msg)
{
    int byte;

    errno = 0;
    if ((byte = fgetc(this->m_fp)) == EOF)
        return fileerr(this, msg);
    val8 = (uint8_t)byte;
    return true;
}

/* Write one byte as an unsigned integer value.
 */
bool fileinfo::writeint8(uint8_t val8, char const *msg)
{
    errno = 0;
    if (fputc(val8, this->m_fp) != EOF)
        return true;
    return fileerr(this, msg);
}

/* Read two bytes as an unsigned integer value stored in little-endian.
 */
bool fileinfo::readint16(uint16_t &val16, char const *msg)
{
    int byte;

    errno = 0;
    if ((byte = fgetc(this->m_fp)) != EOF) {
        val16 = (unsigned char)byte;
        if ((byte = fgetc(this->m_fp)) != EOF) {
            val16 |= (unsigned char)byte << 8;
            return true;
        }
    }
    return fileerr(this, msg);
}

/* Write two bytes as an unsigned integer value in little-endian.
 */
bool fileinfo::writeint16(uint16_t val16, char const *msg)
{
    errno = 0;
    if (fputc(val16 & 0xFF, this->m_fp) != EOF
        && fputc((val16 >> 8) & 0xFF, this->m_fp) != EOF)
        return true;
    return fileerr(this, msg);
}

/* Read four bytes as an unsigned integer value stored in little-endian.
 */
bool fileinfo::readint32(uint32_t &val32, char const *msg)
{
    int byte;
    int shift = 0;
    errno = val32 = 0;
    while (shift <= 24 && (byte = fgetc(this->m_fp)) != EOF) {
        val32 |= (uint32_t)byte << shift;
        shift += 8;
    }
    
    if (shift == 32) return true;
    return fileerr(this, msg);
}

/* Write four bytes as an unsigned integer value in little-endian.
 */
bool fileinfo::writeint32(uint32_t val32, char const *msg)
{
    errno = 0;
    if (fputc(val32 & 0xFF, this->m_fp) != EOF
            && fputc((val32 >> 8) & 0xFF, this->m_fp) != EOF
            && fputc((val32 >> 16) & 0xFF, this->m_fp) != EOF
            && fputc((val32 >> 24) & 0xFF, this->m_fp) != EOF)
        return true;
    return fileerr(this, msg);
}

/* Check if filehandle is open
 */
bool fileinfo::isopen()
{
    return (bool)this->m_fp;
}

/* Write a formatted line
 */
bool fileinfo::writef(const char *format, ...)
{
    va_list argp;
    va_start(argp, format);
    int wchars = vfprintf(this->m_fp, format, argp);
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
    return dirs[t].c_str();
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
std::string getpathforfileindir(int dirInt, char const *filename)
{
    std::string path = getdir(dirInt);
    path += '/';
    path += filename;
    return path;
}

fileinfo::fileinfo(int d, const std::string &fn)
:
m_filename(fn),
m_dir(d)
{}

/* Open a file from of the directories RESDIR, USER_SERIESDATDIR,
 * GLOBAL_SERIESDATDIR, SOLUTIONDIR, or SETTINGSDIR. If the fileinfo structure
 * does not already have a filename assigned to it, use name (after making an
 * independent copy).
 */
bool fileinfo::open(char const *mode, char const *msg)
{
    errno = 0;

    std::string fullpath = getpathforfileindir(m_dir, m_filename.c_str());
    this->m_fp = FOPEN(fullpath.c_str(), mode);

    if (this->m_fp) return true;
    return fileerr(this, msg);
}

bool fileinfo::seek(long int bytes)
{
    return fseek(this->m_fp, bytes, SEEK_SET);
}

/* Save a dir path.
 */
static void savedir(int dir, QString &path)
{
    dirs[dir] = path.toStdString();
}

/* Initialise the directories using Qt standard paths
 */
void initdirs()
{
    auto checkDir = [](QString &d)
    {
        QDir dir(d);
        if (!dir.exists() && !dir.mkpath(".")) {
            die("Unable to create folder %s", d.toUtf8().constData());
        }
    };

    // Get the app resources
    QString appRootDir = QApplication::applicationDirPath();
    #if defined __APPLE__
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
    QString appResDir =  QString(appRootDir + "/res");
    QString appDataDir =  QString(appRootDir + "/data");

    // set user directory
    QString userDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    checkDir(userDir);

    // ~/Library/Application Support/Tile World/data
    QString userDataDir = QString(userDir + "/data");
    checkDir(userDataDir);

    // ~/Library/Application Support/Tile World/solutions
    QString userSolDir = QString(userDir + "/solutions");
    checkDir(userSolDir);

    savedir(RESDIR, appResDir);
    savedir(USER_SERIESDATDIR, userDataDir);
    savedir(GLOBAL_SERIESDATDIR, appDataDir);
    savedir(SOLUTIONDIR, userSolDir);
    savedir(SETTINGSDIR, userDir);
}
