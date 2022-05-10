// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
