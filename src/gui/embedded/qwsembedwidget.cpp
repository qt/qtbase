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

#include "qwsembedwidget.h"

#ifndef QT_NO_QWSEMBEDWIDGET

#include <qwsdisplay_qws.h>
#include <private/qwidget_p.h>
#include <private/qwsdisplay_qws_p.h>
#include <private/qwscommand_qws_p.h>

QT_BEGIN_NAMESPACE

// TODO:
// Must remove window decorations from the embedded window
// Focus In/Out, Keyboard/Mouse...
//
// BUG: what if my parent change parent?

class QWSEmbedWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QWSEmbedWidget);

public:
    QWSEmbedWidgetPrivate(int winId);
    void updateWindow();
    void resize(const QSize &size);

    QWidget *window;
    WId windowId;
    WId embeddedId;
};

QWSEmbedWidgetPrivate::QWSEmbedWidgetPrivate(int winId)
    : window(0), windowId(0), embeddedId(winId)
{
}

void QWSEmbedWidgetPrivate::updateWindow()
{
    Q_Q(QWSEmbedWidget);

    QWidget *win = q->window();
    if (win == window)
        return;

    if (window) {
        window->removeEventFilter(q);
        QWSEmbedCommand command;
        command.setData(windowId, embeddedId, QWSEmbedEvent::StopEmbed);
        QWSDisplay::instance()->d->sendCommand(command);
    }

    window = win;
    if (!window)
        return;
    windowId = window->winId();

    QWSEmbedCommand command;
    command.setData(windowId, embeddedId, QWSEmbedEvent::StartEmbed);
    QWSDisplay::instance()->d->sendCommand(command);
    window->installEventFilter(q);
    q->installEventFilter(q);
}

void QWSEmbedWidgetPrivate::resize(const QSize &size)
{
    if (!window)
        return;

    Q_Q(QWSEmbedWidget);

    QWSEmbedCommand command;
    command.setData(windowId, embeddedId, QWSEmbedEvent::Region,
                    QRect(q->mapToGlobal(QPoint(0, 0)), size));
    QWSDisplay::instance()->d->sendCommand(command);
}

/*!
    \class QWSEmbedWidget
    \since 4.2
    \ingroup qws
    \ingroup advanced

    \brief The QWSEmbedWidget class enables embedded top-level widgets
    in Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    QWSEmbedWidget inherits QWidget and acts as any other widget, but
    in addition it is capable of embedding another top-level widget.

    An example of use is when painting directly onto the screen using
    the QDirectPainter class. Then the reserved region can be embedded
    into an instance of the QWSEmbedWidget class, providing for
    example event handling and size policies for the reserved region.

    All that is required to embed a top-level widget is its window ID.

    \sa {Qt for Embedded Linux Architecture}
*/

/*!
    Constructs a widget with the given \a parent, embedding the widget
    identified by the given window \a id.
*/
QWSEmbedWidget::QWSEmbedWidget(WId id, QWidget *parent)
    : QWidget(*new QWSEmbedWidgetPrivate(id), parent, 0)
{
    Q_D(QWSEmbedWidget);
    d->updateWindow();
}

/*!
    Destroys this widget.
*/
QWSEmbedWidget::~QWSEmbedWidget()
{
    Q_D(QWSEmbedWidget);
    if (!d->window)
        return;

    QWSEmbedCommand command;
    command.setData(d->windowId, d->embeddedId, QWSEmbedEvent::StopEmbed);
    QWSDisplay::instance()->d->sendCommand(command);
}

/*!
    \reimp
*/
bool QWSEmbedWidget::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QWSEmbedWidget);
    if (object == d->window && event->type() == QEvent::Move)
        resizeEvent(0);
    else if (object == this && event->type() == QEvent::Hide)
        d->resize(QSize());
    return QWidget::eventFilter(object, event);
}

/*!
    \reimp
*/
void QWSEmbedWidget::changeEvent(QEvent *event)
{
    Q_D(QWSEmbedWidget);
    if (event->type() == QEvent::ParentChange)
        d->updateWindow();
}

/*!
    \reimp
*/
void QWSEmbedWidget::resizeEvent(QResizeEvent*)
{
    Q_D(QWSEmbedWidget);
    d->resize(rect().size());
}

/*!
    \reimp
*/
void QWSEmbedWidget::moveEvent(QMoveEvent*)
{
    resizeEvent(0);
}

/*!
    \reimp
*/
void QWSEmbedWidget::hideEvent(QHideEvent*)
{
    Q_D(QWSEmbedWidget);
    d->resize(QSize());
}

/*!
    \reimp
*/
void QWSEmbedWidget::showEvent(QShowEvent*)
{
    Q_D(QWSEmbedWidget);
    d->resize(rect().size());
}

QT_END_NAMESPACE

#endif // QT_NO_QWSEMBEDWIDGET
