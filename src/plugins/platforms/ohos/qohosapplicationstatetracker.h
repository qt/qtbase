// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAPPLICATIONSTATETRACKER_H
#define QOHOSAPPLICATIONSTATETRACKER_H

#include <QtCore/qglobal.h>
#include <QtCore/qpointer.h>
#include <QtGui/qwindow.h>
#include <qpa/qwindowsysteminterface_p.h>

QT_BEGIN_NAMESPACE

std::shared_ptr<QWindowSystemEventHandler> makeApplicationStateTracker();

QT_END_NAMESPACE

#endif
