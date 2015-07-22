/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef MAKEFILEDEPS_H
#define MAKEFILEDEPS_H

#include <proitems.h>

#include <qstringlist.h>
#include <qfileinfo.h>

QT_BEGIN_NAMESPACE

struct SourceFile;
struct SourceDependChildren;
class SourceFiles;

class QMakeLocalFileName {
    bool is_null;
    mutable QString real_name, local_name;
public:
    QMakeLocalFileName() : is_null(true) {}
    QMakeLocalFileName(const QString &);
    bool isNull() const { return is_null; }
    inline const QString &real() const { return real_name; }
    const QString &local() const;

    bool operator==(const QMakeLocalFileName &other) const {
        return (this->real_name == other.real_name);
    }
    bool operator!=(const QMakeLocalFileName &other) const {
        return !(*this == other);
    }
};

class QMakeSourceFileInfo
{
private:
    //quick project lookups
    SourceFiles *files, *includes;
    bool files_changed;
    QList<QMakeLocalFileName> depdirs;
    QStringList systemIncludes;

    //sleezy buffer code
    char *spare_buffer;
    int   spare_buffer_size;
    char *getBuffer(int s);

    //actual guts
    bool findMocs(SourceFile *);
    bool findDeps(SourceFile *);
    void dependTreeWalker(SourceFile *, SourceDependChildren *);

    //cache
    QString cachefile;

protected:
    virtual QMakeLocalFileName fixPathForFile(const QMakeLocalFileName &, bool forOpen=false);
    virtual QMakeLocalFileName findFileForDep(const QMakeLocalFileName &, const QMakeLocalFileName &);
    virtual QFileInfo findFileInfo(const QMakeLocalFileName &);

public:

    QMakeSourceFileInfo(const QString &cachefile="");
    virtual ~QMakeSourceFileInfo();

    QList<QMakeLocalFileName> dependencyPaths() const { return depdirs; }
    void setDependencyPaths(const QList<QMakeLocalFileName> &);

    enum DependencyMode { Recursive, NonRecursive };
    inline void setDependencyMode(DependencyMode mode) { dep_mode = mode; }
    inline DependencyMode dependencyMode() const { return dep_mode; }

    void setSystemIncludes(const ProStringList &list)
    { systemIncludes = list.toQStringList(); }

    enum SourceFileType { TYPE_UNKNOWN, TYPE_C, TYPE_UI, TYPE_QRC };
    enum SourceFileSeek { SEEK_DEPS=0x01, SEEK_MOCS=0x02 };
    void addSourceFiles(const ProStringList &, uchar seek, SourceFileType type=TYPE_C);
    void addSourceFile(const QString &, uchar seek, SourceFileType type=TYPE_C);
    bool containsSourceFile(const QString &, SourceFileType type=TYPE_C);
    bool isSystemInclude(const QString &);

    int included(const QString &file);
    QStringList dependencies(const QString &file);

    bool mocable(const QString &file);

    virtual QMap<QString, QStringList> getCacheVerification();
    virtual bool verifyCache(const QMap<QString, QStringList> &);
    void setCacheFile(const QString &cachefile); //auto caching
    void loadCache(const QString &cf);
    void saveCache(const QString &cf);

private:
    DependencyMode dep_mode;
};

QT_END_NAMESPACE

#endif // MAKEFILEDEPS_H
