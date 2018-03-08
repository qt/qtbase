/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtablegenerator.h"

#include <QtCore/QByteArray>
#include <QtCore/QTextCodec>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QString>
#include <QtCore/QSaveFile>
#include <QtCore/QStandardPaths>
#include <private/qcore_unix_p.h>

#include <algorithm>

#include <xkbcommon/xkbcommon.h>

#include <locale.h> // LC_CTYPE
#include <string.h> // strchr, strncmp, etc.
#include <strings.h> // strncasecmp
#include <clocale> // LC_CTYPE

static const quint32 SupportedCacheVersion = 1;

/*
    In short on how and why the "Compose" file is cached:

    The "Compose" file is large, for en_US it's likely located at:
    /usr/share/X11/locale/en_US.UTF-8/Compose
    and it has about 6000 string lines.
    Q(Gui)Applications parse this file each time they're created. On modern CPUs
    it incurs a 4-10 ms startup penalty of each Qt gui app, on older CPUs -
    tens of ms or more.
    Since the "Compose" file (almost) never changes using a pre-parsed
    cache file instead of the "Compose" file is a good idea to improve Qt5
    application startup time by about 5+ ms (or tens of ms on older CPUs).

    The cache file contains the contents of the QComposeCacheFileHeader struct at the
    beginning followed by the pre-parsed contents of the "Compose" file.

    struct QComposeCacheFileHeader stores
    (a) The cache version - in the unlikely event that some day one might need
    to break compatibility.
    (b) The (cache) file size.
    (c) The lastModified field tracks if anything changed since the last time
    the cache file was saved.
    If anything did change then we read the compose file and save (cache) it
    in binary/pre-parsed format, which should happen extremely rarely if at all.
*/

struct QComposeCacheFileHeader
{
    quint32 cacheVersion;
    // The compiler will add 4 padding bytes anyway.
    // Reserve them explicitly to possibly use in the future.
    quint32 reserved;
    quint64 fileSize;
    qint64 lastModified;
};

// localHostName() copied from qtbase/src/corelib/io/qlockfile_unix.cpp
static QByteArray localHostName()
{
    QByteArray hostName(512, Qt::Uninitialized);
    if (gethostname(hostName.data(), hostName.size()) == -1)
        return QByteArray();
    hostName.truncate(strlen(hostName.data()));
    return hostName;
}

/*
    Reads metadata about the Compose file. Later used to determine if the
    compose cache should be updated. The fileSize field will be zero on failure.
*/
static QComposeCacheFileHeader readFileMetadata(const QString &path)
{
    quint64 fileSize = 0;
    qint64 lastModified = 0;
    const QByteArray pathBytes = QFile::encodeName(path);
    QT_STATBUF st;
    if (QT_STAT(pathBytes.data(), &st) == 0) {
        lastModified = st.st_mtime;
        fileSize = st.st_size;
    }
    QComposeCacheFileHeader info = { 0, 0, fileSize, lastModified };
    return info;
}

static const QString getCacheFilePath()
{
    QFile machineIdFile("/var/lib/dbus/machine-id");
    QString machineId;
    if (machineIdFile.exists()) {
        if (machineIdFile.open(QIODevice::ReadOnly))
            machineId = QString::fromLatin1(machineIdFile.readAll().trimmed());
    }
    if (machineId.isEmpty())
        machineId = localHostName();
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        return dirPath + QLatin1String("/qt_compose_cache_big_endian_") + machineId;
    return dirPath + QLatin1String("/qt_compose_cache_little_endian_") + machineId;
}

