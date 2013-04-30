/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
  config.h
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <qmap.h>
#include <qset.h>
#include <qstringlist.h>
#include <qstack.h>
#include <qpair.h>
#include "location.h"

QT_BEGIN_NAMESPACE

/*
  This struct contains all the information for
  one config variable found in a qdocconf file.
 */
struct ConfigVar {
    bool plus_;
    QString name_;
    QStringList values_;
    QString currentPath_;
    Location location_;

  ConfigVar() : plus_(false) { }

  ConfigVar(const QString& name, const QStringList& values, const QString& dir)
    : plus_(true), name_(name), values_(values), currentPath_(dir) { }

  ConfigVar(const QString& name, const QStringList& values, const QString& dir, const Location& loc)
    : plus_(false), name_(name), values_(values), currentPath_(dir), location_(loc) { }
};

/*
  In this multimap, the key is a config variable name.
 */
typedef QMultiMap<QString, ConfigVar> ConfigVarMultimap;

class Config
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::Config)

public:
    Config(const QString& programName);
    ~Config();

    void load(const QString& fileName);
    void setStringList(const QString& var, const QStringList& values);

    const QString& programName() const { return prog; }
    const Location& location() const { return loc; }
    const Location& lastLocation() const { return lastLocation_; }
    bool getBool(const QString& var) const;
    int getInt(const QString& var) const;
    QString getOutputDir() const;
    QSet<QString> getOutputFormats() const;
    QString getString(const QString& var) const;
    QSet<QString> getStringSet(const QString& var) const;
    QStringList getStringList(const QString& var) const;
    QStringList getCanonicalPathList(const QString& var) const;
    QStringList getCleanPathList(const QString& var) const;
    QStringList getPathList(const QString& var) const;
    QRegExp getRegExp(const QString& var) const;
    QList<QRegExp> getRegExpList(const QString& var) const;
    QSet<QString> subVars(const QString& var) const;
    void subVarsAndValues(const QString& var, ConfigVarMultimap& t) const;
    QStringList getAllFiles(const QString& filesVar,
                            const QString& dirsVar,
                            const QSet<QString> &excludedDirs = QSet<QString>(),
                            const QSet<QString> &excludedFiles = QSet<QString>());
    QStringList getExampleQdocFiles(const QSet<QString> &excludedDirs, const QSet<QString> &excludedFiles);
    QStringList getExampleImageFiles(const QSet<QString> &excludedDirs, const QSet<QString> &excludedFiles);

    static QStringList getFilesHere(const QString& dir,
                                    const QString& nameFilter,
                                    const Location &location = Location(),
                                    const QSet<QString> &excludedDirs = QSet<QString>(),
                                    const QSet<QString> &excludedFiles = QSet<QString>());
    static QString findFile(const Location& location,
                            const QStringList &files,
                            const QStringList& dirs,
                            const QString& fileName,
                            QString& userFriendlyFilePath);
    static QString findFile(const Location &location,
                            const QStringList &files,
                            const QStringList &dirs,
                            const QString &fileBase,
                            const QStringList &fileExtensions,
                            QString &userFriendlyFilePath);
    static QString copyFile(const Location& location,
                            const QString& sourceFilePath,
                            const QString& userFriendlySourceFilePath,
                            const QString& targetDirPath);
    static int numParams(const QString& value);
    static bool removeDirContents(const QString& dir);
    static void pushWorkingDir(const QString& dir);
    static QString popWorkingDir();

    QT_STATIC_CONST QString dot;

    static bool generateExamples;
    static QString installDir;
    static QString overrideOutputDir;
    static QSet<QString> overrideOutputFormats;

private:
    static bool isMetaKeyChar(QChar ch);
    void load(Location location, const QString& fileName);

    QString prog;
    Location loc;
    Location lastLocation_;
    ConfigVarMultimap   configVars_;

    static QMap<QString, QString> uncompressedFiles;
    static QMap<QString, QString> extractedDirs;
    static int numInstances;
    static QStack<QString> workingDirs_;
};

