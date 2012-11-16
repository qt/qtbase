/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  config.cpp
*/

#include <qdir.h>
#include <qvariant.h>
#include <qfile.h>
#include <qtemporaryfile.h>
#include <qtextstream.h>
#include <qdebug.h>
#include "config.h"
#include "generator.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*
  An entry on the MetaStack.
 */
class MetaStackEntry
{
public:
    void open();
    void close();

    QStringList accum;
    QStringList next;
};

/*
 */
void MetaStackEntry::open()
{
    next.append(QString());
}

/*
 */
void MetaStackEntry::close()
{
    accum += next;
    next.clear();
}

/*
  ###
*/
class MetaStack : private QStack<MetaStackEntry>
{
public:
    MetaStack();

    void process(QChar ch, const Location& location);
    QStringList getExpanded(const Location& location);
};

MetaStack::MetaStack()
{
    push(MetaStackEntry());
    top().open();
}

void MetaStack::process(QChar ch, const Location& location)
{
    if (ch == QLatin1Char('{')) {
        push(MetaStackEntry());
        top().open();
    }
    else if (ch == QLatin1Char('}')) {
        if (count() == 1)
            location.fatal(tr("Unexpected '}'"));

        top().close();
        QStringList suffixes = pop().accum;
        QStringList prefixes = top().next;

        top().next.clear();
        QStringList::ConstIterator pre = prefixes.constBegin();
        while (pre != prefixes.constEnd()) {
            QStringList::ConstIterator suf = suffixes.constBegin();
            while (suf != suffixes.constEnd()) {
                top().next << (*pre + *suf);
                ++suf;
            }
            ++pre;
        }
    }
    else if (ch == QLatin1Char(',') && count() > 1) {
        top().close();
        top().open();
    }
    else {
        QStringList::Iterator pre = top().next.begin();
        while (pre != top().next.end()) {
            *pre += ch;
            ++pre;
        }
    }
}

QStringList MetaStack::getExpanded(const Location& location)
{
    if (count() > 1)
        location.fatal(tr("Missing '}'"));

    top().close();
    return top().accum;
}

QT_STATIC_CONST_IMPL QString Config::dot = QLatin1String(".");
bool Config::generateExamples = true;
QString Config::overrideOutputDir;
QString Config::installDir;
QSet<QString> Config::overrideOutputFormats;
QMap<QString, QString> Config::extractedDirs;
int Config::numInstances;
QStack<QString> Config::workingDirs_;

/*!
  \class Config
  \brief The Config class contains the configuration variables
  for controlling how qdoc produces documentation.

  Its load() function, reads, parses, and processes a qdocconf file.
 */

/*!
  The constructor sets the \a programName and initializes all
  internal state variables to empty values.
 */
Config::Config(const QString& programName)
    : prog(programName)
{
    loc = Location::null;
    lastLocation_ = Location::null;
    locMap.clear();
    stringPairMap.clear();
    stringListPairMap.clear();
    numInstances++;
}

/*!
  The destructor has nothing special to do.
 */
Config::~Config()
{
}

/*!
  Loads and parses the qdoc configuration file \a fileName.
  This function calls the other load() function, which does
  the loading, parsing, and processing of the configuration
  file.

  Intializes the location variables returned by location()
  and lastLocation().
 */
void Config::load(const QString& fileName)
{
    load(Location::null, fileName);
    if (loc.isEmpty()) {
        loc = Location(fileName);
    }
    else {
        loc.setEtc(true);
    }
    lastLocation_ = Location::null;
}

/*!
  Writes the qdoc configuration data to the named file.
  The previous contents of the file are overwritten.
 */
void Config::unload(const QString& fileName)
{
    QStringPairMap::ConstIterator v = stringPairMap.constBegin();
    while (v != stringPairMap.constEnd()) {
        qDebug() << v.key() << " = " << v.value().second;
        ++v;
    }
    qDebug() << "fileName:" << fileName;
}
/*!
  Joins all the strings in \a values into a single string with the
  individual \a values separated by ' '. Then it inserts the result
  into the string list map with \a var as the key.

  It also inserts the \a values string list into a separate map,
  also with \a var as the key.
 */
