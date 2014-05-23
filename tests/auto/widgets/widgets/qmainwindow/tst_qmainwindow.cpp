/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>


#include <qdockwidget.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qmenubar.h>
#include <qstatusbar.h>
#include <qstyle.h>
#include <qtoolbar.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qtextedit.h>
#include <qstylehints.h>
#include <private/qmainwindowlayout_p.h>
#include <private/qdockarealayout_p.h>

#include "../../../qtest-config.h"

static uchar restoreData41[] = { 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0, 0x0, 0xfc, 0x0, 0x0, 0x0, 0x0, 0xfd, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x64, 0x0, 0x0, 0x1, 0x19, 0xfc, 0x2, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x30, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x45, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x34, 0x1, 0x0, 0x0, 0x0, 0x49, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x38, 0x1, 0x0, 0x0, 0x0, 0x8d, 0x0, 0x0, 0x0, 0x43, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x32, 0x1, 0x0, 0x0, 0x0, 0xd4, 0x0, 0x0, 0x0, 0x45, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x64, 0x0, 0x0, 0x1, 0x19, 0xfc, 0x2, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x45, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x35, 0x1, 0x0, 0x0, 0x0, 0x49, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x39, 0x1, 0x0, 0x0, 0x0, 0x8d, 0x0, 0x0, 0x0, 0x43, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x33, 0x1, 0x0, 0x0, 0x0, 0xd4, 0x0, 0x0, 0x0, 0x45, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x1, 0x89, 0x0, 0x0, 0x0, 0xe, 0xfc, 0x1, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x32, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5f, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x36, 0x1, 0x0, 0x0, 0x0, 0x63, 0x0, 0x0, 0x0, 0x61, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x30, 0x1, 0x0, 0x0, 0x0, 0xc8, 0x0, 0x0, 0x0, 0x5e, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x34, 0x1, 0x0, 0x0, 0x1, 0x2a, 0x0, 0x0, 0x0, 0x5f, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x1, 0x89, 0x0, 0x0, 0x0, 0xe, 0xfc, 0x1, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x33, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5f, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x37, 0x1, 0x0, 0x0, 0x0, 0x63, 0x0, 0x0, 0x0, 0x61, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x31, 0x1, 0x0, 0x0, 0x0, 0xc8, 0x0, 0x0, 0x0, 0x5e, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x35, 0x1, 0x0, 0x0, 0x1, 0x2a, 0x0, 0x0, 0x0, 0x5f, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xc1, 0x0, 0x0, 0x1, 0x19};
static uchar restoreData42[] = { 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0, 0x0, 0xfc, 0x0, 0x0, 0x0, 0x0, 0xfd, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x24, 0x0, 0x0, 0x2, 0x2b, 0xfc, 0x2, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x30, 0x1, 0x0, 0x0, 0x0, 0x2a, 0x0, 0x0, 0x0, 0x88, 0x0, 0x0, 0x0, 0x21, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x34, 0x1, 0x0, 0x0, 0x0, 0xb6, 0x0, 0x0, 0x0, 0x88, 0x0, 0x0, 0x0, 0x21, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x38, 0x1, 0x0, 0x0, 0x1, 0x42, 0x0, 0x0, 0x0, 0x87, 0x0, 0x0, 0x0, 0x21, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x32, 0x1, 0x0, 0x0, 0x1, 0xcd, 0x0, 0x0, 0x0, 0x88, 0x0, 0x0, 0x0, 0x21, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x98, 0x0, 0x0, 0x2, 0x2b, 0xfc, 0x2, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x1, 0x0, 0x0, 0x0, 0x2a, 0x0, 0x0, 0x0, 0x88, 0x0, 0x0, 0x0, 0x21, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x35, 0x1, 0x0, 0x0, 0x0, 0xb6, 0x0, 0x0, 0x0, 0x88, 0x0, 0x0, 0x0, 0x21, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x39, 0x1, 0x0, 0x0, 0x1, 0x42, 0x0, 0x0, 0x0, 0x87, 0x0, 0x0, 0x0, 0x21, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x33, 0x1, 0x0, 0x0, 0x1, 0xcd, 0x0, 0x0, 0x0, 0x88, 0x0, 0x0, 0x0, 0x21, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x4, 0x4e, 0x0, 0x0, 0x0, 0x26, 0xfc, 0x1, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x32, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x12, 0x0, 0x0, 0x0, 0xa, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x36, 0x1, 0x0, 0x0, 0x1, 0x16, 0x0, 0x0, 0x1, 0xe, 0x0, 0x0, 0x0, 0xa, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x30, 0x1, 0x0, 0x0, 0x2, 0x28, 0x0, 0x0, 0x1, 0x14, 0x0, 0x0, 0x0, 0xa, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x34, 0x1, 0x0, 0x0, 0x3, 0x40, 0x0, 0x0, 0x1, 0xe, 0x0, 0x0, 0x0, 0xa, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x4, 0x4e, 0x0, 0x0, 0x0, 0x26, 0xfc, 0x1, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x33, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x12, 0x0, 0x0, 0x0, 0xa, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x37, 0x1, 0x0, 0x0, 0x1, 0x16, 0x0, 0x0, 0x1, 0xe, 0x0, 0x0, 0x0, 0xa, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x31, 0x1, 0x0, 0x0, 0x2, 0x28, 0x0, 0x0, 0x1, 0x14, 0x0, 0x0, 0x0, 0xa, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x35, 0x1, 0x0, 0x0, 0x3, 0x40, 0x0, 0x0, 0x1, 0xe, 0x0, 0x0, 0x0, 0xa, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x3, 0x8a, 0x0, 0x0, 0x2, 0x2b, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x8};
static uchar restoreData43[] = { 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0, 0x0, 0xfd, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x50, 0x0, 0x0, 0x0, 0xa0, 0xfc, 0x2, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x30, 0x1, 0x0, 0x0, 0x0, 0x2a, 0x0, 0x0, 0x0, 0x25, 0x0, 0x0, 0x0, 0x16, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x34, 0x1, 0x0, 0x0, 0x0, 0x53, 0x0, 0x0, 0x0, 0x25, 0x0, 0x0, 0x0, 0x16, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x38, 0x1, 0x0, 0x0, 0x0, 0x7c, 0x0, 0x0, 0x0, 0x25, 0x0, 0x0, 0x0, 0x16, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x32, 0x1, 0x0, 0x0, 0x0, 0xa5, 0x0, 0x0, 0x0, 0x25, 0x0, 0x0, 0x0, 0x16, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x98, 0x0, 0x0, 0x0, 0xa0, 0xfc, 0x2, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x1, 0x0, 0x0, 0x0, 0x2a, 0x0, 0x0, 0x0, 0x25, 0x0, 0x0, 0x0, 0x16, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x35, 0x1, 0x0, 0x0, 0x0, 0x53, 0x0, 0x0, 0x0, 0x25, 0x0, 0x0, 0x0, 0x16, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x39, 0x1, 0x0, 0x0, 0x0, 0x7c, 0x0, 0x0, 0x0, 0x25, 0x0, 0x0, 0x0, 0x16, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x33, 0x1, 0x0, 0x0, 0x0, 0xa5, 0x0, 0x0, 0x0, 0x25, 0x0, 0x0, 0x0, 0x16, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x1, 0xa8, 0x0, 0x0, 0x0, 0x26, 0xfc, 0x1, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x32, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x68, 0x0, 0x0, 0x0, 0x50, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x36, 0x1, 0x0, 0x0, 0x0, 0x6c, 0x0, 0x0, 0x0, 0x66, 0x0, 0x0, 0x0, 0x50, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x30, 0x1, 0x0, 0x0, 0x0, 0xd6, 0x0, 0x0, 0x0, 0x68, 0x0, 0x0, 0x0, 0x50, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x34, 0x1, 0x0, 0x0, 0x1, 0x42, 0x0, 0x0, 0x0, 0x66, 0x0, 0x0, 0x0, 0x50, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x1, 0xa8, 0x0, 0x0, 0x0, 0x26, 0xfc, 0x1, 0x0, 0x0, 0x0, 0x4, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x33, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x68, 0x0, 0x0, 0x0, 0x50, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xc, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x37, 0x1, 0x0, 0x0, 0x0, 0x6c, 0x0, 0x0, 0x0, 0x66, 0x0, 0x0, 0x0, 0x50, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x31, 0x1, 0x0, 0x0, 0x0, 0xd6, 0x0, 0x0, 0x0, 0x68, 0x0, 0x0, 0x0, 0x50, 0x0, 0xff, 0xff, 0xff, 0xfb, 0x0, 0x0, 0x0, 0xe, 0x0, 0x64, 0x0, 0x6f, 0x0, 0x63, 0x0, 0x6b, 0x0, 0x20, 0x0, 0x31, 0x0, 0x35, 0x1, 0x0, 0x0, 0x1, 0x42, 0x0, 0x0, 0x0, 0x66, 0x0, 0x0, 0x0, 0x50, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xb8, 0x0, 0x0, 0x0, 0xa0, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x8, 0xfc, 0x0, 0x0, 0x0, 0x0};
// Local scope class to simulate toolbar move.
// For testcase  void QTBUG21378_animationFinished();
class CToolBarTimer : public QObject
{
    int m_timerId;
    QToolBar *m_tb;
public:
    CToolBarTimer(QToolBar *tb) : m_tb(tb)
    {
        m_timerId = startTimer(200);
    }
    void timerEvent(QTimerEvent*)
    {
        qApp->postEvent(m_tb, new QMouseEvent(QEvent::MouseButtonPress, QPoint(6, 7), Qt::LeftButton, 0, 0));
        qApp->postEvent(m_tb, new QMouseEvent(QEvent::MouseMove, QPoint(7, 8), Qt::LeftButton, Qt::LeftButton, 0));
        qApp->postEvent(m_tb, new QMouseEvent(QEvent::MouseMove, QPoint(27, 23), Qt::LeftButton, Qt::LeftButton, 0));
        qApp->postEvent(m_tb, new QMouseEvent(QEvent::MouseMove, QPoint(30, 27), Qt::LeftButton, Qt::LeftButton, 0));
        qApp->postEvent(m_tb, new QMouseEvent(QEvent::MouseMove, QPoint(162, 109), Qt::LeftButton, Qt::LeftButton, 0));
        qApp->postEvent(m_tb, new QMouseEvent(QEvent::MouseMove, QPoint(10, 4), Qt::LeftButton, Qt::LeftButton, 0));
        qApp->postEvent(m_tb, new QMouseEvent(QEvent::MouseButtonRelease, QPoint(9, 4), Qt::LeftButton, 0, 0));
    }
};

