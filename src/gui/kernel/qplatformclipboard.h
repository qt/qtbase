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

#ifndef QPLATFORMCLIPBOARD_H
#define QPLATFORMCLIPBOARD_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_CLIPBOARD

#include <QtGui/QClipboard>

QT_BEGIN_NAMESPACE


class Q_GUI_EXPORT QPlatformClipboard
{
public:
    Q_DISABLE_COPY_MOVE(QPlatformClipboard)

    QPlatformClipboard() = default;
    virtual ~QPlatformClipboard();

    virtual QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard);
    virtual void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard);
    virtual bool supportsMode(QClipboard::Mode mode) const;
    virtual bool ownsMode(QClipboard::Mode mode) const;
    void emitChanged(QClipboard::Mode mode);
};

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD

#endif //QPLATFORMCLIPBOARD_H
