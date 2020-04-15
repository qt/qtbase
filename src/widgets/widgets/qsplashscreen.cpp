/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsplashscreen.h"

#include "qapplication.h"
#include "qdesktopwidget.h"
#include <private/qdesktopwidget_p.h>
#include "qpainter.h"
#include "qpixmap.h"
#include "qtextdocument.h"
#include "qtextcursor.h"
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>
#include <private/qwidget_p.h>

#ifdef Q_OS_WIN
#  include <QtCore/qt_windows.h>
#else
#  include <time.h>
#endif

QT_BEGIN_NAMESPACE

class QSplashScreenPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QSplashScreen)
public:
    QPixmap pixmap;
    QString currStatus;
    QColor currColor;
    int currAlign;

    inline QSplashScreenPrivate();

    void setPixmap(const QPixmap &p, const QScreen *screen = nullptr);

    static const QScreen *screenFor(const QWidget *w);
};

/*!
   \class QSplashScreen
   \brief The QSplashScreen widget provides a splash screen that can
   be shown during application startup.

   \inmodule QtWidgets

   A splash screen is a widget that is usually displayed when an
   application is being started. Splash screens are often used for
   applications that have long start up times (e.g. database or
   networking applications that take time to establish connections) to
   provide the user with feedback that the application is loading.

   The splash screen appears in the center of the screen. It may be
   useful to add the Qt::WindowStaysOnTopHint to the splash widget's
   window flags if you want to keep it above all the other windows on
   the desktop.

   Some X11 window managers do not support the "stays on top" flag. A
   solution is to set up a timer that periodically calls raise() on
   the splash screen to simulate the "stays on top" effect.

   The most common usage is to show a splash screen before the main
   widget is displayed on the screen. This is illustrated in the
   following code snippet in which a splash screen is displayed and
   some initialization tasks are performed before the application's
   main window is shown:

   \snippet qsplashscreen/main.cpp 0
   \dots
   \snippet qsplashscreen/main.cpp 1

   The user can hide the splash screen by clicking on it with the
   mouse. Since the splash screen is typically displayed before the
   event loop has started running, it is necessary to periodically
   call QCoreApplication::processEvents() to receive the mouse clicks.

   It is sometimes useful to update the splash screen with messages,
   for example, announcing connections established or modules loaded
   as the application starts up:

   \snippet code/src_gui_widgets_qsplashscreen.cpp 0

   QSplashScreen supports this with the showMessage() function. If you
   wish to do your own drawing you can get a pointer to the pixmap
   used in the splash screen with pixmap().  Alternatively, you can
   subclass QSplashScreen and reimplement drawContents().

   In case of having multiple screens, it is also possible to show the
   splash screen on a different screen than the primary one. For example:

   \snippet qsplashscreen/main.cpp 2
*/

/*!
    Construct a splash screen that will display the \a pixmap.

    There should be no need to set the widget flags, \a f, except
    perhaps Qt::WindowStaysOnTopHint.
*/
QSplashScreen::QSplashScreen(const QPixmap &pixmap, Qt::WindowFlags f)
    : QWidget(*(new QSplashScreenPrivate()), nullptr, Qt::SplashScreen | Qt::FramelessWindowHint | f)
{
    setPixmap(pixmap);  // Does an implicit repaint
}

/*!
    \overload
    \since 5.15

    This function allows you to specify the screen for your splashscreen. The
    typical use for this constructor is if you have multiple screens and
    prefer to have the splash screen on a different screen than your primary
    one. In that case pass the proper \a screen.
*/
QSplashScreen::QSplashScreen(QScreen *screen, const QPixmap &pixmap, Qt::WindowFlags f)
    : QWidget(*(new QSplashScreenPrivate()), nullptr, Qt::SplashScreen | Qt::FramelessWindowHint | f)
{
    d_func()->setPixmap(pixmap, screen);
}