// Local class to simulate mousepress on main window.
// For testcase void QTBUG21378_animationFinished();
class CMainWindowTimer : public QObject
{
    int m_timerId;
    QMainWindow *m_w;
public:
    CMainWindowTimer(QMainWindow *pmw) : m_w(pmw)
    {
        m_timerId = startTimer(100);
    }

    void timerEvent(QTimerEvent*)
    {
        qApp->postEvent(m_w, new QMouseEvent(QEvent::MouseButtonPress, QPoint(230, 370), Qt::LeftButton, 0, 0));
        qApp->postEvent(m_w, new QMouseEvent(QEvent::MouseButtonRelease, QPoint(230, 370), Qt::LeftButton, 0, 0));
    }
};

class tst_QMainWindow : public QObject
{
    Q_OBJECT

public:
    tst_QMainWindow();

private slots:
    void getSetCheck();
    void constructor();
    void iconSize();
    void toolButtonStyle();
    void menuBar();
    void centralWidget();
    void takeCentralWidget();
    void corner();
    void addToolBarBreak();
    void insertToolBarBreak();
    void addToolBar();
    void insertToolBar();
    void removeToolBar();
    void toolBarArea();
    void addDockWidget();
    void splitDockWidget();
    void removeDockWidget();
    void dockWidgetArea();
    void restoreState();
    void restoreStateFromPreviousVersion();
    void createPopupMenu();
    void hideBeforeLayout();
#ifdef QT_BUILD_INTERNAL
    void saveRestore();
    void saveRestore_data();
    void statusBar();
#endif
    void isSeparator();
#ifndef QTEST_NO_CURSOR
    void setCursor();
#endif
    void addToolbarAfterShow();
    void centralWidgetSize();
    void dockWidgetSize();
    void QTBUG2774_stylechange();
    void QTBUG15080_restoreState();
#ifdef Q_OS_MAC
    void toggleUnifiedTitleAndToolBarOnMac();
#endif
    void QTBUG21378_animationFinished();
};

// Testing get/set functions
void tst_QMainWindow::getSetCheck()
{
    QMainWindow obj1;
    // QMenuBar * QMainWindow::menuBar()
    // void QMainWindow::setMenuBar(QMenuBar *)
    QPointer<QMenuBar> var1 = new QMenuBar;
    obj1.setMenuBar(var1);
    QCOMPARE(static_cast<QMenuBar *>(var1), obj1.menuBar());
    obj1.setMenuBar((QMenuBar *)0);
    QVERIFY(obj1.menuBar());
    //we now call deleteLater on the previous menubar
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QVERIFY(!var1);
    // delete var1; // No delete, since QMainWindow takes ownership

    // QStatusBar * QMainWindow::statusBar()
    // void QMainWindow::setStatusBar(QStatusBar *)
    QPointer<QStatusBar> var2 = new QStatusBar;
    obj1.setStatusBar(var2);
    QCOMPARE(static_cast<QStatusBar *>(var2), obj1.statusBar());
    obj1.setStatusBar((QStatusBar *)0);
    QVERIFY(obj1.statusBar());
    //we now call deleteLater on the previous menubar
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QVERIFY(!var2);
    // delete var2; // No delete, since QMainWindow takes ownership

    // QWidget * QMainWindow::centralWidget()
    // void QMainWindow::setCentralWidget(QWidget *)
    QWidget *var3 = new QWidget;
    obj1.setCentralWidget(var3);
    QCOMPARE(var3, obj1.centralWidget());
    obj1.setCentralWidget((QWidget *)0);
    QCOMPARE((QWidget *)0, obj1.centralWidget());
    // delete var3; // No delete, since QMainWindow takes ownership

    QWidget *var4 = new QWidget;
    QPointer<QWidget> oldcentralwidget(var4);
    obj1.setCentralWidget(var4);
    obj1.setCentralWidget(new QWidget);
    QCoreApplication::sendPostedEvents(var4, QEvent::DeferredDelete);
    QVERIFY(oldcentralwidget.isNull());
    QVERIFY(obj1.centralWidget()->parent());
}

tst_QMainWindow::tst_QMainWindow()
{
    qRegisterMetaType<Qt::ToolButtonStyle>("Qt::ToolButtonStyle");
}

void tst_QMainWindow::constructor()
{
    QMainWindow mw;
    QVERIFY(mw.parentWidget() == 0);
    QVERIFY(mw.isWindow());

    QMainWindow mw2(&mw);
    QVERIFY(mw2.parentWidget() == &mw);
    QVERIFY(mw2.isWindow());

    QMainWindow mw3(&mw, Qt::FramelessWindowHint);
    QVERIFY(mw3.parentWidget() == &mw);
    QVERIFY(mw3.isWindow());
}