// Returns empty vector on failure
static QVector<QComposeTableElement> loadCache(const QComposeCacheFileHeader &composeInfo)
{
    QVector<QComposeTableElement> vec;
    const QString cacheFilePath = getCacheFilePath();
    QFile inputFile(cacheFilePath);

    if (!inputFile.open(QIODevice::ReadOnly))
        return vec;
    QComposeCacheFileHeader cacheInfo;
    // use a "buffer" variable to make the line after this one more readable.
    char *buffer = reinterpret_cast<char*>(&cacheInfo);

    if (inputFile.read(buffer, sizeof cacheInfo) != sizeof cacheInfo)
        return vec;
    if (cacheInfo.fileSize == 0)
        return vec;
    // using "!=" just in case someone replaced with a backup that existed before
    if (cacheInfo.lastModified != composeInfo.lastModified)
        return vec;
    if (cacheInfo.cacheVersion != SupportedCacheVersion)
        return vec;
    const QByteArray pathBytes = QFile::encodeName(cacheFilePath);
    QT_STATBUF st;
    if (QT_STAT(pathBytes.data(), &st) != 0)
        return vec;
    const off_t fileSize = st.st_size;
    if (fileSize > 1024 * 1024 * 5) {
        // The cache file size is usually about 150KB, so if its size is over
        // say 5MB then somebody inflated the file, abort.
        return vec;
    }
    const off_t bufferSize = fileSize - (sizeof cacheInfo);
    const size_t elemSize = sizeof (struct QComposeTableElement);
    const int elemCount = bufferSize / elemSize;
    const QByteArray ba = inputFile.read(bufferSize);
    const char *data = ba.data();
    // Since we know the number of the (many) elements and their size in
    // advance calling vector.reserve(..) seems reasonable.
    vec.reserve(elemCount);

    for (int i = 0; i < elemCount; i++) {
        const QComposeTableElement *elem =
            reinterpret_cast<const QComposeTableElement*>(data + (i * elemSize));
        vec.push_back(*elem);
    }
    return vec;
}

// Returns true on success, false otherwise.
static bool saveCache(const QComposeCacheFileHeader &info, const QVector<QComposeTableElement> &vec)
{
    const QString filePath = getCacheFilePath();
#if QT_CONFIG(temporaryfile)
    QSaveFile outputFile(filePath);
#else
    QFile outputFile(filePath);
#endif
    if (!outputFile.open(QIODevice::WriteOnly))
        return false;
    const char *data = reinterpret_cast<const char*>(&info);

    if (outputFile.write(data, sizeof info) != sizeof info)
        return false;
    data = reinterpret_cast<const char*>(vec.constData());
    const qint64 size = vec.size() * (sizeof (struct QComposeTableElement));

    if (outputFile.write(data, size) != size)
        return false;
#if QT_CONFIG(temporaryfile)
    return outputFile.commit();
#else
    return true;
#endif
}

TableGenerator::TableGenerator() : m_state(NoErrors),
    m_systemComposeDir(QString())
{
    initPossibleLocations();
    QString composeFilePath = findComposeFile();
#ifdef DEBUG_GENERATOR
// don't use cache when in debug mode.
    if (!composeFilePath.isEmpty())
        qDebug() << "Using Compose file from: " << composeFilePath;
#else
    QComposeCacheFileHeader fileInfo = readFileMetadata(composeFilePath);
    if (fileInfo.fileSize != 0)
        m_composeTable = loadCache(fileInfo);
#endif
    if (m_composeTable.isEmpty() && cleanState()) {
        if (composeFilePath.isEmpty()) {
            m_state = MissingComposeFile;
        } else {
            QFile composeFile(composeFilePath);
            composeFile.open(QIODevice::ReadOnly);
            parseComposeFile(&composeFile);
            orderComposeTable();
            if (m_composeTable.isEmpty()) {
                m_state = EmptyTable;
#ifndef DEBUG_GENERATOR
// don't save cache when in debug mode
            } else {
                fileInfo.cacheVersion = SupportedCacheVersion;
                saveCache(fileInfo, m_composeTable);
#endif
            }
        }
    }
#ifdef DEBUG_GENERATOR
    printComposeTable();
#endif
}

void TableGenerator::initPossibleLocations()
{
    // Compose files come as a part of Xlib library. Xlib doesn't provide
    // a mechanism how to retrieve the location of these files reliably, since it was
    // never meant for external software to parse compose tables directly. Best we
    // can do is to hardcode search paths. To add an extra system path use
    // the QTCOMPOSE environment variable
    m_possibleLocations.reserve(7);
    if (qEnvironmentVariableIsSet("QTCOMPOSE"))
        m_possibleLocations.append(QString::fromLocal8Bit(qgetenv("QTCOMPOSE")));
    m_possibleLocations.append(QStringLiteral("/usr/share/X11/locale"));
    m_possibleLocations.append(QStringLiteral("/usr/local/share/X11/locale"));
    m_possibleLocations.append(QStringLiteral("/usr/lib/X11/locale"));
    m_possibleLocations.append(QStringLiteral("/usr/local/lib/X11/locale"));
    m_possibleLocations.append(QStringLiteral(X11_PREFIX "/share/X11/locale"));
    m_possibleLocations.append(QStringLiteral(X11_PREFIX "/lib/X11/locale"));
}

