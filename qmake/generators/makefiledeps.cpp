/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "makefiledeps.h"
#include "option.h"
#include <qdir.h>
#include <qdatetime.h>
#include <qfileinfo.h>
#include <qbuffer.h>
#include <qplatformdefs.h>
#if defined(Q_OS_UNIX)
# include <unistd.h>
#else
# include <io.h>
#endif
#include <qdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#if defined(_MSC_VER) && _MSC_VER >= 1400
#include <share.h>
#endif

QT_BEGIN_NAMESPACE

// FIXME: a line ending in CRLF gets counted as two lines.
#if 1
#define qmake_endOfLine(c) (c == '\r' || c == '\n')
#else
inline bool qmake_endOfLine(const char &c) { return (c == '\r' || c == '\n'); }
#endif

QMakeLocalFileName::QMakeLocalFileName(const QString &name) : is_null(name.isNull())
{
    if(!name.isEmpty()) {
        if(name.at(0) == QLatin1Char('"') && name.at(name.length()-2) == QLatin1Char('"'))
            real_name = name.mid(1, name.length()-2);
        else
            real_name = name;
    }
}
const QString
&QMakeLocalFileName::local() const
{
    if(!is_null && local_name.isNull())
        local_name = Option::normalizePath(real_name);
    return local_name;
}

struct SourceDependChildren;
struct SourceFile {
    SourceFile() : deps(nullptr), type(QMakeSourceFileInfo::TYPE_UNKNOWN),
                   mocable(0), traversed(0), exists(1),
                   moc_checked(0), dep_checked(0), included_count(0) { }
    ~SourceFile();
    QMakeLocalFileName file;
    SourceDependChildren *deps;
    QMakeSourceFileInfo::SourceFileType type;
    uint mocable : 1, traversed : 1, exists : 1;
    uint moc_checked : 1,  dep_checked : 1;
    uchar included_count;
};
struct SourceDependChildren {
    SourceFile **children;
    int num_nodes, used_nodes;
    SourceDependChildren() : children(nullptr), num_nodes(0), used_nodes(0) { }
    ~SourceDependChildren() { if (children) free(children); children = nullptr; }
    void addChild(SourceFile *s) {
        if(num_nodes <= used_nodes) {
            num_nodes += 200;
            children = (SourceFile**)realloc(children, sizeof(SourceFile*)*(num_nodes));
        }
        children[used_nodes++] = s;
    }
};
SourceFile::~SourceFile() { delete deps; }
class SourceFiles {
    int hash(const char *);
public:
    SourceFiles();
    ~SourceFiles();

    SourceFile *lookupFile(const char *);
    inline SourceFile *lookupFile(const QString &f) { return lookupFile(f.toLatin1().constData()); }
    inline SourceFile *lookupFile(const QMakeLocalFileName &f) { return lookupFile(f.local().toLatin1().constData()); }
    void addFile(SourceFile *, const char *k = nullptr, bool own = true);

    struct SourceFileNode {
        SourceFileNode() : key(nullptr), next(nullptr), file(nullptr), own_file(1) { }
        ~SourceFileNode() {
            delete [] key;
            if(own_file)
                delete file;
        }
        char *key;
        SourceFileNode *next;
        SourceFile *file;
        uint own_file : 1;
    } **nodes;
    int num_nodes;
};
SourceFiles::SourceFiles()
{
    nodes = (SourceFileNode**)malloc(sizeof(SourceFileNode*)*(num_nodes=3037));
    for(int n = 0; n < num_nodes; n++)
        nodes[n] = nullptr;
}

SourceFiles::~SourceFiles()
{
    for(int n = 0; n < num_nodes; n++) {
        for(SourceFileNode *next = nodes[n]; next;) {
            SourceFileNode *next_next = next->next;
            delete next;
            next = next_next;
        }
    }
    free(nodes);
}

int SourceFiles::hash(const char *file)
{
    uint h = 0, g;
    while (*file) {
        h = (h << 4) + *file;
        if ((g = (h & 0xf0000000)) != 0)
            h ^= g >> 23;
        h &= ~g;
        file++;
    }
    return h;
}

SourceFile *SourceFiles::lookupFile(const char *file)
{
    int h = hash(file) % num_nodes;
    for(SourceFileNode *p = nodes[h]; p; p = p->next) {
        if(!strcmp(p->key, file))
            return p->file;
    }
    return nullptr;
}