void tst_QMainWindow::iconSize()
{
    {
        QMainWindow mw;
        QSignalSpy spy(&mw, SIGNAL(iconSizeChanged(QSize)));

        // the default is determined by the style
        const int metric = mw.style()->pixelMetric(QStyle::PM_ToolBarIconSize);
        const QSize defaultIconSize = QSize(metric, metric);
        const QSize smallIconSize = QSize(metric / 2, metric / 2);
        const QSize largeIconSize = QSize(metric * 2, metric * 2);

        // no-op
        QCOMPARE(mw.iconSize(), defaultIconSize);
        mw.setIconSize(defaultIconSize);
        QCOMPARE(mw.iconSize(), defaultIconSize);
        QCOMPARE(spy.size(), 0);

        mw.setIconSize(largeIconSize);
        QCOMPARE(mw.iconSize(), largeIconSize);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.first().first().toSize(), largeIconSize);
        spy.clear();

        // no-op
        QCOMPARE(mw.iconSize(), largeIconSize);
        mw.setIconSize(largeIconSize);
        QCOMPARE(mw.iconSize(), largeIconSize);
        QCOMPARE(spy.size(), 0);

        mw.setIconSize(smallIconSize);
        QCOMPARE(mw.iconSize(), smallIconSize);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.first().first().toSize(), smallIconSize);
        spy.clear();

        // no-op
        QCOMPARE(mw.iconSize(), smallIconSize);
        mw.setIconSize(smallIconSize);
        QCOMPARE(mw.iconSize(), smallIconSize);
        QCOMPARE(spy.size(), 0);

        // setting the icon size to an invalid QSize will reset the
        // iconSize property to the default
        mw.setIconSize(QSize());
        QCOMPARE(mw.iconSize(), defaultIconSize);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.first().first().toSize(), defaultIconSize);
        spy.clear();
    }

    {
        // toolbars should follow the mainwindow's icon size
        QMainWindow mw;
        QToolBar tb;
        mw.addToolBar(&tb);

        QSignalSpy mwSpy(&mw, SIGNAL(iconSizeChanged(QSize)));
        QSignalSpy tbSpy(&tb, SIGNAL(iconSizeChanged(QSize)));

        // the default is determined by the style
        const int metric = mw.style()->pixelMetric(QStyle::PM_ToolBarIconSize);
        const QSize defaultIconSize = QSize(metric, metric);
        const QSize smallIconSize = QSize(metric / 2, metric / 2);
        const QSize largeIconSize = QSize(metric * 2, metric * 2);

        // no-op
        QCOMPARE(mw.iconSize(), defaultIconSize);
        mw.setIconSize(defaultIconSize);
        QCOMPARE(mw.iconSize(), defaultIconSize);
        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(mwSpy.size(), 0);
        QCOMPARE(tbSpy.size(), 0);

        mw.setIconSize(largeIconSize);
        QCOMPARE(mw.iconSize(), largeIconSize);
        QCOMPARE(tb.iconSize(), largeIconSize);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(mwSpy.first().first().toSize(), largeIconSize);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(tbSpy.first().first().toSize(), largeIconSize);
        mwSpy.clear();
        tbSpy.clear();

        // no-op
        QCOMPARE(mw.iconSize(), largeIconSize);
        mw.setIconSize(largeIconSize);
        QCOMPARE(mw.iconSize(), largeIconSize);
        QCOMPARE(tb.iconSize(), largeIconSize);
        QCOMPARE(mwSpy.size(), 0);
        QCOMPARE(tbSpy.size(), 0);

        mw.setIconSize(smallIconSize);
        QCOMPARE(mw.iconSize(), smallIconSize);
        QCOMPARE(tb.iconSize(), smallIconSize);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(mwSpy.first().first().toSize(), smallIconSize);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(tbSpy.first().first().toSize(), smallIconSize);
        mwSpy.clear();
        tbSpy.clear();

        // no-op
        QCOMPARE(mw.iconSize(), smallIconSize);
        mw.setIconSize(smallIconSize);
        QCOMPARE(mw.iconSize(), smallIconSize);
        QCOMPARE(tb.iconSize(), smallIconSize);
        QCOMPARE(mwSpy.size(), 0);
        QCOMPARE(tbSpy.size(), 0);

        // setting the icon size to an invalid QSize will reset the
        // iconSize property to the default
        mw.setIconSize(QSize());
        QCOMPARE(mw.iconSize(), defaultIconSize);
        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(mwSpy.first().first().toSize(), defaultIconSize);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(tbSpy.first().first().toSize(), defaultIconSize);
        mwSpy.clear();
        tbSpy.clear();
    }

    {
        QMainWindow mw;
        QSignalSpy mwSpy(&mw, SIGNAL(iconSizeChanged(QSize)));

        // the default is determined by the style
        const int metric = mw.style()->pixelMetric(QStyle::PM_ToolBarIconSize);
        const QSize smallIconSize = QSize(metric / 2, metric / 2);
        const QSize largeIconSize = QSize(metric * 2, metric * 2);

        mw.setIconSize(smallIconSize);
        QCOMPARE(mw.iconSize(), smallIconSize);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(mwSpy.first().first().toSize(), smallIconSize);
        mwSpy.clear();

        QToolBar tb;
        QSignalSpy tbSpy(&tb, SIGNAL(iconSizeChanged(QSize)));

        mw.addToolBar(&tb);

        // newly added toolbars should also automatically pick up any
        // size set on the main window
        QCOMPARE(tb.iconSize(), smallIconSize);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(tbSpy.first().first().toSize(), smallIconSize);
        tbSpy.clear();

        mw.removeToolBar(&tb);

        // removed toolbars should keep their existing size and ignore
        // mainwindow icon size changes
        mw.setIconSize(largeIconSize);
        QCOMPARE(mw.iconSize(), largeIconSize);
        QCOMPARE(tb.iconSize(), smallIconSize);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(mwSpy.first().first().toSize(), largeIconSize);
        QCOMPARE(tbSpy.size(), 0);
        mwSpy.clear();
    }
}

void tst_QMainWindow::toolButtonStyle()
{
    {
        QMainWindow mw;

        QSignalSpy spy(&mw, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)));

        // no-op
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonIconOnly);
        mw.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(spy.size(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonTextOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextOnly);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(spy.first().first().constData()),
                Qt::ToolButtonTextOnly);
        spy.clear();

        // no-op
        mw.setToolButtonStyle(Qt::ToolButtonTextOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextOnly);
        QCOMPARE(spy.size(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(spy.first().first().constData()),
                Qt::ToolButtonTextBesideIcon);
        spy.clear();

        // no-op
        mw.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(spy.size(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(spy.first().first().constData()),
                Qt::ToolButtonTextUnderIcon);
        spy.clear();

        // no-op
        mw.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(spy.size(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(spy.first().first().constData()),
                Qt::ToolButtonIconOnly);
        spy.clear();

        // no-op
        mw.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(spy.size(), 0);
    }

    {
        // toolbars should follow the mainwindow's tool button style
        QMainWindow mw;
        QToolBar tb;
        mw.addToolBar(&tb);

        QSignalSpy mwSpy(&mw, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)));
        QSignalSpy tbSpy(&tb, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)));

        // no-op
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        mw.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(mwSpy.size(), 0);
        QCOMPARE(tbSpy.size(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonTextOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextOnly);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(mwSpy.first().first().constData()),
                Qt::ToolButtonTextOnly);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(tbSpy.first().first().constData()),
                Qt::ToolButtonTextOnly);
        mwSpy.clear();
        tbSpy.clear();

        // no-op
        mw.setToolButtonStyle(Qt::ToolButtonTextOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextOnly);
        QCOMPARE(mwSpy.size(), 0);
        QCOMPARE(tbSpy.size(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(mwSpy.first().first().constData()),
                Qt::ToolButtonTextBesideIcon);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(tbSpy.first().first().constData()),
                Qt::ToolButtonTextBesideIcon);
        mwSpy.clear();
        tbSpy.clear();

        // no-op
        mw.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mwSpy.size(), 0);
        QCOMPARE(tbSpy.size(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(mwSpy.first().first().constData()),
                Qt::ToolButtonTextUnderIcon);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(tbSpy.first().first().constData()),
                Qt::ToolButtonTextUnderIcon);
        mwSpy.clear();
        tbSpy.clear();

        // no-op
        mw.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(mwSpy.size(), 0);
        QCOMPARE(tbSpy.size(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(mwSpy.first().first().constData()),
                Qt::ToolButtonIconOnly);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(tbSpy.first().first().constData()),
                Qt::ToolButtonIconOnly);
        mwSpy.clear();
        tbSpy.clear();

        // no-op
        mw.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(mwSpy.size(), 0);
        QCOMPARE(tbSpy.size(), 0);
    }

    {
        QMainWindow mw;
        QSignalSpy mwSpy(&mw, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)));

        mw.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(mwSpy.first().first().constData()),
                Qt::ToolButtonTextBesideIcon);
        mwSpy.clear();

        QToolBar tb;
        QSignalSpy tbSpy(&tb, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)));

        mw.addToolBar(&tb);

        // newly added toolbars should also automatically pick up any
        // size set on the main window
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(tbSpy.first().first().constData()),
                Qt::ToolButtonTextBesideIcon);
        tbSpy.clear();

        mw.removeToolBar(&tb);

        // removed toolbars should keep their existing size and ignore
        // mainwindow icon size changes
        mw.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        QCOMPARE(mw.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(mwSpy.size(), 1);
        QCOMPARE(*static_cast<const Qt::ToolButtonStyle *>(mwSpy.first().first().constData()),
                Qt::ToolButtonTextUnderIcon);
        QCOMPARE(tbSpy.size(), 0);
        mwSpy.clear();
    }
}

