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

#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QGraphicsLinearLayout>
#include <QList>

#include "button.h"
#include "menu.h"
#include "themeevent.h"
#include "theme.h"

static const int MinMenuItemWidth = 150;
static const int MinMenuItemHeight = 40;

Menu::Menu(QGraphicsView* parent) : QGraphicsWidget(),
    m_Parent(parent), m_Layout(new QGraphicsLinearLayout(Qt::Vertical, this)),
    m_ButtonContainer(0), m_isMenuVisible(false), m_currentSelectedIndex(-1)
{
    init();
}

Menu::~Menu()
{
    for(int i = 0; i < m_ButtonContainer->count(); i++ ) {
        delete m_ButtonContainer->at(i);
    }
    m_ButtonContainer->clear();

    delete m_ButtonContainer;
    m_ButtonContainer = 0;
}

void Menu::init()
{
    m_ButtonContainer = new QList<Button*>;

    m_Layout->setContentsMargins(0,0,0,0);
    m_Layout->setSpacing(0);

    setMinimumWidth(150);

    setLayout(m_Layout);

    connect(Theme::p(), SIGNAL(themeChanged()), this, SLOT(themeChange()));
}

Button* Menu::addMenuItem(const QString itemName, QObject* receiver, const char* member)
{
    Button* button = new Button(itemName ,this);
    button->setVisible(m_isMenuVisible);
    connect(button, SIGNAL(clicked(bool)), receiver, member);
    connect(button, SIGNAL(clicked(bool)), this, SLOT(menuShowHide()));
    m_ButtonContainer->append(button);
    button->setMinimumWidth(MinMenuItemWidth);
    button->setMinimumHeight(MinMenuItemHeight);
    return button;
}

void Menu::menuShowHide()
{
    m_isMenuVisible ? menuHide() : menuShow();
    m_isMenuVisible = !m_isMenuVisible;
}

void Menu::menuShow()
{
    for(int i = 0; i < m_ButtonContainer->count(); i++) {
        Button* button = m_ButtonContainer->at(i);
        m_Layout->addItem(button);
        button->show();
    }
}

void Menu::menuHide()
{
    for(int i = 0; i < m_ButtonContainer->count(); i++) {
        Button* button = m_ButtonContainer->at(i);
        button->select(false);
        button->hide();
        m_Layout->removeItem(button);
    }
    m_currentSelectedIndex = -1;
}

void Menu::themeChange()
{
    QPixmap pixmap = Theme::p()->pixmap("status_field_middle.svg",
            QSize(MinMenuItemWidth, MinMenuItemHeight));

    for(int i = 0; i < m_ButtonContainer->count(); i++) {
        Button* button = m_ButtonContainer->at(i);
        button->setBackground(pixmap);
    }
    update();
}

void Menu::keyPressEvent(QKeyEvent *event)
{
    //S60 3.x specific
    if(event->key() == 16777235 ) { //Up Arrow
        if(m_currentSelectedIndex > 0) { //One step up
            Button* button = m_ButtonContainer->at(m_currentSelectedIndex);
            button->select(false);
            button->update();

            m_currentSelectedIndex--;
            button = m_ButtonContainer->at(m_currentSelectedIndex);
            button->select(true);
            button->update();
        }
        else { //Jump to bottom
            if(m_currentSelectedIndex >= 0) {
               Button* button = m_ButtonContainer->at(m_currentSelectedIndex);
               button->select(false);
               button->update();
           }
            m_currentSelectedIndex = m_ButtonContainer->count() -1;
            if(m_currentSelectedIndex >= 0) {
                Button* button = m_ButtonContainer->at(m_currentSelectedIndex);
                button->select(true);
                button->update();
            }
        }
    }

    if(event->key() == 16777237 ) { //Down Arrow
        if (m_currentSelectedIndex < m_ButtonContainer->count()-1) { //One step down
            if(m_currentSelectedIndex >= 0) {
                Button* button = m_ButtonContainer->at(m_currentSelectedIndex);
                button->select(false);
                button->update();
            }
            m_currentSelectedIndex++;
            Button* button = m_ButtonContainer->at(m_currentSelectedIndex);
            button->select(true);
            button->update();
        }
        else { //Jump to top
            if(m_currentSelectedIndex >= 0) {
                Button* button = m_ButtonContainer->at(m_currentSelectedIndex);
                button->select(false);
                button->update();
                m_currentSelectedIndex = 0;
                button = m_ButtonContainer->at(m_currentSelectedIndex);
                button->select(true);
                button->update();
            }
        }
    }

    if(event->key() == 17825792 || event->key() == 16842752 || //LSK, center key or enter
            event->key() == 16777221 ) {
        if(m_currentSelectedIndex >= 0) {
            Button* button = m_ButtonContainer->at(m_currentSelectedIndex);
            button->click();
        }
    }

    if(event->key() == 17825793 ) { //RSK
        menuShowHide();
    }
}