void SourceFiles::addFile(SourceFile *p, const char *k, bool own_file)
{
    const QByteArray ba = p->file.local().toLatin1();
    if(!k)
        k = ba.constData();
    int h = hash(k) % num_nodes;
    SourceFileNode *pn = new SourceFileNode;
    pn->own_file = own_file;
    pn->key = qstrdup(k);
    pn->file = p;
    pn->next = nodes[h];
    nodes[h] = pn;
}

void QMakeSourceFileInfo::dependTreeWalker(SourceFile *node, SourceDependChildren *place)
{
    if(node->traversed || !node->exists)
        return;
    place->addChild(node);
    node->traversed = true; //set flag
    if(node->deps) {
        for(int i = 0; i < node->deps->used_nodes; i++)
            dependTreeWalker(node->deps->children[i], place);
    }
}

void QMakeSourceFileInfo::setDependencyPaths(const QVector<QMakeLocalFileName> &l)
{
    // Ensure that depdirs does not contain the same paths several times, to minimize the stats
    QVector<QMakeLocalFileName> ll;
    for (int i = 0; i < l.count(); ++i) {
        if (!ll.contains(l.at(i)))
            ll.append(l.at(i));
    }
    depdirs = ll;
}

QStringList QMakeSourceFileInfo::dependencies(const QString &file)
{
    QStringList ret;
    if(!files)
        return ret;

    if(SourceFile *node = files->lookupFile(QMakeLocalFileName(file))) {
        if(node->deps) {
            /* I stick them into a SourceDependChildren here because it is faster to just
               iterate over the list to stick them in the list, and reset the flag, then it is
               to loop over the tree (about 50% faster I saw) --Sam */
            SourceDependChildren place;
            for(int i = 0; i < node->deps->used_nodes; i++)
                dependTreeWalker(node->deps->children[i], &place);
            if(place.children) {
                for(int i = 0; i < place.used_nodes; i++) {
                    place.children[i]->traversed = false; //reset flag
                    ret.append(place.children[i]->file.real());
                }
           }
       }
    }
    return ret;
}

int
QMakeSourceFileInfo::included(const QString &file)
{
    if (!files)
        return 0;

    if(SourceFile *node = files->lookupFile(QMakeLocalFileName(file)))
        return node->included_count;
    return 0;
}

bool QMakeSourceFileInfo::mocable(const QString &file)
{
    if(SourceFile *node = files->lookupFile(QMakeLocalFileName(file)))
        return node->mocable;
    return false;
}

QMakeSourceFileInfo::QMakeSourceFileInfo(const QString &cf)
{
    //dep_mode
    dep_mode = Recursive;

    //quick project lookups
    includes = files = nullptr;
    files_changed = false;

    //buffer
    spare_buffer = nullptr;
    spare_buffer_size = 0;
}

QMakeSourceFileInfo::~QMakeSourceFileInfo()
{
    //buffer
    if(spare_buffer) {
        free(spare_buffer);
        spare_buffer = nullptr;
        spare_buffer_size = 0;
    }

    //quick project lookup
    delete files;
    delete includes;
}

void QMakeSourceFileInfo::addSourceFiles(const ProStringList &l, uchar seek,
                                         QMakeSourceFileInfo::SourceFileType type)
{
    for(int i=0; i<l.size(); ++i)
        addSourceFile(l.at(i).toQString(), seek, type);
}
void QMakeSourceFileInfo::addSourceFile(const QString &f, uchar seek,
                                        QMakeSourceFileInfo::SourceFileType type)
{
    if(!files)
        files = new SourceFiles;

    QMakeLocalFileName fn(f);
    SourceFile *file = files->lookupFile(fn);
    if(!file) {
        file = new SourceFile;
        file->file = fn;
        files->addFile(file);
    } else {
        if(file->type != type && file->type != TYPE_UNKNOWN && type != TYPE_UNKNOWN)
            warn_msg(WarnLogic, "%s is marked as %d, then %d!", f.toLatin1().constData(),
                     file->type, type);
    }
    if(type != TYPE_UNKNOWN)
        file->type = type;

    if(seek & SEEK_MOCS && !file->moc_checked)
        findMocs(file);
    if(seek & SEEK_DEPS && !file->dep_checked)
        findDeps(file);
}

bool QMakeSourceFileInfo::containsSourceFile(const QString &f, SourceFileType type)
{
    if(SourceFile *file = files->lookupFile(QMakeLocalFileName(f)))
        return (file->type == type || file->type == TYPE_UNKNOWN || type == TYPE_UNKNOWN);
    return false;
}