void tst_QMainWindow::menuBar()
{
    {
        QMainWindow mw;
        QVERIFY(mw.menuBar() != 0);
    }

    {
        QMainWindow mw;
        QPointer<QMenuBar> mb1 = new QMenuBar;
        QPointer<QMenuBar> mb2 = new QMenuBar;

        mw.setMenuBar(mb1);
        QVERIFY(mw.menuBar() != 0);
        QCOMPARE(mw.menuBar(), (QMenuBar *)mb1);
#ifdef Q_OS_WINCE_WM
        QSKIP("With native menubar integration the menubar is not a child");
#endif
        QCOMPARE(mb1->parentWidget(), (QWidget *)&mw);

        mw.setMenuBar(0);
        QVERIFY(mw.menuBar() != 0);
        //we now call deleteLater on the previous menubar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(mb1 == 0);

        mw.setMenuBar(mb2);
        QVERIFY(mw.menuBar() != 0);
        QCOMPARE(mw.menuBar(), (QMenuBar *)mb2);
        QCOMPARE(mb2->parentWidget(), (QWidget *)&mw);

        mw.setMenuBar(0);
        QVERIFY(mw.menuBar() != 0);
        //we now call deleteLater on the previous menubar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(mb2 == 0);

        mb1 = new QMenuBar;
        mw.setMenuBar(mb1);
        QVERIFY(mw.menuBar() != 0);
        QCOMPARE(mw.menuBar(), (QMenuBar *)mb1);

        mb2 = new QMenuBar;
        mw.setMenuBar(mb2);
        QVERIFY(mw.menuBar() != 0);
        QCOMPARE(mw.menuBar(), (QMenuBar *)mb2);
        //we now call deleteLater on the previous menubar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(mb1 == 0);

        mb1 = new QMenuBar;
        mw.setMenuBar(mb1);
        QVERIFY(mw.menuBar() != 0);
        QCOMPARE(mw.menuBar(), (QMenuBar *)mb1);
        //we now call deleteLater on the previous menubar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(mb2 == 0);

        QPointer<QWidget> topLeftCornerWidget = new QWidget;
        mb1->setCornerWidget(topLeftCornerWidget, Qt::TopLeftCorner);
        QPointer<QWidget> topRightCornerWidget = new QWidget;
        mb1->setCornerWidget(topRightCornerWidget, Qt::TopRightCorner);

        mb2 = new QMenuBar;
        mw.setMenuBar(mb2);
        QVERIFY(mw.menuBar() != 0);
        QCOMPARE(mw.menuBar(), (QMenuBar *)mb2);
        //we now call deleteLater on the previous menubar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(mb1 == 0);

        QVERIFY(topLeftCornerWidget);
        QCOMPARE(mb2->cornerWidget(Qt::TopLeftCorner), static_cast<QWidget *>(topLeftCornerWidget));
        QVERIFY(topRightCornerWidget);
        QCOMPARE(mb2->cornerWidget(Qt::TopRightCorner), static_cast<QWidget *>(topRightCornerWidget));

        mw.setMenuBar(0);
        QVERIFY(mw.menuBar() != 0);
        //we now call deleteLater on the previous menubar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(mb2 == 0);

        QVERIFY(!topLeftCornerWidget);
        QVERIFY(!topRightCornerWidget);
    }
}

#ifdef QT_BUILD_INTERNAL
void tst_QMainWindow::statusBar()
{
    {
        QMainWindow mw;
        QVERIFY(mw.statusBar() != 0);
    }

    {
        QMainWindow mw;
        QPointer<QStatusBar> sb1 = new QStatusBar;
        QPointer<QStatusBar> sb2 = new QStatusBar;

        mw.setStatusBar(sb1);
        QVERIFY(mw.statusBar() != 0);
        QCOMPARE(mw.statusBar(), (QStatusBar *)sb1);
        QCOMPARE(sb1->parentWidget(), (QWidget *)&mw);

        mw.setStatusBar(0);
        QVERIFY(mw.statusBar() != 0);
        //we now call deleteLater on the previous statusbar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(sb1 == 0);

        mw.setStatusBar(sb2);
        QVERIFY(mw.statusBar() != 0);
        QCOMPARE(mw.statusBar(), (QStatusBar *)sb2);
        QCOMPARE(sb2->parentWidget(), (QWidget *)&mw);

        mw.setStatusBar(0);
        QVERIFY(mw.statusBar() != 0);
        //we now call deleteLater on the previous statusbar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(sb2 == 0);

        sb1 = new QStatusBar;
        mw.setStatusBar(sb1);
        QVERIFY(mw.statusBar() != 0);
        QCOMPARE(mw.statusBar(), (QStatusBar *)sb1);
        QCOMPARE(sb1->parentWidget(), (QWidget *)&mw);

        sb2 = new QStatusBar;
        mw.setStatusBar(sb2);
        QVERIFY(mw.statusBar() != 0);
        QCOMPARE(mw.statusBar(), (QStatusBar *)sb2);
        QCOMPARE(sb2->parentWidget(), (QWidget *)&mw);
        //we now call deleteLater on the previous statusbar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(sb1 == 0);

        sb1 = new QStatusBar;
        mw.setStatusBar(sb1);
        QVERIFY(mw.statusBar() != 0);
        QCOMPARE(mw.statusBar(), (QStatusBar *)sb1);
        QCOMPARE(sb1->parentWidget(), (QWidget *)&mw);
        //we now call deleteLater on the previous statusbar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(sb2 == 0);

        sb2 = new QStatusBar;
        mw.setStatusBar(sb2);
        QVERIFY(mw.statusBar() != 0);
        QCOMPARE(mw.statusBar(), (QStatusBar *)sb2);
        QCOMPARE(sb2->parentWidget(), (QWidget *)&mw);
        //we now call deleteLater on the previous statusbar
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(sb1 == 0);
    }

    {
        // deleting the status bar should remove it from the main window
        QMainWindow mw;
        QStatusBar *sb = mw.statusBar();
        QMainWindowLayout *l = mw.findChild<QMainWindowLayout *>();
        QVERIFY(l);
        int indexOfSb = l->indexOf(sb);
        QVERIFY(indexOfSb != -1);
        delete sb;
        indexOfSb = l->indexOf(sb);
        QVERIFY(indexOfSb == -1);
    }
}
#endif

void tst_QMainWindow::centralWidget()
{
    {
        QMainWindow mw;
        QVERIFY(mw.centralWidget() == 0);
    }

    {
        QMainWindow mw;
        QPointer<QWidget> w1 = new QWidget;
        QPointer<QWidget> w2 = new QWidget;

        QVERIFY(mw.centralWidget() == 0);

        mw.setCentralWidget(w1);
        QVERIFY(mw.centralWidget() != 0);
        QCOMPARE(mw.centralWidget(), (QWidget *)w1);
        QCOMPARE(w1->parentWidget(), (QWidget *)&mw);

        mw.setCentralWidget(w2);
        QVERIFY(mw.centralWidget() != 0);
        QCOMPARE(mw.centralWidget(), (QWidget *)w2);
        QCOMPARE(w2->parentWidget(), (QWidget *)&mw);

        mw.setCentralWidget(0);
        QVERIFY(mw.centralWidget() == 0);

        //we now call deleteLater on the previous central widgets
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(w1 == 0);
        QVERIFY(w2 == 0);
    }

    {
        // do it again, this time with the mainwindow shown, since
        // this tends will activate the layout when setting the new
        // central widget

        QMainWindow mw;
        mw.show();

        QPointer<QWidget> w1 = new QWidget;
        QPointer<QWidget> w2 = new QWidget;

        QVERIFY(mw.centralWidget() == 0);

        mw.setCentralWidget(w1);
        QVERIFY(mw.centralWidget() != 0);
        QCOMPARE(mw.centralWidget(), (QWidget *)w1);
        QCOMPARE(w1->parentWidget(), (QWidget *)&mw);

        mw.setCentralWidget(w2);
        QVERIFY(mw.centralWidget() != 0);
        QCOMPARE(mw.centralWidget(), (QWidget *)w2);
        QCOMPARE(w2->parentWidget(), (QWidget *)&mw);

        mw.setCentralWidget(0);
        QVERIFY(mw.centralWidget() == 0);

        //we now call deleteLater on the previous central widgets
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(w1 == 0);
        QVERIFY(w2 == 0);
    }

}

void tst_QMainWindow::takeCentralWidget() {
    // test if takeCentralWidget works
    QMainWindow mw;

    QPointer<QWidget> w1 = new QWidget;

    QVERIFY(mw.centralWidget() == 0);

    mw.setCentralWidget(w1);

    QWidget *oldCentralWidget = mw.takeCentralWidget();
    QVERIFY(oldCentralWidget == w1.data());

    // ensure that takeCentralWidget doesn't end up calling deleteLater
    // on the central widget
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QVERIFY(mw.centralWidget() == 0);
    QVERIFY(!w1.isNull());
    QVERIFY(w1->parent() == 0);

    mw.setCentralWidget(w1);
    // ensure that the deleteLater called by setCentralWidget
    // gets executed
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QVERIFY(mw.centralWidget() == w1.data());

    QPointer<QWidget> w2 = new QWidget;

    mw.setCentralWidget(w2);
    // ensure w2 gets deleted
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QVERIFY(w1.isNull());

    QVERIFY(mw.centralWidget() == w2.data());

    QWidget *hopefullyW2 = mw.takeCentralWidget();
    QVERIFY(mw.centralWidget() == 0);
    // ensure that takeCentralWidget doesn't end up calling deleteLater
    // on the central widget
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);

    QVERIFY(!w2.isNull());
    QCOMPARE(w2.data(), hopefullyW2);
}

