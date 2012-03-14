/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLATFORMINTEGRATION_COCOA_H
#define QPLATFORMINTEGRATION_COCOA_H

#include <Cocoa/Cocoa.h>

#include "qcocoaautoreleasepool.h"
#include "qcocoacursor.h"
#include "qcocoadrag.h"

#include <QtCore/QScopedPointer>
#include <QtGui/QPlatformIntegration>

QT_BEGIN_NAMESPACE

class QCocoaScreen : public QPlatformScreen
{
public:
    QCocoaScreen(int screenIndex);
    ~QCocoaScreen();

    QRect geometry() const { return m_geometry; }
    int depth() const { return m_depth; }
    QImage::Format format() const { return m_format; }
    QSizeF physicalSize() const { return m_physicalSize; }
    QPlatformCursor *cursor() const  { return m_cursor; }

public:
    NSScreen *m_screen;
    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QSizeF m_physicalSize;
    QCocoaCursor *m_cursor;
};

class QCocoaIntegration : public QPlatformIntegration
{
public:
    QCocoaIntegration();
    ~QCocoaIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const;
    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *widget) const;

    QAbstractEventDispatcher *guiThreadEventDispatcher() const;
    QPlatformFontDatabase *fontDatabase() const;

    QPlatformNativeInterface *nativeInterface() const;
    QPlatformInputContext *inputContext() const;
    QPlatformAccessibility *accessibility() const;
    QPlatformDrag *drag() const;

    QStringList themeNames() const;
    QPlatformTheme *createPlatformTheme(const QString &name) const;

private:

    QScopedPointer<QPlatformFontDatabase> mFontDb;
    QAbstractEventDispatcher *mEventDispatcher;

    QScopedPointer<QPlatformInputContext> mInputContext;
    QScopedPointer<QPlatformAccessibility> mAccessibility;
    QScopedPointer<QPlatformTheme> mPlatformTheme;
    QList<QCocoaScreen *> mScreens;
    QScopedPointer<QCocoaDrag> mCocoaDrag;
};

QT_END_NAMESPACE

#endif

