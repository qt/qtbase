/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

//#define QMAKE_USE_CACHE

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
    SourceFile() : deps(0), type(QMakeSourceFileInfo::TYPE_UNKNOWN),
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
    SourceDependChildren() : children(0), num_nodes(0), used_nodes(0) { }
    ~SourceDependChildren() { if(children) free(children); children = 0; }
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
    void addFile(SourceFile *, const char *k=0, bool own=true);

    struct SourceFileNode {
        SourceFileNode() : key(0), next(0), file(0), own_file(1) { }
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
        nodes[n] = 0;
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
    return 0;
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

void QMakeSourceFileInfo::setDependencyPaths(const QList<QMakeLocalFileName> &l)
{
    // Ensure that depdirs does not contain the same paths several times, to minimize the stats
    QList<QMakeLocalFileName> ll;
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
    includes = files = 0;
    files_changed = false;

    //buffer
    spare_buffer = 0;
    spare_buffer_size = 0;

    //cache
    cachefile = cf;
    if(!cachefile.isEmpty())
        loadCache(cachefile);
}

QMakeSourceFileInfo::~QMakeSourceFileInfo()
{
    //cache
    if(!cachefile.isEmpty() /*&& files_changed*/)
        saveCache(cachefile);

    //buffer
    if(spare_buffer) {
        free(spare_buffer);
        spare_buffer = 0;
        spare_buffer_size = 0;
    }

    //quick project lookup
    delete files;
    delete includes;
}

void QMakeSourceFileInfo::setCacheFile(const QString &cf)
{
    cachefile = cf;
    loadCache(cachefile);
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
    for (int n = 0; n < needle_len && x < buffer_len;
         n++, x = skipEscapedLineEnds(buffer, buffer_len, x + 1, lines)) {
        if (buffer[x] != needle[n])
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
    // It might be a C++11 raw string.
    bool israw = false;
    if (buffer[offset] == '"' && offset > 0) {
        int explore = offset - 1;
        while (explore > 0 && buffer[explore] != 'R') {
            if (buffer[explore] == '8' || buffer[explore] == 'u' || buffer[explore] == 'U') {
                explore--;
            } else if (explore > 1 && qmake_endOfLine(buffer[explore])
                       && buffer[explore - 1] == '\\') {
                explore -= 2;
            } else if (explore > 2 && buffer[explore] == '\n'
                       && buffer[explore - 1] == '\r'
                       && buffer[explore - 2] == '\\') {
                explore -= 3;
            } else {
                break;
            }
        }
        israw = (buffer[explore] == 'R');
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
    char *buffer = 0;
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

    for(int x = 0; x < buffer_len; ++x) {
        bool try_local = true;
        char *inc = 0;
        if(file->type == QMakeSourceFileInfo::TYPE_UI) {
            // skip whitespaces
            while (x < buffer_len && (buffer[x] == ' ' || buffer[x] == '\t'))
                ++x;
            if (buffer[x] == '<') {
                ++x;
                if (buffer_len >= x + 12 && !strncmp(buffer + x, "includehint", 11) &&
                    (buffer[x + 11] == ' ' || buffer[x + 11] == '>')) {
                    for (x += 11; buffer[x] != '>'; ++x) {} // skip
                    int inc_len = 0;
                    for (x += 1 ; buffer[x + inc_len] != '<'; ++inc_len) {} // skip
                    buffer[x + inc_len] = '\0';
                    inc = buffer + x;
                } else if (buffer_len >= x + 13 && !strncmp(buffer + x, "customwidget", 12) &&
                           (buffer[x + 12] == ' ' || buffer[x + 12] == '>')) {
                    for (x += 13; buffer[x] != '>'; ++x) {} // skip up to >
                    while(x < buffer_len) {
                        for (x++; buffer[x] != '<'; ++x) {} // skip up to <
                        x++;
                        if(buffer_len >= x + 7 && !strncmp(buffer+x, "header", 6) &&
                           (buffer[x + 6] == ' ' || buffer[x + 6] == '>')) {
                            for (x += 7; buffer[x] != '>'; ++x) {} // skip up to >
                            int inc_len = 0;
                            for (x += 1 ; buffer[x + inc_len] != '<'; ++inc_len) {} // skip
                            buffer[x + inc_len] = '\0';
                            inc = buffer + x;
                            break;
                        } else if(buffer_len >= x + 14 && !strncmp(buffer+x, "/customwidget", 13) &&
                                  (buffer[x + 13] == ' ' || buffer[x + 13] == '>')) {
                            x += 14;
                            break;
                        }
                    }
                } else if(buffer_len >= x + 8 && !strncmp(buffer + x, "include", 7) &&
                          (buffer[x + 7] == ' ' || buffer[x + 7] == '>')) {
                    for (x += 8; buffer[x] != '>'; ++x) {
                        if (buffer_len >= x + 9 && buffer[x] == 'i' &&
                            !strncmp(buffer + x, "impldecl", 8)) {
                            for (x += 8; buffer[x] != '='; ++x) {} // skip
                            if (buffer[x] != '=')
                                continue;
                            for (++x; buffer[x] == '\t' || buffer[x] == ' '; ++x) {} // skip
                            char quote = 0;
                            if (buffer[x] == '\'' || buffer[x] == '"') {
                                quote = buffer[x];
                                ++x;
                            }
                            int val_len;
                            for(val_len = 0; true; ++val_len) {
                                if(quote) {
                                    if (buffer[x + val_len] == quote)
                                        break;
                                } else if (buffer[x + val_len] == '>' ||
                                           buffer[x + val_len] == ' ') {
                                    break;
                                }
                            }
//?                            char saved = buffer[x + val_len];
                            buffer[x + val_len] = '\0';
                            if(!strcmp(buffer+x, "in implementation")) {
                                //### do this
                            }
                        }
                    }
                    int inc_len = 0;
                    for (x += 1 ; buffer[x + inc_len] != '<'; ++inc_len) {} // skip
                    buffer[x + inc_len] = '\0';
                    inc = buffer + x;
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
                for(; x < buffer_len; ++x) {
                    x = SKIP_BSNL(x);
                    if (buffer[x] == ' ' || buffer[x] == '\t') {
                        // keep going
                    } else if (buffer[x] == '/') {
                        int extralines = 0;
                        int y = skipEscapedLineEnds(buffer, buffer_len, x + 1, &extralines);
                        if (buffer[y] == '/') { // C++-style comment
                            line_count += extralines;
                            x = SKIP_BSNL(y + 1);
                            while (x < buffer_len && !qmake_endOfLine(buffer[x]))
                                x = SKIP_BSNL(x + 1); // skip

                            cpp_state = AtStart;
                            ++line_count;
                        } else if (buffer[y] == '*') { // C-style comment
                            line_count += extralines;
                            x = y;
                            while (++x < buffer_len) {
                                x = SKIP_BSNL(x);
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
                        inc = 0;

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
                    // ... and fall through to handle buffer[x] as such.
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
            SourceFile *dep = includes->lookupFile(inc);
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
                        for(QList<QMakeLocalFileName>::Iterator it = depdirs.begin(); it != depdirs.end(); ++it) {
                            QMakeLocalFileName f((*it).real() + Option::dir_sep + lfn.real());
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
                        includes->addFile(dep, inc, false);
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
    char *buffer = 0;
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
    bool ignore[2] = { false, false }; // [0] for Q_OBJECT, [1] for Q_GADGET
 /* qmake ignore Q_GADGET */
 /* qmake ignore Q_OBJECT */
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
                static const char interesting[][9] = { "Q_OBJECT", "Q_GADGET" };
                for (int interest = 0; interest < 2; ++interest) {
                    if (ignore[interest])
                        continue;

                    int matchlen = 0, extralines = 0;
                    if (matchWhileUnsplitting(buffer, buffer_len, y,
                                              interesting[interest],
                                              strlen(interesting[interest]),
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


void QMakeSourceFileInfo::saveCache(const QString &cf)
{
#ifdef QMAKE_USE_CACHE
    if(cf.isEmpty())
        return;

    QFile file(QMakeLocalFileName(cf).local());
    if(file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << qmake_version() << endl << endl; //version
        { //cache verification
            QMap<QString, QStringList> verify = getCacheVerification();
             stream << verify.count() << endl;
             for(QMap<QString, QStringList>::iterator it = verify.begin();
                 it != verify.end(); ++it) {
                 stream << it.key() << endl << it.value().join(';') << endl;
             }
             stream << endl;
        }
        if(files->nodes) {
            for(int file = 0; file < files->num_nodes; ++file) {
                for(SourceFiles::SourceFileNode *node = files->nodes[file]; node; node = node->next) {
                    stream << node->file->file.local() << endl; //source
                    stream << node->file->type << endl; //type

                    //depends
                    stream << ";";
                    if(node->file->deps) {
                        for(int depend = 0; depend < node->file->deps->used_nodes; ++depend) {
                            if(depend)
                                stream << ";";
                            stream << node->file->deps->children[depend]->file.local();
                        }
                    }
                    stream << endl;

                    stream << node->file->mocable << endl; //mocable
                    stream << endl; //just for human readability
                }
            }
        }
        stream.flush();
        file.close();
    }
#else
    Q_UNUSED(cf);
#endif
}

void QMakeSourceFileInfo::loadCache(const QString &cf)
{
    if(cf.isEmpty())
        return;

#ifdef QMAKE_USE_CACHE
    QMakeLocalFileName cache_file(cf);
    int fd = open(QMakeLocalFileName(cf).local().toLatin1(), O_RDONLY);
    if(fd == -1)
        return;
    QFileInfo cache_fi = findFileInfo(cache_file);
    if(!cache_fi.exists() || cache_fi.isDir())
        return;

    QFile file;
    if(!file.open(QIODevice::ReadOnly, fd))
        return;
    QTextStream stream(&file);

    if(stream.readLine() == qmake_version()) { //version check
        stream.skipWhiteSpace();

        bool verified = true;
        { //cache verification
            QMap<QString, QStringList> verify;
            int len = stream.readLine().toInt();
            for(int i = 0; i < len; ++i) {
                QString var = stream.readLine();
                QString val = stream.readLine();
                verify.insert(var, val.split(';', QString::SkipEmptyParts));
            }
            verified = verifyCache(verify);
        }
        if(verified) {
            stream.skipWhiteSpace();
            if(!files)
                files = new SourceFiles;
            while(!stream.atEnd()) {
                QString source = stream.readLine();
                QString type = stream.readLine();
                QString depends = stream.readLine();
                QString mocable = stream.readLine();
                stream.skipWhiteSpace();

                QMakeLocalFileName fn(source);
                QFileInfo fi = findFileInfo(fn);

                SourceFile *file = files->lookupFile(fn);
                if(!file) {
                    file = new SourceFile;
                    file->file = fn;
                    files->addFile(file);
                    file->type = (SourceFileType)type.toInt();
                    file->exists = fi.exists();
                }
                if(fi.exists() && fi.lastModified() < cache_fi.lastModified()) {
                    if(!file->dep_checked) { //get depends
                        if(!file->deps)
                            file->deps = new SourceDependChildren;
                        file->dep_checked = true;
                        QStringList depend_list = depends.split(";", QString::SkipEmptyParts);
                        for(int depend = 0; depend < depend_list.size(); ++depend) {
                            QMakeLocalFileName dep_fn(depend_list.at(depend));
                            QFileInfo dep_fi(findFileInfo(dep_fn));
                            SourceFile *dep = files->lookupFile(dep_fn);
                            if(!dep) {
                                dep = new SourceFile;
                                dep->file = dep_fn;
                                dep->exists = dep_fi.exists();
                                dep->type = QMakeSourceFileInfo::TYPE_UNKNOWN;
                                files->addFile(dep);
                            }
                            dep->included_count++;
                            file->deps->addChild(dep);
                        }
                    }
                    if(!file->moc_checked) { //get mocs
                        file->moc_checked = true;
                        file->mocable = mocable.toInt();
                    }
                }
            }
        }
    }
#endif
}

QMap<QString, QStringList> QMakeSourceFileInfo::getCacheVerification()
{
    return QMap<QString, QStringList>();
}

bool QMakeSourceFileInfo::verifyCache(const QMap<QString, QStringList> &v)
{
    return v == getCacheVerification();
}

QT_END_NAMESPACE
