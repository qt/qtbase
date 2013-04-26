/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessible.h"

#include "qaccessible2_p.h"
#include "qaccessiblecache_p.h"
#include "qaccessibleplugin.h"
#include "qaccessibleobject.h"
#include "qaccessiblebridge.h"
#include <QtCore/qtextboundaryfinder.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/QGuiApplication>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformaccessibility.h>
#include <qpa/qplatformintegration.h>

#include <QtCore/qdebug.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qhash.h>
#include <private/qfactoryloader_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAccessible
    \brief The QAccessible class provides enums and static functions
    related to accessibility.
    \internal

    \ingroup accessibility
    \inmodule QtWidgets

    This class is part of \l {Accessibility for QWidget Applications}.

    Accessible applications can be used by people who are not able to
    use applications by conventional means.

    The functions in this class are used for communication between
    accessible applications (also called AT Servers) and
    accessibility tools (AT Clients), such as screen readers and
    braille displays. Clients and servers communicate in the following way:

    \list
    \li  \e{AT Servers} notify the clients about events through calls to the
        updateAccessibility() function.

    \li  \e{AT Clients} request information about the objects in the server.
        The QAccessibleInterface class is the core interface, and encapsulates
        this information in a pure virtual API. Implementations of the interface
        are provided by Qt through the queryAccessibleInterface() API.
    \endlist

    The communication between servers and clients is initialized by
    the setRootObject() function. Function pointers can be installed
    to replace or extend the default behavior of the static functions
    in QAccessible.

    Qt supports Microsoft Active Accessibility (MSAA), Mac OS X
    Accessibility, and the Unix/X11 AT-SPI standard. Other backends
    can be supported using QAccessibleBridge.

    In addition to QAccessible's static functions, Qt offers one
    generic interface, QAccessibleInterface, that can be used to wrap
    all widgets and objects (e.g., QPushButton). This single
    interface provides all the metadata necessary for the assistive
    technologies. Qt provides implementations of this interface for
    its built-in widgets as plugins.

    When you develop custom widgets, you can create custom subclasses
    of QAccessibleInterface and distribute them as plugins (using
    QAccessiblePlugin) or compile them into the application.
    Likewise, Qt's predefined accessibility support can be built as
    plugin (the default) or directly into the Qt library. The main
    advantage of using plugins is that the accessibility classes are
    only loaded into memory if they are actually used; they don't
    slow down the common case where no assistive technology is being
    used.

    Qt also includes two convenience classes, QAccessibleObject and
    QAccessibleWidget, that inherit from QAccessibleInterface and
    provide the lowest common denominator of metadata (e.g., widget
    geometry, window title, basic help text). You can use them as
    base classes when wrapping your custom QObject or QWidget
    subclasses.

    \sa QAccessibleInterface
*/


/*!
    \class QAccessible::State

    \inmodule QtGui

    This structure defines bit flags that indicate
    the state of an accessible object. The values are:

    \value active                  The object is the active window or the active sub-element in a container (that would get focus when focusing the container).
    \value adjustable              The object represents an adjustable value, e.g. sliders.
    \value animated                The object's appearance changes frequently.
    \value busy                    The object cannot accept input at the moment.
    \value checkable               The object is checkable.
    \value checked                 The object's check box is checked.
    \value checkStateMixed         The third state of checkboxes (half checked in tri-state check boxes).
    \value collapsed               The object is collapsed, e.g. a closed listview item, or an iconified window.
    \value defaultButton           The object represents the default button in a dialog.
    \value defunct                 The object no longer exists.
    \value editable                The object has a text carret (and often implements the text interface).
    \value expandable              The object is expandable, mostly used for cells in a tree view.
    \value expanded                The object is expanded, currently its children are visible.
    \value extSelectable           The object supports extended selection.
    \value focusable               The object can receive focus. Only objects in the active window can receive focus.
    \value focused                 The object has keyboard focus.
    \value hasPopup                The object opens a popup.
    \value hotTracked              The object's appearance is sensitive to the mouse cursor position.
    \value invalid                 The object is no longer valid (because it has been deleted).
    \value invalidEntry            Input validation current input invalid.
    \value invisible               The object is not visible to the user.
    \value linked                  The object is linked to another object, e.g. a hyperlink.
    \value marqueed                The object displays scrolling contents, e.g. a log view.
    \value modal                   The object blocks input from other objects.
    \value movable                 The object can be moved.
    \value multiLine               The object has multiple lines of text (word wrap), as opposed to a single line.
    \value multiSelectable         The object supports multiple selected items.
    \value offscreen               The object is clipped by the visible area. Objects that are off screen are also invisible.
    \value passwordEdit            The object is a password field, e.g. a line edit for entering a Password.
    \value playsSound              The object produces sound when interacted with.
    \value pressed                 The object is pressed.
    \value readOnly                The object can usually be edited, but is explicitly set to read-only.
    \value selectable              The object is selectable.
    \value selectableText          The object has text which can be selected. This is different from selectable which refers to the object's children.
    \value selected                The object is selected.
    \value selfVoicing             The object describes itself through speech or sound.
    \value sizeable                The object can be resized, e.g. top-level windows.
    \value summaryElement          The object summarizes the state of the window and should be treated with priority.
    \value supportsAutoCompletion  The object has auto-completion, for example in line edits or combo boxes.
    \value traversed               The object is linked and has been visited.
    \value updatesFrequently       The object changes frequently and needs to be refreshed when accessing it.
    \value disabled                The object is unavailable to the user, e.g. a disabled widget.

    Implementations of QAccessibleInterface::state() return a combination
    of these flags.
*/

/*!
    \fn QAccessible::State::State()

    Constructs a new QAccessible::State with all states set to false.
*/