void Config::setStringList(const QString& var, const QStringList& values)
{
    stringPairMap[var].first = QDir::currentPath();
    stringPairMap[var].second = values.join(QLatin1Char(' '));
    stringListPairMap[var].first = QDir::currentPath();
    stringListPairMap[var].second = values;
}

/*!
  Looks up the configuarion variable \a var in the string
  map and returns the boolean value.
 */
bool Config::getBool(const QString& var) const
{
    return QVariant(getString(var)).toBool();
}

/*!
  Looks up the configuration variable \a var in the string list
  map. Iterates through the string list found, interpreting each
  string in the list as an integer and adding it to a total sum.
  Returns the sum.
 */
int Config::getInt(const QString& var) const
{
    QStringList strs = getStringList(var);
    QStringList::ConstIterator s = strs.constBegin();
    int sum = 0;

    while (s != strs.constEnd()) {
        sum += (*s).toInt();
        ++s;
    }
    return sum;
}

/*!
  Function to return the correct outputdir.
  outputdir can be set using the qdocconf or the command-line
  variable -outputdir.
  */
QString Config::getOutputDir() const
{
    if (overrideOutputDir.isNull())
        return getString(QLatin1String(CONFIG_OUTPUTDIR));
    else
        return overrideOutputDir;
}

/*!
  Function to return the correct outputformats.
  outputformats can be set using the qdocconf or the command-line
  variable -outputformat.
  */
QSet<QString> Config::getOutputFormats() const
{
    if (overrideOutputFormats.isEmpty())
        return getStringSet(QLatin1String(CONFIG_OUTPUTFORMATS));
    else
        return overrideOutputFormats;
}

/*!
  First, this function looks up the configuration variable \a var
  in the location map and, if found, sets the internal variable
  \c{lastLocation_} to the Location that \a var maps to.

  Then it looks up the configuration variable \a var in the string
  map and returns the string that \a var maps to.
 */
QString Config::getString(const QString& var) const
{
    if (!locMap[var].isEmpty())
        (Location&) lastLocation_ = locMap[var];
    return stringPairMap[var].second;
}

/*!
  This function looks up the variable \a var in the location map
  and, if found, sets the internal variable \c{lastLocation_} to the
  location that \a var maps to.

  Then it looks up \a var in the configuration variable map and,
  if found, constructs a path from the pair value, which consists
  of the directory path of the configuration file where the value
  came from, and the value itself. The constructed path is returned.
 */
QString Config::getPath(const QString& var) const
{
    if (!locMap[var].isEmpty())
        (Location&) lastLocation_ = locMap[var];
    QString path;
    if (stringPairMap.contains(var)) {
        path = QDir(stringPairMap[var].first + "/" + stringPairMap[var].second).absolutePath();
    }
    return path;
}

/*!
  Looks up the configuration variable \a var in the string
  list map, converts the string list it maps to into a set
  of strings, and returns the set.
 */
QSet<QString> Config::getStringSet(const QString& var) const
{
    return QSet<QString>::fromList(getStringList(var));
}

/*!
  First, this function looks up the configuration variable \a var
  in the location map and, if found, sets the internal variable
  \c{lastLocation_} to the Location that \a var maps to.

  Then it looks up the configuration variable \a var in the string
  list map, and returns the string list that \a var maps to.
 */
QStringList Config::getStringList(const QString& var) const
{
    if (!locMap[var].isEmpty())
        (Location&) lastLocation_ = locMap[var];
    return stringListPairMap[var].second;
}


/*!
   \brief Returns the a path list where all paths are canonicalized, then
          made relative to the config file.
   \param var The variable containing the list of paths.
   \see   Location::canonicalRelativePath()
 */
