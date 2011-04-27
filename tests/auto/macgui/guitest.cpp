/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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


#include "guitest.h"
#include <QDebug>
#include <QWidget>
#include <QStack>
#include <QTimer>
#include <QtTest/QtTest>

#ifdef Q_OS_MAC
#   include <private/qt_mac_p.h>
#endif


/*
    Not really a test, just prints interface info.
*/
class PrintTest : public TestBase
{
public:
    bool operator()(InterfaceChildPair candidate) 
    {
        qDebug() << "";
        qDebug() << "Name" << candidate.iface->text(QAccessible::Name, candidate.possibleChild);
        qDebug() << "Pos" <<  candidate.iface->rect(candidate.possibleChild);
        if (candidate.possibleChild == 0)
            qDebug() << "Number of children" << candidate.iface->childCount();
        return false;
    }
};

class NameTest : public TestBase
{
public:
    NameTest(const QString &text, QAccessible::Text textType) : text(text), textType(textType) {}
    QString text;
    QAccessible::Text textType;

    bool operator()(InterfaceChildPair candidate) 
    {
        return (candidate.iface->text(textType, candidate.possibleChild) == text);
    }
};

void WidgetNavigator::printAll(QWidget *widget)
{
    QAccessibleInterface * const iface = QAccessible::queryAccessibleInterface(widget);
    deleteInDestructor(iface);
    printAll(InterfaceChildPair(iface, 0));
}

void WidgetNavigator::printAll(InterfaceChildPair interface) 
{
    PrintTest printTest;
    recursiveSearch(&printTest, interface.iface, interface.possibleChild);
}

InterfaceChildPair WidgetNavigator::find(QAccessible::Text textType, const QString &text, QWidget *start)
{
    QAccessibleInterface * const iface = QAccessible::queryAccessibleInterface(start);
    deleteInDestructor(iface);
    return find(textType, text, iface);
}

InterfaceChildPair WidgetNavigator::find(QAccessible::Text textType, const QString &text, QAccessibleInterface *start)
{
    NameTest nameTest(text, textType);
    return recursiveSearch(&nameTest, start, 0);
}

/*
    Recursiveley navigates the accessible hiearchy looking for an interfafce that
    passsed the Test (meaning it returns true).
*/
InterfaceChildPair WidgetNavigator::recursiveSearch(TestBase *test, QAccessibleInterface *iface, int possibleChild)
{
    QStack<InterfaceChildPair> todoInterfaces;
    todoInterfaces.push(InterfaceChildPair(iface, possibleChild));

    while (todoInterfaces.isEmpty() == false) {
        InterfaceChildPair testInterface = todoInterfaces.pop();
        
        if ((*test)(testInterface))
            return testInterface;
            
        if (testInterface.possibleChild != 0)
            continue;

        const int numChildren = testInterface.iface->childCount();
        for (int i = 0; i < numChildren; ++i) {
            QAccessibleInterface *childInterface = 0;
            int newPossibleChild = testInterface.iface->navigate(QAccessible::Child, i + 1, &childInterface);
            if (childInterface) {
                todoInterfaces.push(InterfaceChildPair(childInterface, newPossibleChild));
                deleteInDestructor(childInterface);
            } else if (newPossibleChild != -1) {
                todoInterfaces.push(InterfaceChildPair(testInterface.iface, newPossibleChild));
            }
        }
    }
    return InterfaceChildPair();
}

void WidgetNavigator::deleteInDestructor(QAccessibleInterface * interface)
{
    interfaces.insert(interface);
}

QWidget *WidgetNavigator::getWidget(InterfaceChildPair interface)
{
    return qobject_cast<QWidget *>(interface.iface->object());
}

WidgetNavigator::~WidgetNavigator()
{
    foreach(QAccessibleInterface *interface, interfaces) {
        delete interface;
    }
}

///////////////////////////////////////////////////////////////////////////////

namespace NativeEvents {
#ifdef Q_OS_MAC
   void mouseClick(const QPoint &globalPos, Qt::MouseButtons buttons, MousePosition updateMouse)
    {
        CGPoint position;
        position.x = globalPos.x();
        position.y = globalPos.y();
       
        const bool updateMousePosition = (updateMouse == UpdatePosition);
        
        // Mouse down.
        CGPostMouseEvent(position, updateMousePosition, 3, 
                        (buttons & Qt::LeftButton) ? true : false, 
                        (buttons & Qt::MidButton/* Middlebutton! */) ? true : false, 
                        (buttons & Qt::RightButton) ? true : false);

        // Mouse up.
        CGPostMouseEvent(position, updateMousePosition, 3, false, false, false);	
    }
#else
# error Oops, NativeEvents::mouseClick() is not implemented on this platform.
#endif
};

