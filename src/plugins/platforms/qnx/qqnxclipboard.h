/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QQNXCLIPBOARD_H
#define QQNXCLIPBOARD_H

#include <QtCore/qglobal.h>

#if !defined(QT_NO_CLIPBOARD)
#include <qpa/qplatformclipboard.h>

QT_BEGIN_NAMESPACE

class QQnxClipboard : public QPlatformClipboard
{
public:
    QQnxClipboard();
    ~QQnxClipboard();
    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;

private:
    class MimeData;
    MimeData *m_mimeData;
};

QT_END_NAMESPACE

#endif //QT_NO_CLIPBOARD
#endif //QQNXCLIPBOARD_H
