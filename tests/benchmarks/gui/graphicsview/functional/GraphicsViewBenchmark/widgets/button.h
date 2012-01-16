/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
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

#ifndef BUTTON_H
#define BUTTON_H

#include <QGraphicsWidget>

class ButtonPrivate;
class QTextDocument;

class QPixmap; 
class QFont;

class Button : public QGraphicsWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Button)

public:

    Button(const QString &text, QGraphicsItem *parent=0, QSizeF minimumSize = QSizeF());
    virtual ~Button();

signals:

    void clicked(bool checked = false);
    void pressed();
    void released();

public slots:

    void themeChange();
    void setText(const QString &text);
    QString text();

public:
    
    void setBackground(QPixmap& background);
    bool isDown();
    void select(bool select){m_selected = select;}
    void click() {emit clicked();}

private:

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, 
            QWidget *widget = 0);
    QSizeF sizeHint(Qt::SizeHint which, 
        const QSizeF &constraint = QSizeF()) const;

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
    Q_DISABLE_COPY(Button)
    ButtonPrivate *d_ptr;
    QPixmap m_background;
    QFont m_font;
    bool m_selected;
};

#endif // BUTTON_H
