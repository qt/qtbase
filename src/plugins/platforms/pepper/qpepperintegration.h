/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPEPPERINTEGRATION_H
#define QPEPPERINTEGRATION_H

#include <QtCore/QAbstractEventDispatcher>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformfontdatabase.h>

QT_BEGIN_NAMESPACE

class QPepperEventTranslator;
class QPepperClipboard;
class QPepperCompositor;
class QPepperEventDispatcher;
class QPepperFontDatabase;
class QPepperScreen;
class QPepperServices;
class QPepperWindow;
class QPlatformClipboard;

class QPepperIntegration : public QObject, public QPlatformIntegration
{
    Q_OBJECT
public:
    static QPepperIntegration *create();
    static QPepperIntegration *get();

    QPepperIntegration();
    ~QPepperIntegration();

    QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const Q_DECL_OVERRIDE;
    QAbstractEventDispatcher *createEventDispatcher() const Q_DECL_OVERRIDE;
    QPlatformFontDatabase *fontDatabase() const Q_DECL_OVERRIDE;
    QPlatformClipboard *clipboard() const Q_DECL_OVERRIDE;
    QPlatformServices *services() const Q_DECL_OVERRIDE;
    QVariant styleHint(StyleHint hint) const Q_DECL_OVERRIDE;
    Qt::WindowState defaultWindowState(Qt::WindowFlags) const Q_DECL_OVERRIDE;

    QStringList themeNames() const Q_DECL_OVERRIDE;
    QPlatformTheme *createPlatformTheme(const QString &name) const Q_DECL_OVERRIDE;

    QPepperCompositor *pepperCompositor() const;
    QPepperEventTranslator *pepperEventTranslator() const;
    void processEvents();
    void resizeScreen(QSize size, qreal devicePixelRatio);
    QPepperWindow *topLevelWindow() const;

private Q_SLOTS:
    void getWindowAt(const QPoint &point, QWindow **window);
    void getKeyWindow(QWindow **window);

private:
    mutable QPepperClipboard *m_clipboard;
    mutable QPepperEventDispatcher *m_eventDispatcher;
    mutable QPepperFontDatabase *m_fontDatabase;
    mutable QPepperServices *m_services;
    mutable QPepperWindow *m_topLevelWindow;
    QPepperCompositor *m_compositor;
    QPepperEventTranslator *m_eventTranslator;
    QPepperScreen *m_screen;
};

QT_END_NAMESPACE

#endif
