/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
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

#include <QtCore>

const QString quadQuote = QStringLiteral("\"\""); // Closes one string, opens a new one.

static QString utf8encode(const QByteArray &array) // turns e.g. tranøy.no to tran\xc3\xb8y.no
{
    QString result;
    result.reserve(array.length() + array.length() / 3);
    bool wasHex = false;
    for (int i = 0; i < array.length(); ++i) {
        char c = array.at(i);
        // if char is non-ascii, escape it
        if (c < 0x20 || uchar(c) >= 0x7f) {
            result += "\\x" + QString::number(uchar(c), 16);
            wasHex = true;
        } else {
            // if previous char was escaped, we need to make sure the next char is not
            // interpreted as part of the hex value, e.g. "äc.com" -> "\xabc.com"; this
            // should be "\xab""c.com"
            bool isHexChar = ((c >= '0' && c <= '9') ||
                              (c >= 'a' && c <= 'f') ||
                              (c >= 'A' && c <= 'F'));
            if (wasHex && isHexChar)
                result += quadQuote;
            result += c;
            wasHex = false;
        }
    }
    return result;
}

/*
    Digest public suffix data into efficiently-searchable form.

    Takes the public suffix list (see usage message), a list of DNS domains
    whose child domains should not be presumed to trust one another, and
    converts it to a form that lets qtbase/src/corelib/io/qtldurl.cpp's query
    functions find entries efficiently.

    Each line of the suffix file (aside from comments and blanks) gives a suffix
    (starting with a dot) with an optional prefix of '*' (to include every
    immediate child) or of '!'  (to exclude the suffix, e.g. from a '*' line for
    a tail of it).  A line with neither of these prefixes is an exact match.

    Each line is hashed and the hash is reduced modulo the number of lines
    (tldCount); lines are grouped by reduced hash and separated by '\0' bytes
    within each group. Conceptually, the groups are then emitted to a single
    huge string, along with a table (tldIndices[tldCount]) of indices into that
    string of the starts of the the various groups.

    However, that huge string would exceed the 64k limit at least one compiler
    imposes on a single string literal, so we actually split up the huge string
    into an array of chunks, each less than 64k in size. Each group is written
    to a single chunk (so we start a new chunk if the next group would take the
    present chunk over the limit). There are tldChunkCount chunks; their lengths
    are saved in tldChunks[tldChunkCount]; the chunks themselves in
    tldData[tldChunkCount]. See qtldurl.cpp's containsTLDEntry() for how to
    search for a string in the resulting data.
*/

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 3) {
        printf("\nUsage: ./%s inputFile outputFile\n\n", argv[0]);
        printf("'inputFile' should be a list of effective TLDs, one per line,\n");
        printf("as obtained from http://publicsuffix.org/. To create indices and data\n");
        printf("file, do the following:\n\n");
        printf("       wget https://publicsuffix.org/list/public_suffix_list.dat -O public_suffix_list.dat\n");
        printf("       grep -v '^//' public_suffix_list.dat | grep . > public_suffix_list.dat.trimmed\n");
        printf("       ./%s public_suffix_list.dat.trimmed public_suffix_list.cpp\n\n", argv[0]);
        printf("Now replace the code in qtbase/src/corelib/io/qurltlds_p.h with public_suffix_list.cpp's contents\n\n");
        return 1;
    }
    QFile file(argv[1]);
    if (!file.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "Failed to open input file (%s); see %s -usage", argv[1], argv[0]);
        return 1;
    }

    QFile outFile(argv[2]);
    if (!outFile.open(QIODevice::WriteOnly)) {
        file.close();
        fprintf(stderr, "Failed to open output file (%s); see %s -usage", argv[2], argv[0]);
        return 1;
    }

    // Write tldData[] and tldIndices[] in one scan of the (input) file, but
    // buffer tldData[] so we don'te interleave them in the outFile.
    QByteArray outDataBufferBA;
    QBuffer outDataBuffer(&outDataBufferBA);
    outDataBuffer.open(QIODevice::WriteOnly);

    int lineCount = 0;
    while (!file.atEnd()) {
        file.readLine();
        lineCount++;
    }
    outFile.write("static const quint16 tldCount = ");
    outFile.write(QByteArray::number(lineCount));
    outFile.write(";\n");

    file.reset();
    QVector<QString> strings(lineCount);
    while (!file.atEnd()) {
        QString st = QString::fromUtf8(file.readLine()).trimmed();
        int num = qt_hash(st) % lineCount;
        QString &entry = strings[num];
        st = utf8encode(st.toUtf8());

        // For domain 1.com, we could get something like a.com\01.com, which
        // would be misinterpreted as octal 01, so we need to separate such
        // strings with quotes:
        if (!entry.isEmpty() && st.at(0).isDigit())
            entry.append(quadQuote);

        entry.append(st);
        entry.append("\\0");
    }
    outFile.write("static const quint32 tldIndices[] = {\n");
    outDataBuffer.write("\nstatic const char *tldData[] = {");

    int totalUtf8Size = 0;
    int chunkSize = 0; // strlen of the current chunk (sizeof is bigger by 1)
    QStringList chunks;
    for (int a = 0; a < lineCount; a++) {
        outFile.write(QByteArray::number(totalUtf8Size));
        outFile.write(",\n");
        const QString &entry = strings.at(a);
        if (!entry.isEmpty()) {
            const int zeroCount = entry.count(QLatin1String("\\0"));
            const int utf8CharsCount = entry.count(QLatin1String("\\x"));
            const int quoteCount = entry.count('"');
            const int stringUtf8Size = entry.count() - (zeroCount + quoteCount + utf8CharsCount * 3);
            chunkSize += stringUtf8Size;
            // MSVC 2015 chokes if sizeof(a single string) > 0xffff
            if (chunkSize >= 0xffff) {
                static int chunkCount = 0;
                qWarning() << "chunk" << ++chunkCount << "has length" << chunkSize - stringUtf8Size;
                outDataBuffer.write(",\n");
                chunks.append(QString::number(totalUtf8Size));
                chunkSize = 0;
            }
            totalUtf8Size += stringUtf8Size;

            outDataBuffer.write("\n\"");
            outDataBuffer.write(entry.toUtf8());
            outDataBuffer.write("\"");
        }
    }
    chunks.append(QString::number(totalUtf8Size));
    outFile.write(QByteArray::number(totalUtf8Size));
    outFile.write("\n};\n");

    outDataBuffer.write("\n};\n");
    outDataBuffer.close();
    outFile.write(outDataBufferBA);

    // write chunk information
    outFile.write("\nstatic const quint16 tldChunkCount = ");
    outFile.write(QByteArray::number(chunks.count()));
    outFile.write(";\nstatic const quint32 tldChunks[] = {");
    outFile.write(chunks.join(", ").toLatin1());
    outFile.write("};\n");
    outFile.close();
    printf("Data generated to %s - now revise qtbase/src/corelib/io/qurltlds_p.h to use this data.\n", argv[2]);
    return 0;
}