/*!
    \enum QAccessible::Event

    This enum type defines accessible event types.

    \omitvalue InvalidEvent                 Internal: Used when creating subclasses of QAccessibleEvent.
    \value AcceleratorChanged               The keyboard accelerator for an action has been changed.
    \value ActionChanged                    An action has been changed.
    \value ActiveDescendantChanged
    \value Alert                            A system alert (e.g., a message from a QMessageBox)
    \value AttributeChanged
    \value ContextHelpEnd                   Context help (QWhatsThis) for an object is finished.
    \value ContextHelpStart                 Context help (QWhatsThis) for an object is initiated.
    \value DefaultActionChanged             The default QAccessible::Action for the accessible
                                            object has changed.
    \value DescriptionChanged               The object's QAccessible::Description changed.
    \value DialogEnd                        A dialog (QDialog) has been hidden
    \value DialogStart                      A dialog (QDialog) has been set visible.
    \value DocumentContentChanged           The contents of a text document have changed.
    \value DocumentLoadComplete             A document has been loaded.
    \value DocumentLoadStopped              A document load has been stopped.
    \value DocumentReload                   A document reload has been initiated.
    \value DragDropEnd                      A drag and drop operation is about to finished.
    \value DragDropStart                    A drag and drop operation is about to be initiated.
    \value Focus                            An object has gained keyboard focus.
    \value ForegroundChanged                A window has been activated (i.e., a new window has
                                            gained focus on the desktop).
    \value HelpChanged                      The QAccessible::Help text property of an object has
                                            changed.
    \value HyperlinkEndIndexChanged         The end position of the display text for a hypertext
                                            link has changed.
    \value HyperlinkNumberOfAnchorsChanged  The number of anchors in a hypertext link has changed,
                                            perhaps because the display text has been split to
                                            provide more than one link.
    \value HyperlinkSelectedLinkChanged     The link for the selected hypertext link has changed.
    \value HyperlinkStartIndexChanged       The start position of the display text for a hypertext
                                            link has changed.
    \value HypertextChanged                 The display text for a hypertext link has changed.
    \value HypertextLinkActivated           A hypertext link has been activated, perhaps by being
                                            clicked or via a key press.
    \value HypertextLinkSelected            A hypertext link has been selected.
    \value HypertextNLinksChanged
    \value LocationChanged                  An object's location on the screen has changed.
    \value MenuCommand                      A menu item is triggered.
    \value MenuEnd                          A menu has been closed (Qt uses PopupMenuEnd for all
                                            menus).
    \value MenuStart                        A menu has been opened on the menubar (Qt uses
                                            PopupMenuStart for all menus).
    \value NameChanged                      The QAccessible::Name property of an object has changed.
    \value ObjectAttributeChanged
    \value ObjectCreated                    A new object is created.
    \value ObjectDestroyed                  An object is deleted.
    \value ObjectHide                       An object is hidden; for example, with QWidget::hide().
                                            Any children the object that is hidden has do not send
                                            this event. It is not sent when an object is hidden as
                                            it is being obcured by others.
    \value ObjectReorder                    A layout or item view  has added, removed, or moved an
                                            object (Qt does not use this event).
    \value ObjectShow                       An object is displayed; for example, with
                                            QWidget::show().
    \value PageChanged
    \value ParentChanged                    An object's parent object changed.
    \value PopupMenuEnd                     A pop-up menu has closed.
    \value PopupMenuStart                   A pop-up menu has opened.
    \value ScrollingEnd                     A scrollbar scroll operation has ended (the mouse has
                                            released the slider handle).
    \value ScrollingStart                   A scrollbar scroll operation is about to start; this may
                                            be caused by a mouse press on the slider handle, for
                                            example.
    \value SectionChanged
    \value SelectionAdd                     An item has been added to the selection in an item view.
    \value SelectionRemove                  An item has been removed from an item view selection.
    \value Selection                        The selection has changed in a menu or item view.
    \value SelectionWithin                  Several changes to a selection has occurred in an item
                                            view.
    \value SoundPlayed                      A sound has been played by an object
    \omitvalue StateChanged                 The QAccessible::State of an object has changed.
                                            This value is used internally for the QAccessibleStateChangeEvent.
    \value TableCaptionChanged              A table caption has been changed.
    \value TableColumnDescriptionChanged    The description of a table column, typically found in
                                            the column's header, has been changed.
    \value TableColumnHeaderChanged         A table column header has been changed.
    \omitvalue TableModelChanged                The model providing data for a table has been changed.
    \value TableRowDescriptionChanged       The description of a table row, typically found in the
                                            row's header, has been changed.
    \value TableRowHeaderChanged            A table row header has been changed.
    \value TableSummaryChanged              The summary of a table has been changed.
    \omitvalue TextAttributeChanged
    \omitvalue TextCaretMoved                   The caret has moved in an editable widget.
                                            The caret represents the cursor position in an editable
                                            widget with the input focus.
    \value TextColumnChanged                A text column has been changed.
    \omitvalue TextInserted                     Text has been inserted into an editable widget.
    \omitvalue TextRemoved                      Text has been removed from an editable widget.
    \omitvalue TextSelectionChanged             The selected text has changed in an editable widget.
    \omitvalue TextUpdated                      The text has been update in an editable widget.
    \omitvalue ValueChanged                     The QAccessible::Value of an object has changed.
    \value VisibleDataChanged

    The values for this enum are defined to be the same as those defined in the
    \l{AccessibleEventID.idl File Reference}{IAccessible2} and
    \l{Microsoft Active Accessibility Event Constants}{MSAA} specifications.
*/

/*!
    \enum QAccessible::Role

    This enum defines the role of an accessible object. The roles are:

    \value AlertMessage     An object that is used to alert the user.
    \value Animation        An object that displays an animation.
    \value Application      The application's main window.
    \value Assistant        An object that provids interactive help.
    \value Border           An object that represents a border.
    \value ButtonDropDown   A button that drops down a list of items.
    \value ButtonDropGrid   A button that drops down a grid.
    \value ButtonMenu       A button that drops down a menu.
    \value Canvas           An object that displays graphics that the user can interact with.
    \value Caret            An object that represents the system caret (text cursor).
    \value Cell             A cell in a table.
    \value Chart            An object that displays a graphical representation of data.
    \value CheckBox         An object that represents an option that can be checked or unchecked. Some options provide a "mixed" state, e.g. neither checked nor unchecked.
    \value Client           The client area in a window.
    \value Clock            A clock displaying time.
    \value Column           A column of cells, usually within a table.
    \value ColumnHeader     A header for a column of data.
    \value ComboBox         A list of choices that the user can select from.
    \value Cursor           An object that represents the mouse cursor.
    \value Desktop          The object represents the desktop or workspace.
    \value Dial             An object that represents a dial or knob.
    \value Dialog           A dialog box.
    \value Document         A document window, usually in an MDI environment.
    \value EditableText     Editable text
    \value Equation         An object that represents a mathematical equation.
    \value Graphic          A graphic or picture, e.g. an icon.
    \value Grip             A grip that the user can drag to change the size of widgets.
    \value Grouping         An object that represents a logical grouping of other objects.
    \value HelpBalloon      An object that displays help in a separate, short lived window.
    \value HotkeyField      A hotkey field that allows the user to enter a key sequence.
    \value Indicator        An indicator that represents a current value or item.
    \value LayeredPane      An object that can contain layered children, e.g. in a stack.
    \value Link             A link to something else.
    \value List             A list of items, from which the user can select one or more items.
    \value ListItem         An item in a list of items.
    \value MenuBar          A menu bar from which menus are opened by the user.
    \value MenuItem         An item in a menu or menu bar.
    \value NoRole           The object has no role. This usually indicates an invalid object.
    \value PageTab          A page tab that the user can select to switch to a different page in a dialog.
    \value PageTabList      A list of page tabs.
    \value Pane             A generic container.
    \value PopupMenu        A menu which lists options that the user can select to perform an action.
    \value ProgressBar      The object displays the progress of an operation in progress.
    \value PropertyPage     A property page where the user can change options and settings.
    \value Button           A button.
    \value RadioButton      An object that represents an option that is mutually exclusive with other options.
    \value Row              A row of cells, usually within a table.
    \value RowHeader        A header for a row of data.
    \value ScrollBar        A scroll bar, which allows the user to scroll the visible area.
    \value Separator        A separator that divides space into logical areas.
    \value Slider           A slider that allows the user to select a value within a given range.
    \value Sound            An object that represents a sound.
    \value SpinBox          A spin box widget that allows the user to enter a value within a given range.
    \value Splitter         A splitter distributing available space between its child widgets.
    \value StaticText       Static text, such as labels for other widgets.
    \value StatusBar        A status bar.
    \value Table            A table representing data in a grid of rows and columns.
    \value Terminal         A terminal or command line interface.
    \value TitleBar         The title bar caption of a window.
    \value ToolBar          A tool bar, which groups widgets that the user accesses frequently.
    \value ToolTip          A tool tip which provides information about other objects.
    \value Tree             A list of items in a tree structure.
    \value TreeItem         An item in a tree structure.
    \value UserRole         The first value to be used for user defined roles.
    \value Whitespace       Blank space between other objects.
    \value Window           A top level window.
*/