QStringList Config::getCanonicalRelativePathList(const QString& var) const
{
    if (!locMap[var].isEmpty())
        (Location&) lastLocation_ = locMap[var];
    QStringList t;
    QStringListPairMap::const_iterator it = stringListPairMap.constFind(var);
    if (it != stringListPairMap.constEnd()) {
        const QStringList& sl = it.value().second;
        if (!sl.isEmpty()) {
            t.reserve(sl.size());
            for (int i=0; i<sl.size(); ++i) {
                const QString &canonicalized = location().canonicalRelativePath(sl[i]);
                t.append(canonicalized);
            }
        }
    }
    return t;
}

/*!
  This function should only be called when the configuration
  variable \a var maps to a string list that contains file paths.
  It cleans the paths with QDir::cleanPath() before returning
  them.

  First, this function looks up the configuration variable \a var
  in the location map and, if found, sets the internal variable
  \c{lastLocation_} the Location that \a var maps to.

  Then it looks up the configuration variable \a var in the string
  list map, which maps to a string list that contains file paths.
  These paths might not be clean, so QDir::cleanPath() is called
  for each one. The string list returned contains cleaned paths.
 */
QStringList Config::getCleanPathList(const QString& var) const
{
    if (!locMap[var].isEmpty())
        (Location&) lastLocation_ = locMap[var];
    QStringList t;
    QStringListPairMap::const_iterator it = stringListPairMap.constFind(var);
    if (it != stringListPairMap.constEnd()) {
        const QStringList& sl = it.value().second;
        if (!sl.isEmpty()) {
            t.reserve(sl.size());
            for (int i=0; i<sl.size(); ++i) {
                t.append(QDir::cleanPath(sl[i]));
            }
        }
    }
    return t;
}

/*!
  This function should only be called when the configuration
  variable \a var maps to a string list that contains file paths.
  It cleans the paths with QDir::cleanPath() before returning
  them.

  First, this function looks up the configuration variable \a var
  in the location map and, if found, sets the internal variable
  \c{lastLocation_} the Location that \a var maps to.

  Then it looks up the configuration variable \a var in the string
  list map, which maps to a string list that contains file paths.
  These paths might not be clean, so QDir::cleanPath() is called
  for each one. The string list returned contains cleaned paths.
 */
QStringList Config::getPathList(const QString& var) const
{
    if (!locMap[var].isEmpty())
        (Location&) lastLocation_ = locMap[var];
    QStringList t;
    QStringListPairMap::const_iterator it = stringListPairMap.constFind(var);
    if (it != stringListPairMap.constEnd()) {
        const QStringList& sl = it.value().second;
        const QString d = it.value().first;
        if (!sl.isEmpty()) {
            t.reserve(sl.size());
            for (int i=0; i<sl.size(); ++i) {
                QFileInfo fileInfo;
                QString path = d + "/" + QDir::cleanPath(sl[i]);
                fileInfo.setFile(path);
                if (!fileInfo.exists())
                    lastLocation_.warning(tr("File '%1' does not exist").arg(path));
                else
                    t.append(path);
            }
        }
    }
    return t;
}


/*!
  Calls getRegExpList() with the control variable \a var and
  iterates through the resulting list of regular expressions,
  concatening them with some extras characters to form a single
  QRegExp, which is returned/

  \sa getRegExpList()
 */
QRegExp Config::getRegExp(const QString& var) const
{
    QString pattern;
    QList<QRegExp> subRegExps = getRegExpList(var);
    QList<QRegExp>::ConstIterator s = subRegExps.constBegin();

    while (s != subRegExps.constEnd()) {
        if (!(*s).isValid())
            return *s;
        if (!pattern.isEmpty())
            pattern += QLatin1Char('|');
        pattern += QLatin1String("(?:") + (*s).pattern() + QLatin1Char(')');
        ++s;
    }
    if (pattern.isEmpty())
        pattern = QLatin1String("$x"); // cannot match
    return QRegExp(pattern);
}

/*!
  Looks up the configuration variable \a var in the string list
  map, converts the string list to a list of regular expressions,
  and returns it.
 */
