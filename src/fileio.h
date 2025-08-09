/* fileio.h: Simple file/directory access functions with error-handling.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef HEADER_fileio_h_
#define HEADER_fileio_h_

#include <cstdio>

/* enum for different directories
 */
enum {
    RESDIR,
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
extern std::string getpathforfileindir(int dirInt, char const *filename);

class fileinfo
{
public:
    fileinfo(int dirInt, const std::string &fn);
    ~fileinfo();

    /* The following functions correspond directly to C's standard I/O
     * functions. If msg is nullptr, no error will be displayed if
     * the operation fails. If msg points to a string, an error will
     * be displayed. The text of msg will be used only if errno is
     * zero; otherwise a message appropriate to the error will be used.
     */
    void rewind();
    bool read(void *data, unsigned long size, char const *msg = nullptr);
    bool write(void const *data, unsigned long size, char const *msg = nullptr);
    void close();

    /* testend() forces a check for EOF by attempting to read a byte
     * from the file, and ungetting the byte if one is successfully read.
     */
    bool testend();

    /* The following functions read and write an unsigned integer value
     * from the current position in the given file. For the multi-byte
     * values, the value is assumed to be stored in little-endian.
     */
    bool readint8(uint8_t &val8, char const *msg = nullptr);
    bool writeint8(uint8_t val8, char const *msg = nullptr);
    bool readint16(uint16_t &val16, char const *msg = nullptr);
    bool writeint16(uint16_t val16, char const *msg = nullptr);
    bool readint32(uint32_t &val32, char const *msg = nullptr);
    bool writeint32(uint32_t val32, char const *msg = nullptr);

    /* Read size bytes from the given file and return the bytes in a
     * newly allocated buffer.
     */
    unsigned char *readbuf(unsigned long size, char const *msg);

    /* Read one full line from fp and store the first len characters,
     * including any trailing newline. len receives the length of the line
     * stored in buf, minus any trailing newline, upon return.
     */
    bool getline(char *buf, const int len);

    /* Open a file using the given mode. FALSE is returned if the directory
     * could not be created.
     */
    bool open(char const *mode, char const *msg);

    /* Jump to a specific number of bytes from beginning of file
     */
    bool seek(long int bytes);

    /* Test if the filehandle is open
     */
    bool isopen();

    /* Alias for printf
     */
    bool writef(const char *format, ...);

    /* Display a simple error message prefixed by the name of the given
     * file. If errno is set, a message appropriate to the value is used;
     * otherwise the text pointed to by msg is used. If msg is nullptr, the
     * function does nothing. The return value is always FALSE.
     */
    bool fileerr_(char const *msg, char const *cfile, unsigned long lineno);

    /* Access the name var
     */
    inline const char *name() const
        {return m_filename.c_str();}

private:

    std::string  m_filename;      /* the name of the file */
    int          m_dir;    /* the path of the file */
    FILE        *m_fp  = nullptr;        /* the real file handle */
};


#define fileerr(file, msg)  ((file)->fileerr_(msg, __FILE__, __LINE__))

#endif