/*!
    \enum QAccessible::RelationFlag

    This enum type defines bit flags that can be combined to indicate
    the relationship between two accessible objects.

    \value Label            The first object is the label of the second object.
    \value Labelled         The first object is labelled by the second object.
    \value Controller       The first object controls the second object.
    \value Controlled       The first object is controlled by the second object.
    \value AllRelations     Used as a mask to specify that we are interesting in information
                            about all relations

    Implementations of relations() return a combination of these flags.
    Some values are mutually exclusive.
*/

/*!
    \enum QAccessible::Text

    This enum specifies string information that an accessible object
    returns.

    \value Name         The name of the object. This can be used both
                        as an identifier or a short description by
                        accessible clients.
    \value Description  A short text describing the object.
    \value Value        The value of the object.
    \value Help         A longer text giving information about how to use the object.
    \value Accelerator  The keyboard shortcut that executes the object's default action.
    \value UserText     The first value to be used for user defined text.
    \omitvalue DebugDescription
*/

/*!
    \enum QAccessible::InterfaceType

    \l QAccessibleInterface supports several sub interfaces.
    In order to provide more information about some objects, their accessible
    representation should implement one or more of these interfaces.
    When subclassing one of these interfaces, \l QAccessibleInterface::interface_cast also needs to be implemented.

    \value TextInterface            For text that supports selections or is more than one line. Simple labels do not need to implement this interface.
    \value EditableTextInterface    For text that can be edited by the user.
    \value ValueInterface           For objects that are used to manipulate a value, for example slider or scroll bar.
    \value ActionInterface          For interactive objects that allow the user to trigger an action. Basically everything that allows for example mouse interaction.
    \omitvalue ImageInterface       For objects that represent an image. This interface is generally less important.
    \value TableInterface           For lists, tables and trees.
    \value TableCellInterface       For cells in a TableInterface object.

    \sa QAccessibleInterface::interface_cast, QAccessibleTextInterface, QAccessibleEditableTextInterface, QAccessibleValueInterface, QAccessibleActionInterface, QAccessibleTableInterface, QAccessibleTableCellInterface
*/

/*!
    \fn QAccessibleInterface::~QAccessibleInterface()

    Destroys the object.
*/




/* accessible widgets plugin discovery stuff */
#ifndef QT_NO_ACCESSIBILITY
#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QAccessibleFactoryInterface_iid, QLatin1String("/accessible")))
#endif
#endif

Q_GLOBAL_STATIC(QList<QAccessible::InterfaceFactory>, qAccessibleFactories)
typedef QHash<QString, QAccessiblePlugin*> QAccessiblePluginsHash;
Q_GLOBAL_STATIC(QAccessiblePluginsHash, qAccessiblePlugins);

QAccessible::UpdateHandler QAccessible::updateHandler = 0;
QAccessible::RootObjectHandler QAccessible::rootObjectHandler = 0;

static bool cleanupAdded = false;

#ifndef QT_NO_ACCESSIBILITY
static QPlatformAccessibility *platformAccessibility()
{
    QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
    return pfIntegration ? pfIntegration->accessibility() : 0;
}
#endif

/*!
    \internal
*/
void QAccessible::cleanup()
{
#ifndef QT_NO_ACCESSIBILITY
    if (QPlatformAccessibility *pfAccessibility = platformAccessibility())
        pfAccessibility->cleanup();
#endif
}

static void qAccessibleCleanup()
{
    qAccessibleFactories()->clear();
}

/*!
    \typedef QAccessible::InterfaceFactory

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet code/src_gui_accessible_qaccessible.cpp 1

    The function receives a QString and a QObject pointer, where the
    QString is the key identifying the interface. The QObject is used
    to pass on to the QAccessibleInterface so that it can hold a reference
    to it.

    If the key and the QObject does not have a corresponding
    QAccessibleInterface, a null-pointer will be returned.

    Installed factories are called by queryAccessibilityInterface() until
    one provides an interface.
*/

/*!
    \typedef QAccessible::UpdateHandler

    \internal

    A function pointer type. Use a function with this prototype to install
    your own update function.

    The function is called by updateAccessibility().
*/

/*!
    \typedef QAccessible::RootObjectHandler

    \internal

    A function pointer type. Use a function with this prototype to install
    your own root object handler.

    The function is called by setRootObject().
*/


/*!
    Installs the InterfaceFactory \a factory. The last factory added
    is the first one used by queryAccessibleInterface().
*/
void QAccessible::installFactory(InterfaceFactory factory)
{
    if (!factory)
        return;

    if (!cleanupAdded) {
        qAddPostRoutine(qAccessibleCleanup);
        cleanupAdded = true;
    }
    if (qAccessibleFactories()->contains(factory))
        return;
    qAccessibleFactories()->append(factory);
}

/*!
    Removes \a factory from the list of installed InterfaceFactories.
*/
void QAccessible::removeFactory(InterfaceFactory factory)
{
    qAccessibleFactories()->removeAll(factory);
}

/*!
    \internal

    Installs the given \a handler as the function to be used by
    updateAccessibility(), and returns the previously installed
    handler.
*/
QAccessible::UpdateHandler QAccessible::installUpdateHandler(UpdateHandler handler)
{
    UpdateHandler old = updateHandler;
    updateHandler = handler;
    return old;
}

/*!
    Installs the given \a handler as the function to be used by setRootObject(),
    and returns the previously installed handler.
*/
QAccessible::RootObjectHandler QAccessible::installRootObjectHandler(RootObjectHandler handler)
{
    RootObjectHandler old = rootObjectHandler;
    rootObjectHandler = handler;
    return old;
}

Q_GLOBAL_STATIC(QAccessibleCache, qAccessibleCache)

