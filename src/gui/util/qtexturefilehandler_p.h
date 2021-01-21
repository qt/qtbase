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

#ifndef QTEXTUREFILEHANDLER_P_H
#define QTEXTUREFILEHANDLER_P_H

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

#include "qtexturefiledata_p.h"

QT_BEGIN_NAMESPACE

class QTextureFileHandler
{
public:
    QTextureFileHandler(QIODevice *device, const QByteArray &logName = QByteArray())
        : m_device(device)
    {
        m_logName = !logName.isEmpty() ? logName : QByteArrayLiteral("(unknown)");
    }
    virtual ~QTextureFileHandler() {}

    virtual QTextureFileData read() = 0;
    QIODevice *device() const { return m_device; }
    QByteArray logName() const { return m_logName; }

private:
    QIODevice *m_device = nullptr;
    QByteArray m_logName;
};

QT_END_NAMESPACE

#endif // QTEXTUREFILEHANDLER_P_H