#if QT_DEPRECATED_SINCE(5, 15)
/*!
    \overload
    \obsolete Use the constructor taking a \c {QScreen *} instead

    This function allows you to specify a parent for your splashscreen. The
    typical use for this constructor is if you have a multiple screens and
    prefer to have the splash screen on a different screen than your primary
    one. In that case pass the proper desktop() as the \a parent.
*/
QSplashScreen::QSplashScreen(QWidget *parent, const QPixmap &pixmap, Qt::WindowFlags f)
    : QWidget(*new QSplashScreenPrivate, parent, Qt::SplashScreen | Qt::FramelessWindowHint | f)
{
    // Does an implicit repaint. Explicitly pass parent as QObject::parent()
    // is still 0 here due to QWidget's special handling.
    d_func()->setPixmap(pixmap, QSplashScreenPrivate::screenFor(parent));
}
#endif

/*!
  Destructor.
*/
QSplashScreen::~QSplashScreen()
{
}

/*!
    \reimp
*/
void QSplashScreen::mousePressEvent(QMouseEvent *)
{
    hide();
}

/*!
    This overrides QWidget::repaint(). It differs from the standard repaint
    function in that it also calls QCoreApplication::processEvents() to ensure
    the updates are displayed, even when there is no event loop present.
*/
void QSplashScreen::repaint()
{
    QWidget::repaint();
    QCoreApplication::processEvents();
}

/*!
    \fn QSplashScreen::messageChanged(const QString &message)

    This signal is emitted when the message on the splash screen
    changes. \a message is the new message and is a null-string
    when the message has been removed.

    \sa showMessage(), clearMessage()
*/



/*!
    Draws the \a message text onto the splash screen with color \a
    color and aligns the text according to the flags in \a alignment.
    This function calls repaint() to make sure the splash screen is
    repainted immediately. As a result the message is kept up
    to date with what your application is doing (e.g. loading files).

    \sa Qt::Alignment, clearMessage(), message()
*/
void QSplashScreen::showMessage(const QString &message, int alignment,
                                const QColor &color)
{
    Q_D(QSplashScreen);
    d->currStatus = message;
    d->currAlign = alignment;
    d->currColor = color;
    emit messageChanged(d->currStatus);
    repaint();
}

/*!
    \since 5.2

    Returns the message that is currently displayed on the splash screen.

    \sa showMessage(), clearMessage()
*/

QString QSplashScreen::message() const
{
    Q_D(const QSplashScreen);
    return d->currStatus;
}

/*!
    Removes the message being displayed on the splash screen

    \sa showMessage()
 */
void QSplashScreen::clearMessage()
{
    d_func()->currStatus.clear();
    emit messageChanged(d_func()->currStatus);
    repaint();
}

// A copy of Qt Test's qWaitForWindowExposed() and qSleep().
inline static bool waitForWindowExposed(QWindow *window, int timeout = 1000)
{
    enum { TimeOutMs = 10 };
    QElapsedTimer timer;
    timer.start();
    while (!window->isExposed()) {
        const int remaining = timeout - int(timer.elapsed());
        if (remaining <= 0)
            break;
        QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
#if defined(Q_OS_WINRT)
        WaitForSingleObjectEx(GetCurrentThread(), TimeOutMs, false);
#elif defined(Q_OS_WIN)
        Sleep(uint(TimeOutMs));
#else
        struct timespec ts = { TimeOutMs / 1000, (TimeOutMs % 1000) * 1000 * 1000 };
        nanosleep(&ts, nullptr);
#endif
    }
    return window->isExposed();
}

/*!
    Makes the splash screen wait until the widget \a mainWin is displayed
    before calling close() on itself.
*/

void QSplashScreen::finish(QWidget *mainWin)
{
    if (mainWin) {
        if (!mainWin->windowHandle())
            mainWin->createWinId();
        waitForWindowExposed(mainWin->windowHandle());
    }
    close();
}

/*!
    Sets the pixmap that will be used as the splash screen's image to
    \a pixmap.
*/
void QSplashScreen::setPixmap(const QPixmap &pixmap)
{
    d_func()->setPixmap(pixmap, QSplashScreenPrivate::screenFor(this));
}

// In setPixmap(), resize and try to position on a screen according to:
// 1) If the screen for the given widget is available, use that
// 2) If a QDesktopScreenWidget is found in the parent hierarchy, use that (see docs on
//    QSplashScreen(QWidget *, QPixmap).
// 3) If a widget with associated QWindow is found, use that
// 4) When nothing can be found, try to center it over the cursor

