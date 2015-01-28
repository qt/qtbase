/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
