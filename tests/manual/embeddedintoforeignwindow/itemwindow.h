// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
   const auto copy = m_items; /* needed? */ \
   for (Item *i : copy) \
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