/*!
    If a QAccessibleInterface implementation exists for the given \a object,
    this function returns a pointer to the implementation; otherwise it
    returns 0.

    The function calls all installed factory functions (from most
    recently installed to least recently installed) until one is found
    that provides an interface for the class of \a object. If no
    factory can provide an accessibility implementation for the class
    the function loads installed accessibility plugins, and tests if
    any of the plugins can provide the implementation.

    If no implementation for the object's class is available, the
    function tries to find an implementation for the object's parent
    class, using the above strategy.

    All interfaces are managed by an internal cache and should not be deleted.
*/
QAccessibleInterface *QAccessible::queryAccessibleInterface(QObject *object)
{
    if (!object)
        return 0;

    if (Id id = qAccessibleCache->objectToId.value(object))
        return qAccessibleCache->interfaceForId(id);

    // Create a QAccessibleInterface for the object class. Start by the most
    // derived class and walk up the class hierarchy.
    const QMetaObject *mo = object->metaObject();
    while (mo) {
        const QString cn = QLatin1String(mo->className());

        // Check if the class has a InterfaceFactory installed.
        for (int i = qAccessibleFactories()->count(); i > 0; --i) {
            InterfaceFactory factory = qAccessibleFactories()->at(i - 1);
            if (QAccessibleInterface *iface = factory(cn, object)) {
                qAccessibleCache->insert(object, iface);
                Q_ASSERT(qAccessibleCache->objectToId.contains(object));
                return iface;
            }
        }
#ifndef QT_NO_ACCESSIBILITY
#ifndef QT_NO_LIBRARY
        // Find a QAccessiblePlugin (factory) for the class name. If there's
        // no entry in the cache try to create it using the plugin loader.
        if (!qAccessiblePlugins()->contains(cn)) {
            QAccessiblePlugin *factory = 0; // 0 means "no plugin found". This is cached as well.
            const int index = loader()->indexOf(cn);
            if (index != -1)
                factory = qobject_cast<QAccessiblePlugin *>(loader()->instance(index));
            qAccessiblePlugins()->insert(cn, factory);
        }

        // At this point the cache should contain a valid factory pointer or 0:
        Q_ASSERT(qAccessiblePlugins()->contains(cn));
        QAccessiblePlugin *factory = qAccessiblePlugins()->value(cn);
        if (factory) {
            QAccessibleInterface *result = factory->create(cn, object);
            if (result) {   // Need this condition because of QDesktopScreenWidget
                qAccessibleCache->insert(object, result);
                Q_ASSERT(qAccessibleCache->objectToId.contains(object));
            }
            return result;
        }
#endif
#endif
        mo = mo->superClass();
    }

#ifndef QT_NO_ACCESSIBILITY
    if (object == qApp) {
        QAccessibleInterface *appInterface = new QAccessibleApplication;
        qAccessibleCache->insert(object, appInterface);
        Q_ASSERT(qAccessibleCache->objectToId.contains(qApp));
        return appInterface;
    }
#endif

    return 0;
}

/*!
    \internal
    Required to ensure that manually created interfaces
    are properly memory managed.

    Must only be called exactly once per interface.
    This is implicitly called when calling queryAccessibleInterface,
    so it's only required when re-implementing for example
    the child function and returning the child after new-ing
    a QAccessibleInterface subclass.
 */
QAccessible::Id QAccessible::registerAccessibleInterface(QAccessibleInterface *iface)
{
    Q_ASSERT(iface);
    return qAccessibleCache->insert(iface->object(), iface);
}

/*!
    \internal
    Removes the interface belonging to this id from the cache and
    deletes it. The id becomes invalid an may be re-used by the
    cache.
*/
void QAccessible::deleteAccessibleInterface(Id id)
{
    qAccessibleCache->deleteInterface(id);
}

/*!
    \internal
    Returns the unique ID for the accessibleInterface.
*/
QAccessible::Id QAccessible::uniqueId(QAccessibleInterface *iface)
{
    Id id = qAccessibleCache->idToInterface.key(iface);
    if (!id)
        id = registerAccessibleInterface(iface);
    return id;
}

/*!
    \internal
    Returns the QAccessibleInterface belonging to the id.

    Returns 0 if the id is invalid.
*/
QAccessibleInterface *QAccessible::accessibleInterface(Id id)
{
    return qAccessibleCache->idToInterface.value(id);
}


/*!
    Returns true if the platform requested accessibility information.

    This function will return false until a tool such as a screen reader
    accessed the accessibility framework. It is still possible to use
    \l QAccessible::queryAccessibleInterface even if accessibility is not
    active. But there will be no notifications sent to the platform.

    It is recommended to use this function to prevent expensive notifications
    via updateAccessibility() when they are not needed.
*/
bool QAccessible::isActive()
{
#ifndef QT_NO_ACCESSIBILITY
    if (QPlatformAccessibility *pfAccessibility = platformAccessibility())
        return pfAccessibility->isActive();
#endif
    return false;
}


/*!
  Sets the root object of the accessible objects of this application
  to \a object.  All other accessible objects are reachable using object
  navigation from the root object.

  Normally, it isn't necessary to call this function, because Qt sets
  the QApplication object as the root object immediately before the
  event loop is entered in QApplication::exec().

  Use QAccessible::installRootObjectHandler() to redirect the function
  call to a customized handler function.

  \sa queryAccessibleInterface()
*/
void QAccessible::setRootObject(QObject *object)
{
    if (rootObjectHandler) {
        rootObjectHandler(object);
        return;
    }

#ifndef QT_NO_ACCESSIBILITY
    if (QPlatformAccessibility *pfAccessibility = platformAccessibility())
        pfAccessibility->setRootObject(object);
#endif
}

/*!
  Notifies about a change that might be relevant for accessibility clients.

  \a event provides details about the change. These include the source
  of the change and the nature of the change.  The \a event should
  contain enough information give meaningful notifications.

  For example, the type \c ValueChange indicates that the position of
  a slider has been changed.

  Call this function whenever the state of your accessible object or
  one of its sub-elements has been changed either programmatically
  (e.g. by calling QLabel::setText()) or by user interaction.

  If there are no accessibility tools listening to this event, the
  performance penalty for calling this function is small, but if
  determining the parameters of the call is expensive you can test
  QAccessible::isActive() to avoid unnecessary computation.
*/
void QAccessible::updateAccessibility(QAccessibleEvent *event)
{
    if (!isActive())
        return;

#ifndef QT_NO_ACCESSIBILITY
    if (event->type() == QAccessible::TableModelChanged) {
        Q_ASSERT(event->object());
        if (QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(event->object())) {
            if (iface->tableInterface())
                iface->tableInterface()->modelChange(static_cast<QAccessibleTableModelChangeEvent*>(event));
        }
    }

    if (updateHandler) {
        updateHandler(event);
        return;
    }

    if (QPlatformAccessibility *pfAccessibility = platformAccessibility())
        pfAccessibility->notifyAccessibilityUpdate(event);
#endif
}

#if QT_DEPRECATED_SINCE(5, 0)
/*!
    \obsolete
    \fn void QAccessible::updateAccessibility(QObject *object, int child, Event reason);

    \brief Use QAccessible::updateAccessibility(QAccessibleEvent*) instead.
*/
#endif

