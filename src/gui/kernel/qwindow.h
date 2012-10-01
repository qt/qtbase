/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QWINDOW_H
#define QWINDOW_H

#include <QtCore/QObject>
#include <QtCore/QEvent>
#include <QtCore/QMargins>
#include <QtCore/QRect>

#include <QtCore/qnamespace.h>

#include <QtGui/qsurface.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/qwindowdefs.h>

#include <QtGui/qicon.h>

#ifndef QT_NO_CURSOR
#include <QtGui/qcursor.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QWindowPrivate;

class QExposeEvent;
class QFocusEvent;
class QMoveEvent;
class QResizeEvent;
class QShowEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
#ifndef QT_NO_WHEELEVENT
class QWheelEvent;
#endif
class QTouchEvent;
#ifndef QT_NO_TABLETEVENT
class QTabletEvent;
#endif

class QPlatformSurface;
class QPlatformWindow;
class QBackingStore;
class QScreen;
class QAccessibleInterface;

class Q_GUI_EXPORT QWindow : public QObject, public QSurface
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWindow)

    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle)
    Q_PROPERTY(QString windowFilePath READ windowFilePath WRITE setWindowFilePath)
    Q_PROPERTY(QIcon windowIcon READ windowIcon WRITE setWindowIcon)
    Q_PROPERTY(Qt::WindowModality windowModality READ windowModality WRITE setWindowModality NOTIFY windowModalityChanged)
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(QPoint pos READ pos WRITE setPos)
    Q_PROPERTY(QSize size READ size WRITE resize)
    Q_PROPERTY(QRect geometry READ geometry WRITE setGeometry)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(Qt::ScreenOrientation contentOrientation READ contentOrientation WRITE reportContentOrientationChange NOTIFY contentOrientationChanged)
#ifndef QT_NO_CURSOR
    Q_PROPERTY(QCursor cursor READ cursor WRITE setCursor RESET unsetCursor)
#endif

public:

    explicit QWindow(QScreen *screen = 0);
    explicit QWindow(QWindow *parent);
    virtual ~QWindow();

    void setSurfaceType(SurfaceType surfaceType);
    SurfaceType surfaceType() const;

    bool isVisible() const;

    void create();

    WId winId() const;

    QWindow *parent() const;
    void setParent(QWindow *parent);

    bool isTopLevel() const;

    bool isModal() const;
    Qt::WindowModality windowModality() const;
    void setWindowModality(Qt::WindowModality windowModality);

    void setFormat(const QSurfaceFormat &format);
    QSurfaceFormat format() const;
    QSurfaceFormat requestedFormat() const;

    void setWindowFlags(Qt::WindowFlags flags);
    Qt::WindowFlags windowFlags() const;
    Qt::WindowType windowType() const;

    QString windowTitle() const;

    void setOpacity(qreal level);
    void requestActivateWindow();

    bool isActive() const;

    void reportContentOrientationChange(Qt::ScreenOrientation orientation);
    Qt::ScreenOrientation contentOrientation() const;

    bool requestWindowOrientation(Qt::ScreenOrientation orientation);
    Qt::ScreenOrientation windowOrientation() const;

    Qt::WindowState windowState() const;
    void setWindowState(Qt::WindowState state);

    void setTransientParent(QWindow *parent);
    QWindow *transientParent() const;

    enum AncestorMode {
        ExcludeTransients,
        IncludeTransients
    };

    bool isAncestorOf(const QWindow *child, AncestorMode mode = IncludeTransients) const;

    bool isExposed() const;

    QSize minimumSize() const;
    QSize maximumSize() const;
    QSize baseSize() const;
    QSize sizeIncrement() const;

    void setMinimumSize(const QSize &size);
    void setMaximumSize(const QSize &size);
    void setBaseSize(const QSize &size);
    void setSizeIncrement(const QSize &size);

    void setGeometry(int posx, int posy, int w, int h) { setGeometry(QRect(posx, posy, w, h)); }
    void setGeometry(const QRect &rect);
    QRect geometry() const;

    QMargins frameMargins() const;
    QRect frameGeometry() const;

    QPoint framePos() const;
    void setFramePos(const QPoint &point);

    inline int width() const { return geometry().width(); }
    inline int height() const { return geometry().height(); }
    inline int x() const { return geometry().x(); }
    inline int y() const { return geometry().y(); }

    inline QSize size() const { return geometry().size(); }
    inline QPoint pos() const { return geometry().topLeft(); }

    inline void setPos(const QPoint &pt) { setGeometry(QRect(pt, size())); }
    inline void setPos(int posx, int posy) { setPos(QPoint(posx, posy)); }

    void resize(const QSize &newSize);
    inline void resize(int w, int h) { resize(QSize(w, h)); }

    void setWindowFilePath(const QString &filePath);
    QString windowFilePath() const;

    void setWindowIcon(const QIcon &icon);
    QIcon windowIcon() const;

    void destroy();

    QPlatformWindow *handle() const;

    bool setKeyboardGrabEnabled(bool grab);
    bool setMouseGrabEnabled(bool grab);

    QScreen *screen() const;
    void setScreen(QScreen *screen);

    virtual QAccessibleInterface *accessibleRoot() const;
    virtual QObject *focusObject() const;

    QPoint mapToGlobal(const QPoint &pos) const;
    QPoint mapFromGlobal(const QPoint &pos) const;

#ifndef QT_NO_CURSOR
    QCursor cursor() const;
    void setCursor(const QCursor &);
    void unsetCursor();
#endif

public Q_SLOTS:
    void setVisible(bool visible);

    void show();
    void hide();

    void showMinimized();
    void showMaximized();
    void showFullScreen();
    void showNormal();

    bool close();
    void raise();
    void lower();

    void setWindowTitle(const QString &);

    void setX(int arg)
    {
        if (x() != arg)
            setGeometry(QRect(arg, y(), width(), height()));
    }

    void setY(int arg)
    {
        if (y() != arg)
            setGeometry(QRect(x(), arg, width(), height()));
    }

    void setWidth(int arg)
    {
        if (width() != arg)
            setGeometry(QRect(x(), y(), arg, height()));
    }

    void setHeight(int arg)
    {
        if (height() != arg)
            setGeometry(QRect(x(), y(), width(), arg));
    }

Q_SIGNALS:
    void screenChanged(QScreen *screen);
    void windowModalityChanged(Qt::WindowModality windowModality);

    void xChanged(int arg);
    void yChanged(int arg);

    void widthChanged(int arg);
    void heightChanged(int arg);

    void visibleChanged(bool arg);
    void contentOrientationChanged(Qt::ScreenOrientation orientation);

    void focusObjectChanged(QObject *object);

private Q_SLOTS:
    void screenDestroyed(QObject *screen);

protected:
    virtual void exposeEvent(QExposeEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void moveEvent(QMoveEvent *);
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);

    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);

    virtual bool event(QEvent *);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
#ifndef QT_NO_WHEELEVENT
    virtual void wheelEvent(QWheelEvent *);
#endif
    virtual void touchEvent(QTouchEvent *);
#ifndef QT_NO_TABLETEVENT
    virtual void tabletEvent(QTabletEvent *);
#endif
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result);

    QWindow(QWindowPrivate &dd, QWindow *parent);

private:
    QPlatformSurface *surfaceHandle() const;

    Q_DISABLE_COPY(QWindow)

    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend Q_GUI_EXPORT QWindowPrivate *qt_window_private(QWindow *window);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWINDOW_H
