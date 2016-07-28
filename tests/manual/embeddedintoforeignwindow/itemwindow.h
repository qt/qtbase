/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef ITEMWINDOW_H
#define ITEMWINDOW_H

#include <QtGui/QKeySequence>
#include <QtGui/QRasterWindow>

QT_USE_NAMESPACE

// ItemWindow: Primitive UI item classes for use with a ItemWindow based
// on QRasterWindow without widgets allowing a simple UI without widgets.

// Basic item to be used in a ItemWindow
class Item : public QObject {
    Q_OBJECT
public:
    explicit Item(QObject *parent = nullptr) : QObject(parent) {}

    virtual void paint(QPainter &painter) = 0;
    virtual void mouseEvent(QMouseEvent *) {}
    virtual void keyEvent(QKeyEvent *) {}
};

// Positionable text item
class TextItem : public Item {
    Q_OBJECT
public:
    explicit TextItem(const QString &text, const QRect &rect, const QColor &col,
                      QObject *parent = nullptr)
        : Item(parent), m_text(text), m_rect(rect), m_col(col) {}

    void paint(QPainter &painter) override;

    QRect rect() const { return m_rect; }

private:
    const QString m_text;
    const QRect m_rect;
    const QColor m_col;
};

// "button" item
class ButtonItem : public TextItem {
    Q_OBJECT
public:
    explicit ButtonItem(const QString &text, const QRect &rect, const QColor &col,
                        QObject *parent = nullptr)
        : TextItem(text, rect, col, parent), m_shortcut(0) {}

    void mouseEvent(QMouseEvent *mouseEvent) override;
    void keyEvent(QKeyEvent *keyEvent) override;

    int shortcut() const { return m_shortcut; }
    void setShortcut(int shortcut) { m_shortcut = shortcut; }

signals:
    void clicked();

private:
    int m_shortcut;
};

#define PROPAGATE_EVENT(windowHandler, eventClass, itemHandler) \
void windowHandler(eventClass *e) override \
{  \
   foreach (Item *i, m_items) \
        i->itemHandler(e); \
}

class ItemWindow : public QRasterWindow {
    Q_OBJECT
public:
    explicit ItemWindow(QWindow *parent = nullptr) : QRasterWindow(parent), m_background(Qt::white) {}

    void addItem(Item *item) { m_items.append(item); }

    QColor background() const { return m_background; }
    void setBackground(const QColor &background) { m_background = background; }

protected:
    void paintEvent(QPaintEvent *) override;
    PROPAGATE_EVENT(mousePressEvent, QMouseEvent, mouseEvent)
    PROPAGATE_EVENT(mouseReleaseEvent, QMouseEvent, mouseEvent)
    PROPAGATE_EVENT(mouseDoubleClickEvent, QMouseEvent, mouseEvent)
    PROPAGATE_EVENT(mouseMoveEvent, QMouseEvent, mouseEvent)
    PROPAGATE_EVENT(keyPressEvent, QKeyEvent, keyEvent)
    PROPAGATE_EVENT(keyReleaseEvent, QKeyEvent, keyEvent)

private:
    QList<Item *> m_items;
    QColor m_background;
};

#endif // ITEMWINDOW_H
