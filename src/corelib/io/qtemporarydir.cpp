/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtemporarydir.h"

#ifndef QT_NO_TEMPORARYFILE

#include "qdiriterator.h"
#include "qplatformdefs.h"
#include <QDebug>

#if defined(QT_BUILD_CORE_LIB)
#include "qcoreapplication.h"
#endif

#include <stdlib.h> // mkdtemp
#ifdef Q_OS_WIN
#include <windows.h>
#include <private/qfsfileengine_p.h>
#endif

QT_BEGIN_NAMESPACE

//************* QTemporaryDirPrivate
class QTemporaryDirPrivate
{
public:
    QTemporaryDirPrivate();
    ~QTemporaryDirPrivate();

    void create(const QString &templateName);

    QString path;
    bool autoRemove;
    bool success;
};

QTemporaryDirPrivate::QTemporaryDirPrivate()
    : autoRemove(true),
      success(false)
{
}

QTemporaryDirPrivate::~QTemporaryDirPrivate()
{
}

static QString defaultTemplateName()
{
    QString baseName;
#if defined(QT_BUILD_CORE_LIB)
    baseName = QCoreApplication::applicationName();
    if (baseName.isEmpty())
#endif
        baseName = QLatin1String("qt_temp");

    return QDir::tempPath() + QLatin1Char('/') + baseName + QLatin1String("-XXXXXX");
}

void QTemporaryDirPrivate::create(const QString &templateName)
{
#ifdef Q_OS_WIN
    QString buffer = templateName;
    // Windows' mktemp believes 26 temp files per process ought to be enough for everyone (!)
    // Let's add a few random chars then, before the XXXXXX template.
    for (int i = 0 ; i < 4 ; ++i)
        buffer += QChar((qrand() & 0xffff) % (26) + 'A');
    if (!buffer.endsWith(QLatin1String("XXXXXX")))
        buffer += QLatin1String("XXXXXX");
    QFileSystemEntry baseEntry(buffer);
    QFileSystemEntry::NativePath basePath = baseEntry.nativeFilePath();
    wchar_t* array = (wchar_t*)basePath.utf16();
    if (_wmktemp(array) && ::CreateDirectory(array, 0)) {
        success = true;
        QFileSystemEntry entry(QString::fromWCharArray(array), QFileSystemEntry::FromNativePath());
        path = entry.filePath();
    }
#else
    QByteArray buffer = QFile::encodeName(templateName);
    if (!buffer.endsWith("XXXXXX"))
        buffer += "XXXXXX";
    if (mkdtemp(buffer.data())) { // modifies buffer
        success = true;
        path = QFile::decodeName(buffer.constData());
    }
#endif
}

//************* QTemporaryDir

/*!
    \class QTemporaryDir
    \reentrant
    \brief The QTemporaryDir class creates a unique directory for temporary use.

    \ingroup io


    QTemporaryDir is used to create unique temporary dirs safely.
    The dir itself is created by the constructor. The name of the
    temporary directory is guaranteed to be unique (i.e., you are
    guaranteed to not overwrite an existing dir), and the directory will
    subsequently be removed upon destruction of the QTemporaryDir
    object. The directory name is either auto-generated, or created based
    on a template, which is passed to QTemporaryDir's constructor.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtemporarydir.cpp 0

    It is very important to test that the temporary directory could be
    created, using isValid(). Do not use exists(), since a default-constructed
    QDir represents the current directory, which exists.

    The path to the temporary dir can be found by calling path().

    A temporary directory will have some static part of the name and some
    part that is calculated to be unique. The default path will be
    determined from QCoreApplication::applicationName() (otherwise \c qt_temp) and will
    be placed into the temporary path as returned by QDir::tempPath().
    If you specify your own path, a relative path will not be placed in the
    temporary directory by default, but be relative to the current working directory.
    In all cases, a random string will be appended to the path in order to make it unique.

    \sa QDir::tempPath(), QDir, QTemporaryFile
*/

QTemporaryDir::QTemporaryDir()
    : d_ptr(new QTemporaryDirPrivate)
{
    d_ptr->create(defaultTemplateName());
}

QTemporaryDir::QTemporaryDir(const QString &templateName)
    : d_ptr(new QTemporaryDirPrivate)
{
    if (templateName.isEmpty())
        d_ptr->create(defaultTemplateName());
    else
        d_ptr->create(templateName);
}

/*!
    Destroys the temporary directory object.
    If auto remove mode was set, it will automatically delete the directory
    including all its contents.

    \sa autoRemove()
*/
QTemporaryDir::~QTemporaryDir()
{
    if (d_ptr->autoRemove)
        remove();
}

/*!
   Returns true if the QTemporaryDir was created successfully.
*/
bool QTemporaryDir::isValid() const
{
    return d_ptr->success;
}

/*!
   Returns the path to the temporary directory.
   Empty if the QTemporaryDir could not be created.
*/
QString QTemporaryDir::path() const
{
    return d_ptr->path;
}

/*!
   Returns true if the QTemporaryDir is in auto remove
   mode. Auto-remove mode will automatically delete the directory from
   disk upon destruction. This makes it very easy to create your
   QTemporaryDir object on the stack, fill it with files, do something with
   the files, and finally on function return it will automatically clean up
   after itself.

   Auto-remove is on by default.

   \sa setAutoRemove(), remove()
*/
bool QTemporaryDir::autoRemove() const
{
    return d_ptr->autoRemove;
}

/*!
    Sets the QTemporaryDir into auto-remove mode if \a b is true.

    Auto-remove is on by default.

    \sa autoRemove(), remove()
*/
void QTemporaryDir::setAutoRemove(bool b)
{
    d_ptr->autoRemove = b;
}

/*!
    Removes the temporary directory, including all its contents.
*/
bool QTemporaryDir::remove()
{
    if (!d_ptr->success)
        return false;
    Q_ASSERT(!path().isEmpty());
    Q_ASSERT(path() != QLatin1String("."));

    return QDir(path()).removeRecursively();
}

QT_END_NAMESPACE

#endif // QT_NO_TEMPORARYFILE