/*!
    \internal
    \brief getBoundaries is a helper function to find the accessible text boundaries for QTextCursor based documents.
    \param documentCursor a valid cursor bound to the document (not null). It needs to ba at the position to look for the boundary
    \param boundaryType the type of boundary to find
    \return the boundaries as pair
*/
QPair< int, int > QAccessible::qAccessibleTextBoundaryHelper(const QTextCursor &offsetCursor, TextBoundaryType boundaryType)
{
    Q_ASSERT(!offsetCursor.isNull());

    QTextCursor endCursor = offsetCursor;
    endCursor.movePosition(QTextCursor::End);
    int characterCount = endCursor.position();

    QPair<int, int> result;
    QTextCursor cursor = offsetCursor;
    switch (boundaryType) {
    case CharBoundary:
        result.first = cursor.position();
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        result.second = cursor.position();
        break;
    case WordBoundary:
        cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
        result.first = cursor.position();
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        result.second = cursor.position();
        break;
    case SentenceBoundary: {
        // QCursor does not provide functionality to move to next sentence.
        // We therefore find the current block, then go through the block using
        // QTextBoundaryFinder and find the sentence the \offset represents
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        result.first = cursor.position();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        result.second = cursor.position();
        QString blockText = cursor.selectedText();
        const int offsetWithinBlockText = offsetCursor.position() - result.first;
        QTextBoundaryFinder sentenceFinder(QTextBoundaryFinder::Sentence, blockText);
        sentenceFinder.setPosition(offsetWithinBlockText);
        int prevBoundary = offsetWithinBlockText;
        int nextBoundary = offsetWithinBlockText;
        if (!(sentenceFinder.boundaryReasons() & QTextBoundaryFinder::StartOfItem))
            prevBoundary = sentenceFinder.toPreviousBoundary();
        nextBoundary = sentenceFinder.toNextBoundary();
        if (nextBoundary != -1)
            result.second = result.first + nextBoundary;
        if (prevBoundary != -1)
            result.first += prevBoundary;
        break; }
    case LineBoundary:
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        result.first = cursor.position();
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        result.second = cursor.position();
        break;
    case ParagraphBoundary:
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        result.first = cursor.position();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        result.second = cursor.position();
        break;
    case NoBoundary:
        result.first = 0;
        result.second = characterCount;
        break;
    }
    return result;
}

/*!
    \class QAccessibleInterface
    \brief The QAccessibleInterface class defines an interface that exposes information
    about accessible objects.
    \internal

    \ingroup accessibility
    \inmodule QtGui

    This class is part of \l {Accessibility for QWidget Applications}.

    Accessibility tools (also called AT Clients), such as screen readers
    or braille displays, require high-level information about
    accessible objects in an application. Accessible objects provide
    specialized input and output methods, making it possible for users
    to use accessibility tools with enabled applications (AT Servers).

    Every element that the user needs to interact with or react to is
    an accessible object, and should provide this information. These
    are mainly visual objects, such as widgets and widget elements, but
    can also be content, such as sounds.

    The AT client uses three basic concepts to acquire information
    about any accessible object in an application:
    \list
    \li \e Properties The client can read information about
    accessible objects. In some cases the client can also modify these
    properties; such as text in a line edit.
    \li \e Actions The client can invoke actions like pressing a button
    or .
    \li \e{Relationships and Navigation} The client can traverse from one
    accessible object to another, using the relationships between objects.
    \endlist

    The QAccessibleInterface defines the API for these three concepts.

    \section1 Relationships and Navigation

    The functions childCount() and indexOfChild() return the number of
    children of an accessible object and the index a child object has
    in its parent. The childAt() function returns a child QAccessibleInterface
    that is found at a position. The child does not have to be a direct
    child. This allows bypassing intermediate layers when the parent already knows the
    top-most child. childAt() is used for hit testing (finding the object
    under the mouse).

    The relations() function provides information about the relations an
    object has to other objects, and parent() and child() allows
    traversing from one object to another object.

    \section1 Properties

    The central property of an accessible objects is what role() it
    has. Different objects can have the same role, e.g. both the "Add
    line" element in a scroll bar and the \c OK button in a dialog have
    the same role, "button". The role implies what kind of
    interaction the user can perform with the user interface element.

    An object's state() property is a combination of different state
    flags and can describe both how the object's state differs from a
    "normal" state, e.g. it might be unavailable, and also how it
    behaves, e.g. it might be selectable.

    The text() property provides textual information about the object.
    An object usually has a name, but can provide extended information
    such as a description, help text, or information about any
    keyboard accelerators it provides. Some objects allow changing the
    text() property through the setText() function, but this
    information is in most cases read-only.

    The rect() property provides information about the geometry of an
    accessible object. This information is usually only available for
    visual objects.

    \section1 Interfaces

    To enable the user to interact with an accessible object the
    object must implement QAccessibleActionInterface in addition to
    QAccessibleInterface.
    Objects that support selections can define actions to change the selection.

    There are several other interfaces that should be implemented as required.
    QAccessibleTextInterface should be used for bigger texts edits such as document views.
    This interface should not be implemented for labels/single line edits.
    The complementary QAccessibleEditableTextInterface should be added when the
    Text is editable.

    For sliders, scrollbars and other numerical value selectors QAccessibleValueInterface
    should be implemented.

    Lists, tables and trees should implement QAccessibleTableInterface.

    \sa QAccessible, QAccessibleActionInterface, QAccessibleTextInterface, QAccessibleEditableTextInterface, QAccessibleValueInterface, QAccessibleTableInterface
*/

/*!
    \fn bool QAccessibleInterface::isValid() const

    Returns true if all the data necessary to use this interface
    implementation is valid (e.g. all pointers are non-null);
    otherwise returns false.

    \sa object()
*/

/*!
    \fn QObject *QAccessibleInterface::object() const

    Returns a pointer to the QObject this interface implementation provides
    information for.

    \sa isValid()
*/

/*!
    \fn int QAccessibleInterface::childCount() const

    Returns the number of children that belong to this object. A child
    can provide accessibility information on its own (e.g. a child
    widget), or be a sub-element of this accessible object.

    All objects provide this information.

    \sa indexOfChild()
*/

/*!
    \fn int QAccessibleInterface::indexOfChild(const QAccessibleInterface *child) const

    Returns the 0-based index of the object \a child in this object's
    children list, or -1 if \a child is not a child of this object.

    All objects provide this information about their children.

    \sa childCount()
*/

/*!
    Returns the meaningful relations to other widgets. Usually this will not return parent/child
    relations, unless they are handled in a specific way such as in tree views.
    It will typically return the labelled-by and label relations.

    It is possible to filter the relations by using \a match.
    It should never return itself.

    \sa parent(), child()
*/
QVector<QPair<QAccessibleInterface*, QAccessible::Relation> >
QAccessibleInterface::relations(QAccessible::Relation /*match = QAccessible::AllRelations*/) const
{
    return QVector<QPair<QAccessibleInterface*, QAccessible::Relation> >();
}