void tst_QMainWindow::corner()
{
    {
        QMainWindow mw;

        QCOMPARE(mw.corner(Qt::TopLeftCorner), Qt::TopDockWidgetArea);
        QCOMPARE(mw.corner(Qt::TopRightCorner), Qt::TopDockWidgetArea);
        QCOMPARE(mw.corner(Qt::BottomLeftCorner), Qt::BottomDockWidgetArea);
        QCOMPARE(mw.corner(Qt::BottomRightCorner), Qt::BottomDockWidgetArea);
    }

    {
        QMainWindow mw;

        mw.setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        QCOMPARE(mw.corner(Qt::TopLeftCorner), Qt::LeftDockWidgetArea);
        mw.setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
        QCOMPARE(mw.corner(Qt::TopLeftCorner), Qt::TopDockWidgetArea);

        mw.setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        QCOMPARE(mw.corner(Qt::TopRightCorner), Qt::RightDockWidgetArea);
        mw.setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
        QCOMPARE(mw.corner(Qt::TopRightCorner), Qt::TopDockWidgetArea);

        mw.setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        QCOMPARE(mw.corner(Qt::BottomLeftCorner), Qt::LeftDockWidgetArea);
        mw.setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
        QCOMPARE(mw.corner(Qt::BottomLeftCorner), Qt::BottomDockWidgetArea);

        mw.setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
        QCOMPARE(mw.corner(Qt::BottomRightCorner), Qt::RightDockWidgetArea);
        mw.setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
        QCOMPARE(mw.corner(Qt::BottomRightCorner), Qt::BottomDockWidgetArea);
    }
}

void tst_QMainWindow::addToolBarBreak()
{
    {
        QMainWindow mw;
        QToolBar tb1(&mw);
        mw.addToolBar(Qt::TopToolBarArea, &tb1);
        mw.addToolBarBreak(Qt::TopToolBarArea);
        QToolBar tb2(&mw);
        mw.addToolBar(Qt::TopToolBarArea, &tb2);
        mw.addToolBarBreak(Qt::TopToolBarArea);
        QToolBar tb3(&mw);
        mw.addToolBar(Qt::TopToolBarArea, &tb3);
        mw.addToolBarBreak(Qt::TopToolBarArea);
        QToolBar tb4(&mw);
        mw.addToolBar(Qt::TopToolBarArea, &tb4);

        mw.layout()->invalidate();
        mw.layout()->activate();

        QCOMPARE(tb1.x(), 0);
        QCOMPARE(tb1.y(), 0);
        QCOMPARE(tb2.x(), 0);
        QVERIFY(tb1.y() != tb2.y());
        QCOMPARE(tb3.x(), 0);
        QVERIFY(tb2.y() != tb3.y());
        QCOMPARE(tb4.x(), 0);
        QVERIFY(tb3.y() != tb4.y());
    }

    {
        QMainWindow mw;
        // should not crash, should get a warning instead
        QTest::ignoreMessage(QtWarningMsg, "QMainWindow::addToolBarBreak: invalid 'area' argument");
        mw.addToolBarBreak(Qt::NoToolBarArea);
    }
}

void tst_QMainWindow::insertToolBarBreak()
{
    QMainWindow mw;
    QToolBar tb1(&mw);
    mw.addToolBar(Qt::TopToolBarArea, &tb1);
    QToolBar tb2(&mw);
    mw.addToolBar(Qt::TopToolBarArea, &tb2);
    QToolBar tb3(&mw);
    mw.addToolBar(Qt::TopToolBarArea, &tb3);
    QToolBar tb4(&mw);
    mw.addToolBar(Qt::TopToolBarArea, &tb4);

    mw.insertToolBarBreak(&tb2);
    mw.insertToolBarBreak(&tb3);
    mw.insertToolBarBreak(&tb4);

    mw.layout()->invalidate();
    mw.layout()->activate();

    QCOMPARE(tb1.x(), 0);
    QCOMPARE(tb1.y(), 0);
    QCOMPARE(tb2.x(), 0);
    QVERIFY(tb1.y() != tb2.y());
    QCOMPARE(tb3.x(), 0);
    QVERIFY(tb2.y() != tb3.y());
    QCOMPARE(tb4.x(), 0);
    QVERIFY(tb3.y() != tb4.y());

    QVERIFY(!mw.toolBarBreak(&tb1));
    QVERIFY(mw.toolBarBreak(&tb4));
    mw.removeToolBarBreak(&tb4);
    QVERIFY(!mw.toolBarBreak(&tb4));

}

static bool findWidgetRecursively(QLayoutItem *li, QWidget *w)
{
    QLayout *lay = li->layout();
    if (!lay)
        return false;
    int i = 0;
    QLayoutItem *child;
    while ((child = lay->itemAt(i))) {
        if (child->widget() == w) {
            return true;
        } else if (findWidgetRecursively(child, w)) {
            return true;
        } else {
            ++i;
        }
    }
    return false;
}

void tst_QMainWindow::addToolBar()
{
    Qt::ToolBarArea areas[] = {
        Qt::LeftToolBarArea,
        Qt::RightToolBarArea,
        Qt::TopToolBarArea,
        Qt::BottomToolBarArea
    };
    const int areaCount = sizeof(areas) / sizeof(Qt::ToolBarArea);

    for (int i = 0; i < areaCount; ++i) {
        Qt::ToolBarArea area = areas[i];

        QMainWindow mw;
        QToolBar tb(&mw);
        QVERIFY(!findWidgetRecursively(mw.layout(), &tb));
        mw.addToolBar(area, &tb);
        QVERIFY(findWidgetRecursively(mw.layout(), &tb));
    }

    {
        // addToolBar() with no area, equivalent to top
        QMainWindow mw;
        QToolBar tb(&mw);
        QVERIFY(!findWidgetRecursively(mw.layout(), &tb));
        mw.addToolBar(&tb);
        QVERIFY(findWidgetRecursively(mw.layout(), &tb));
    }

    {
        QMainWindow mw;
        QToolBar tb(&mw);
        // should not crash, should get a warning instead
        QTest::ignoreMessage(QtWarningMsg, "QMainWindow::addToolBar: invalid 'area' argument");
        mw.addToolBar(Qt::NoToolBarArea, &tb);
    }
}

void tst_QMainWindow::insertToolBar()
{
    Qt::ToolBarArea areas[] = {
        Qt::LeftToolBarArea,
        Qt::RightToolBarArea,
        Qt::TopToolBarArea,
        Qt::BottomToolBarArea
    };
    const int areaCount = sizeof(areas) / sizeof(Qt::ToolBarArea);

    for (int i = 0; i < areaCount; ++i) {
        Qt::ToolBarArea area = areas[i];

        QMainWindow mw;
        QToolBar tb1(&mw);
        mw.addToolBar(area, &tb1);
        QToolBar tb2(&mw);
        mw.insertToolBar(&tb1, &tb2);
        QVERIFY(findWidgetRecursively(mw.layout(), &tb1));
        QVERIFY(findWidgetRecursively(mw.layout(), &tb2));
    }

    {
        QMainWindow window;
        QToolBar *bar1 = new QToolBar(QObject::tr("bar1"), &window);
        bar1->addWidget(new QPushButton(QObject::tr("bar1")));
        QToolBar *bar2 = new QToolBar(QLatin1String("bar2"));
        bar2->addWidget(new QPushButton(QLatin1String("bar2")));
        QToolBar *bar3 = new QToolBar(QLatin1String("bar3"));
        bar3->addWidget(new QPushButton(QLatin1String("bar3")));

        window.addToolBar(bar1);
        window.addToolBar(bar3);
        window.insertToolBar(bar1,bar2);
        window.insertToolBar(bar1, bar3);

        QVERIFY(!window.isVisible());
        QVERIFY(!bar1->isVisible());
        QVERIFY(!bar2->isVisible());
        QVERIFY(!bar3->isVisible());

        window.show();

        QVERIFY(window.isVisible());
        QVERIFY(bar1->isVisible());
        QVERIFY(bar2->isVisible());
        QVERIFY(bar3->isVisible());
    }
}

void tst_QMainWindow::removeToolBar()
{
    Qt::ToolBarArea areas[] = {
        Qt::LeftToolBarArea,
        Qt::RightToolBarArea,
        Qt::TopToolBarArea,
        Qt::BottomToolBarArea
    };
    const int areaCount = sizeof(areas) / sizeof(Qt::ToolBarArea);

    for (int i = 0; i < areaCount; ++i) {
        Qt::ToolBarArea area = areas[i];

        QMainWindow mw;
        QToolBar tb1(&mw);
        mw.addToolBar(area, &tb1);
        QToolBar tb2(&mw);
        mw.insertToolBar(&tb1, &tb2);
        QVERIFY(findWidgetRecursively(mw.layout(), &tb1));
        QVERIFY(findWidgetRecursively(mw.layout(), &tb2));

        mw.removeToolBar(&tb1);
        QVERIFY(!findWidgetRecursively(mw.layout(), &tb1));
        QVERIFY(findWidgetRecursively(mw.layout(), &tb2));

        mw.removeToolBar(&tb2);
        QVERIFY(!findWidgetRecursively(mw.layout(), &tb1));
        QVERIFY(!findWidgetRecursively(mw.layout(), &tb2));
    }
}