#if QT_DEPRECATED_SINCE(5, 15)
static inline int screenNumberOf(const QDesktopScreenWidget *dsw)
{
    auto desktopWidgetPrivate =
        static_cast<QDesktopWidgetPrivate *>(qt_widget_private(QApplication::desktop()));
    return desktopWidgetPrivate->screens.indexOf(const_cast<QDesktopScreenWidget *>(dsw));
}
#endif

const QScreen *QSplashScreenPrivate::screenFor(const QWidget *w)
{
    if (w && w->screen())
        return w->screen();

    for (const QWidget *p = w; p !=nullptr ; p = p->parentWidget()) {
#if QT_DEPRECATED_SINCE(5, 15)
        if (auto dsw = qobject_cast<const QDesktopScreenWidget *>(p))
            return QGuiApplication::screens().value(screenNumberOf(dsw));
#endif
        if (QWindow *window = p->windowHandle())
            return window->screen();
    }

#if QT_CONFIG(cursor)
    // Note: We could rely on QPlatformWindow::initialGeometry() to center it
    // over the cursor, but not all platforms (namely Android) use that.
    if (QGuiApplication::screens().size() > 1) {
        if (auto screenAtCursor = QGuiApplication::screenAt(QCursor::pos()))
            return screenAtCursor;
    }
#endif // cursor
    return QGuiApplication::primaryScreen();
}

void QSplashScreenPrivate::setPixmap(const QPixmap &p, const QScreen *screen)
{
    Q_Q(QSplashScreen);

    pixmap = p;
    q->setAttribute(Qt::WA_TranslucentBackground, pixmap.hasAlpha());

    QRect r(QPoint(), pixmap.size() / pixmap.devicePixelRatio());
    q->resize(r.size());
    if (screen)
        q->move(screen->geometry().center() - r.center());
    if (q->isVisible())
        q->repaint();
}

/*!
    Returns the pixmap that is used in the splash screen. The image
    does not have any of the text drawn by showMessage() calls.
*/
const QPixmap QSplashScreen::pixmap() const
{
    return d_func()->pixmap;
}

/*!
    \internal
*/
inline QSplashScreenPrivate::QSplashScreenPrivate() : currAlign(Qt::AlignLeft)
{
}

/*!
    Draw the contents of the splash screen using painter \a painter.
    The default implementation draws the message passed by showMessage().
    Reimplement this function if you want to do your own drawing on
    the splash screen.
*/
void QSplashScreen::drawContents(QPainter *painter)
{
    Q_D(QSplashScreen);
    painter->setPen(d->currColor);
    QRect r = rect().adjusted(5, 5, -5, -5);
    if (Qt::mightBeRichText(d->currStatus)) {
        QTextDocument doc;
#ifdef QT_NO_TEXTHTMLPARSER
        doc.setPlainText(d->currStatus);
#else
        doc.setHtml(d->currStatus);
#endif
        doc.setTextWidth(r.width());
        QTextCursor cursor(&doc);
        cursor.select(QTextCursor::Document);
        QTextBlockFormat fmt;
        fmt.setAlignment(Qt::Alignment(d->currAlign));
        fmt.setLayoutDirection(layoutDirection());
        cursor.mergeBlockFormat(fmt);
        const QSizeF txtSize = doc.size();
        if (d->currAlign & Qt::AlignBottom)
            r.setTop(r.height() - txtSize.height());
        else if (d->currAlign & Qt::AlignVCenter)
            r.setTop(r.height() / 2 - txtSize.height() / 2);
        painter->save();
        painter->translate(r.topLeft());
        doc.drawContents(painter);
        painter->restore();
    } else {
        painter->drawText(r, d->currAlign, d->currStatus);
    }
}

/*! \reimp */
bool QSplashScreen::event(QEvent *e)
{
    if (e->type() == QEvent::Paint) {
        Q_D(QSplashScreen);
        QPainter painter(this);
        painter.setLayoutDirection(layoutDirection());
        if (!d->pixmap.isNull())
            painter.drawPixmap(QPoint(), d->pixmap);
        drawContents(&painter);
    }
    return QWidget::event(e);
}

QT_END_NAMESPACE

#include "moc_qsplashscreen.cpp"