QString TableGenerator::findComposeFile()
{
    // check if XCOMPOSEFILE points to a Compose file
    if (qEnvironmentVariableIsSet("XCOMPOSEFILE")) {
        const QString path = QFile::decodeName(qgetenv("XCOMPOSEFILE"));
        if (QFile::exists(path))
            return path;
        else
            qWarning("$XCOMPOSEFILE doesn't point to an existing file");
    }

    // check if user’s home directory has a file named .XCompose
    if (cleanState()) {
        QString path = qgetenv("HOME") + QLatin1String("/.XCompose");
        if (QFile::exists(path))
            return path;
    }

    // check for the system provided compose files
    if (cleanState()) {
        QString table = composeTableForLocale();
        if (cleanState()) {
            if (table.isEmpty())
                // no table mappings for the system's locale in the compose.dir
                m_state = UnsupportedLocale;
            else {
                QString path = QDir(systemComposeDir()).filePath(table);
                if (QFile::exists(path))
                    return path;
            }
        }
    }
    return QString();
}

QString TableGenerator::composeTableForLocale()
{
    QByteArray loc = locale().toUpper().toUtf8();
    QString table = readLocaleMappings(loc);
    if (table.isEmpty())
        table = readLocaleMappings(readLocaleAliases(loc));
    return table;
}

bool TableGenerator::findSystemComposeDir()
{
    bool found = false;
    for (int i = 0; i < m_possibleLocations.size(); ++i) {
        QString path = m_possibleLocations.at(i);
        if (QFile::exists(path + QLatin1String("/compose.dir"))) {
            m_systemComposeDir = path;
            found = true;
            break;
        }
    }

    if (!found) {
        // should we ask to report this in the qt bug tracker?
        m_state = UnknownSystemComposeDir;
        qWarning("Qt Warning: Could not find a location of the system's Compose files. "
             "Consider setting the QTCOMPOSE environment variable.");
    }

    return found;
}

QString TableGenerator::systemComposeDir()
{
    if (m_systemComposeDir.isNull()
            && !findSystemComposeDir()) {
        return QLatin1String("$QTCOMPOSE");
    }

    return m_systemComposeDir;
}

QString TableGenerator::locale() const
{
    char *name = setlocale(LC_CTYPE, (char *)0);
    return QLatin1String(name);
}

QString TableGenerator::readLocaleMappings(const QByteArray &locale)
{
    QString file;
    if (locale.isEmpty())
        return file;

    QFile mappings(systemComposeDir() + QLatin1String("/compose.dir"));
    if (mappings.open(QIODevice::ReadOnly)) {
        const int localeNameLength = locale.size();
        const char * const localeData = locale.constData();

        char l[1024];
        // formating of compose.dir has some inconsistencies
        while (!mappings.atEnd()) {
            int read = mappings.readLine(l, sizeof(l));
            if (read <= 0)
                break;

            char *line = l;
            if (*line >= 'a' && *line <= 'z') {
                // file name
                while (*line && *line != ':' && *line != ' ' && *line != '\t')
                    ++line;
                if (!*line)
                    continue;
                const char * const composeFileNameEnd = line;
                *line = '\0';
                ++line;

                // locale name
                while (*line && (*line == ' ' || *line == '\t'))
                    ++line;
                const char * const lc = line;
                while (*line && *line != ' ' && *line != '\t' && *line != '\n')
                    ++line;
                *line = '\0';
                if (localeNameLength == (line - lc) && !strncasecmp(lc, localeData, line - lc)) {
                    file = QString::fromLocal8Bit(l, composeFileNameEnd - l);
                    break;
                }
            }
        }
        mappings.close();
    }
    return file;
}

