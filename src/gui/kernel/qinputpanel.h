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

#ifndef QINPUTPANEL_H
#define QINPUTPANEL_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QInputPanelPrivate;
class QWindow;
class QRectF;
class QTransform;

class Q_GUI_EXPORT QInputPanel : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QInputPanel)
    Q_PROPERTY(QObject *inputItem READ inputItem WRITE setInputItem NOTIFY inputItemChanged)
    Q_PROPERTY(QRectF cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged)
    Q_PROPERTY(QRectF keyboardRectangle READ keyboardRectangle NOTIFY keyboardRectangleChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool animating READ isAnimating NOTIFY animatingChanged)

public:
    QObject *inputItem() const;
    void setInputItem(QObject *inputItemChanged);

    // the window containing the editor
    QWindow *inputWindow() const;

    QTransform inputItemTransform() const;
    void setInputItemTransform(const QTransform &transform);

    // in window coordinates
    QRectF cursorRectangle() const; // ### what if we have rotations for the item?

    // keyboard geometry in window coords
    QRectF keyboardRectangle();

    enum Action {
        Click,
        ContextMenu
    };

    bool visible() const;
    void setVisible(bool visible);

    bool isAnimating() const;

public Q_SLOTS:
    void show();
    void hide();

    void update(Qt::InputMethodQueries queries);
    void reset();
    void commit();

    void invokeAction(Action a, int cursorPosition);

Q_SIGNALS:
    void inputItemChanged();
    void cursorRectangleChanged();
    void keyboardRectangleChanged();
    void visibleChanged();
    void animatingChanged();

private:
    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend class QPlatformInputContext;
    QInputPanel();
    ~QInputPanel();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