QList<QRegExp> Config::getRegExpList(const QString& var) const
{
    QStringList strs = getStringList(var);
    QStringList::ConstIterator s = strs.constBegin();
    QList<QRegExp> regExps;

    while (s != strs.constEnd()) {
        regExps += QRegExp(*s);
        ++s;
    }
    return regExps;
}

/*!
  This function is slower than it could be. What it does is
  find all the keys that begin with \a var + dot and return
  the matching keys in a set, stripped of the matching prefix
  and dot.
 */
QSet<QString> Config::subVars(const QString& var) const
{
    QSet<QString> result;
    QString varDot = var + QLatin1Char('.');
    QStringPairMap::ConstIterator v = stringPairMap.constBegin();
    while (v != stringPairMap.constEnd()) {
        if (v.key().startsWith(varDot)) {
            QString subVar = v.key().mid(varDot.length());
            int dot = subVar.indexOf(QLatin1Char('.'));
            if (dot != -1)
                subVar.truncate(dot);
            result.insert(subVar);
        }
        ++v;
    }
    return result;
}

/*!
  Same as subVars(), but in this case we return a string map
  with the matching keys (stripped of the prefix \a var and
  mapped to their values. The pairs are inserted into \a t
 */
void Config::subVarsAndValues(const QString& var, QStringPairMap& t) const
{
    QString varDot = var + QLatin1Char('.');
    QStringPairMap::ConstIterator v = stringPairMap.constBegin();
    while (v != stringPairMap.constEnd()) {
        if (v.key().startsWith(varDot)) {
            QString subVar = v.key().mid(varDot.length());
            int dot = subVar.indexOf(QLatin1Char('.'));
            if (dot != -1)
                subVar.truncate(dot);
            t.insert(subVar,v.value());
        }
        ++v;
    }
}

/*!
  Builds and returns a list of file pathnames for the file
  type specified by \a filesVar (e.g. "headers" or "sources").
  The files are found in the directories specified by
  \a dirsVar, and they are filtered by \a defaultNameFilter
  if a better filter can't be constructed from \a filesVar.
  The directories in \a excludedDirs are avoided. The files
  in \a excludedFiles are not included in the return list.
 */
QStringList Config::getAllFiles(const QString &filesVar,
                                const QString &dirsVar,
                                const QSet<QString> &excludedDirs,
                                const QSet<QString> &excludedFiles)
{
    QStringList result = getStringList(filesVar);
    QStringList dirs = getCanonicalRelativePathList(dirsVar);

    QString nameFilter = getString(filesVar + dot + QLatin1String(CONFIG_FILEEXTENSIONS));

    QStringList::ConstIterator d = dirs.constBegin();
    while (d != dirs.constEnd()) {
        result += getFilesHere(*d, nameFilter, location(), excludedDirs, excludedFiles);
        ++d;
    }
    return result;
}

QStringList Config::getExampleQdocFiles(const QSet<QString> &excludedDirs,
                                        const QSet<QString> &excludedFiles)
{
    QStringList result;
    QStringList dirs = getCanonicalRelativePathList("exampledirs");
    QString nameFilter = " *.qdoc";

    QStringList::ConstIterator d = dirs.constBegin();
    while (d != dirs.constEnd()) {
        result += getFilesHere(*d, nameFilter, location(), excludedDirs, excludedFiles);
        ++d;
    }
    return result;
}

QStringList Config::getExampleImageFiles(const QSet<QString> &excludedDirs,
                                         const QSet<QString> &excludedFiles)
{
    QStringList result;
    QStringList dirs = getCanonicalRelativePathList("exampledirs");
    QString nameFilter = getString(CONFIG_EXAMPLES + dot + QLatin1String(CONFIG_IMAGEEXTENSIONS));

    QStringList::ConstIterator d = dirs.constBegin();
    while (d != dirs.constEnd()) {
        result += getFilesHere(*d, nameFilter, location(), excludedDirs, excludedFiles);
        ++d;
    }
    return result;
}

