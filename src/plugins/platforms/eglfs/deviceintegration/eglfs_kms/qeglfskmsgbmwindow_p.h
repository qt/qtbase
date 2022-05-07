/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
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
******************************************************************************/

#ifndef QEGLFSKMSGBMWINDOW_H
#define QEGLFSKMSGBMWINDOW_H

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

#include "private/qeglfswindow_p.h"

QT_BEGIN_NAMESPACE

class QEglFSKmsGbmIntegration;

class Q_EGLFS_EXPORT QEglFSKmsGbmWindow : public QEglFSWindow
{
public:
    QEglFSKmsGbmWindow(QWindow *w, const QEglFSKmsGbmIntegration *integration)
        : QEglFSWindow(w),
          m_integration(integration)
    { }

    ~QEglFSKmsGbmWindow() { destroy(); }

    void resetSurface() override;
    void invalidateSurface() override;

private:
    const QEglFSKmsGbmIntegration *m_integration;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMWINDOW_H
