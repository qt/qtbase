// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*
 * This is a precompiled header file for use in Xcode / Mac GCC /
 * GCC >= 3.4 / VC to greatly speed the building of Qt. It may also be
 * of use to people developing their own project, but it is probably
 * better to define your own header.  Use of this header is currently
 * UNSUPPORTED.
 */

#include "../../gui/kernel/qt_gui_pch.h"

#if defined __cplusplus
#include <qtwidgetsexports.h>
#include <qtwidgetsglobal.h>
#include <qapplication.h>
#include <qabstractbutton.h>
#include <qabstractscrollarea.h>
#include <qabstractslider.h>
#if QT_CONFIG(action)
#  include <qaction.h>
#endif
#include <qcommonstyle.h>
#include <qlayout.h>
#include <qstyle.h>
#include <qwidget.h>
#endif
