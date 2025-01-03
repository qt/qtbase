// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//! [0]
    QQuickView *view = new QQuickView(parent);
    view->create();
    QGuiApplication::platformNativeInterface()->setWindowProperty(view->handle(), "qnxWindowGroup",
                                                                  group);
//! [0]

//! [1]
    QQuickView *view = new QQuickView(parent);
    view->create();
    QGuiApplication::platformNativeInterface()->setWindowProperty(view->handle(), "qnxWindowGroup",
                                                                  QVariant());
//! [1]
