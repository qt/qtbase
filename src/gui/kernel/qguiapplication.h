/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGUIAPPLICATION_QPA_H
#define QGUIAPPLICATION_QPA_H

#include <QtCore/qcoreapplication.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/qlocale.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QGuiApplicationPrivate;
class QPlatformNativeInterface;
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
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection)

    Q_PROPERTY(bool quitOnLastWindowClosed  READ quitOnLastWindowClosed WRITE setQuitOnLastWindowClosed)

public:
    QGuiApplication(int &argc, char **argv, int = ApplicationFlags);
    virtual ~QGuiApplication();

    static QWindowList topLevelWindows();
    static QWindow *topLevelAt(const QPoint &pos);

    static QWindow *activeWindow();
    static QScreen *primaryScreen();
    static QList<QScreen *> screens();

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

    static Qt::KeyboardModifiers keyboardModifiers();
    static Qt::MouseButtons mouseButtons();

    static void setLayoutDirection(Qt::LayoutDirection direction);
    static Qt::LayoutDirection layoutDirection();

    static inline bool isRightToLeft() { return layoutDirection() == Qt::RightToLeft; }
    static inline bool isLeftToRight() { return layoutDirection() == Qt::LeftToRight; }

    static QLocale keyboardInputLocale();
    static Qt::LayoutDirection keyboardInputDirection();

    QStyleHints *styleHints() const;

    static QPlatformNativeInterface *platformNativeInterface();

    static void setQuitOnLastWindowClosed(bool quit);
    static bool quitOnLastWindowClosed();

    static int exec();
    bool notify(QObject *, QEvent *);

Q_SIGNALS:
    void fontDatabaseChanged();
    void screenAdded(QScreen *screen);
    void lastWindowClosed();

protected:
    bool event(QEvent *);
    bool compressEvent(QEvent *, QObject *receiver, QPostEventList *);

    QGuiApplication(QGuiApplicationPrivate &p);

private:
    Q_DISABLE_COPY(QGuiApplication)
    Q_DECLARE_PRIVATE(QGuiApplication)

#ifndef QT_NO_GESTURES
    friend class QGestureManager;
#endif
    friend class QFontDatabasePrivate;
    friend class QPlatformIntegration;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGUIAPPLICATION_QPA_H
