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
