// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef __MENU_H__
#define __MENU_H__

#include <QGraphicsWidget>
#include <QList>

class QGraphicsView;
class QGraphicsLinearLayout;
class Button;

class Menu : public QGraphicsWidget
{
    Q_OBJECT
public:
    Menu(QGraphicsView* parent);
    ~Menu();

public:
    Button* addMenuItem(const QString itemName, QObject* receiver, const char* member);
    inline bool menuVisible() { return m_isMenuVisible; }
    virtual void keyPressEvent(QKeyEvent *event);

public slots:
    void themeChange();

public slots:
    void menuShowHide();

private:
    void init();
    void menuShow();
    void menuHide();

private:
    Q_DISABLE_COPY(Menu)
    QGraphicsView* m_Parent;
    QGraphicsLinearLayout* m_Layout;
    QList<Button*>* m_ButtonContainer;
    bool m_isMenuVisible;
    int m_currentSelectedIndex;
};

#endif // __MENU_H__