/*!
  \a fileName is the path of the file to find.

  \a files and \a dirs are the lists where we must find the
  components of \a fileName.

  \a location is used for obtaining the file and line numbers
  for report qdoc errors.
 */
QString Config::findFile(const Location& location,
                         const QStringList& files,
                         const QStringList& dirs,
                         const QString& fileName,
                         QString& userFriendlyFilePath)
{
    if (fileName.isEmpty() || fileName.startsWith(QLatin1Char('/'))) {
        userFriendlyFilePath = fileName;
        return fileName;
    }

    QFileInfo fileInfo;
    QStringList components = fileName.split(QLatin1Char('?'));
    QString firstComponent = components.first();

    QStringList::ConstIterator f = files.constBegin();
    while (f != files.constEnd()) {
        if (*f == firstComponent ||
                (*f).endsWith(QLatin1Char('/') + firstComponent)) {
            fileInfo.setFile(*f);
            if (!fileInfo.exists())
                location.fatal(tr("File '%1' does not exist").arg(*f));
            break;
        }
        ++f;
    }

    if (fileInfo.fileName().isEmpty()) {
        QStringList::ConstIterator d = dirs.constBegin();
        while (d != dirs.constEnd()) {
            fileInfo.setFile(QDir(*d), firstComponent);
            if (fileInfo.exists()) {
                break;
            }
            ++d;
        }
    }

    userFriendlyFilePath = QString();
    if (!fileInfo.exists())
        return QString();

    QStringList::ConstIterator c = components.constBegin();
    for (;;) {
        bool isArchive = (c != components.constEnd() - 1);
        QString userFriendly = *c;

        userFriendlyFilePath += userFriendly;

        if (isArchive) {
            QString extracted = extractedDirs[fileInfo.filePath()];
            ++c;
            fileInfo.setFile(QDir(extracted), *c);
        }
        else
            break;

        userFriendlyFilePath += QLatin1Char('?');
    }
    return fileInfo.filePath();
}

/*!
 */
QString Config::findFile(const Location& location,
                         const QStringList& files,
                         const QStringList& dirs,
                         const QString& fileBase,
                         const QStringList& fileExtensions,
                         QString& userFriendlyFilePath)
{
    QStringList::ConstIterator e = fileExtensions.constBegin();
    while (e != fileExtensions.constEnd()) {
        QString filePath = findFile(location,
                                    files,
                                    dirs,
                                    fileBase + QLatin1Char('.') + *e,
                                    userFriendlyFilePath);
        if (!filePath.isEmpty())
            return filePath;
        ++e;
    }
    return findFile(location, files, dirs, fileBase, userFriendlyFilePath);
}

/*!
  Copies the \a sourceFilePath to the file name constructed by
  concatenating \a targetDirPath and the file name from the
  \a userFriendlySourceFilePath. \a location is for identifying
  the file and line number where a qdoc error occurred. The
  constructed output file name is returned.
 */
QString Config::copyFile(const Location& location,
                         const QString& sourceFilePath,
                         const QString& userFriendlySourceFilePath,
                         const QString& targetDirPath)
{
    QFile inFile(sourceFilePath);
    if (!inFile.open(QFile::ReadOnly)) {
        location.warning(tr("Cannot open input file for copy: '%1': %2")
                         .arg(sourceFilePath).arg(inFile.errorString()));
        return QString();
    }

    QString outFileName = userFriendlySourceFilePath;
    int slash = outFileName.lastIndexOf(QLatin1Char('/'));
    if (slash != -1)
        outFileName = outFileName.mid(slash);
    if ((outFileName.size()) > 0 && (outFileName[0] != '/'))
        outFileName = targetDirPath + QLatin1Char('/') + outFileName;
    else
        outFileName = targetDirPath + outFileName;
    QFile outFile(outFileName);
    if (!outFile.open(QFile::WriteOnly)) {
        location.warning(tr("Cannot open output file for copy: '%1': %2")
                         .arg(outFileName).arg(outFile.errorString()));
        return QString();
    }

    char buffer[1024];
    int len;
    while ((len = inFile.read(buffer, sizeof(buffer))) > 0) {
        outFile.write(buffer, len);
    }
    return outFileName;
}