QByteArray TableGenerator::readLocaleAliases(const QByteArray &locale)
{
    QFile aliases(systemComposeDir() + QLatin1String("/locale.alias"));
    QByteArray fullLocaleName;
    if (aliases.open(QIODevice::ReadOnly)) {
        while (!aliases.atEnd()) {
            char l[1024];
            int read = aliases.readLine(l, sizeof(l));
            char *line = l;
            if (read && ((*line >= 'a' && *line <= 'z') ||
                         (*line >= 'A' && *line <= 'Z'))) {
                const char *alias = line;
                while (*line && *line != ':' && *line != ' ' && *line != '\t')
                    ++line;
                if (!*line)
                    continue;
                *line = 0;
                if (locale.size() == (line - alias)
                        && !strncasecmp(alias, locale.constData(), line - alias)) {
                    // found a match for alias, read the real locale name
                    ++line;
                    while (*line && (*line == ' ' || *line == '\t'))
                        ++line;
                    const char *fullName = line;
                    while (*line && *line != ' ' && *line != '\t' && *line != '\n')
                        ++line;
                    *line = 0;
                    fullLocaleName = fullName;
#ifdef DEBUG_GENERATOR
                    qDebug() << "Alias for: " << alias << "is: " << fullLocaleName;
                    break;
#endif
                }
            }
        }
        aliases.close();
    }
    return fullLocaleName;
}

bool TableGenerator::processFile(const QString &composeFileName)
{
    QFile composeFile(composeFileName);
    if (composeFile.open(QIODevice::ReadOnly)) {
        parseComposeFile(&composeFile);
        return true;
    }
    qWarning() << QString(QLatin1String("Qt Warning: Compose file: \"%1\" can't be found"))
                  .arg(composeFile.fileName());
    return false;
}

TableGenerator::~TableGenerator()
{
}

QVector<QComposeTableElement> TableGenerator::composeTable() const
{
    return m_composeTable;
}

void TableGenerator::parseComposeFile(QFile *composeFile)
{
#ifdef DEBUG_GENERATOR
    qDebug() << "TableGenerator::parseComposeFile: " << composeFile->fileName();
#endif

    char line[1024];
    while (!composeFile->atEnd()) {
        composeFile->readLine(line, sizeof(line));
        if (*line == '<')
            parseKeySequence(line);
        else if (!strncmp(line, "include", 7))
            parseIncludeInstruction(QString::fromLocal8Bit(line));
    }

    composeFile->close();
}

void TableGenerator::parseIncludeInstruction(QString line)
{
    // Parse something that looks like:
    // include "/usr/share/X11/locale/en_US.UTF-8/Compose"
    QString quote = QStringLiteral("\"");
    line.remove(0, line.indexOf(quote) + 1);
    line.chop(line.length() - line.indexOf(quote));

    // expand substitutions if present
    line.replace(QLatin1String("%H"), QString(qgetenv("HOME")));
    line.replace(QLatin1String("%L"), systemComposeDir() +  QLatin1Char('/')  + composeTableForLocale());
    line.replace(QLatin1String("%S"), systemComposeDir());

    processFile(line);
}

ushort TableGenerator::keysymToUtf8(quint32 sym)
{
    QByteArray chars;
    int bytes;
    chars.resize(8);
    bytes = xkb_keysym_to_utf8(sym, chars.data(), chars.size());
    if (bytes == -1)
        qWarning("TableGenerator::keysymToUtf8 - buffer too small");

    chars.resize(bytes-1);

#ifdef DEBUG_GENERATOR
    QTextCodec *codec = QTextCodec::codecForLocale();
    qDebug() << QString("keysym - 0x%1 : utf8 - %2").arg(QString::number(sym, 16))
                                                    .arg(codec->toUnicode(chars));
#endif
    return QString::fromUtf8(chars).at(0).unicode();
}

static inline int fromBase8(const char *s, const char *end)
{
    int result = 0;
    while (*s && s != end) {
        if (*s < '0' || *s > '7')
            return 0;
        result *= 8;
        result += *s - '0';
        ++s;
    }
    return result;
}

