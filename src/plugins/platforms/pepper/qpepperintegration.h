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
