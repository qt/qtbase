/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QTEXTUREFILEDATA_P_H
#define QTEXTUREFILEDATA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qtguiglobal.h>
#include <QSharedDataPointer>
#include <QLoggingCategory>
#include <QDebug>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQtGuiTextureIO)

class QTextureFileDataPrivate;

class Q_GUI_EXPORT QTextureFileData
{
public:
    QTextureFileData();
    QTextureFileData(const QTextureFileData &other);
    QTextureFileData &operator=(const QTextureFileData &other);
    ~QTextureFileData();

    bool isNull() const;
    bool isValid() const;

    void clear();

    QByteArray data() const;
    void setData(const QByteArray &data);

    int dataOffset(int level = 0) const;
    void setDataOffset(int offset, int level = 0);

    int dataLength(int level = 0) const;
    void setDataLength(int length, int level = 0);

    int numLevels() const;
    void setNumLevels(int num);

    QSize size() const;
    void setSize(const QSize &size);

    quint32 glFormat() const;
    void setGLFormat(quint32 format);

    quint32 glInternalFormat() const;
    void setGLInternalFormat(quint32 format);

    quint32 glBaseInternalFormat() const;
    void setGLBaseInternalFormat(quint32 format);

    QByteArray logName() const;
    void setLogName(const QByteArray &name);

private:
    QSharedDataPointer<QTextureFileDataPrivate> d;
};

Q_DECLARE_TYPEINFO(QTextureFileData, Q_MOVABLE_TYPE);

Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QTextureFileData &d);

QT_END_NAMESPACE

#endif // QABSTRACTLAYOUTSTYLEINFO_P_H
