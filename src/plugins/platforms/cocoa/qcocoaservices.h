// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QCOCOADESKTOPSERVICES_H
#define QCOCOADESKTOPSERVICES_H

#include <QtCore/qurl.h>

#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

class QCocoaServices : public QPlatformServices
{
public:
    bool hasCapability(Capability capability) const override;

    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;
    bool handleUrl(const QUrl &url);

    QPlatformServiceColorPicker *colorPicker(QWindow *parent) override;

private:
    QUrl m_handlingUrl;
};

QT_END_NAMESPACE

#endif // QCOCOADESKTOPSERVICES_H
