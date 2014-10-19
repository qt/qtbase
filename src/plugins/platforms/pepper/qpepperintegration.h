/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLATFORMINTEGRATION_PEPPER_H
#define QPLATFORMINTEGRATION_PEPPER_H

#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>

#include "qpepperscreen.h"
#include "qpepperplatformwindow.h"
#include "qpepperglcontext.h"
#include "qpepperinstance.h"
#include "qpeppereventdispatcher.h"

QT_BEGIN_NAMESPACE

class QPlatformFontDatabase; 
class QPepperFontDatabase;
class QPepperCompositor;
class QPepperJavascriptBridge;
class QPepperPlatformWindow;
class QAbstractEventDispatcher;
class QPepperIntegration : public QObject, public QPlatformIntegration
{
    Q_OBJECT
public:
    static QPepperIntegration *createPepperIntegration();
    static QPepperIntegration *getPepperIntegration();
    QPepperIntegration();
    ~QPepperIntegration();
    virtual bool hasOpenGL() const;

    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
    QAbstractEventDispatcher* createEventDispatcher() const;

    QPlatformFontDatabase *fontDatabase() const;

    QStringList themeNames() const;
    QPlatformTheme *createPlatformTheme(const QString &name) const;

    QVariant styleHint(StyleHint hint) const;
    Qt::WindowState defaultWindowState(Qt::WindowFlags) const;

    void setPepperInstance(QPepperInstance *instance);
    QPepperInstance *pepperInstance() const;
    QPepperCompositor *pepperCompositor() const;

    PepperEventTranslator *pepperEventTranslator();
    void processEvents();

    bool wantsOpenGLGraphics() const;
    void resizeScreen(QSize size, qreal devicePixelRatio);

private Q_SLOTS:
    void getWindowAt(const QPoint & point, QWindow **window);
    void getKeyWindow(QWindow **window);
    void handleMessage(const QByteArray &tag, const QString &message);
public:
    QPepperScreen *m_screen;
    QPepperInstance *m_pepperInstance;
    QPepperCompositor *m_compositor;
    PepperEventTranslator *m_eventTranslator;
    mutable QPepperEventDispatcher *m_pepperEventDispatcher;
    QPepperJavascriptBridge *m_javascriptBridge;

    mutable QPepperPlatformWindow *m_topLevelWindow;
    mutable QPepperFontDatabase *m_fontDatabase;

};

QT_END_NAMESPACE

#endif