bool QMakeSourceFileInfo::isSystemInclude(const QString &name)
{
    if (QDir::isRelativePath(name)) {
        // if we got a relative path here, it's either an -I flag with a relative path
        // or an include file we couldn't locate. Either way, conclude it's not
        // a system include.
        return false;
    }

    for (int i = 0; i < systemIncludes.size(); ++i) {
        // check if name is located inside the system include dir:
        QDir systemDir(systemIncludes.at(i));
        QString relativePath = systemDir.relativeFilePath(name);

        // the relative path might be absolute if we're crossing drives on Windows
        if (QDir::isAbsolutePath(relativePath) || relativePath.startsWith("../"))
            continue;
        debug_msg(5, "File/dir %s is in system dir %s, skipping",
                  qPrintable(name), qPrintable(systemIncludes.at(i)));
        return true;
    }
    return false;
}

char *QMakeSourceFileInfo::getBuffer(int s) {
    if(!spare_buffer || spare_buffer_size < s)
        spare_buffer = (char *)realloc(spare_buffer, spare_buffer_size=s);
    return spare_buffer;
}

#ifndef S_ISDIR
#define S_ISDIR(x) (x & _S_IFDIR)
#endif

QMakeLocalFileName QMakeSourceFileInfo::fixPathForFile(const QMakeLocalFileName &f, bool)
{
    return f;
}

QMakeLocalFileName QMakeSourceFileInfo::findFileForDep(const QMakeLocalFileName &/*dep*/,
                                                       const QMakeLocalFileName &/*file*/)
{
    return QMakeLocalFileName();
}

QFileInfo QMakeSourceFileInfo::findFileInfo(const QMakeLocalFileName &dep)
{
    return QFileInfo(dep.real());
}

static int skipEscapedLineEnds(const char *buffer, int buffer_len, int offset, int *lines)
{
    // Join physical lines to make logical lines, as in the C preprocessor
    while (offset + 1 < buffer_len
           && buffer[offset] == '\\'
           && qmake_endOfLine(buffer[offset + 1])) {
        offset += 2;
        ++*lines;
        if (offset < buffer_len
            && buffer[offset - 1] == '\r'
            && buffer[offset] == '\n') // CRLF
            offset++;
    }
    return offset;
}

static bool matchWhileUnsplitting(const char *buffer, int buffer_len, int start,
                                  const char *needle, int needle_len,
                                  int *matchlen, int *lines)
{
    int x = start;
    for (int n = 0; n < needle_len;
         n++, x = skipEscapedLineEnds(buffer, buffer_len, x + 1, lines)) {
        if (x >= buffer_len || buffer[x] != needle[n])
            return false;
    }
    // That also skipped any remaining BSNLs immediately after the match.

    // Tell caller how long the match was:
    *matchlen = x - start;

    return true;
}

