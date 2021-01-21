/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QGUIAPPLICATION_H
#define QGUIAPPLICATION_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qwindowdefs.h>
#include <QtGui/qinputmethod.h>
#include <QtCore/qlocale.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>

QT_BEGIN_NAMESPACE


class QSessionManager;
class QGuiApplicationPrivate;
class QPlatformNativeInterface;
class QPlatformIntegration;
class QPalette;
class QScreen;
class QStyleHints;

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<QGuiApplication *>(QCoreApplication::instance()))

#if defined(qGuiApp)
#undef qGuiApp
#endif
#define qGuiApp (static_cast<QGuiApplication *>(QCoreApplication::instance()))

class Q_GUI_EXPORT QGuiApplication : public QCoreApplication
{
    Q_OBJECT
    Q_PROPERTY(QIcon windowIcon READ windowIcon WRITE setWindowIcon)
    Q_PROPERTY(QString applicationDisplayName READ applicationDisplayName WRITE setApplicationDisplayName NOTIFY applicationDisplayNameChanged)
    Q_PROPERTY(QString desktopFileName READ desktopFileName WRITE setDesktopFileName)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged)
    Q_PROPERTY(QString platformName READ platformName STORED false)
    Q_PROPERTY(bool quitOnLastWindowClosed  READ quitOnLastWindowClosed WRITE setQuitOnLastWindowClosed)
    Q_PROPERTY(QScreen *primaryScreen READ primaryScreen NOTIFY primaryScreenChanged STORED false)

public:
#ifdef Q_QDOC
    QGuiApplication(int &argc, char **argv);
#else
    QGuiApplication(int &argc, char **argv, int = ApplicationFlags);
#endif
    ~QGuiApplication();

    static void setApplicationDisplayName(const QString &name);
    static QString applicationDisplayName();

    static void setDesktopFileName(const QString &name);
    static QString desktopFileName();

    static QWindowList allWindows();
    static QWindowList topLevelWindows();
    static QWindow *topLevelAt(const QPoint &pos);

    static void setWindowIcon(const QIcon &icon);
    static QIcon windowIcon();

    static QString platformName();

    static QWindow *modalWindow();

    static QWindow *focusWindow();
    static QObject *focusObject();

    static QScreen *primaryScreen();
    static QList<QScreen *> screens();
    static QScreen *screenAt(const QPoint &point);

    qreal devicePixelRatio() const;

#ifndef QT_NO_CURSOR
    static QCursor *overrideCursor();
    static void setOverrideCursor(const QCursor &);
    static void changeOverrideCursor(const QCursor &);
    static void restoreOverrideCursor();
#endif

    static QFont font();
    static void setFont(const QFont &);

#ifndef QT_NO_CLIPBOARD
    static QClipboard *clipboard();
#endif

    static QPalette palette();
    static void setPalette(const QPalette &pal);

    static Qt::KeyboardModifiers keyboardModifiers();
    static Qt::KeyboardModifiers queryKeyboardModifiers();
    static Qt::MouseButtons mouseButtons();

    static void setLayoutDirection(Qt::LayoutDirection direction);
    static Qt::LayoutDirection layoutDirection();

    static inline bool isRightToLeft() { return layoutDirection() == Qt::RightToLeft; }
    static inline bool isLeftToRight() { return layoutDirection() == Qt::LeftToRight; }

    static QStyleHints *styleHints();
    static void setDesktopSettingsAware(bool on);
    static bool desktopSettingsAware();

    static QInputMethod *inputMethod();

    static QPlatformNativeInterface *platformNativeInterface();

    static QFunctionPointer platformFunction(const QByteArray &function);

    static void setQuitOnLastWindowClosed(bool quit);
    static bool quitOnLastWindowClosed();

    static Qt::ApplicationState applicationState();

    static void setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy policy);
    static Qt::HighDpiScaleFactorRoundingPolicy highDpiScaleFactorRoundingPolicy();

    static int exec();
    bool notify(QObject *, QEvent *) override;

#ifndef QT_NO_SESSIONMANAGER
    // session management
    bool isSessionRestored() const;
    QString sessionId() const;
    QString sessionKey() const;
    bool isSavingSession() const;

    static bool isFallbackSessionManagementEnabled();
    static void setFallbackSessionManagementEnabled(bool);
#endif

    static void sync();
Q_SIGNALS:
    void fontDatabaseChanged();
    void screenAdded(QScreen *screen);
    void screenRemoved(QScreen *screen);
    void primaryScreenChanged(QScreen *screen);
    void lastWindowClosed();
    void focusObjectChanged(QObject *focusObject);
    void focusWindowChanged(QWindow *focusWindow);
    void applicationStateChanged(Qt::ApplicationState state);
    void layoutDirectionChanged(Qt::LayoutDirection direction);
#ifndef QT_NO_SESSIONMANAGER
    void commitDataRequest(QSessionManager &sessionManager);
    void saveStateRequest(QSessionManager &sessionManager);
#endif
    void paletteChanged(const QPalette &pal);
    void applicationDisplayNameChanged();
    void fontChanged(const QFont &font);

protected:
    bool event(QEvent *) override;
    bool compressEvent(QEvent *, QObject *receiver, QPostEventList *) override;

    QGuiApplication(QGuiApplicationPrivate &p);

private:
    Q_DISABLE_COPY(QGuiApplication)
    Q_DECLARE_PRIVATE(QGuiApplication)

    Q_PRIVATE_SLOT(d_func(), void _q_updateFocusObject(QObject *object))

#ifndef QT_NO_GESTURES
    friend class QGestureManager;
#endif
    friend class QFontDatabasePrivate;
    friend class QPlatformIntegration;
#ifndef QT_NO_SESSIONMANAGER
    friend class QPlatformSessionManager;
#endif
};

QT_END_NAMESPACE

#endif // QGUIAPPLICATION_H