/*!
  Finds the largest unicode digit in \a value in the range
  1..7 and returns it.
 */
int Config::numParams(const QString& value)
{
    int max = 0;
    for (int i = 0; i != value.length(); i++) {
        uint c = value[i].unicode();
        if (c > 0 && c < 8)
            max = qMax(max, (int)c);
    }
    return max;
}

/*!
  Removes everything from \a dir. This function is recursive.
  It doesn't remove \a dir itself, but if it was called
  recursively, then the caller will remove \a dir.
 */
bool Config::removeDirContents(const QString& dir)
{
    QDir dirInfo(dir);
    QFileInfoList entries = dirInfo.entryInfoList();

    bool ok = true;

    QFileInfoList::Iterator it = entries.begin();
    while (it != entries.end()) {
        if ((*it).isFile()) {
            if (!dirInfo.remove((*it).fileName()))
                ok = false;
        }
        else if ((*it).isDir()) {
            if ((*it).fileName() != QLatin1String(".") && (*it).fileName() != QLatin1String("..")) {
                if (removeDirContents((*it).absoluteFilePath())) {
                    if (!dirInfo.rmdir((*it).fileName()))
                        ok = false;
                }
                else {
                    ok = false;
                }
            }
        }
        ++it;
    }
    return ok;
}

/*!
  Returns true if \a ch is a letter, number, '_', '.',
  '{', '}', or ','.
 */
bool Config::isMetaKeyChar(QChar ch)
{
    return ch.isLetterOrNumber()
            || ch == QLatin1Char('_')
            || ch == QLatin1Char('.')
            || ch == QLatin1Char('{')
            || ch == QLatin1Char('}')
            || ch == QLatin1Char(',');
}

/*!
  Load, parse, and process a qdoc configuration file. This
  function is only called by the other load() function, but
  this one is recursive, i.e., it calls itself when it sees
  an \c{include} statement in the qdoc configuration file.
 */
