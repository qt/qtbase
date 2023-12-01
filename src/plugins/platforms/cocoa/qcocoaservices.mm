// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcocoaservices.h"

#include <AppKit/NSWorkspace.h>
#include <AppKit/NSColorSampler.h>
#include <Foundation/NSURL.h>

#include <QtCore/QUrl>
#include <QtGui/private/qcoregraphics_p.h>

QT_BEGIN_NAMESPACE

bool QCocoaServices::openUrl(const QUrl &url)
{
    return [[NSWorkspace sharedWorkspace] openURL:url.toNSURL()];
}

bool QCocoaServices::openDocument(const QUrl &url)
{
    return openUrl(url);
}

class QCocoaColorPicker : public QPlatformServiceColorPicker
{
public:
    QCocoaColorPicker() : m_colorSampler([NSColorSampler new]) {}
    ~QCocoaColorPicker() { [m_colorSampler release]; }

    void pickColor() override
    {
        [m_colorSampler showSamplerWithSelectionHandler:^(NSColor *selectedColor) {
            emit colorPicked(qt_mac_toQColor(selectedColor));
        }];
    }
private:
    NSColorSampler *m_colorSampler = nullptr;
};


QPlatformServiceColorPicker *QCocoaServices::colorPicker(QWindow *parent)
{
    Q_UNUSED(parent);
    return new QCocoaColorPicker;
}

bool QCocoaServices::hasCapability(Capability capability) const
{
    switch (capability) {
    case ColorPicking: return true;
    default: return false;
    }
}

QT_END_NAMESPACE
