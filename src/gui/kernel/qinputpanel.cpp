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

#include <qinputpanel.h>
#include <private/qinputpanel_p.h>
#include <qplatforminputcontext_qpa.h>
#include <private/qguiapplication_p.h>

QInputPanel::QInputPanel()
    : QObject(*new QInputPanelPrivate)
{
}

QInputPanel::~QInputPanel()
{
}


QObject *QInputPanel::inputItem() const
{
    Q_D(const QInputPanel);
    return d->inputItem.data();
}

void QInputPanel::setInputItem(QObject *inputItem)
{
    Q_D(QInputPanel);

    if (d->inputItem.data() == inputItem)
        return;

    d->inputItem = inputItem;
    emit inputItemChanged();
}

QWindow *QInputPanel::inputWindow() const
{
    return qApp->activeWindow();
}

QTransform QInputPanel::inputItemTransform() const
{
    Q_D(const QInputPanel);
    return d->inputItemTransform;
}

void QInputPanel::setInputItemTransform(const QTransform &transform)
{
    Q_D(QInputPanel);
    if (d->inputItemTransform == transform)
        return;

    d->inputItemTransform = transform;
    emit cursorRectangleChanged();
}

QRectF QInputPanel::cursorRectangle() const
{
    Q_D(const QInputPanel);

    if (!d->inputItem)
        return QRectF();

    QInputMethodQueryEvent query(Qt::ImCursorRectangle);
    QGuiApplication::sendEvent(d->inputItem.data(), &query);
    QRect r = query.value(Qt::ImCursorRectangle).toRect();
    if (!r.isValid())
        return QRect();

    return d->inputItemTransform.mapRect(r);
}

QRectF QInputPanel::keyboardRectangle()
{
    QPlatformInputContext *ic = QGuiApplicationPrivate::platformIntegration()->inputContext();
    if (ic)
        return ic->keyboardRect();
    return QRectF();
}

void QInputPanel::show()
{
    setVisible(true);
}

void QInputPanel::hide()
{
    setVisible(false);
}

bool QInputPanel::visible() const
{
    Q_D(const QInputPanel);

    return d->visible;
}

void QInputPanel::setVisible(bool visible)
{
    Q_D(QInputPanel);
    if (d->visible == visible)
        return;

    d->visible = visible;
    emit visibleChanged();
}

bool QInputPanel::isAnimating() const
{
    QPlatformInputContext *ic = QGuiApplicationPrivate::platformIntegration()->inputContext();
    if (ic)
        return ic->isAnimating();
    return false;
}


void QInputPanel::update(Qt::InputMethodQueries queries)
{
    Q_D(QInputPanel);

    if (!d->inputItem)
        return;

    QPlatformInputContext *ic = QGuiApplicationPrivate::platformIntegration()->inputContext();
    if (ic)
        ic->update(queries);

    if (queries & Qt::ImCursorRectangle)
        emit cursorRectangleChanged();
}

void QInputPanel::reset()
{
    QPlatformInputContext *ic = QGuiApplicationPrivate::platformIntegration()->inputContext();
    if (ic)
        ic->reset();
}

void QInputPanel::commit()
{
    QPlatformInputContext *ic = QGuiApplicationPrivate::platformIntegration()->inputContext();
    if (ic)
        ic->commit();
}

void QInputPanel::invokeAction(Action a, int cursorPosition)
{
    QPlatformInputContext *ic = QGuiApplicationPrivate::platformIntegration()->inputContext();
    if (ic)
        ic->invokeAction(a, cursorPosition);
}

#include "moc_qinputpanel.cpp"