/*!
    Returns the object that has the keyboard focus.

    The object returned can be any descendant, including itself.
*/
QAccessibleInterface *QAccessibleInterface::focusChild() const
{
    return 0;
}

/*!
    \fn QAccessibleInterface *QAccessibleInterface::childAt(int x, int y) const

    Returns the QAccessibleInterface of a child that contains the screen coordinates (\a x, \a y).
    If there are no children at this position this function returns 0.
    The returned accessible must be a child, but not necessarily a direct child.

    This function is only relyable for visible objects (invisible
    object might not be laid out correctly).

    All visual objects provide this information.

    A default implementation is provided for objects inheriting QAccessibleObject. This will iterate
    over all children. If the widget manages its children (e.g. a table) it will be more efficient
    to write a specialized implementation.

    \sa rect()
*/

/*!
    \fn QAccessibleInterface* QAccessibleInterface::parent() const

    Returns the QAccessibleInterface of the parent in the accessible object hierarchy.

    Returns 0 if no parent exists (e.g. for the top level application object).

    \sa child()
*/

/*!
    \fn QAccessibleInterface* QAccessibleInterface::child(int index) const

    Returns the accessible child with index \a index.
    0-based index. The number of children of an object can be checked with childCount.

    Returns 0 when asking for an invalid child (e.g. when the child became invalid in the meantime).

    \sa childCount(), parent()
*/

/*!
    \fn QString QAccessibleInterface::text(QAccessible::Text t) const

    Returns the value of the text property \a t of the object.

    The \l QAccessible::Name is a string used by clients to identify, find, or
    announce an accessible object for the user. All objects must have
    a name that is unique within their container. The name can be
    used differently by clients, so the name should both give a
    short description of the object and be unique.

    An accessible object's \l QAccessible::Description provides textual information
    about an object's visual appearance. The description is primarily
    used to provide greater context for vision-impaired users, but is
    also used for context searching or other applications. Not all
    objects have a description. An "OK" button would not need a
    description, but a tool button that shows a picture of a smiley
    would.

    The \l QAccessible::Value of an accessible object represents visual information
    contained by the object, e.g. the text in a line edit. Usually,
    the value can be modified by the user. Not all objects have a
    value, e.g. static text labels don't, and some objects have a
    state that already is the value, e.g. toggle buttons.

    The \l QAccessible::Help text provides information about the function and
    usage of an accessible object. Not all objects provide this
    information.

    The \l QAccessible::Accelerator is a keyboard shortcut that activates the
    object's default action. A keyboard shortcut is the underlined
    character in the text of a menu, menu item or widget, and is
    either the character itself, or a combination of this character
    and a modifier key like Alt, Ctrl or Shift. Command controls like
    tool buttons also have shortcut keys and usually display them in
    their tooltip.

    All objects provide a string for \l QAccessible::Name.

    \sa role(), state()
*/

/*!
    \fn void QAccessibleInterface::setText(QAccessible::Text t, const QString &text)

    Sets the text property \a t of the object to \a text.

    Note that the text properties of most objects are read-only
    so calling this function might have no effect.

    \sa text()
*/

/*!
    \fn QRect QAccessibleInterface::rect() const

    Returns the geometry of the object. The geometry is in screen coordinates.

    This function is only reliable for visible objects (invisible
    objects might not be laid out correctly).

    All visual objects provide this information.

    \sa childAt()
*/

/*!
    \fn QAccessible::Role QAccessibleInterface::role() const

    Returns the role of the object.
    The role of an object is usually static.

    All accessible objects have a role.

    \sa text(), state()
*/

/*!
    \fn QAccessible::State QAccessibleInterface::state() const

    Returns the current state of the object.
    The returned value is a combination of the flags in
    the QAccessible::StateFlag enumeration.

    All accessible objects have a state.

    \sa text(), role()
*/


/*!
    \fn QAccessibleEditableTextInterface *QAccessibleInterface::editableTextInterface()
    \internal
*/

/*!
    Returns the accessible's foreground color if applicable or an invalid QColor.

    \sa backgroundColor()
*/
QColor QAccessibleInterface::foregroundColor() const
{
    return QColor();
}

/*!
    Returns the accessible's background color if applicable or an invalid QColor.

    \sa foregroundColor()
*/
QColor QAccessibleInterface::backgroundColor() const
{
    return QColor();
}

QAccessibleInterface::~QAccessibleInterface()
{
}

/*!
    \fn QAccessibleTextInterface *QAccessibleInterface::textInterface()
    \internal
*/

/*!
    \fn QAccessibleValueInterface *QAccessibleInterface::valueInterface()
    \internal
*/

/*!
    \fn QAccessibleTableInterface *QAccessibleInterface::tableInterface()
    \internal
*/

/*!
    \fn QAccessibleTableCellInterface *QAccessibleInterface::tableCellInterface()
    \internal
*/

/*!
    \fn QAccessibleActionInterface *QAccessibleInterface::actionInterface()
    \internal
*/

/*!
    \fn QAccessibleImageInterface *QAccessibleInterface::imageInterface()
    \internal
*/

/*!
    \class QAccessibleEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleEvent class contains parameters that describe updates in the
    accessibility framework.

    This class is used with \l QAccessible::updateAccessibility().

    The event type is one of the values of \l QAccessible::Event, which
    determines the subclass of QAccessibleEvent that applies.

    To enable in process screen readers, all events must be sent after the change has happened.
*/

/*! \fn QAccessibleEvent::QAccessibleEvent(QObject *object, QAccessible::Event type)

    Constructs a QAccessibleEvent to notify that \a object has changed.
    The event \a type explains what changed.
 */

/*! \fn QAccessibleEvent::~QAccessibleEvent()
  Destroys the event.
*/

/*! \fn QAccessible::Event QAccessibleEvent::type() const
  Returns the event type.
*/

/*! \fn QObject* QAccessibleEvent::object() const
  Returns the event object.
*/

/*! \fn void QAccessibleEvent::setChild(int child)
  Sets the child index to \a child.
*/

/*! \fn int QAccessibleEvent::child() const
  Returns the child index.
*/


/*!
    \class QAccessibleValueChangeEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleValueChangeEvent describes a change in value for an accessible object.

    It contains the new value.

    This class is used with \l QAccessible::updateAccessibility().
*/

/*! \fn QAccessibleValueChangeEvent::QAccessibleValueChangeEvent(QObject *object, const QVariant &value)
    Constructs a new QAccessibleValueChangeEvent for \a object.
    The event contains the new \a value.
*/
/*! \fn void QAccessibleValueChangeEvent::setValue(const QVariant & value)
    Sets the new \a value for this event.
*/
/*!
    \fn QVariant QAccessibleValueChangeEvent::value() const

    Returns the new value of the accessible object of this event.
*/

