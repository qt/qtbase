/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QWINRTSERVICES_H
#define QWINRTSERVICES_H

#include <qpa/qplatformservices.h>
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE

class QWinRTServicesPrivate;
class QWinRTServices : public QPlatformServices
{
public:
    explicit QWinRTServices();
    ~QWinRTServices() override = default;

    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;

private:
    QScopedPointer<QWinRTServicesPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTServices)
};

QT_END_NAMESPACE

#endif // QWINRTSERVICES_H