/* Advance from an opening quote at buffer[offset] to the matching close quote. */
static int scanPastString(char *buffer, int buffer_len, int offset, int *lines)
{
    // http://en.cppreference.com/w/cpp/language/string_literal
    // It might be a C++11 raw string.
    bool israw = false;
    if (buffer[offset] == '"' && offset > 0) {
        int explore = offset - 1;
        bool prefix = false; // One of L, U, u or u8 may appear before R
        bool saw8 = false; // Partial scan of u8
        while (explore >= 0) {
            // Cope with backslash-newline interruptions of the prefix:
            if (explore > 0
                && qmake_endOfLine(buffer[explore])
                && buffer[explore - 1] == '\\') {
                explore -= 2;
            } else if (explore > 1
                       && buffer[explore] == '\n'
                       && buffer[explore - 1] == '\r'
                       && buffer[explore - 2] == '\\') {
                explore -= 3;
                // Remaining cases can only decrement explore by one at a time:
            } else if (saw8 && buffer[explore] == 'u') {
                explore--;
                saw8 = false;
                prefix = true;
            } else if (saw8 || prefix) {
                break;
            } else if (explore > 1 && buffer[explore] == '8') {
                explore--;
                saw8 = true;
            } else if (buffer[explore] == 'L'
                       || buffer[explore] == 'U'
                       || buffer[explore] == 'u') {
                explore--;
                prefix = true;
            } else if (buffer[explore] == 'R') {
                if (israw)
                    break;
                explore--;
                israw = true;
            } else {
                break;
            }
        }
        // Check the R (with possible prefix) isn't just part of an identifier:
        if (israw && explore >= 0
            && (isalnum(buffer[explore]) || buffer[explore] == '_')) {
            israw = false;
        }
    }

    if (israw) {
#define SKIP_BSNL(pos) skipEscapedLineEnds(buffer, buffer_len, (pos), lines)

        offset = SKIP_BSNL(offset + 1);
        const char *const delim = buffer + offset;
        int clean = offset;
        while (offset < buffer_len && buffer[offset] != '(') {
            if (clean < offset)
                buffer[clean++] = buffer[offset];
            else
                clean++;

            offset = SKIP_BSNL(offset + 1);
        }
        /*
          Not checking correctness (trust real compiler to do that):
          - no controls, spaces, '(', ')', '\\' or (presumably) '"' in delim;
          - at most 16 bytes in delim

          Raw strings are surely defined after phase 2, when BSNLs are resolved;
          so the delimiter's exclusion of '\\' and space (including newlines)
          applies too late to save us the need to cope with BSNLs in it.
        */

        const int delimlen = buffer + clean - delim;
        int matchlen = delimlen, extralines = 0;
        while ((offset = SKIP_BSNL(offset + 1)) < buffer_len
               && (buffer[offset] != ')'
                   || (delimlen > 0 &&
                       !matchWhileUnsplitting(buffer, buffer_len,
                                              offset + 1, delim, delimlen,
                                              &matchlen, &extralines))
                   || buffer[offset + 1 + matchlen] != '"')) {
            // skip, but keep track of lines
            if (qmake_endOfLine(buffer[offset]))
                ++*lines;
            extralines = 0;
        }
        *lines += extralines; // from the match
        // buffer[offset] is ')'
        offset += 1 + matchlen; // 1 for ')', then delim
        // buffer[offset] is '"'

#undef SKIP_BSNL
    } else { // Traditional string or char literal:
        const char term = buffer[offset];
        while (++offset < buffer_len && buffer[offset] != term) {
            if (buffer[offset] == '\\')
                ++offset;
            else if (qmake_endOfLine(buffer[offset]))
                ++*lines;
        }
    }

    return offset;
}