/*!
    \class QAccessibleStateChangeEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleStateChangeEvent notfies the accessibility framework
    that the state of an object has changed.

    This class is used with \l QAccessible::updateAccessibility().

    \sa QAccessibleInterface::state()
*/
/*! \fn QAccessibleStateChangeEvent::QAccessibleStateChangeEvent(QObject *object, QAccessible::State state)
    Constructs a new QAccessibleStateChangeEvent for \a object.
    The difference to the object's previous state is in \a state.
*/
/*!
    \fn QAccessible::State QAccessibleStateChangeEvent::changedStates() const
    \internal

    \brief Returns the states that have been changed.

    Be aware that the returned states are the ones that have changed,
    to find out about the state of an object, use QAccessibleInterface::state().

    For example, if an object used to have the focus but loses it,
    the object's state will have focused set to \c false. This event on the
    other hand tells about the change and has focused set to \c true since
    the focus state is changed from \c true to \c false.
*/


/*!
    \class QAccessibleTableModelChangeEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleTableModelChangeEvent signifies a change in a table, list, or tree where cells
    are added or removed.
    If the change affected a number of rows, firstColumn and lastColumn will return \c -1.
    Likewise for columns, the row functions may return \c -1.

    This class is used with \l QAccessible::updateAccessibility().
*/

/*! \enum QAccessibleTableModelChangeEvent::ModelChangeType
    This enum describes different types of changes in the table model.
    \value ModelReset      The model has been reset, all previous knowledge about the model is now invalid.
    \value DataChanged     No cells have been added or removed, but the data of the specified cell range is invalid.
    \value RowsInserted    New rows have been inserted.
    \value ColumnsInserted New columns have been inserted.
    \value RowsRemoved     Rows have been removed.
    \value ColumnsRemoved  Columns have been removed.
*/
/*! \fn QAccessibleTableModelChangeEvent::QAccessibleTableModelChangeEvent(QObject *object, ModelChangeType changeType)
    Constructs a new QAccessibleTableModelChangeEvent for \a object of with \a changeType.
*/
/*! \fn int QAccessibleTableModelChangeEvent::firstColumn() const
    Returns the first changed column.
*/
/*! \fn int QAccessibleTableModelChangeEvent::firstRow() const
    Returns the first changed row.
*/
/*! \fn int QAccessibleTableModelChangeEvent::lastColumn() const
    Returns the last changed column.
*/
/*! \fn int QAccessibleTableModelChangeEvent::lastRow() const
    Returns the last changed row.
*/
/*! \fn QAccessibleTableModelChangeEvent::ModelChangeType QAccessibleTableModelChangeEvent::modelChangeType() const
    Returns the type of change.
*/
/*! \fn void QAccessibleTableModelChangeEvent::setFirstColumn(int column)
    Sets the first changed \a column.
*/
/*! \fn void QAccessibleTableModelChangeEvent::setFirstRow(int row)
    Sets the first changed \a row.
*/
/*! \fn void QAccessibleTableModelChangeEvent::setLastColumn(int column)
    Sets the last changed \a column.
*/
/*! \fn void QAccessibleTableModelChangeEvent::setLastRow(int row)
    Sets the last changed \a row.
*/
/*! \fn void QAccessibleTableModelChangeEvent::setModelChangeType(ModelChangeType changeType)
    Sets the type of change to \a changeType.
*/


/*!
    \class QAccessibleTextCursorEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleEvent class

    This class is used with \l QAccessible::updateAccessibility().
*/
/*! \fn QAccessibleTextCursorEvent::QAccessibleTextCursorEvent(QObject *object, int cursorPosition)
    Create a new QAccessibleTextCursorEvent for \a object.
    The \a cursorPosition is the new cursor position.
*/
/*! \fn int QAccessibleTextCursorEvent::cursorPosition() const
    Returns the cursor position.
*/
/*! \fn void QAccessibleTextCursorEvent::setCursorPosition(int position)
    Sets the cursor \a position for this event.
*/

/*!
    \class QAccessibleTextInsertEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleEvent class

    This class is used with \l QAccessible::updateAccessibility().
*/
/*! \fn QAccessibleTextInsertEvent::QAccessibleTextInsertEvent(QObject *object, int position, const QString &text)
    Constructs a new QAccessibleTextInsertEvent event for \a object.
    The \a text has been inserted at \a position.
    By default, it is assumed that the cursor has moved to the end
    of the selection. If that is not the case, one needs to manually
    set it with \l QAccessibleTextCursorEvent::setCursorPosition for this event.
*/
/*! \fn int QAccessibleTextInsertEvent::changePosition() const
    Returns the position where the text was inserted.
*/
/*! \fn QString QAccessibleTextInsertEvent::textInserted() const
    Returns the text that has been inserted.
*/

/*!
    \class QAccessibleTextRemoveEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleEvent class

    This class is used with \l QAccessible::updateAccessibility().
*/
/*! \fn QAccessibleTextRemoveEvent::QAccessibleTextRemoveEvent(QObject *object, int position, const QString &text)
    Constructs a new QAccessibleTextRemoveEvent event for \a object.
    The \a text has been removed at \a position.
    By default it is assumed that the cursor has moved to \a position.
    If that is not the case, one needs to manually
    set it with \l QAccessibleTextCursorEvent::setCursorPosition for this event.
*/

/*! \fn int QAccessibleTextRemoveEvent::changePosition() const
    Returns the position where the text was removed.
*/
/*! \fn QString QAccessibleTextRemoveEvent::textRemoved() const
    Returns the text that has been removed.
*/

/*!
    \class QAccessibleTextUpdateEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleEvent class notifies about text changes.
    This is for accessibles that support editable text such as line edits.
    This event occurs for example when a portion of selected text
    gets replaced by pasting a new text or in override mode of editors.

    This class is used with \l QAccessible::updateAccessibility().
*/
/*! \fn QAccessibleTextUpdateEvent::QAccessibleTextUpdateEvent(QObject *object, int position, const QString &oldText, const QString &text)
    Constructs a new QAccessibleTextUpdateEvent for \a object.
    The text change takes place at \a position where the \a oldText was removed and \a text inserted instead.
*/
/*! \fn int QAccessibleTextUpdateEvent::changePosition() const
    Returns where the change took place.
*/
/*! \fn QString QAccessibleTextUpdateEvent::textInserted() const
    Returns the inserted text.
*/
/*! \fn QString QAccessibleTextUpdateEvent::textRemoved() const
    Returns the removed text.
*/

/*!
    \class QAccessibleTextSelectionEvent
    \internal
    \ingroup accessibility
    \inmodule QtGui

    \brief The QAccessibleEvent class

    This class is used with \l QAccessible::updateAccessibility().
*/
/*! \fn QAccessibleTextSelectionEvent::QAccessibleTextSelectionEvent(QObject *object, int start, int end)
    Constructs a new QAccessibleTextSelectionEvent for \a object.
    The new selection this event notifies about is from position \a start to \a end.
*/
/*! \fn int QAccessibleTextSelectionEvent::selectionEnd() const
    Returns the position of the last selected character.
*/
/*! \fn int QAccessibleTextSelectionEvent::selectionStart() const
    Returns the position of the first selected character.
*/
/*! \fn void QAccessibleTextSelectionEvent::setSelection(int start, int end)
    Sets the selection for this event from position \a start to \a end.
*/