void Config::load(Location location, const QString& fileName)
{
    pushWorkingDir(QFileInfo(fileName).path());
    QDir::setCurrent(QFileInfo(fileName).path());
    QRegExp keySyntax(QLatin1String("\\w+(?:\\.\\w+)*"));

#define SKIP_CHAR() \
    do { \
    location.advance(c); \
    ++i; \
    c = text.at(i); \
    cc = c.unicode(); \
} while (0)

#define SKIP_SPACES() \
    while (c.isSpace() && cc != '\n') \
    SKIP_CHAR()

#define PUT_CHAR() \
    word += c; \
    SKIP_CHAR();

    if (location.depth() > 16)
        location.fatal(tr("Too many nested includes"));

    QFile fin(fileName);
    if (!fin.open(QFile::ReadOnly | QFile::Text)) {
        if (!Config::installDir.isEmpty()) {
            int prefix = location.filePath().length() - location.fileName().length();
            fin.setFileName(Config::installDir + "/" + fileName.right(fileName.length() - prefix));
        }
        if (!fin.open(QFile::ReadOnly | QFile::Text))
            location.fatal(tr("Cannot open file '%1': %2").arg(fileName).arg(fin.errorString()));
    }

    QTextStream stream(&fin);
    stream.setCodec("UTF-8");
    QString text = stream.readAll();
    text += QLatin1String("\n\n");
    text += QLatin1Char('\0');
    fin.close();

    location.push(fileName);
    location.start();

    int i = 0;
    QChar c = text.at(0);
    uint cc = c.unicode();
    while (i < (int) text.length()) {
        if (cc == 0)
            ++i;
        else if (c.isSpace()) {
            SKIP_CHAR();
        }
        else if (cc == '#') {
            do {
                SKIP_CHAR();
            } while (cc != '\n');
        }
        else if (isMetaKeyChar(c)) {
            Location keyLoc = location;
            bool plus = false;
            QString stringValue;
            QStringList stringListValue;
            QString word;
            bool inQuote = false;
            bool prevWordQuoted = true;
            bool metWord = false;

            MetaStack stack;
            do {
                stack.process(c, location);
                SKIP_CHAR();
            } while (isMetaKeyChar(c));

            QStringList keys = stack.getExpanded(location);
            SKIP_SPACES();

            if (keys.count() == 1 && keys.first() == QLatin1String("include")) {
                QString includeFile;

                if (cc != '(')
                    location.fatal(tr("Bad include syntax"));
                SKIP_CHAR();
                SKIP_SPACES();

                while (!c.isSpace() && cc != '#' && cc != ')') {

                    if (cc == '$') {
                        QString var;
                        SKIP_CHAR();
                        while (c.isLetterOrNumber() || cc == '_') {
                            var += c;
                            SKIP_CHAR();
                        }
                        if (!var.isEmpty()) {
                            char *val = getenv(var.toLatin1().data());
                            if (val == 0) {
                                location.fatal(tr("Environment variable '%1' undefined").arg(var));
                            }
                            else {
                                includeFile += QString::fromLatin1(val);
                            }
                        }
                    } else {
                        includeFile += c;
                        SKIP_CHAR();
                    }
                }
                SKIP_SPACES();
                if (cc != ')')
                    location.fatal(tr("Bad include syntax"));
                SKIP_CHAR();
                SKIP_SPACES();
                if (cc != '#' && cc != '\n')
                    location.fatal(tr("Trailing garbage"));

                /*
                  Here is the recursive call.
                 */
                load(location, QFileInfo(QFileInfo(fileName).dir(), includeFile).filePath());
            }
            else {
                /*
                  It wasn't an include statement, so it's something else.
                 */
                if (cc == '+') {
                    plus = true;
                    SKIP_CHAR();
                }
                if (cc != '=')
                    location.fatal(tr("Expected '=' or '+=' after key"));
                SKIP_CHAR();
                SKIP_SPACES();

                for (;;) {
                    if (cc == '\\') {
                        int metaCharPos;

                        SKIP_CHAR();
                        if (cc == '\n') {
                            SKIP_CHAR();
                        }
                        else if (cc > '0' && cc < '8') {
                            word += QChar(c.digitValue());
                            SKIP_CHAR();
                        }
                        else if ((metaCharPos = QString::fromLatin1("abfnrtv").indexOf(c)) != -1) {
                            word += QLatin1Char("\a\b\f\n\r\t\v"[metaCharPos]);
                            SKIP_CHAR();
                        }
                        else {
                            PUT_CHAR();
                        }
                    }
                    else if (c.isSpace() || cc == '#') {
                        if (inQuote) {
                            if (cc == '\n')
                                location.fatal(tr("Unterminated string"));
                            PUT_CHAR();
                        }
                        else {
                            if (!word.isEmpty()) {
                                if (metWord)
                                    stringValue += QLatin1Char(' ');
                                stringValue += word;
                                stringListValue << word;
                                metWord = true;
                                word.clear();
                                prevWordQuoted = false;
                            }
                            if (cc == '\n' || cc == '#')
                                break;
                            SKIP_SPACES();
                        }
                    }
                    else if (cc == '"') {
                        if (inQuote) {
                            if (!prevWordQuoted)
                                stringValue += QLatin1Char(' ');
                            stringValue += word;
                            if (!word.isEmpty())
                                stringListValue << word;
                            metWord = true;
                            word.clear();
                            prevWordQuoted = true;
                        }
                        inQuote = !inQuote;
                        SKIP_CHAR();
                    }
                    else if (cc == '$') {
                        QString var;
                        SKIP_CHAR();
                        while (c.isLetterOrNumber() || cc == '_') {
                            var += c;
                            SKIP_CHAR();
                        }
                        if (!var.isEmpty()) {
                            char *val = getenv(var.toLatin1().data());
                            if (val == 0) {
                                location.fatal(tr("Environment variable '%1' undefined").arg(var));
                            }
                            else {
                                word += QString::fromLatin1(val);
                            }
                        }
                    }
                    else {
                        if (!inQuote && cc == '=')
                            location.fatal(tr("Unexpected '='"));
                        PUT_CHAR();
                    }
                }

                QStringList::ConstIterator key = keys.constBegin();
                while (key != keys.constEnd()) {
                    if (!keySyntax.exactMatch(*key))
                        keyLoc.fatal(tr("Invalid key '%1'").arg(*key));

                    if (plus) {
                        if (locMap[*key].isEmpty()) {
                            locMap[*key] = keyLoc;
                        }
                        else {
                            locMap[*key].setEtc(true);
                        }
                        if (stringPairMap[*key].second.isEmpty()) {
                            stringPairMap[*key].first = QDir::currentPath();
                            stringPairMap[*key].second = stringValue;
                        }
                        else {
                            stringPairMap[*key].second += QLatin1Char(' ') + stringValue;
                        }
                        stringListPairMap[*key].first = QDir::currentPath();
                        stringListPairMap[*key].second += stringListValue;
                    }
                    else {
                        locMap[*key] = keyLoc;
                        stringPairMap[*key].first = QDir::currentPath();
                        stringPairMap[*key].second = stringValue;
                        stringListPairMap[*key].first = QDir::currentPath();
                        stringListPairMap[*key].second = stringListValue;
                    }
                    ++key;
                }
            }
        }
        else {
            location.fatal(tr("Unexpected character '%1' at beginning of line").arg(c));
        }
    }
    popWorkingDir();
    if (!workingDirs_.isEmpty())
        QDir::setCurrent(QFileInfo(workingDirs_.top()).path());
}