bool QMakeSourceFileInfo::findDeps(SourceFile *file)
{
    if(file->dep_checked || file->type == TYPE_UNKNOWN)
        return true;
    files_changed = true;
    file->dep_checked = true;

    const QMakeLocalFileName sourceFile = fixPathForFile(file->file, true);

    struct stat fst;
    char *buffer = nullptr;
    int buffer_len = 0;
    {
        int fd;
#if defined(_MSC_VER) && _MSC_VER >= 1400
        if (_sopen_s(&fd, sourceFile.local().toLatin1().constData(),
            _O_RDONLY, _SH_DENYNO, _S_IREAD) != 0)
            fd = -1;
#else
        fd = open(sourceFile.local().toLatin1().constData(), O_RDONLY);
#endif
        if (fd == -1 || fstat(fd, &fst) || S_ISDIR(fst.st_mode)) {
            if (fd != -1)
                QT_CLOSE(fd);
            return false;
        }
        buffer = getBuffer(fst.st_size);
        for(int have_read = 0;
            (have_read = QT_READ(fd, buffer + buffer_len, fst.st_size - buffer_len));
            buffer_len += have_read) ;
        QT_CLOSE(fd);
    }
    if(!buffer)
        return false;
    if(!file->deps)
        file->deps = new SourceDependChildren;

    int line_count = 1;
    enum {
        /*
          States of C preprocessing (for TYPE_C only), after backslash-newline
          elimination and skipping comments and spaces (i.e. in ANSI X3.159-1989
          section 2.1.1.2's phase 4).  We're about to study buffer[x] to decide
          on which transition to do.
         */
        AtStart, // start of logical line; a # may start a preprocessor directive
        HadHash, // saw a # at start, looking for preprocessor keyword
        WantName, // saw #include or #import, waiting for name
        InCode // after directive, parsing non-#include directive or in actual code
    } cpp_state = AtStart;

    int x = 0;
    if (buffer_len >= 3) {
        const unsigned char *p = (unsigned char *)buffer;
        // skip UTF-8 BOM, if present
        if (p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF)
            x += 3;
    }
    for (; x < buffer_len; ++x) {
        bool try_local = true;
        char *inc = nullptr;
        if(file->type == QMakeSourceFileInfo::TYPE_UI) {
            // skip whitespaces
            while (x < buffer_len && (buffer[x] == ' ' || buffer[x] == '\t'))
                ++x;
            if (buffer[x] == '<') {
                ++x;
                if (buffer_len >= x + 12 && !strncmp(buffer + x, "includehint", 11) &&
                    (buffer[x + 11] == ' ' || buffer[x + 11] == '>')) {
                    for (x += 11; x < buffer_len && buffer[x] != '>'; ++x) {} // skip
                    int inc_len = 0;
                    for (++x; x + inc_len < buffer_len && buffer[x + inc_len] != '<'; ++inc_len) {} // skip
                    if (x + inc_len < buffer_len) {
                        buffer[x + inc_len] = '\0';
                        inc = buffer + x;
                    }
                } else if (buffer_len >= x + 13 && !strncmp(buffer + x, "customwidget", 12) &&
                           (buffer[x + 12] == ' ' || buffer[x + 12] == '>')) {
                    for (x += 13; x < buffer_len && buffer[x] != '>'; ++x) {} // skip up to >
                    while(x < buffer_len) {
                        while (++x < buffer_len && buffer[x] != '<') {} // skip up to <
                        x++;
                        if(buffer_len >= x + 7 && !strncmp(buffer+x, "header", 6) &&
                           (buffer[x + 6] == ' ' || buffer[x + 6] == '>')) {
                            for (x += 7; x < buffer_len && buffer[x] != '>'; ++x) {} // skip up to >
                            int inc_len = 0;
                            for (++x; x + inc_len < buffer_len && buffer[x + inc_len] != '<';
                                 ++inc_len) {} // skip
                            if (x + inc_len < buffer_len) {
                                buffer[x + inc_len] = '\0';
                                inc = buffer + x;
                            }
                            break;
                        } else if(buffer_len >= x + 14 && !strncmp(buffer+x, "/customwidget", 13) &&
                                  (buffer[x + 13] == ' ' || buffer[x + 13] == '>')) {
                            x += 14;
                            break;
                        }
                    }
                } else if(buffer_len >= x + 8 && !strncmp(buffer + x, "include", 7) &&
                          (buffer[x + 7] == ' ' || buffer[x + 7] == '>')) {
                    for (x += 8; x < buffer_len && buffer[x] != '>'; ++x) {
                        if (buffer_len >= x + 9 && buffer[x] == 'i' &&
                            !strncmp(buffer + x, "impldecl", 8)) {
                            for (x += 8; x < buffer_len && buffer[x] != '='; ++x) {} // skip
                            while (++x < buffer_len && (buffer[x] == '\t' || buffer[x] == ' ')) {} // skip
                            char quote = 0;
                            if (x < buffer_len && (buffer[x] == '\'' || buffer[x] == '"')) {
                                quote = buffer[x];
                                ++x;
                            }
                            int val_len;
                            for (val_len = 0; x + val_len < buffer_len; ++val_len) {
                                if(quote) {
                                    if (buffer[x + val_len] == quote)
                                        break;
                                } else if (buffer[x + val_len] == '>' ||
                                           buffer[x + val_len] == ' ') {
                                    break;
                                }
                            }
//?                            char saved = buffer[x + val_len];
                            if (x + val_len < buffer_len) {
                                buffer[x + val_len] = '\0';
                                if (!strcmp(buffer + x, "in implementation")) {
                                    //### do this
                                }
                            }
                        }
                    }
                    int inc_len = 0;
                    for (++x; x + inc_len < buffer_len && buffer[x + inc_len] != '<';
                         ++inc_len) {} // skip

                    if (x + inc_len < buffer_len) {
                        buffer[x + inc_len] = '\0';
                        inc = buffer + x;
                    }
                }
            }
            //read past new line now..
            for (; x < buffer_len && !qmake_endOfLine(buffer[x]); ++x) {} // skip
            ++line_count;
        } else if(file->type == QMakeSourceFileInfo::TYPE_QRC) {
        } else if(file->type == QMakeSourceFileInfo::TYPE_C) {
            // We've studied all buffer[i] for i < x
            for (; x < buffer_len; ++x) {
                // How to handle backslash-newline (BSNL) pairs:
#define SKIP_BSNL(pos) skipEscapedLineEnds(buffer, buffer_len, (pos), &line_count)

                // Seek code or directive, skipping comments and space:
                for (; (x = SKIP_BSNL(x)) < buffer_len; ++x) {
                    if (buffer[x] == ' ' || buffer[x] == '\t') {
                        // keep going
                    } else if (buffer[x] == '/') {
                        int extralines = 0;
                        int y = skipEscapedLineEnds(buffer, buffer_len, x + 1, &extralines);
                        if (y >= buffer_len) {
                            x = y;
                            break;
                        } else if (buffer[y] == '/') { // C++-style comment
                            line_count += extralines;
                            x = SKIP_BSNL(y + 1);
                            while (x < buffer_len && !qmake_endOfLine(buffer[x]))
                                x = SKIP_BSNL(x + 1); // skip

                            cpp_state = AtStart;
                            ++line_count;
                        } else if (buffer[y] == '*') { // C-style comment
                            line_count += extralines;
                            x = y;
                            while ((x = SKIP_BSNL(++x)) < buffer_len) {
                                if (buffer[x] == '*') {
                                    extralines = 0;
                                    y = skipEscapedLineEnds(buffer, buffer_len,
                                                            x + 1, &extralines);
                                    if (y < buffer_len && buffer[y] == '/') {
                                        line_count += extralines;
                                        x = y; // for loop shall step past this
                                        break;
                                    }
                                } else if (qmake_endOfLine(buffer[x])) {
                                    ++line_count;
                                }
                            }
                        } else {
                            // buffer[x] is the division operator
                            break;
                        }
                    } else if (qmake_endOfLine(buffer[x])) {
                        ++line_count;
                        cpp_state = AtStart;
                    } else {
                        /* Drop out of phases 1, 2, 3, into phase 4 */
                        break;
                    }
                }
                // Phase 4 study of buffer[x]:

                if(x >= buffer_len)
                    break;

                switch (cpp_state) {
                case HadHash:
                {
                    // Read keyword; buffer[x] starts first preprocessing token after #
                    const char *const keyword = buffer + x;
                    int clean = x;
                    while (x < buffer_len && buffer[x] >= 'a' && buffer[x] <= 'z') {
                        // skip over keyword, consolidating it if it contains BSNLs
                        // (see WantName's similar code consolidating inc, below)
                        if (clean < x)
                            buffer[clean++] = buffer[x];
                        else
                            clean++;

                        x = SKIP_BSNL(x + 1);
                    }
                    const int keyword_len = buffer + clean - keyword;
                    x--; // Still need to study buffer[x] next time round for loop.

                    cpp_state =
                        ((keyword_len == 7 && !strncmp(keyword, "include", 7)) // C & Obj-C
                      || (keyword_len == 6 && !strncmp(keyword, "import", 6))) // Obj-C
                        ? WantName : InCode;
                    break;
                }

                case WantName:
                {
                    char term = buffer[x];
                    if (term == '<') {
                        try_local = false;
                        term = '>';
                    } else if (term != '"') {
                        /*
                          Possibly malformed, but this may be something like:
                          #include IDENTIFIER
                          which does work, if #define IDENTIFIER "filename" is
                          in effect.  This is beyond this noddy preprocessor's
                          powers of tracking.  So give up and resume searching
                          for a directive.  We haven't made sense of buffer[x],
                          so back up to ensure we do study it (now as code) next
                          time round the loop.
                        */
                        x--;
                        cpp_state = InCode;
                        continue;
                    }

                    x = SKIP_BSNL(x + 1);
                    inc = buffer + x;
                    int clean = x; // offset if we need to clear \-newlines
                    for (; x < buffer_len && buffer[x] != term; x = SKIP_BSNL(x + 1)) {
                        if (qmake_endOfLine(buffer[x])) { // malformed
                            cpp_state = AtStart;
                            ++line_count;
                            break;
                        }

                        /*
                          If we do skip any BSNLs, we need to consolidate the
                          surviving text by copying to lower indices.  For that
                          to be possible, we also have to keep 'clean' advanced
                          in step with x even when we've yet to see any BSNLs.
                        */
                        if (clean < x)
                            buffer[clean++] = buffer[x];
                        else
                            clean++;
                    }
                    if (cpp_state == WantName)
                        buffer[clean] = '\0';
                    else // i.e. malformed
                        inc = nullptr;

                    cpp_state = InCode; // hereafter
                    break;
                }

                case AtStart:
                    // Preprocessor directive?
                    if (buffer[x] == '#') {
                        cpp_state = HadHash;
                        break;
                    }
                    cpp_state = InCode;
                    Q_FALLTHROUGH(); // to handle buffer[x] as such.
                case InCode:
                    // matching quotes (string literals and character literals)
                    if (buffer[x] == '\'' || buffer[x] == '"') {
                        x = scanPastString(buffer, buffer_len, x, &line_count);
                        // for loop's ++x shall step over the closing quote.
                    }
                    // else: buffer[x] is just some code; move on.
                    break;
                }

                if (inc) // We were in WantName and found a name.
                    break;
#undef SKIP_BSNL
            }
            if(x >= buffer_len)
                break;
        }

        if(inc) {
            if(!includes)
                includes = new SourceFiles;
            /* QTBUG-72383: Local includes "foo.h" must first be resolved relative to the
             * sourceDir, only global includes <bar.h> are unique. */
            SourceFile *dep = try_local ? nullptr : includes->lookupFile(inc);
            if(!dep) {
                bool exists = false;
                QMakeLocalFileName lfn(inc);
                if(QDir::isRelativePath(lfn.real())) {
                    if(try_local) {
                        QDir sourceDir = findFileInfo(sourceFile).dir();
                        QMakeLocalFileName f(sourceDir.absoluteFilePath(lfn.local()));
                        if(findFileInfo(f).exists()) {
                            lfn = fixPathForFile(f);
                            exists = true;
                        }
                    }
                    if(!exists) { //path lookup
                        for (const QMakeLocalFileName &depdir : qAsConst(depdirs)) {
                            QMakeLocalFileName f(depdir.real() + Option::dir_sep + lfn.real());
                            QFileInfo fi(findFileInfo(f));
                            if(fi.exists() && !fi.isDir()) {
                                lfn = fixPathForFile(f);
                                exists = true;
                                break;
                            }
                        }
                    }
                    if(!exists) { //heuristic lookup
                        lfn = findFileForDep(QMakeLocalFileName(inc), file->file);
                        if((exists = !lfn.isNull()))
                            lfn = fixPathForFile(lfn);
                    }
                } else {
                    exists = QFile::exists(lfn.real());
                }
                if (!lfn.isNull() && !isSystemInclude(lfn.real())) {
                    dep = files->lookupFile(lfn);
                    if(!dep) {
                        dep = new SourceFile;
                        dep->file = lfn;
                        dep->type = QMakeSourceFileInfo::TYPE_C;
                        files->addFile(dep);
                        /* QTBUG-72383: Local includes "foo.h" are keyed by the resolved
                         * path (stored in dep itself), only global includes <bar.h> are
                         * unique keys immediately. */
                        const char *key = try_local ? nullptr : inc;
                        includes->addFile(dep, key, false);
                    }
                    dep->exists = exists;
                }
            }
            if(dep && dep->file != file->file) {
                dep->included_count++;
                if(dep->exists) {
                    debug_msg(5, "%s:%d Found dependency to %s", file->file.real().toLatin1().constData(),
                              line_count, dep->file.local().toLatin1().constData());
                    file->deps->addChild(dep);
                }
            }
        }
    }
    if(dependencyMode() == Recursive) { //done last because buffer is shared
        for(int i = 0; i < file->deps->used_nodes; i++) {
            if(!file->deps->children[i]->deps)
                findDeps(file->deps->children[i]);
        }
    }
    return true;
}