///////////////////////////////////////////////////////////////////////////////

GuiTester::GuiTester()
{
    clearSequence();
}

GuiTester::~GuiTester()
{
    foreach(DelayedAction *action, actions)
        delete action;
}

bool checkPixel(QColor pixel, QColor expected)
{
    const int allowedDiff = 20;

    return !(qAbs(pixel.red() - expected.red()) > allowedDiff ||
            qAbs(pixel.green() - expected.green()) > allowedDiff ||
            qAbs(pixel.blue() - expected.blue()) > allowedDiff);
}

/*
    Tests that the pixels inside rect in image all have the given color. 
*/
bool GuiTester::isFilled(const QImage image, const QRect &rect, const QColor &color)
{
    for (int y = rect.top(); y <= rect.bottom(); ++y)
        for (int x = rect.left(); x <= rect.right(); ++x) {
            const QColor pixel = image.pixel(x, y);
            if (checkPixel(pixel, color) == false) {
//                qDebug()<< "Wrong pixel value at" << x << y << pixel.red() << pixel.green() << pixel.blue();
                return false;
            }
        }
    return true;
}


/*
    Tests that stuff is painted to the pixels inside rect.
    This test fails if any lines in the given direction have pixels 
    of only one color.
*/
bool GuiTester::isContent(const QImage image, const QRect &rect, Directions directions)
{
    if (directions & Horizontal) {
        for (int y = rect.top(); y <= rect.bottom(); ++y) {
            QColor currentColor = image.pixel(rect.left(), y);
            bool fullRun = true;
            for (int x = rect.left() + 1; x <= rect.right(); ++x) {
                if (checkPixel(image.pixel(x, y), currentColor) == false) {
                    fullRun = false;
                    break;
                }
            }
            if (fullRun) {
//                qDebug() << "Single-color line at horizontal line " << y  << currentColor;
                return false;
            }
        }
        return true;
    } 

    if (directions & Vertical) {
       for (int x = rect.left(); x <= rect.right(); ++x) {
            QRgb currentColor = image.pixel(x, rect.top());
            bool fullRun = true;
            for (int y = rect.top() + 1; y <= rect.bottom(); ++y) {
                if (checkPixel(image.pixel(x, y), currentColor) == false) {
                    fullRun = false;
                    break;
                }
            }
            if (fullRun) {
//                qDebug() << "Single-color line at vertical line" << x << currentColor;
                return false;
            }
        }
        return true;
    }
    return false; // shut the compiler up.
}

void DelayedAction::run()
{
    if (next)
        QTimer::singleShot(next->delay, next, SLOT(run()));
};

/*
    Schedules a mouse click at an interface using a singleShot timer.
    Only one click can be scheduled at a time.
*/
ClickLaterAction::ClickLaterAction(InterfaceChildPair interface, Qt::MouseButtons buttons)
{
    this->useInterface = true;
    this->interface = interface;
    this->buttons = buttons;
}

/*
    Schedules a mouse click at a widget using a singleShot timer.
    Only one click can be scheduled at a time.
*/
ClickLaterAction::ClickLaterAction(QWidget *widget, Qt::MouseButtons buttons)
{
    this->useInterface = false;
    this->widget  = widget;
    this->buttons = buttons;
}

void ClickLaterAction::run()
{
    if (useInterface) {
        const QPoint globalCenter = interface.iface->rect(interface.possibleChild).center();
        NativeEvents::mouseClick(globalCenter, buttons);
    } else { // use widget
        const QSize halfSize = widget->size() / 2;
        const QPoint globalCenter = widget->mapToGlobal(QPoint(halfSize.width(), halfSize.height()));
        NativeEvents::mouseClick(globalCenter, buttons);
    }
    DelayedAction::run();
}

void GuiTester::clickLater(InterfaceChildPair interface, Qt::MouseButtons buttons, int delay)
{
    clearSequence();
    addToSequence(new ClickLaterAction(interface, buttons), delay);
    runSequence();
}

void GuiTester::clickLater(QWidget *widget, Qt::MouseButtons buttons, int delay)
{
    clearSequence();
    addToSequence(new ClickLaterAction(widget, buttons), delay);
    runSequence();
}

void GuiTester::clearSequence()
{
    startAction = new DelayedAction();
    actions.insert(startAction);
    lastAction = startAction;
}

void GuiTester::addToSequence(DelayedAction *action, int delay)
{
    actions.insert(action);
    action->delay = delay;
    lastAction->next = action;
    lastAction = action;
}

void GuiTester::runSequence()
{
    QTimer::singleShot(0, startAction, SLOT(run()));
}

void GuiTester::exitLoopSlot()
{
    QTestEventLoop::instance().exitLoop();
}

