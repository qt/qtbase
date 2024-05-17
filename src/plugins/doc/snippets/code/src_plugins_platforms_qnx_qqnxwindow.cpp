// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
