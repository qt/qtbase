/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qdecoration_qws.h"

#include "qapplication.h"
#include "qdrawutil.h"
#include "qpainter.h"
#include "qregion.h"
#include "qwhatsthis.h"

#include "qmenu.h"
#include "private/qwidget_p.h"
#include "qwsmanager_qws.h"

QT_BEGIN_NAMESPACE

/*!
    \class QDecoration
    \ingroup qws

    \brief The QDecoration class is a base class for window
    decorations in Qt for Embedded Linux

    Note that this class is non-portable and only available in
    \l{Qt for Embedded Linux}.

    \l{Qt for Embedded Linux} provides window management of top level windows
    and several ready made decorations (i.e., \c Default, \c Styled
    and \c Windows). Custom decorations can be implemented by
    subclassing the QDecoration class and creating a decoration plugin
    (derived from QDecorationPlugin). The default
    implementation of the QDecorationFactory class will automatically
    detect the plugin, and load the decoration into the application at
    run-time using Qt's \l {How to Create Qt Plugins}{plugin
    system}. To actually apply a decoration, use the
    QApplication::qwsSetDecoration() function.

    When creating a custom decoration, implement the paint() function
    to paint the border and title decoration, and the region()
    function to return the regions the decoration
    occupies. Reimplement the regionClicked() and
    regionDoubleClicked() functions to respond to mouse clicks (the
    default implementations responds to (single) clicks on items in a
    widget's system menu and double clicks on a widget's title).

    QDecoration provides the DecorationRegion enum that describes the
    various regions of the window decoration, and the regionAt()
    function to determine the region containing a given point. The
    QDecoration class also provides the DecorationState enum
    describing the state of a given region, e.g. whether it is active
    or not.

    In addition, it is possible to build the system menu for a given
    top level widget using the buildSysMenu() function; whenever an
    action in this menu is triggered, the menuTriggered() function is
    called automatically.

    Finally, the QDecoration class provides a couple of static
    functions, startMove() and startResize(), which start a move or
    resize action by making the appropriate decoration region active
    and grabbing the mouse input.

    \sa QDecorationFactory, QDecorationPlugin, {Qt for Embedded Linux
    Architecture}
*/

/*!
    \fn QDecoration::QDecoration()

    Constructs a decoration object.
*/

/*!
    \fn QDecoration::~QDecoration()

    Destroys this decoration object.
*/

/*!
    \enum QDecoration::DecorationRegion

    This enum describes the various regions of the window decoration.

    \value All The entire region used by the window decoration.

    \value Top    The top border used to vertically resize the window.
    \value Bottom The bottom border used to vertically resize the window.
    \value Left   The left border used to horizontally resize the window.
    \value Right  The right border used to horizontally resize the window.
    \value TopLeft    The top-left corner of the window used to resize the
                      window both horizontally and vertically.
    \value TopRight   The top-right corner of the window used to resize the
                      window both horizontally and vertically.
    \value BottomLeft The bottom-left corner of the window used to resize the
                      window both horizontally and vertically.
    \value BottomRight The bottom-right corner of the window used to resize the
                      window both horizontally and vertically.
    \value Borders    All the regions used to describe the window's borders.

    \value Title    The region containing the window title, used
                    to move the window by dragging with the mouse cursor.
    \value Close    The region occupied by the close button. Clicking in this
                    region closes the window.
    \value Minimize The region occupied by the minimize button. Clicking in
                    this region minimizes the window.
    \value Maximize The region occupied by the maximize button. Clicking in
                    this region maximizes the window.
    \value Normalize The region occupied by a button used to restore a window's
                     normal size. Clicking in this region restores a maximized
                     window to its previous size. The region used for this
                     button is often also the Maximize region.
    \value Menu     The region occupied by the window's menu button. Clicking
                    in this region opens the window operations (system) menu.
    \value Help     The region occupied by the window's help button. Clicking
                    in this region causes the context-sensitive help function
                    to be enabled.
    \value Resize   The region used to resize the window.
    \value Move     The region used to move the window.
    \value None      No region.

    \sa region(), regionAt(), DecorationState
*/

/*!
    \enum QDecoration::DecorationState

    This enum describes the various states of a decoration region.

    \value Normal The region is active
    \value Disabled The region is inactive.
    \value Hover The cursor is hovering over the region.
    \value Pressed The region is pressed.

    \sa paint(), DecorationRegion
*/

/*!
    \fn QRegion QDecoration::region(const QWidget *widget, const QRect & rectangle, int decorationRegion)

    Implement this function to return the region specified by \a
    decorationRegion for the given top level \a widget.

    The \a rectangle parameter specifies the rectangle the decoration
    is wrapped around. The \a decorationRegion is a bitmask of the
    values described by the DecorationRegion enum.

    \sa regionAt(), paint()
*/

/*!
    \fn QRegion QDecoration::region(const QWidget *widget, int decorationRegion)
    \overload
*/

/*!
    \fn bool QDecoration::paint(QPainter *painter, const QWidget *widget, int decorationRegion,
                                DecorationState state)

    Implement this function to paint the border and title decoration
    for the specified top level \a widget using the given \a painter
    and decoration \a state. The specified \a decorationRegion is a
    bitmask of the values described by the DecorationRegion enum.

    Note that \l{Qt for Embedded Linux} expects this function to return true if
    any of the widget's decorations are repainted; otherwise it should
    return false.

    \sa region()
*/

