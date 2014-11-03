/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
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
