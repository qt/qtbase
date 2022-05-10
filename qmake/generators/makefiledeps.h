// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MAKEFILEDEPS_H
#define MAKEFILEDEPS_H

#include <proitems.h>

#include <qfileinfo.h>
#include <qlist.h>
#include <qstringlist.h>

QT_BEGIN_NAMESPACE

struct SourceFile;
struct SourceDependChildren;
class SourceFiles;

class QMakeLocalFileName
{
    QString real_name;
    mutable QString local_name;
public:
    QMakeLocalFileName() = default;
    QMakeLocalFileName(const QString &);
    bool isNull() const { return real_name.isNull(); }
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

protected:
    virtual QMakeLocalFileName fixPathForFile(const QMakeLocalFileName &, bool forOpen=false);
    virtual QMakeLocalFileName findFileForDep(const QMakeLocalFileName &, const QMakeLocalFileName &);
    virtual QFileInfo findFileInfo(const QMakeLocalFileName &);

public:

    QMakeSourceFileInfo();
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

private:
    DependencyMode dep_mode;
};

QT_END_NAMESPACE

#endif // MAKEFILEDEPS_H