#define CONFIG_ALIAS                    "alias"
#define CONFIG_BASE                     "base"
#define CONFIG_BASEDIR                  "basedir"
#define CONFIG_CODEINDENT               "codeindent"
#define CONFIG_DEFINES                  "defines"
#define CONFIG_DEPENDS                  "depends"
#define CONFIG_DESCRIPTION              "description"
#define CONFIG_EDITION                  "edition"
#define CONFIG_ENDHEADER                "endheader"
#define CONFIG_EXAMPLEDIRS              "exampledirs"
#define CONFIG_EXAMPLES                 "examples"
#define CONFIG_EXAMPLESINSTALLPATH      "examplesinstallpath"
#define CONFIG_EXCLUDEDIRS              "excludedirs"
#define CONFIG_EXCLUDEFILES             "excludefiles"
#define CONFIG_EXTRAIMAGES              "extraimages"
#define CONFIG_FALSEHOODS               "falsehoods"
#define CONFIG_FORMATTING               "formatting"
#define CONFIG_GENERATEINDEX            "generateindex"
#define CONFIG_HEADERDIRS               "headerdirs"
#define CONFIG_HEADERS                  "headers"
#define CONFIG_HEADERSCRIPTS            "headerscripts"
#define CONFIG_HEADERSTYLES             "headerstyles"
#define CONFIG_IGNOREDIRECTIVES         "ignoredirectives"
#define CONFIG_IGNORETOKENS             "ignoretokens"
#define CONFIG_IMAGEDIRS                "imagedirs"
#define CONFIG_IMAGES                   "images"
#define CONFIG_INDEXES                  "indexes"
#define CONFIG_LANGUAGE                 "language"
#define CONFIG_MACRO                    "macro"
#define CONFIG_MANIFESTMETA             "manifestmeta"
#define CONFIG_NATURALLANGUAGE          "naturallanguage"
#define CONFIG_NOLINKERRORS             "nolinkerrors"
#define CONFIG_OBSOLETELINKS            "obsoletelinks"
#define CONFIG_OUTPUTDIR                "outputdir"
#define CONFIG_OUTPUTENCODING           "outputencoding"
#define CONFIG_OUTPUTLANGUAGE           "outputlanguage"
#define CONFIG_OUTPUTFORMATS            "outputformats"
#define CONFIG_OUTPUTPREFIXES           "outputprefixes"
#define CONFIG_PROJECT                  "project"
#define CONFIG_QHP                      "qhp"
#define CONFIG_QUOTINGINFORMATION       "quotinginformation"
#define CONFIG_SCRIPTDIRS               "scriptdirs"
#define CONFIG_SCRIPTS                  "scripts"
#define CONFIG_SHOWINTERNAL             "showinternal"
#define CONFIG_SOURCEDIRS               "sourcedirs"
#define CONFIG_SOURCEENCODING           "sourceencoding"
#define CONFIG_SOURCES                  "sources"
#define CONFIG_SPURIOUS                 "spurious"
#define CONFIG_STYLEDIRS                "styledirs"
#define CONFIG_STYLE                    "style"
#define CONFIG_STYLES                   "styles"
#define CONFIG_STYLESHEETS              "stylesheets"
#define CONFIG_SYNTAXHIGHLIGHTING       "syntaxhighlighting"
#define CONFIG_TEMPLATEDIR              "templatedir"
#define CONFIG_TABSIZE                  "tabsize"
#define CONFIG_TAGFILE                  "tagfile"
#define CONFIG_TRANSLATORS              "translators"
#define CONFIG_URL                      "url"
#define CONFIG_VERSION                  "version"
#define CONFIG_VERSIONSYM               "versionsym"
#define CONFIG_FILEEXTENSIONS           "fileextensions"
#define CONFIG_IMAGEEXTENSIONS          "imageextensions"
#define CONFIG_QMLONLY                  "qmlonly"

QT_END_NAMESPACE

#endif