/*!
    Returns the QAccessibleInterface associated with the event.
    The caller of this function takes ownership of the returned interface.
*/
QAccessibleInterface *QAccessibleEvent::accessibleInterface() const
{
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(m_object);
    if (!iface || !iface->isValid()) {
        static bool hasWarned = false;
        if (!iface && !hasWarned) {
            qWarning() << "Problem creating accessible interface for: " << m_object << endl
                       << "Make sure to deploy Qt with accessibility plugins.";
            hasWarned = true;
        }
        return 0;
    }

    if (m_child >= 0) {
        QAccessibleInterface *child = iface->child(m_child);
        if (child) {
            iface = child;
        } else {
            qWarning() << "Cannot creat accessible child interface for object: " << m_object << " index: " << m_child;
        }
    }
    return iface;
}

/*!
    Returns the window associated with the underlying object.
    For instance, QAccessibleWidget reimplements this and returns
    the windowHandle() of the QWidget.

    It is used on some platforms to be able to notify the AT client about
    state changes.
    The backend will traverse up all ancestors until it finds a window.
    (This means that at least one interface among the ancestors should
    return a valid QWindow pointer).

    The default implementation of this returns 0.
    \internal
  */
QWindow *QAccessibleInterface::window() const
{
    return 0;
}

/*!
    \internal
    Method to allow extending this class without breaking binary compatibility.
    The actual behavior and format of \a data depends on \a id argument
    which must be defined if the class is to be extended with another virtual
    function.
    Currently, this is unused.
*/
void QAccessibleInterface::virtual_hook(int /*id*/, void * /*data*/)
{
}

/*!
    \fn void *QAccessibleInterface::interface_cast(QAccessible::InterfaceType type)

    Returns a specialized accessibility interface \a type from the
    generic QAccessibleInterface.

    This function must be reimplemented when providing more
    information about a widget or object through the specialized
    interfaces. For example a line edit should implement the
    QAccessibleTextInterface and QAccessibleEditableTextInterface.

    Qt's QLineEdit for example has its accessibility support
    implemented in QAccessibleLineEdit.

    \code
void *QAccessibleLineEdit::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    else if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}
    \endcode

    \sa QAccessible::InterfaceType, QAccessibleTextInterface,
    QAccessibleEditableTextInterface, QAccessibleValueInterface,
    QAccessibleActionInterface, QAccessibleTableInterface,
    QAccessibleTableCellInterface
*/

/*! \internal */
const char *qAccessibleRoleString(QAccessible::Role role)
{
    if (role >=0x40)
         role = QAccessible::UserRole;
    static int roleEnum = QAccessible::staticMetaObject.indexOfEnumerator("Role");
    return QAccessible::staticMetaObject.enumerator(roleEnum).valueToKey(role);
}

/*! \internal */
const char *qAccessibleEventString(QAccessible::Event event)
{
    static int eventEnum = QAccessible::staticMetaObject.indexOfEnumerator("Event");
    return QAccessible::staticMetaObject.enumerator(eventEnum).valueToKey(event);
}

/*! \internal */
bool operator==(const QAccessible::State &first, const QAccessible::State &second)
{
    return memcmp(&first, &second, sizeof(QAccessible::State)) == 0;
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug d, const QAccessibleInterface *iface)
{
    if (!iface) {
        d << "QAccessibleInterface(null)";
        return d;
    }
    d.nospace();
    d << "QAccessibleInterface(" << hex << (void *) iface << dec;
    if (iface->isValid()) {
        d << " name=" << iface->text(QAccessible::Name) << " ";
        d << "role=" << qAccessibleRoleString(iface->role()) << " ";
        if (iface->childCount())
            d << "childc=" << iface->childCount() << " ";
        if (iface->object()) {
            d << "obj=" << iface->object();
        }
        QStringList stateStrings;
        QAccessible::State st = iface->state();
        if (st.focusable)
            stateStrings << QLatin1String("focusable");
        if (st.focused)
            stateStrings << QLatin1String("focused");
        if (st.selected)
            stateStrings << QLatin1String("selected");
        if (st.invisible)
            stateStrings << QLatin1String("invisible");

        if (!stateStrings.isEmpty())
            d << stateStrings.join(QLatin1Char('|'));

        if (!st.invisible)
            d << "rect=" << iface->rect();

    } else {
        d << " invalid";
    }
    d << ")";
    return d.space();
}

/*! \internal */
QDebug operator<<(QDebug d, const QAccessibleEvent &ev)
{
    if (!&ev) {
        d << "QAccessibleEvent(null)";
        return d;
    }
    d.nospace() << "QAccessibleEvent(object=" << hex << ev.object();
    d.nospace() << dec;
    d.nospace() << "child=" << ev.child();
    d << " event=" << qAccessibleEventString(ev.type());
    if (ev.type() == QAccessible::StateChanged) {
        QAccessible::State changed = static_cast<const QAccessibleStateChangeEvent*>(&ev)->changedStates();
        d << "State changed:";
        if (changed.disabled) d << "disabled";
        if (changed.selected) d << "selected";
        if (changed.focusable) d << "focusable";
        if (changed.focused) d << "focused";
        if (changed.pressed) d << "pressed";
        if (changed.checkable) d << "checkable";
        if (changed.checked) d << "checked";
        if (changed.checkStateMixed) d << "checkStateMixed";
        if (changed.readOnly) d << "readOnly";
        if (changed.hotTracked) d << "hotTracked";
        if (changed.defaultButton) d << "defaultButton";
        if (changed.expanded) d << "expanded";
        if (changed.collapsed) d << "collapsed";
        if (changed.busy) d << "busy";
        if (changed.expandable) d << "expandable";
        if (changed.marqueed) d << "marqueed";
        if (changed.animated) d << "animated";
        if (changed.invisible) d << "invisible";
        if (changed.offscreen) d << "offscreen";
        if (changed.sizeable) d << "sizeable";
        if (changed.movable) d << "movable";
        if (changed.selfVoicing) d << "selfVoicing";
        if (changed.selectable) d << "selectable";
        if (changed.linked) d << "linked";
        if (changed.traversed) d << "traversed";
        if (changed.multiSelectable) d << "multiSelectable";
        if (changed.extSelectable) d << "extSelectable";
        if (changed.passwordEdit) d << "passwordEdit"; // used to be Protected
        if (changed.hasPopup) d << "hasPopup";
        if (changed.modal) d << "modal";

        // IA2 - we chose to not add some IA2 states for now
        // Below the ones that seem helpful
        if (changed.active) d << "active";
        if (changed.invalid) d << "invalid"; // = defunct
        if (changed.editable) d << "editable";
        if (changed.multiLine) d << "multiLine";
        if (changed.selectableText) d << "selectableText";
        if (changed.supportsAutoCompletion) d << "supportsAutoCompletion";

    }
    d.nospace() << ")";
    return d.space();
}

#endif

QT_END_NAMESPACE