void tst_QMainWindow::toolBarArea()
{
    Qt::ToolBarArea areas[] = {
        Qt::LeftToolBarArea,
        Qt::RightToolBarArea,
        Qt::TopToolBarArea,
        Qt::BottomToolBarArea
    };
    const int areaCount = sizeof(areas) / sizeof(Qt::ToolBarArea);

    for (int i = 0; i < areaCount; ++i) {
        Qt::ToolBarArea area = areas[i];

        QMainWindow mw;
        QToolBar tb(&mw);

        for (int j = 0; j < areaCount; ++j) {
            Qt::ToolBarArea otherArea = areas[j];

            mw.addToolBar(area, &tb);
            QCOMPARE(mw.toolBarArea(&tb), area);
            mw.addToolBar(otherArea, &tb);
            QCOMPARE(mw.toolBarArea(&tb), otherArea);
        }
    }

    {
        // addToolBar() with no area, equivalent to top
        QMainWindow mw;
        QToolBar tb(&mw);

        for (int j = 0; j < areaCount; ++j) {
            Qt::ToolBarArea otherArea = areas[j];

            mw.addToolBar(&tb);
            QCOMPARE(mw.toolBarArea(&tb), Qt::TopToolBarArea);
            mw.addToolBar(otherArea, &tb);
            QCOMPARE(mw.toolBarArea(&tb), otherArea);
        }
    }
}

void tst_QMainWindow::addDockWidget()
{
    Qt::DockWidgetArea areas[] = {
        Qt::LeftDockWidgetArea,
        Qt::RightDockWidgetArea,
        Qt::TopDockWidgetArea,
        Qt::BottomDockWidgetArea
    };
    const int areaCount = sizeof(areas) / sizeof(Qt::DockWidgetArea);

    for (int i = 0; i < areaCount; ++i) {
        Qt::DockWidgetArea area = areas[i];

        QMainWindow mw;
        QDockWidget dw(&mw);
        QVERIFY(!findWidgetRecursively(mw.layout(), &dw));
        mw.addDockWidget(area, &dw);
        QVERIFY(findWidgetRecursively(mw.layout(), &dw));
    }

    for (int i = 0; i < areaCount; ++i) {
        Qt::DockWidgetArea area = areas[i];

        {
            QMainWindow mw;
            QDockWidget dw(&mw);
            QVERIFY(!findWidgetRecursively(mw.layout(), &dw));
            mw.addDockWidget(area, &dw, Qt::Horizontal);
            QVERIFY(findWidgetRecursively(mw.layout(), &dw));
        }

        {
            QMainWindow mw;
            QDockWidget dw(&mw);
            QVERIFY(!findWidgetRecursively(mw.layout(), &dw));
            mw.addDockWidget(area, &dw, Qt::Vertical);
            QVERIFY(findWidgetRecursively(mw.layout(), &dw));
        }
    }

    {
        QMainWindow mw;
        QDockWidget dw(&mw);
        // should not crash, should get a warning instead
        QTest::ignoreMessage(QtWarningMsg, "QMainWindow::addDockWidget: invalid 'area' argument");
        mw.addDockWidget(Qt::NoDockWidgetArea, &dw);
    }
}

void tst_QMainWindow::splitDockWidget()
{
    Qt::DockWidgetArea areas[] = {
        Qt::LeftDockWidgetArea,
        Qt::RightDockWidgetArea,
        Qt::TopDockWidgetArea,
        Qt::BottomDockWidgetArea
    };
    const int areaCount = sizeof(areas) / sizeof(Qt::DockWidgetArea);

    for (int i = 0; i < areaCount; ++i) {
        Qt::DockWidgetArea area = areas[i];

        {
            QMainWindow mw;
            QDockWidget dw1(&mw);
            QVERIFY(!findWidgetRecursively(mw.layout(), &dw1));
            mw.addDockWidget(area, &dw1);
            QVERIFY(findWidgetRecursively(mw.layout(), &dw1));
            QDockWidget dw2(&mw);
            QVERIFY(!findWidgetRecursively(mw.layout(), &dw2));
            mw.splitDockWidget(&dw1, &dw2, Qt::Horizontal);
            QVERIFY(findWidgetRecursively(mw.layout(), &dw2));
        }

        {
            QMainWindow mw;
            QDockWidget dw1(&mw);
            QVERIFY(!findWidgetRecursively(mw.layout(), &dw1));
            mw.addDockWidget(area, &dw1);
            QVERIFY(findWidgetRecursively(mw.layout(), &dw1));
            QDockWidget dw2(&mw);
            QVERIFY(!findWidgetRecursively(mw.layout(), &dw2));
            mw.splitDockWidget(&dw1, &dw2, Qt::Horizontal);
            QVERIFY(findWidgetRecursively(mw.layout(), &dw2));
        }
    }
}

void tst_QMainWindow::removeDockWidget()
{
    Qt::DockWidgetArea areas[] = {
        Qt::LeftDockWidgetArea,
        Qt::RightDockWidgetArea,
        Qt::TopDockWidgetArea,
        Qt::BottomDockWidgetArea
    };
    const int areaCount = sizeof(areas) / sizeof(Qt::DockWidgetArea);

    for (int i = 0; i < areaCount; ++i) {
        Qt::DockWidgetArea area = areas[i];

        QMainWindow mw;
        QDockWidget dw1(&mw);
        mw.addDockWidget(area, &dw1);
        QDockWidget dw2(&mw);
        mw.addDockWidget(area, &dw2);
        QVERIFY(findWidgetRecursively(mw.layout(), &dw1));
        QVERIFY(findWidgetRecursively(mw.layout(), &dw2));

        mw.removeDockWidget(&dw1);
        QVERIFY(!findWidgetRecursively(mw.layout(), &dw1));
        QVERIFY(findWidgetRecursively(mw.layout(), &dw2));

        mw.removeDockWidget(&dw2);
        QVERIFY(!findWidgetRecursively(mw.layout(), &dw1));
        QVERIFY(!findWidgetRecursively(mw.layout(), &dw2));
    }
}

void tst_QMainWindow::dockWidgetArea()
{
    Qt::DockWidgetArea areas[] = {
        Qt::LeftDockWidgetArea,
        Qt::RightDockWidgetArea,
        Qt::TopDockWidgetArea,
        Qt::BottomDockWidgetArea
    };
    const int areaCount = sizeof(areas) / sizeof(Qt::DockWidgetArea);

    for (int i = 0; i < areaCount; ++i) {
        Qt::DockWidgetArea area = areas[i];

        QMainWindow mw;
        QDockWidget dw(&mw);

        for (int j = 0; j < areaCount; ++j) {
            Qt::DockWidgetArea otherArea = areas[i];

            mw.addDockWidget(area, &dw);
            QCOMPARE(mw.dockWidgetArea(&dw), area);
            mw.addDockWidget(otherArea, &dw);
            QCOMPARE(mw.dockWidgetArea(&dw), otherArea);
        }
    }
}

void tst_QMainWindow::restoreState()
{
    QMainWindow mw;
    QToolBar tb(&mw);
    mw.addToolBar(Qt::TopToolBarArea, &tb);
    QDockWidget dw(&mw);
    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);

    QByteArray state;

    state = mw.saveState();
    QVERIFY(mw.restoreState(state));

    state = mw.saveState(1);
    QVERIFY(!mw.restoreState(state));
    QVERIFY(mw.restoreState(state, 1));
}

//tests the restoration of the previous versions of window settings
void tst_QMainWindow::restoreStateFromPreviousVersion()
{
    QList<QByteArray> restoreData;
    restoreData << QByteArray((char*)restoreData41, sizeof(restoreData41))
         << QByteArray((char*)restoreData42, sizeof(restoreData42))
         << QByteArray((char*)restoreData43, sizeof(restoreData43));

    foreach(QByteArray ba, restoreData) {

        QMainWindow win;
        win.setCentralWidget(new QTextEdit);
        QVector<QDockWidget*> docks;

        for(int i = 0; i < 16; ++i) {
            const QString name = QString::fromLatin1("dock %1").arg(i);
            QDockWidget *dock = new QDockWidget(name);
            docks += dock;
            dock->setObjectName(name);
            win.addDockWidget(Qt::TopDockWidgetArea, dock);
        }
        win.show();
        QCOMPARE(win.restoreState(ba), true);

        for( int i = 0; i < docks.size(); ++i) {
            QCOMPARE( win.dockWidgetArea(docks[i]), Qt::DockWidgetArea(1 << i%4));
        }
    }

}


