/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmocscanner application of the Qt Toolkit.
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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <unordered_set>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>

struct ScanResult
{
    std::string fileName;
    bool foundMocRelevantMacro = false;
    std::vector<std::string> includedMocFiles;
};

struct Option
{
    static int debug_level;
};
int Option::debug_level = 0;

static void debug_msg(int level, const char *fmt, ...)
{
    if (level < 3)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

#define qmake_endOfLine(c) (c == '\r' || c == '\n')

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

static std::vector<std::string> findIncludes(const char *fileName, char *buffer, int buffer_len)
{
    std::vector<std::string> includes;
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

            if (x >= buffer_len)
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
        if (x >= buffer_len)
            break;

        if (inc) {
            includes.push_back(inc);
        }
    }
    return includes;
}

static bool isCWordChar(char c) {
    return c == '_'
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9');
}

static bool scanBufferForMocRelevantMacros(const char *fileName, char *buffer, int buffer_len)
{
    int line_count = 1;
    bool ignore[3] = { false, false, false }; // [0] for Q_OBJECT, [1] for Q_GADGET, [2] for Q_NAMESPACE
 /* qmake ignore Q_GADGET */
 /* qmake ignore Q_OBJECT */
 /* qmake ignore Q_NAMESPACE */
    for (int x = 0; x < buffer_len; x++) {
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
                            if (buffer_len >= (x + 20) &&
                               !strncmp(buffer + x + 1, "make ignore Q_OBJECT", 20)) {
                                debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_OBJECT\"",
                                          fileName, line_count);
                                x += 20;
                                ignore[0] = true;
                            } else if (buffer_len >= (x + 20) &&
                                      !strncmp(buffer + x + 1, "make ignore Q_GADGET", 20)) {
                                debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_GADGET\"",
                                          fileName, line_count);
                                x += 20;
                                ignore[1] = true;
                            } else if (buffer_len >= (x + 23) &&
                                      !strncmp(buffer + x + 1, "make ignore Q_NAMESPACE", 23)) {
                                debug_msg(2, "Mocgen: %s:%d Found \"qmake ignore Q_NAMESPACE\"",
                                          fileName, line_count);
                                x += 23;
                                ignore[2] = true;
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
                static const char interesting[][12] = { "Q_OBJECT", "Q_GADGET", "Q_NAMESPACE"};
                for (int interest = 0; interest < 3; ++interest) {
                    if (ignore[interest])
                        continue;

                    int matchlen = 0, extralines = 0;
                    size_t needle_len = strlen(interesting[interest]);
                    assert(needle_len <= INT_MAX);
                    if (matchWhileUnsplitting(buffer, buffer_len, y,
                                              interesting[interest],
                                              static_cast<int>(needle_len),
                                              &matchlen, &extralines)
                        && y + matchlen < buffer_len
                        && !isCWordChar(buffer[y + matchlen])) {
                        if (Option::debug_level) {
                            buffer[y + matchlen] = '\0';
                            debug_msg(2, "Mocgen: %s:%d Found MOC symbol %s",
                                      fileName,
                                      line_count + morelines, buffer + y);
                        }
                        return true;
                    }
                }
            }
        }
#undef SKIP_BSNL
    }
    return false;
}

static bool startsWith(const std::string &str, const char *needle)
{
    return str.find(needle) == 0;
}

static bool endsWith(const std::string &str, const char *needle)
{
    return str.rfind(needle) == str.size() - strlen(needle);
}

static std::string fileNameFromPath(const std::string &path)
{
    int separatorIndex = path.rfind("/");
    if (separatorIndex == -1)
        return path;
    return path.substr(separatorIndex + 1);
}

static ScanResult scanFile(const std::string &fileName)
{
    ScanResult result;
    result.fileName = fileName;

    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open())
        return result;

    file.seekg(0, std::ios_base::end);
    auto size = file.tellg();
    if (size == 0)
        return result;
    file.seekg(0, std::ios_base::beg);

    std::vector<char> buffer;
    buffer.resize(size);
    char *bufferPtr = &(*buffer.begin());
    file.read(bufferPtr, size);

    file.close();

    result.foundMocRelevantMacro = scanBufferForMocRelevantMacros(fileName.c_str(), bufferPtr, size);

    for (const auto &include: findIncludes(fileName.c_str(), bufferPtr, size)) {
        if (startsWith(include, "moc_") && endsWith(include, ".cpp")) {
            std::string mocHeader = include.substr(strlen("moc_"),
                                                   include.size() - strlen("moc_") - strlen(".cpp"));
            mocHeader.append(".h");
            result.includedMocFiles.push_back(mocHeader);
        } else if (endsWith(include, ".moc")) {
            std::string mocHeader = include.substr(0, include.size() - strlen(".moc"));
            mocHeader.append(".cpp");
            result.includedMocFiles.push_back(mocHeader);
        }
    }

    return result;
}

static void writeSetToFile(const char *fileName, const std::set<std::string> &content)
{
    std::ofstream file(fileName, std::ios::binary);
    for (const auto &entry: content) {
        const std::string &fileName = entry;
        file << fileName << std::endl;
    }
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <sources file> <included mocs output file> <built mocs output file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    std::vector<std::string> sourcesAndHeaders;
    {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "failed to open input file " << argv[1] << std::endl;
            return EXIT_FAILURE;
        }
        for (std::string line; std::getline(file, line);)
            sourcesAndHeaders.push_back(line);
    }

    std::unordered_set<std::string> mocCandidates;
    std::unordered_set<std::string> mocFilesInIncludeStatements;

    for (const auto &sourceOrHeader: sourcesAndHeaders) {
        auto result = scanFile(sourceOrHeader);
        if (result.foundMocRelevantMacro)
            mocCandidates.insert(result.fileName);
        for (const auto &header: result.includedMocFiles)
            mocFilesInIncludeStatements.insert(header);
    }

    std::set<std::string> mocFilesToInclude;
    std::set<std::string> mocFilesToBuild;

    for (const auto &candidate: mocCandidates) {
        std::string fileName = fileNameFromPath(candidate);
        if (mocFilesInIncludeStatements.find(fileName) != mocFilesInIncludeStatements.end())
            mocFilesToInclude.insert(candidate);
        else
            mocFilesToBuild.insert(candidate);
    }

    writeSetToFile(argv[2], mocFilesToInclude);
    writeSetToFile(argv[3], mocFilesToBuild);

    return EXIT_SUCCESS;
}
