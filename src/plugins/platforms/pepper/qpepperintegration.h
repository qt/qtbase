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

#ifndef QPEPPERINTEGRATION_H
#define QPEPPERINTEGRATION_H

#include "qpeppereventdispatcher.h"
#include "qpepperglcontext.h"
#include "qpepperinstance_p.h"
#include "qpepperscreen.h"
#include "qpepperwindow.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

class QPlatformFontDatabase;
class QPepperFontDatabase;
class QPlatformClipboard;
class QPepperClipboard;
class QPepperCompositor;
class QPepperWindow;
class QPepperServices;
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
    QAbstractEventDispatcher *createEventDispatcher() const;

    QPlatformFontDatabase *fontDatabase() const;
    QPlatformClipboard *clipboard() const;

    QStringList themeNames() const;
    QPlatformTheme *createPlatformTheme(const QString &name) const;

    QPlatformServices *services() const;

    QVariant styleHint(StyleHint hint) const;
    Qt::WindowState defaultWindowState(Qt::WindowFlags) const;

    QPepperCompositor *pepperCompositor() const;

    PepperEventTranslator *pepperEventTranslator();
    void processEvents();

    bool wantsOpenGLGraphics() const;
    void resizeScreen(QSize size, qreal devicePixelRatio);

private Q_SLOTS:
    void getWindowAt(const QPoint &point, QWindow **window);
    void getKeyWindow(QWindow **window);
    void handleMessage(const QByteArray &tag, const QString &message);

public:
    QPepperScreen *m_screen;
    QPepperCompositor *m_compositor;
    PepperEventTranslator *m_eventTranslator;
    mutable QPepperEventDispatcher *m_pepperEventDispatcher;
    mutable QPepperWindow *m_topLevelWindow;
    mutable QPepperFontDatabase *m_fontDatabase;
    mutable QPepperClipboard *m_clipboard;
    mutable QPepperServices *m_services;
};

QT_END_NAMESPACE

#endif
