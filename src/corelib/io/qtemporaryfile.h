/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QTEMPORARYFILE_H
#define QTEMPORARYFILE_H

#include <QtCore/qiodevice.h>
#include <QtCore/qfile.h>

#ifdef open
#error qtemporaryfile.h must be included before any header file that defines open
#endif

QT_BEGIN_NAMESPACE


#ifndef QT_NO_TEMPORARYFILE

class QTemporaryFilePrivate;
class QLockFilePrivate;

class Q_CORE_EXPORT QTemporaryFile : public QFile
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif
    Q_DECLARE_PRIVATE(QTemporaryFile)

public:
    QTemporaryFile();
    explicit QTemporaryFile(const QString &templateName);
#ifndef QT_NO_QOBJECT
    explicit QTemporaryFile(QObject *parent);
    QTemporaryFile(const QString &templateName, QObject *parent);
#endif
    ~QTemporaryFile();

    bool autoRemove() const;
    void setAutoRemove(bool b);

    // ### Hides open(flags)
    bool open() { return open(QIODevice::ReadWrite); }

    QString fileName() const override;
    QString fileTemplate() const;
    void setFileTemplate(const QString &name);

    // Hides QFile::rename
    bool rename(const QString &newName);

#if QT_DEPRECATED_SINCE(5,1)
    QT_DEPRECATED inline static QTemporaryFile *createLocalFile(const QString &fileName)
        { return createNativeFile(fileName); }
    QT_DEPRECATED inline static QTemporaryFile *createLocalFile(QFile &file)
        { return createNativeFile(file); }
#endif
    inline static QTemporaryFile *createNativeFile(const QString &fileName)
        { QFile file(fileName); return createNativeFile(file); }
    static QTemporaryFile *createNativeFile(QFile &file);

protected:
    bool open(OpenMode flags) override;

private:
    friend class QFile;
    friend class QLockFilePrivate;
    Q_DISABLE_COPY(QTemporaryFile)
};

#endif // QT_NO_TEMPORARYFILE

QT_END_NAMESPACE

#endif // QTEMPORARYFILE_H
