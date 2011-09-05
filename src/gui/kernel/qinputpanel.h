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

#ifndef QINPUTPANEL_H
#define QINPUTPANEL_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QInputPanelPrivate;
class QWindow;
class QRectF;

class Q_GUI_EXPORT QInputPanel : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QInputPanel)
    Q_PROPERTY(QObject *inputItem READ inputItem WRITE setInputItem NOTIFY inputItemChanged)
    Q_PROPERTY(QRectF cursorRect READ cursorRect NOTIFY cursorRectChanged)
    Q_PROPERTY(QRectF keyboardRect READ keyboardRect NOTIFY keyboardRectChanged)
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(bool animating READ isAnimating NOTIFY animatingChanged)

public:
    QObject *inputItem() const;
    void setInputItem(QObject *inputItemChanged);

    // the window containing the editor
    QWindow *inputWindow() const;

    QTransform inputItemTransform() const;
    void setInputItemTranform(const QTransform &transform);

    // in window coordinates
    QRectF cursorRect() const; // ### what if we have rotations for the item?

    // keyboard geometry in window coords
    QRectF keyboardRect();

    enum Action {
        Click,
        ContextMenu
    };

    bool isOpen() const;
    void setOpen(bool open);

    bool isAnimating() const;

public Q_SLOTS:
    void open();
    void close();

    void update(Qt::InputMethodQueries queries);
    void reset();

    void invokeAction(Action a, int cursorPosition);

Q_SIGNALS:
    void inputItemChanged();
    void cursorRectChanged();
    void keyboardRectChanged();
    void openChanged();
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