static inline int fromBase16(const char *s, const char *end)
{
    int result = 0;
    while (*s && s != end) {
        result *= 16;
        if (*s >= '0' && *s <= '9')
            result += *s - '0';
        else if (*s >= 'a' && *s <= 'f')
            result += *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F')
            result += *s - 'A' + 10;
        else
            return 0;
        ++s;
    }
    return result;
}

void TableGenerator::parseKeySequence(char *line)
{
    // we are interested in the lines with the following format:
    // <Multi_key> <numbersign> <S> : "♬"   U266c # BEAMED SIXTEENTH NOTE
    char *keysEnd = strchr(line, ':');
    if (!keysEnd)
        return;

    QComposeTableElement elem;
    // find the composed value - strings may be direct text encoded in the locale
    // for which the compose file is to be used, or an escaped octal or hexadecimal
    // character code. Octal codes are specified as "\123" and hexadecimal codes as "\0x123a".
    char *composeValue = strchr(keysEnd, '"');
    if (!composeValue)
        return;
    ++composeValue;

    char *composeValueEnd = strchr(composeValue, '"');
    if (!composeValueEnd)
        return;

    // if composed value is a quotation mark adjust the end pointer
    if (composeValueEnd[1] == '"')
        ++composeValueEnd;

    if (*composeValue == '\\' && composeValue[1] >= '0' && composeValue[1] <= '9') {
        // handle octal and hex code values
        char detectBase = composeValue[2];
        if (detectBase == 'x') {
            // hexadecimal character code
            elem.value = keysymToUtf8(fromBase16(composeValue + 3, composeValueEnd));
        } else {
            // octal character code
            elem.value = keysymToUtf8(fromBase8(composeValue + 1, composeValueEnd));
        }
    } else {
        // handle direct text encoded in the locale
        if (*composeValue == '\\')
            ++composeValue;
        elem.value = QString::fromLocal8Bit(composeValue, composeValueEnd - composeValue).at(0).unicode();
        ++composeValue;
    }

#ifdef DEBUG_GENERATOR
    // find the comment
    elem.comment = QString::fromLocal8Bit(composeValueEnd + 1).trimmed();
#endif

    // find the key sequence and convert to X11 keysym
    char *k = line;
    const char *kend = keysEnd;

    for (int i = 0; i < QT_KEYSEQUENCE_MAX_LEN; i++) {
        // find the next pair of angle brackets and get the contents within
        while (k < kend && *k != '<')
            ++k;
        char *sym = ++k;
        while (k < kend && *k != '>')
            ++k;
        *k = '\0';
        if (k < kend) {
            elem.keys[i] = xkb_keysym_from_name(sym, (xkb_keysym_flags)0);
            if (elem.keys[i] == XKB_KEY_NoSymbol) {
                if (!strcmp(sym, "dead_inverted_breve"))
                    elem.keys[i] = XKB_KEY_dead_invertedbreve;
                else if (!strcmp(sym, "dead_double_grave"))
                    elem.keys[i] = XKB_KEY_dead_doublegrave;
#ifdef DEBUG_GENERATOR
                else
                    qWarning() << QString("Qt Warning - invalid keysym: %1").arg(sym);
#endif
            }
        } else {
            elem.keys[i] = 0;
        }
    }
    m_composeTable.append(elem);
}

void TableGenerator::printComposeTable() const
{
#ifdef DEBUG_GENERATOR
# ifndef QT_NO_DEBUG_STREAM
    if (m_composeTable.isEmpty())
        return;

    QDebug ds = qDebug() << "output:\n";
    ds.nospace();
    const int tableSize = m_composeTable.size();
    for (int i = 0; i < tableSize; ++i) {
        const QComposeTableElement &elem = m_composeTable.at(i);
        ds << "{ {";
        for (int j = 0; j < QT_KEYSEQUENCE_MAX_LEN; j++) {
            ds << hex << showbase << elem.keys[j] << ", ";
        }
        ds << "}, " << hex << showbase << elem.value << ", \"\" }, // " << elem.comment << " \n";
    }
# endif
#endif
}

void TableGenerator::orderComposeTable()
{
    // Stable-sorting to ensure that the item that appeared before the other in the
    // original container will still appear first after the sort. This property is
    // needed to handle the cases when user re-defines already defined key sequence
    std::stable_sort(m_composeTable.begin(), m_composeTable.end(), ByKeys());
}