/*!
    \fn int QDecoration::regionAt(const QWidget *widget, const QPoint &point)

    Returns the type of the first region of the specified top level \a
    widget containing the given \a point.

    The return value is one of the DecorationRegion enum's values. Use
    the region() function to retrieve the actual region. If none of
    the widget's regions contain the point, this function returns \l
    None.

    \sa region()
*/
int QDecoration::regionAt(const QWidget *w, const QPoint &point)
{
    int regions[] = {
        TopLeft, Top, TopRight, Left, Right, BottomLeft, Bottom, BottomRight, // Borders first
        Menu, Title, Help, Minimize, Normalize, Maximize, Close,                         // then buttons
        None
    };

//     char *regions_str[] = {
//         "TopLeft", "Top", "TopRight", "Left", "Right", "BottomLeft", "Bottom", "BottomRight",
//         "Menu", "Title", "Help", "Minimize", "Normalize", "Maximize", "Close",
//         "None"
//     };

    // First check to see if within all regions at all
    QRegion reg = region(w, w->geometry(), All);
    if (!reg.contains(point)) {
        return None;
    }

    int i = 0;
    while (regions[i]) {
        reg = region(w, w->geometry(), regions[i]);
        if (reg.contains(point)) {
//            qDebug("In region %s", regions_str[i]);
            return regions[i];
        }
        ++i;
    }
    return None;
}

#ifndef QT_NO_MENU
/*!
    Builds the system menu for the given top level \a widget, adding
    \gui Restore, \gui Move, \gui Size, \gui Minimize, \gui Maximize
    and \gui Close actions to the given \a menu.

    \sa menuTriggered()
*/
void QDecoration::buildSysMenu(QWidget *widget, QMenu *menu)
{
    QDecorationAction *act = new QDecorationAction(QLatin1String("Restore"),
                                                   menu, Maximize);
    act->setEnabled(widget->windowState() & Qt::WindowMaximized);
    menu->addAction(act);
    act = new QDecorationAction(QLatin1String("Move"), menu, Move);
    act->setEnabled(!(widget->windowState() & Qt::WindowMaximized));
    menu->addAction(act);
    menu->addAction(new QDecorationAction(QLatin1String("Size"), menu, Resize));
    act = new QDecorationAction(QLatin1String("Minimize"), menu, Minimize);
    menu->addAction(act);
    act = new QDecorationAction(QLatin1String("Maximize"), menu, Maximize);
    act->setDisabled(widget->windowState() & Qt::WindowMaximized);
    menu->addAction(act);
    menu->addSeparator();
    menu->addAction(new QDecorationAction(QLatin1String("Close"), menu, Close));
}

/*!
    This function is called whenever an action in a top level widget's
    menu is triggered, and simply calls the regionClicked() function
    passing the \a widget and \a action parameters as arguments.

    \sa buildSysMenu()
*/
void QDecoration::menuTriggered(QWidget *widget, QAction *action)
{
    QDecorationAction *decAction = static_cast<QDecorationAction *>(action);
    regionClicked(widget, decAction->reg);
}
#endif // QT_NO_MENU

/*!
    \fn void QDecoration::regionClicked(QWidget *widget, int region)

    Handles the event that the specified \a region in the given top
    level \a widget is activated by a single click (the \a region
    parameter is described using the DecorationRegion enum).

    This function is called whenever a region in a top level widget is
    clicked; the default implementation responds to clicks on items in
    the system menu, performing the requested actions.

    \sa regionDoubleClicked(), region()
*/
void QDecoration::regionClicked(QWidget *widget, int reg)
{
    switch(reg) {
    case Move:
        startMove(widget);
        break;
    case Resize:
        startResize(widget);
        break;
    case Help:
#ifndef QT_NO_WHATSTHIS
        if (QWhatsThis::inWhatsThisMode())
            QWhatsThis::leaveWhatsThisMode();
        else
            QWhatsThis::enterWhatsThisMode();
#endif
        break;
    case Close:
        widget->close();
        break;
    case Normalize:
        widget->showNormal();
        break;
    case Maximize:
        if (widget->windowState() & Qt::WindowMaximized)
            widget->showNormal();
        else
            widget->showMaximized();
        break;
    }
}

/*!
    \fn void QDecoration::regionDoubleClicked(QWidget *widget, int region)

    Handles the event that the specified \a region in the given top
    level \a widget is activated by a double click (the region
    parameter is described using the DecorationRegion enum).

    This function is called whenever a region in a top level widget is
    double clicked; the default implementation responds to a double
    click on the widget's title, toggling its size between the maximum
    and its normal size.

    \sa regionClicked(), region()
*/
void QDecoration::regionDoubleClicked(QWidget *widget, int reg)
{
    switch(reg)
    {
        case Title: {
            if (widget->windowState() & Qt::WindowMaximized)
                widget->showNormal();
            else
                widget->showMaximized();
            break;
        }
    }
}

/*!
    Starts to move the given top level \a widget by making its \l
    Title region active and grabbing the mouse input.

    \sa startResize()
*/
void QDecoration::startMove(QWidget *widget)
{
#ifdef QT_NO_QWS_MANAGER
    Q_UNUSED(widget);
#else
    QWSManager *manager = widget->d_func()->topData()->qwsManager;
    if (manager)
        manager->startMove();
#endif
}

/*!
    Starts to resize the given top level \a widget by making its \l
    BottomRight region active and grabbing the mouse input.

    \sa startMove()
*/
void QDecoration::startResize(QWidget *widget)
{
#ifdef QT_NO_QWS_MANAGER
    Q_UNUSED(widget);
#else
    QWSManager *manager = widget->d_func()->topData()->qwsManager;
    if (manager)
        manager->startResize();
#endif
}


QT_END_NAMESPACE
