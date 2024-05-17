// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "quiwindow.h"

#include "qiostheme.h"

#include <QtCore/qscopedvaluerollback.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include <UIKit/UIKit.h>

@implementation QUIWindow

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame]))
        self->_sendingEvent = NO;

    return self;
}

- (instancetype)initWithWindowScene:(UIWindowScene *)windowScene
{
    if ((self = [super initWithWindowScene:windowScene]))
        self->_sendingEvent = NO;

    QIOSTheme::applyTheme(self);
    return self;
}

- (void)sendEvent:(UIEvent *)event
{
    QScopedValueRollback<BOOL> sendingEvent(self->_sendingEvent, YES);
    [super sendEvent:event];
}

#if !defined(Q_OS_VISIONOS)
- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
    [super traitCollectionDidChange:previousTraitCollection];

    if (!qGuiApp)
        return;

    Qt::ColorScheme colorScheme = self.traitCollection.userInterfaceStyle
                              == UIUserInterfaceStyleDark
                              ? Qt::ColorScheme::Dark
                              : Qt::ColorScheme::Light;

    if (self.screen == UIScreen.mainScreen) {
        // Check if the current userInterfaceStyle reports a different appearance than
        // the platformTheme's appearance. We might have set that one based on the UIScreen
        if (previousTraitCollection.userInterfaceStyle != self.traitCollection.userInterfaceStyle
            || QGuiApplicationPrivate::platformTheme()->colorScheme() != colorScheme) {
            QIOSTheme::initializeSystemPalette();
            QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
        }
    }
}
#endif

@end
