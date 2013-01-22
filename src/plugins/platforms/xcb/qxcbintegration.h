/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QXCBINTEGRATION_H
#define QXCBINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class QXcbConnection;
class QAbstractEventDispatcher;
class QXcbNativeInterface;
class QXcbScreen;

#if !defined(QT_NO_OPENGL) && defined(XCB_USE_GLX)
class QOpenGLDefaultContextInfo;
#endif

class QXcbIntegration : public QPlatformIntegration
{
public:
    QXcbIntegration(const QStringList &parameters);
    ~QXcbIntegration();

    QPlatformWindow *createPlatformWindow(QWindow *window) const;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
#endif
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;

    bool hasCapability(Capability cap) const;
    QAbstractEventDispatcher *guiThreadEventDispatcher() const;

    void moveToScreen(QWindow *window, int screen);

    QPlatformFontDatabase *fontDatabase() const;

    QPlatformNativeInterface *nativeInterface()const;

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *clipboard() const;
#endif
#ifndef QT_NO_DRAGANDDROP
    QPlatformDrag *drag() const;
#endif

    QPlatformInputContext *inputContext() const;

#ifndef QT_NO_ACCESSIBILITY
    QPlatformAccessibility *accessibility() const;
#endif

    QPlatformServices *services() const;

    Qt::KeyboardModifiers queryKeyboardModifiers() const;

    QStringList themeNames() const;
    QPlatformTheme *createPlatformTheme(const QString &name) const;

    void removeDefaultOpenGLContextInfo(QXcbScreen *screen);

private:
    QList<QXcbConnection *> m_connections;

    QScopedPointer<QPlatformFontDatabase> m_fontDatabase;
    QScopedPointer<QXcbNativeInterface> m_nativeInterface;

    QScopedPointer<QPlatformInputContext> m_inputContext;
    QAbstractEventDispatcher *m_eventDispatcher;

#if !defined(QT_NO_OPENGL) && defined(XCB_USE_GLX)
    mutable QHash<QXcbScreen *, QOpenGLDefaultContextInfo *> m_defaultContextInfos;
#endif

#ifndef QT_NO_ACCESSIBILITY
    QScopedPointer<QPlatformAccessibility> m_accessibility;
#endif

    QScopedPointer<QPlatformServices> m_services;

    friend class QXcbConnection; // access QPlatformIntegration::screenAdded()
};

QT_END_NAMESPACE

#endif