static bool isCWordChar(char c) {
    return c == '_'
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9');
}

bool QMakeSourceFileInfo::findMocs(SourceFile *file)
{
    if(file->moc_checked)
        return true;
    files_changed = true;
    file->moc_checked = true;

    int buffer_len = 0;
    char *buffer = nullptr;
    {
        struct stat fst;
        int fd;
#if defined(_MSC_VER) && _MSC_VER >= 1400
        if (_sopen_s(&fd, fixPathForFile(file->file, true).local().toLocal8Bit().constData(),
            _O_RDONLY, _SH_DENYNO, _S_IREAD) != 0)
            fd = -1;
#else
        fd = open(fixPathForFile(file->file, true).local().toLocal8Bit().constData(), O_RDONLY);
#endif
        if (fd == -1 || fstat(fd, &fst) || S_ISDIR(fst.st_mode)) {
            if (fd != -1)
                QT_CLOSE(fd);
            return false; //shouldn't happen
        }
        buffer = getBuffer(fst.st_size);
        while (int have_read = QT_READ(fd, buffer + buffer_len, fst.st_size - buffer_len))
            buffer_len += have_read;

        QT_CLOSE(fd);
    }

    debug_msg(2, "findMocs: %s", file->file.local().toLatin1().constData());
    int line_count = 1;
    // [0] for Q_OBJECT, [1] for Q_GADGET, [2] for Q_NAMESPACE, [3] for Q_NAMESPACE_EXPORT
    bool ignore[4] = { false, false, false, false };
 /* qmake ignore Q_GADGET */
 /* qmake ignore Q_OBJECT */
 /* qmake ignore Q_NAMESPACE */
 /* qmake ignore Q_NAMESPACE_EXPORT */
    for(int x = 0; x < buffer_len; x++) {
#define SKIP_BSNL(pos) skipEscapedLineEnds(buffer, buffer_len, (pos), &line_count)
        x = SKIP_BSNL(x);
        if (buffer[x] == '/') {
            int extralines = 0;
            int y = skipEscapedLineEnds(buffer, buffer_len, x + 1, &extralines);
            if (buffer_len > y) {
                // If comment, advance to the character that ends it:
                if (buffer[y] == '/') { // C++-style comment
                    line_count += extralines;
                    x = y;
                    do {
                        x = SKIP_BSNL(x + 1);
                    } while (x < buffer_len && !qmake_endOfLine(buffer[x]));

                } else if (buffer[y] == '*') { // C-style comment
                    line_count += extralines;
                    x = SKIP_BSNL(y + 1);
                    for (; x < buffer_len; x = SKIP_BSNL(x + 1)) {
                        if (buffer[x] == 't' || buffer[x] == 'q') { // ignore
                            if(buffer_len >= (x + 20) &&
                               !strncmp(buffer + x + 1, "make ignore Q_OBJECT", 20)) {
                                debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_OBJECT\"",
                                          file->file.real().toLatin1().constData(), line_count);
                                x += 20;
                                ignore[0] = true;
                            } else if(buffer_len >= (x + 20) &&
                                      !strncmp(buffer + x + 1, "make ignore Q_GADGET", 20)) {
                                debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_GADGET\"",
                                          file->file.real().toLatin1().constData(), line_count);
                                x += 20;
                                ignore[1] = true;
                            } else if (buffer_len >= (x + 23) &&
                                      !strncmp(buffer + x + 1, "make ignore Q_NAMESPACE", 23)) {
                                debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_NAMESPACE\"",
                                          file->file.real().toLatin1().constData(), line_count);
                                x += 23;
                                ignore[2] = true;
                            } else if (buffer_len >= (x + 30) &&
                                       !strncmp(buffer + x + 1, "make ignore Q_NAMESPACE_EXPORT", 30)) {
                                 debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_NAMESPACE_EXPORT\"",
                                           file->file.real().toLatin1().constData(), line_count);
                                 x += 30;
                                 ignore[3] = true;
                            }
                        } else if (buffer[x] == '*') {
                            extralines = 0;
                            y = skipEscapedLineEnds(buffer, buffer_len, x + 1, &extralines);
                            if (buffer_len > y && buffer[y] == '/') {
                                line_count += extralines;
                                x = y;
                                break;
                            }
                        } else if (Option::debug_level && qmake_endOfLine(buffer[x])) {
                            ++line_count;
                        }
                    }
                }
                // else: don't update x, buffer[x] is just the division operator.
            }
        } else if (buffer[x] == '\'' || buffer[x] == '"') {
            x = scanPastString(buffer, buffer_len, x, &line_count);
            // Leaves us on closing quote; for loop's x++ steps us past it.
        }

        if (x < buffer_len && Option::debug_level && qmake_endOfLine(buffer[x]))
            ++line_count;
        if (buffer_len > x + 8 && !isCWordChar(buffer[x])) {
            int morelines = 0;
            int y = skipEscapedLineEnds(buffer, buffer_len, x + 1, &morelines);
            if (buffer[y] == 'Q') {
                static const char interesting[][19] = { "Q_OBJECT", "Q_GADGET", "Q_NAMESPACE", "Q_NAMESPACE_EXPORT" };
                for (int interest = 0; interest < 4; ++interest) {
                    if (ignore[interest])
                        continue;

                    int matchlen = 0, extralines = 0;
                    size_t needle_len = strlen(interesting[interest]);
                    Q_ASSERT(needle_len <= INT_MAX);
                    if (matchWhileUnsplitting(buffer, buffer_len, y,
                                              interesting[interest],
                                              static_cast<int>(needle_len),
                                              &matchlen, &extralines)
                        && y + matchlen < buffer_len
                        && !isCWordChar(buffer[y + matchlen])) {
                        if (Option::debug_level) {
                            buffer[y + matchlen] = '\0';
                            debug_msg(2, "Mocgen: %s:%d Found MOC symbol %s",
                                      file->file.real().toLatin1().constData(),
                                      line_count + morelines, buffer + y);
                        }
                        file->mocable = true;
                        return true;
                    }
                }
            }
        }
#undef SKIP_BSNL
    }
    return true;
}

QT_END_NAMESPACE
