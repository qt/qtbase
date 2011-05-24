/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QWSMANAGER_QWS_H
#define QWSMANAGER_QWS_H

#include <QtGui/qpixmap.h>
#include <QtCore/qobject.h>
#include <QtGui/qdecoration_qws.h>
#include <QtGui/qevent.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_QWS_MANAGER

class QAction;
class QPixmap;
class QWidget;
class QPopupMenu;
class QRegion;
class QMouseEvent;
class QWSManagerPrivate;

class Q_GUI_EXPORT QWSManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWSManager)
public:
    explicit QWSManager(QWidget *);
    ~QWSManager();

    static QDecoration *newDefaultDecoration();

    QWidget *widget();
    static QWidget *grabbedMouse();
    void maximize();
    void startMove();
    void startResize();

    QRegion region();
    QRegion &cachedRegion();

protected Q_SLOTS:
    void menuTriggered(QAction *action);

protected:
    void handleMove(QPoint g);

    virtual bool event(QEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void paintEvent(QPaintEvent *);
    bool repaintRegion(int region, QDecoration::DecorationState state);

    void menu(const QPoint &);

private:
    friend class QWidget;
    friend class QETWidget;
    friend class QWidgetPrivate;
    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QWidgetBackingStore;
    friend class QWSWindowSurface;
    friend class QGLDrawable;
};

QT_BEGIN_INCLUDE_NAMESPACE
#include <QtGui/qdecorationdefault_qws.h>
QT_END_INCLUDE_NAMESPACE

#endif // QT_NO_QWS_MANAGER

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWSMANAGER_QWS_H