void tst_QMainWindow::createPopupMenu()
{
    {
        QMainWindow mainwindow;
        QVERIFY(!mainwindow.createPopupMenu());

        QToolBar toolbar1(&mainwindow);
        toolbar1.setWindowTitle("toolbar1");
        QToolBar toolbar2(&mainwindow);
        toolbar2.setWindowTitle("toolbar2");

        mainwindow.addToolBar(&toolbar1);
        mainwindow.addToolBar(&toolbar2);

        QDockWidget dockwidget1(&mainwindow);
        dockwidget1.setWindowTitle("dockwidget1");
        QDockWidget dockwidget2(&mainwindow);
        dockwidget2.setWindowTitle("dockwidget2");
        QDockWidget dockwidget3(&mainwindow);
        dockwidget3.setWindowTitle("dockwidget3");
        QDockWidget dockwidget4(&mainwindow);
        dockwidget4.setWindowTitle("dockwidget4");

        mainwindow.addDockWidget(Qt::LeftDockWidgetArea, &dockwidget1);
        mainwindow.addDockWidget(Qt::LeftDockWidgetArea, &dockwidget2);
        mainwindow.addDockWidget(Qt::LeftDockWidgetArea, &dockwidget3);
        mainwindow.addDockWidget(Qt::LeftDockWidgetArea, &dockwidget4);


        QMenu *menu = mainwindow.createPopupMenu();
        QVERIFY(menu != 0);
        QList<QAction *> actions = menu->actions();
        QCOMPARE(actions.size(), 7);

        QCOMPARE(actions.at(0), dockwidget1.toggleViewAction());
        QCOMPARE(actions.at(1), dockwidget2.toggleViewAction());
        QCOMPARE(actions.at(2), dockwidget3.toggleViewAction());
        QCOMPARE(actions.at(3), dockwidget4.toggleViewAction());
        QVERIFY(actions.at(4)->isSeparator());
        QCOMPARE(actions.at(5), toolbar1.toggleViewAction());
        QCOMPARE(actions.at(6), toolbar2.toggleViewAction());

        delete menu;

        mainwindow.removeToolBar(&toolbar1);
        mainwindow.removeDockWidget(&dockwidget1);
        mainwindow.removeDockWidget(&dockwidget4);

        menu = mainwindow.createPopupMenu();
        QVERIFY(menu != 0);
        actions = menu->actions();
        QCOMPARE(actions.size(), 4);

        QCOMPARE(actions.at(0), dockwidget2.toggleViewAction());
        QCOMPARE(actions.at(1), dockwidget3.toggleViewAction());
        QVERIFY(actions.at(2)->isSeparator());
        QCOMPARE(actions.at(3), toolbar2.toggleViewAction());

        delete menu;
    }
}

class MyDockWidget : public QDockWidget
{
public:
    MyDockWidget(QWidget * = 0) {
        create(); // otherwise hide() doesn't result in a hide event
    }
};

class MyWidget : public QWidget
{
public:
    MyWidget(QWidget *parent = 0) : QWidget(parent)
    {
    }

    QSize sizeHint() const
    {
        return QSize(200, 200);
    }
};

void tst_QMainWindow::hideBeforeLayout()
{
    QMainWindow win;
    QDockWidget *dock = new MyDockWidget(&win);
    dock->setWidget(new QLabel("hello"));
    win.addDockWidget(Qt::LeftDockWidgetArea, dock);
    dock->hide();
    win.resize(300, 300);
    win.show();
    dock->show();
    QVERIFY(dock->geometry().bottomRight().x() >= 0);
}

struct AddDockWidget
{
    enum Mode { AddMode, SplitMode, TabMode };

    AddDockWidget() {}
    AddDockWidget(const QString &_name, Qt::DockWidgetArea _a)
        : name(_name), mode(AddMode), a(_a) {}
    AddDockWidget(const QString &_name, const QString &_other, Qt::Orientation _o)
        : name(_name), mode(SplitMode), o(_o), other(_other) {}
    AddDockWidget(const QString &_name, const QString &_other)
        : name(_name), mode(TabMode), other(_other) {}

    QString name;
    Mode mode;

    Qt::Orientation o;
    Qt::DockWidgetArea a;
    QString other;

    void apply(QMainWindow *mw) const;
};

typedef QList<AddDockWidget> AddList;
Q_DECLARE_METATYPE(AddList)

void AddDockWidget::apply(QMainWindow *mw) const
{
    QDockWidget *dw = new QDockWidget();
    QWidget *w = new QWidget();
    w->setMinimumSize(100, 50);
    dw->setWidget(w);

    dw->setObjectName(name);
    dw->setWindowTitle(name);

    QDockWidget *other = 0;
    if (mode == SplitMode || mode == TabMode) {
        other = mw->findChild<QDockWidget*>(this->other);
        QVERIFY(other != 0);
    }

    switch (mode) {
        case AddMode:
            mw->addDockWidget(a, dw);
            break;
        case SplitMode:
            mw->splitDockWidget(other, dw, o);
            break;
        case TabMode:
            mw->tabifyDockWidget(other, dw);
            break;
    }
}

#ifdef QT_BUILD_INTERNAL
struct MoveSeparator
{
    MoveSeparator() {}
    MoveSeparator(int _delta, const QString &_name)
        : delta(_delta), name(_name) {}
    MoveSeparator(int _delta, int _area)
        : delta(_delta), area(_area) {}

    int delta;
    int area;
    QString name;

    void apply(QMainWindow *mw) const;
};

typedef QList<MoveSeparator> MoveList;
Q_DECLARE_METATYPE(MoveList)

void MoveSeparator::apply(QMainWindow *mw) const
{
    QMainWindowLayout *l = mw->findChild<QMainWindowLayout *>();
    QVERIFY(l);

    QList<int> path;
    if (name.isEmpty()) {
        path << area;
    } else {
        QDockWidget *dw = mw->findChild<QDockWidget*>(name);
        QVERIFY(dw != 0);
        path = l->layoutState.dockAreaLayout.indexOf(dw);
    }
    QVERIFY(!path.isEmpty());

    // For QTBUG-15689:
    // Following commands is used to simulate user move action. A better way should
    // use QMainWindowLayout::startSeparatorMove/separatorMove/endSeparatorMove,
    // but then we will need real start/end position coordinates.
    l->layoutState.dockAreaLayout.fallbackToSizeHints = false;
    l->layoutState.dockAreaLayout.separatorMove(path, QPoint(0, 0), QPoint(delta, delta));
}
#endif

QMap<QString, QRect> dockWidgetGeometries(QMainWindow *mw)
{
    QMap<QString, QRect> result;
    QList<QDockWidget*> dockWidgets = mw->findChildren<QDockWidget*>();
    foreach (QDockWidget *dw, dockWidgets)
        result.insert(dw->objectName(), dw->geometry());
    return result;
}

#define COMPARE_DOCK_WIDGET_GEOS(_oldGeos, _newGeos) \
{ \
    QMap<QString, QRect> __oldGeos = _oldGeos; \
    QMap<QString, QRect> __newGeos = _newGeos; \
    QCOMPARE(__newGeos.keys(), __oldGeos.keys()); \
    QStringList __keys = __newGeos.keys(); \
    foreach (const QString &key, __keys) { \
        QRect __r1 = __oldGeos[key]; \
        QRect __r2 = __newGeos[key]; \
        if (__r1 != __r2) \
            qWarning() << key << __r1 << __r2; \
    } \
    QCOMPARE(__newGeos, __oldGeos); \
}