QStringList Config::getFilesHere(const QString& uncleanDir,
                                 const QString& nameFilter,
                                 const Location &location,
                                 const QSet<QString> &excludedDirs,
                                 const QSet<QString> &excludedFiles)
{
    //
    QString dir = location.isEmpty() ? QDir::cleanPath(uncleanDir) : location.canonicalRelativePath(uncleanDir);
    QStringList result;
    if (excludedDirs.contains(dir))
        return result;

    QDir dirInfo(dir);
    QStringList fileNames;
    QStringList::const_iterator fn;

    dirInfo.setNameFilters(nameFilter.split(QLatin1Char(' ')));
    dirInfo.setSorting(QDir::Name);
    dirInfo.setFilter(QDir::Files);
    fileNames = dirInfo.entryList();
    fn = fileNames.constBegin();
    while (fn != fileNames.constEnd()) {
        if (!fn->startsWith(QLatin1Char('~'))) {
            QString s = dirInfo.filePath(*fn);
            QString c = QDir::cleanPath(s);
            if (!excludedFiles.contains(c)) {
                result.append(c);
            }
        }
        ++fn;
    }

    dirInfo.setNameFilters(QStringList(QLatin1String("*")));
    dirInfo.setFilter(QDir::Dirs|QDir::NoDotAndDotDot);
    fileNames = dirInfo.entryList();
    fn = fileNames.constBegin();
    while (fn != fileNames.constEnd()) {
        result += getFilesHere(dirInfo.filePath(*fn), nameFilter, location, excludedDirs, excludedFiles);
        ++fn;
    }
    return result;
}

/*!
  Push \a dir onto the stack of working directories.
 */
void Config::pushWorkingDir(const QString& dir)
{
    workingDirs_.push(dir);
}

/*!
  If the stack of working directories is not empty, pop the
  top entry and return it. Otherwise return an empty string.
 */
QString Config::popWorkingDir()
{
    if (!workingDirs_.isEmpty()) {
        return workingDirs_.pop();
    }
    qDebug() << "RETURNED EMPTY WORKING DIR";
    return QString();
}

QT_END_NAMESPACE