#ifdef QT_BUILD_INTERNAL
void tst_QMainWindow::saveRestore_data()
{
    QTest::addColumn<AddList >("addList");
    QTest::addColumn<MoveList >("moveList");

    QTest::newRow("1") << (AddList()
                            << AddDockWidget("left", Qt::LeftDockWidgetArea))
                       << (MoveList()
                            << MoveSeparator(100, QInternal::LeftDock));

    QTest::newRow("2") << (AddList()
                            << AddDockWidget("left", Qt::LeftDockWidgetArea)
                            << AddDockWidget("right", Qt::RightDockWidgetArea))
                       << (MoveList()
                            << MoveSeparator(70, QInternal::LeftDock)
                            << MoveSeparator(-40, QInternal::RightDock));
    QTest::newRow("3") << (AddList()
                            << AddDockWidget("left", Qt::LeftDockWidgetArea)
                            << AddDockWidget("right1", Qt::RightDockWidgetArea)
                            << AddDockWidget("right2", Qt::RightDockWidgetArea))
                       << (MoveList()
                            << MoveSeparator(70, QInternal::LeftDock)
                            << MoveSeparator(-40, QInternal::RightDock));

    QTest::newRow("4") << (AddList()
                            << AddDockWidget("left", Qt::LeftDockWidgetArea)
                            << AddDockWidget("right1", Qt::RightDockWidgetArea)
                            << AddDockWidget("right2a", Qt::RightDockWidgetArea)
                            << AddDockWidget("right2b", "right2a", Qt::Horizontal)
                          )
                       << (MoveList()
                          << MoveSeparator(70, QInternal::LeftDock)
                          << MoveSeparator(-40, QInternal::RightDock)
                          << MoveSeparator(-30, "right1")
                          << MoveSeparator(30, "right2a")
                          );
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QMainWindow::saveRestore()
{
    QFETCH(AddList, addList);
    QFETCH(MoveList, moveList);

    QByteArray stateData;
    QMap<QString, QRect> dockWidgetGeos;
    QSize size;

    {
        QMainWindow mainWindow;
        mainWindow.setDockNestingEnabled(true);
        QTextEdit centralWidget("The rain in Spain falls mainly on the plains");
        mainWindow.setCentralWidget(&centralWidget);

        foreach (const AddDockWidget &adw, addList)
            adw.apply(&mainWindow);

        mainWindow.show();

        foreach (const MoveSeparator &ms, moveList)
            ms.apply(&mainWindow);

        dockWidgetGeos = dockWidgetGeometries(&mainWindow);
        size = mainWindow.size();
        stateData = mainWindow.saveState();

        mainWindow.layout()->setGeometry(mainWindow.rect());
        COMPARE_DOCK_WIDGET_GEOS(dockWidgetGeos, dockWidgetGeometries(&mainWindow));
    }

    // restoreState() after show
    {
        QMainWindow mainWindow;
        mainWindow.setDockNestingEnabled(true);
        QTextEdit centralWidget("The rain in Spain falls mainly on the plains");
        mainWindow.setCentralWidget(&centralWidget);

        foreach (const AddDockWidget &adw, addList)
            adw.apply(&mainWindow);

        mainWindow.show();
        mainWindow.restoreState(stateData);

        COMPARE_DOCK_WIDGET_GEOS(dockWidgetGeos, dockWidgetGeometries(&mainWindow));
    }

    // restoreState() before show
    {
        QMainWindow mainWindow;
        mainWindow.setDockNestingEnabled(true);
        QTextEdit centralWidget("The rain in Spain falls mainly on the plains");
        mainWindow.setCentralWidget(&centralWidget);

        foreach (const AddDockWidget &adw, addList)
            adw.apply(&mainWindow);
        mainWindow.resize(size);
        mainWindow.restoreState(stateData);

        mainWindow.show();
        COMPARE_DOCK_WIDGET_GEOS(dockWidgetGeos, dockWidgetGeometries(&mainWindow));
    }
}
#endif

void tst_QMainWindow::isSeparator()
{
    QMainWindow mw;
    QDockWidget *dockw = new QDockWidget();
    mw.addDockWidget(Qt::LeftDockWidgetArea, dockw);
    mw.addDockWidget(Qt::LeftDockWidgetArea, new QDockWidget());
    dockw->resize(10,10);
    mw.show();

    //In case the separator size is 1, we increase it to 3 inside the QMainWindow class
    const int margin = mw.style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent, 0, &mw) == 1 ? 2 : 0;
    QVERIFY(!mw.isSeparator(QPoint(4, dockw->pos().y())));
    QVERIFY(!mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height() - 1 - margin)));
    QVERIFY( mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height())));
    QVERIFY( mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height() + margin)));
    QVERIFY(!mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height() + 15)));

}

class MainWindow : public QMainWindow {
    public:
        using QMainWindow::event;
};

#ifndef QTEST_NO_CURSOR
void tst_QMainWindow::setCursor()
{
    MainWindow mw;
    QCursor cur = Qt::WaitCursor;
    mw.setCursor(cur);
    QCOMPARE(cur.shape(), mw.cursor().shape());
    mw.resize(200,200);
    mw.show();
    QTest::qWait(50);
    QCOMPARE(cur.shape(), mw.cursor().shape());

    QHoverEvent enterE(QEvent::HoverEnter, QPoint(10,10), QPoint());
    mw.event(&enterE);
    QTest::qWait(50);
    QCOMPARE(cur.shape(), mw.cursor().shape());

    QHoverEvent leaveE(QEvent::HoverLeave, QPoint(), QPoint());
    mw.event(&leaveE);
    QTest::qWait(50);
    QCOMPARE(cur.shape(), mw.cursor().shape());
}
#endif

void tst_QMainWindow::addToolbarAfterShow()
{
    //this is for task 243119
    QMainWindow mainWindow;
    mainWindow.show();

    QToolBar toolBar;
    mainWindow.addToolBar(&toolBar);

    QTRY_VERIFY(!toolBar.isHidden());
}

void tst_QMainWindow::centralWidgetSize()
{
    if (qGuiApp->styleHints()->showIsFullScreen())
        QSKIP("The platform is auto maximizing, so the test makes no sense");;

    QMainWindow mainWindow;
    mainWindow.menuBar()->addMenu("menu");

    MyWidget widget;
    mainWindow.setCentralWidget(&widget);

    mainWindow.show();
    QTRY_COMPARE(widget.size(), widget.sizeHint());
}

void tst_QMainWindow::dockWidgetSize()
{
    QMainWindow mainWindow;
    mainWindow.menuBar()->addMenu("menu");

    MyWidget widget;
    mainWindow.setCentralWidget(&widget);

    QDockWidget dock;
    dock.setWidget(new MyWidget);
    mainWindow.addDockWidget(Qt::TopDockWidgetArea, &dock);

    mainWindow.show();
    QTest::qWait(100);
    if (mainWindow.size() == mainWindow.sizeHint()) {
        QCOMPARE(widget.size(), widget.sizeHint());
        QCOMPARE(dock.widget()->size(), dock.widget()->sizeHint());
    } else {
        //otherwise the screen is too small and the size are irrelevant
    }
}

void tst_QMainWindow::QTBUG2774_stylechange()
{

    QMainWindow mw;
    QDockWidget *dockw = new QDockWidget();
    mw.addDockWidget(Qt::LeftDockWidgetArea, dockw);
    mw.addDockWidget(Qt::LeftDockWidgetArea, new QDockWidget());
    QTextEdit *central = new QTextEdit(&mw);
    mw.setCentralWidget(central);
    dockw->resize(10,10);
    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    int centralOriginalWidth = central->width();

    QVERIFY(!mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height() - 3)));
    QVERIFY( mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height())));
    QVERIFY(!mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height() + 30)));


    {
        mw.setStyleSheet("QMainWindow::separator {  width: 50px; height:50px; }");
        QTRY_VERIFY(central->width() < centralOriginalWidth);
        QVERIFY( mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height())));
        QVERIFY( mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height() + 49)));
    }

    {
        mw.setStyleSheet("QMainWindow::separator {  width: 0px; height: 0px; }");
        QTRY_VERIFY(central->width() > centralOriginalWidth);
        QVERIFY(!mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height())));
        QVERIFY(!mw.isSeparator(QPoint(4, dockw->pos().y() + dockw->size().height() + 1)));
    }
}

void tst_QMainWindow::QTBUG15080_restoreState()
{
    QByteArray state;

    //save state
    {
        QMainWindow mw1;
        QDockWidget * dw1 = new  QDockWidget();
        dw1->setObjectName("Left DockWidget");
        mw1.addDockWidget(Qt::LeftDockWidgetArea, dw1);
        mw1.setCentralWidget(new QTextEdit());
        mw1.show();
        QApplication::processEvents();
        dw1->setFixedWidth(101);
        QApplication::processEvents();

        state = mw1.saveState();
    }

    //restore state

    QMainWindow mw2;
    QDockWidget * dw2 = new  QDockWidget();
    dw2->setObjectName("Left DockWidget");
    mw2.addDockWidget(Qt::LeftDockWidgetArea, dw2);
    mw2.setCentralWidget(new QTextEdit());
    mw2.restoreState(state);
    //QTBUG15080 caused by setStyleSheet
    mw2.setStyleSheet("color:red");
    mw2.show();
    QApplication::processEvents();

    QCOMPARE(dw2->width(), 101);
}

#ifdef Q_OS_MAC
void tst_QMainWindow::toggleUnifiedTitleAndToolBarOnMac()
{
    QMainWindow mw;
    QToolBar *tb = new QToolBar;
    tb->addAction("Test");
    mw.addToolBar(tb);
    mw.setUnifiedTitleAndToolBarOnMac(true);
    mw.show();
    QRect frameGeometry = mw.frameGeometry();
    mw.setUnifiedTitleAndToolBarOnMac(false);
    QVERIFY(frameGeometry.topLeft() == mw.frameGeometry().topLeft());
    mw.setUnifiedTitleAndToolBarOnMac(true);
    QVERIFY(frameGeometry.topLeft() == mw.frameGeometry().topLeft());
}
#endif

void tst_QMainWindow::QTBUG21378_animationFinished()
{
    QMainWindow w;
    QToolBar *pToolBar = new QToolBar;
    pToolBar->setFloatable(false);
    w.addToolBar(pToolBar);
    w.resize(800, 600);
    w.show();
    CToolBarTimer *tbMoveTimer = new CToolBarTimer(pToolBar);
    CMainWindowTimer *mwClickTimer = new CMainWindowTimer(&w);

    QTest::qWait(5000);
    delete tbMoveTimer;
    delete mwClickTimer;
    QVERIFY(true);
}
QTEST_MAIN(tst_QMainWindow)
#include "tst_qmainwindow.moc"
