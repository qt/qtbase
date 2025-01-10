// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qhash.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qapplication.h>
#include <private/qwidget_p.h>
#include <private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformdialoghelper.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/qaction.h>

#include "qdialogbuttonbox.h"
#include "qdialogbuttonbox_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QDialogButtonBox
    \since 4.2
    \brief The QDialogButtonBox class is a widget that presents buttons in a
    layout that is appropriate to the current widget style.

    \ingroup dialog-classes
    \inmodule QtWidgets

    Dialogs and message boxes typically present buttons in a layout that
    conforms to the interface guidelines for that platform. Invariably,
    different platforms have different layouts for their dialogs.
    QDialogButtonBox allows a developer to add buttons to it and will
    automatically use the appropriate layout for the user's desktop
    environment.

    Most buttons for a dialog follow certain roles. Such roles include:

    \list
    \li Accepting or rejecting the dialog.
    \li Asking for help.
    \li Performing actions on the dialog itself (such as resetting fields or
       applying changes).
    \endlist

    There can also be alternate ways of dismissing the dialog which may cause
    destructive results.

    Most dialogs have buttons that can almost be considered standard (e.g.
    \uicontrol OK and \uicontrol Cancel buttons). It is sometimes convenient to create these
    buttons in a standard way.

    There are a couple ways of using QDialogButtonBox. One ways is to create
    the buttons (or button texts) yourself and add them to the button box,
    specifying their role.

    \snippet dialogs/dialogs.cpp buttonbox

    Alternatively, QDialogButtonBox provides several standard buttons (e.g. OK, Cancel, Save)
    that you can use. They exist as flags so you can OR them together in the constructor.

    \snippet dialogs/tabdialog/tabdialog.cpp 2

    You can mix and match normal buttons and standard buttons.

    Currently the buttons are laid out in the following way if the button box is horizontal:
    \table
    \row \li \inlineimage buttonbox-gnomelayout-horizontal.png GnomeLayout Horizontal
         \li Button box laid out in horizontal GnomeLayout
    \row \li \inlineimage buttonbox-kdelayout-horizontal.png KdeLayout Horizontal
         \li Button box laid out in horizontal KdeLayout
    \row \li \inlineimage buttonbox-maclayout-horizontal.png MacLayout Horizontal
         \li Button box laid out in horizontal MacLayout
    \row \li \inlineimage buttonbox-winlayout-horizontal.png  WinLayout Horizontal
         \li Button box laid out in horizontal WinLayout
    \endtable

    The buttons are laid out the following way if the button box is vertical:

    \table
    \row \li GnomeLayout
         \li KdeLayout
         \li MacLayout
         \li WinLayout
    \row \li \inlineimage buttonbox-gnomelayout-vertical.png GnomeLayout Vertical
         \li \inlineimage buttonbox-kdelayout-vertical.png KdeLayout Vertical
         \li \inlineimage buttonbox-maclayout-vertical.png MacLayout Vertical
         \li \inlineimage buttonbox-winlayout-vertical.png WinLayout Vertical
    \endtable

    Additionally, button boxes that contain only buttons with ActionRole or
    HelpRole can be considered modeless and have an alternate look on \macos:

    \table
    \row \li modeless horizontal MacLayout
         \li \inlineimage buttonbox-mac-modeless-horizontal.png Screenshot of modeless horizontal MacLayout
    \row \li modeless vertical MacLayout
         \li \inlineimage buttonbox-mac-modeless-vertical.png Screenshot of modeless vertical MacLayout
    \endtable

    When a button is clicked in the button box, the clicked() signal is emitted
    for the actual button is that is pressed. For convenience, if the button
    has an AcceptRole, RejectRole, or HelpRole, the accepted(), rejected(), or
    helpRequested() signals are emitted respectively.

    If you want a specific button to be default you need to call
    QPushButton::setDefault() on it yourself. However, if there is no default
    button set and to preserve which button is the default button across
    platforms when using the QPushButton::autoDefault property, the first push
    button with the accept role is made the default button when the
    QDialogButtonBox is shown,

    \sa QMessageBox, QPushButton, QDialog
*/
QDialogButtonBoxPrivate::QDialogButtonBoxPrivate(Qt::Orientation orient)
    : orientation(orient), buttonLayout(nullptr), center(false)
{
    struct EventFilter : public QObject
    {
        EventFilter(QDialogButtonBoxPrivate *d) : d(d) {};

        bool eventFilter(QObject *obj, QEvent *event) override
        {
            QAbstractButton *button = qobject_cast<QAbstractButton *>(obj);
            return button ? d->handleButtonShowAndHide(button, event) : false;
        }

    private:
        QDialogButtonBoxPrivate *d;

    };

    filter.reset(new EventFilter(this));
}

void QDialogButtonBoxPrivate::initLayout()
{
    Q_Q(QDialogButtonBox);
    layoutPolicy = QDialogButtonBox::ButtonLayout(q->style()->styleHint(QStyle::SH_DialogButtonLayout, nullptr, q));
    bool createNewLayout = buttonLayout == nullptr
        || (orientation == Qt::Horizontal && qobject_cast<QVBoxLayout *>(buttonLayout) != 0)
        || (orientation == Qt::Vertical && qobject_cast<QHBoxLayout *>(buttonLayout) != 0);
    if (createNewLayout) {
        delete buttonLayout;
        if (orientation == Qt::Horizontal)
            buttonLayout = new QHBoxLayout(q);
        else
            buttonLayout = new QVBoxLayout(q);
    }

    int left, top, right, bottom;
    setLayoutItemMargins(QStyle::SE_PushButtonLayoutItem);
    getLayoutItemMargins(&left, &top, &right, &bottom);
    buttonLayout->setContentsMargins(-left, -top, -right, -bottom);

    if (!q->testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::ButtonBox);
        if (orientation == Qt::Vertical)
            sp.transpose();
        q->setSizePolicy(sp);
        q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
}

void QDialogButtonBoxPrivate::resetLayout()
{
    initLayout();
    layoutButtons();
}

void QDialogButtonBoxPrivate::addButtonsToLayout(const QList<QAbstractButton *> &buttonList,
                                                 bool reverse)
{
    int start = reverse ? buttonList.size() - 1 : 0;
    int end = reverse ? -1 : buttonList.size();
    int step = reverse ? -1 : 1;

    for (int i = start; i != end; i += step) {
        QAbstractButton *button = buttonList.at(i);
        buttonLayout->addWidget(button);
        button->show();
    }
}

void QDialogButtonBoxPrivate::layoutButtons()
{
    Q_Q(QDialogButtonBox);
    const int MacGap = 36 - 8;    // 8 is the default gap between a widget and a spacer item

    QBoolBlocker blocker(ignoreShowAndHide);
    for (int i = buttonLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = buttonLayout->takeAt(i);
        if (QWidget *widget = item->widget())
            widget->hide();
        delete item;
    }

    int tmpPolicy = layoutPolicy;

    static const int M = 5;
    static const int ModalRoles[M] = { QPlatformDialogHelper::AcceptRole, QPlatformDialogHelper::RejectRole,
        QPlatformDialogHelper::DestructiveRole, QPlatformDialogHelper::YesRole, QPlatformDialogHelper::NoRole };
    if (tmpPolicy == QDialogButtonBox::MacLayout) {
        bool hasModalButton = false;
        for (int i = 0; i < M; ++i) {
            if (!buttonLists[ModalRoles[i]].isEmpty()) {
                hasModalButton = true;
                break;
            }
        }
        if (!hasModalButton)
            tmpPolicy = 4;  // Mac modeless
    }

    const int *currentLayout = QPlatformDialogHelper::buttonLayout(
        orientation, static_cast<QPlatformDialogHelper::ButtonLayout>(tmpPolicy));

    if (center)
        buttonLayout->addStretch();

    const QList<QAbstractButton *> &acceptRoleList = buttonLists[QPlatformDialogHelper::AcceptRole];

    while (*currentLayout != QPlatformDialogHelper::EOL) {
        int role = (*currentLayout & ~QPlatformDialogHelper::Reverse);
        bool reverse = (*currentLayout & QPlatformDialogHelper::Reverse);

        switch (role) {
        case QPlatformDialogHelper::Stretch:
            if (!center)
                buttonLayout->addStretch();
            break;
        case QPlatformDialogHelper::AcceptRole: {
            if (acceptRoleList.isEmpty())
                break;
            // Only the first one
            QAbstractButton *button = acceptRoleList.first();
            buttonLayout->addWidget(button);
            button->show();
        }
            break;
        case QPlatformDialogHelper::AlternateRole:
            if (acceptRoleList.size() > 1)
                addButtonsToLayout(acceptRoleList.mid(1), reverse);
            break;
        case QPlatformDialogHelper::DestructiveRole:
            {
                const QList<QAbstractButton *> &list = buttonLists[role];

                /*
                    Mac: Insert a gap on the left of the destructive
                    buttons to ensure that they don't get too close to
                    the help and action buttons (but only if there are
                    some buttons to the left of the destructive buttons
                    (and the stretch, whence buttonLayout->count() > 1
                    and not 0)).
                */
                if (tmpPolicy == QDialogButtonBox::MacLayout
                        && !list.isEmpty() && buttonLayout->count() > 1)
                    buttonLayout->addSpacing(MacGap);

                addButtonsToLayout(list, reverse);

                /*
                    Insert a gap between the destructive buttons and the
                    accept and reject buttons.
                */
                if (tmpPolicy == QDialogButtonBox::MacLayout && !list.isEmpty())
                    buttonLayout->addSpacing(MacGap);
            }
            break;
        case QPlatformDialogHelper::RejectRole:
        case QPlatformDialogHelper::ActionRole:
        case QPlatformDialogHelper::HelpRole:
        case QPlatformDialogHelper::YesRole:
        case QPlatformDialogHelper::NoRole:
        case QPlatformDialogHelper::ApplyRole:
        case QPlatformDialogHelper::ResetRole:
            addButtonsToLayout(buttonLists[role], reverse);
        }
        ++currentLayout;
    }

    QWidget *lastWidget = nullptr;
    q->setFocusProxy(nullptr);
    for (int i = 0; i < buttonLayout->count(); ++i) {
        QLayoutItem *item = buttonLayout->itemAt(i);
        if (QWidget *widget = item->widget()) {
            if (lastWidget)
                QWidget::setTabOrder(lastWidget, widget);
            else
                q->setFocusProxy(widget);
            lastWidget = widget;
        }
    }

    if (center)
        buttonLayout->addStretch();
}

QPushButton *QDialogButtonBoxPrivate::createButton(QDialogButtonBox::StandardButton sbutton,
                                                   LayoutRule layoutRule)
{
    Q_Q(QDialogButtonBox);
    int icon = 0;

    switch (sbutton) {
    case QDialogButtonBox::Ok:
        icon = QStyle::SP_DialogOkButton;
        break;
    case QDialogButtonBox::Save:
        icon = QStyle::SP_DialogSaveButton;
        break;
    case QDialogButtonBox::Open:
        icon = QStyle::SP_DialogOpenButton;
        break;
    case QDialogButtonBox::Cancel:
        icon = QStyle::SP_DialogCancelButton;
        break;
    case QDialogButtonBox::Close:
        icon = QStyle::SP_DialogCloseButton;
        break;
    case QDialogButtonBox::Apply:
        icon = QStyle::SP_DialogApplyButton;
        break;
    case QDialogButtonBox::Reset:
        icon = QStyle::SP_DialogResetButton;
        break;
    case QDialogButtonBox::Help:
        icon = QStyle::SP_DialogHelpButton;
        break;
    case QDialogButtonBox::Discard:
        icon = QStyle::SP_DialogDiscardButton;
        break;
    case QDialogButtonBox::Yes:
        icon = QStyle::SP_DialogYesButton;
        break;
    case QDialogButtonBox::No:
        icon = QStyle::SP_DialogNoButton;
        break;
    case QDialogButtonBox::YesToAll:
        icon = QStyle::SP_DialogYesToAllButton;
        break;
    case QDialogButtonBox::NoToAll:
        icon = QStyle::SP_DialogNoToAllButton;
        break;
    case QDialogButtonBox::SaveAll:
        icon = QStyle::SP_DialogSaveAllButton;
        break;
    case QDialogButtonBox::Abort:
        icon = QStyle::SP_DialogAbortButton;
        break;
    case QDialogButtonBox::Retry:
        icon = QStyle::SP_DialogRetryButton;
        break;
    case QDialogButtonBox::Ignore:
        icon = QStyle::SP_DialogIgnoreButton;
        break;
    case QDialogButtonBox::RestoreDefaults:
        icon = QStyle::SP_RestoreDefaultsButton;
        break;
    case QDialogButtonBox::NoButton:
        return nullptr;
        ;
    }
    QPushButton *button = new QPushButton(QGuiApplicationPrivate::platformTheme()->standardButtonText(sbutton), q);
    QStyle *style = q->style();
    if (style->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons, nullptr, q) && icon != 0)
        button->setIcon(style->standardIcon(QStyle::StandardPixmap(icon), nullptr, q));
    if (style != QApplication::style()) // Propagate style
        button->setStyle(style);
    standardButtonHash.insert(button, sbutton);
    QPlatformDialogHelper::ButtonRole role = QPlatformDialogHelper::buttonRole(static_cast<QPlatformDialogHelper::StandardButton>(sbutton));
    if (Q_UNLIKELY(role == QPlatformDialogHelper::InvalidRole))
        qWarning("QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    else
        addButton(button, static_cast<QDialogButtonBox::ButtonRole>(role), layoutRule);
#if QT_CONFIG(shortcut)
    const QKeySequence standardShortcut = QGuiApplicationPrivate::platformTheme()->standardButtonShortcut(sbutton);
    if (!standardShortcut.isEmpty())
        button->setShortcut(standardShortcut);
#endif
    return button;
}

void QDialogButtonBoxPrivate::addButton(QAbstractButton *button, QDialogButtonBox::ButtonRole role,
                                        LayoutRule layoutRule, AddRule addRule)
{
    buttonLists[role].append(button);
    switch (addRule) {
    case AddRule::Connect:
        QObjectPrivate::connect(button, &QAbstractButton::clicked,
                               this, &QDialogButtonBoxPrivate::handleButtonClicked);
        QObjectPrivate::connect(button, &QAbstractButton::destroyed,
                               this, &QDialogButtonBoxPrivate::handleButtonDestroyed);
        button->installEventFilter(filter.get());
        break;
    case AddRule::SkipConnect:
        break;
    }

    switch (layoutRule) {
    case LayoutRule::DoLayout:
        layoutButtons();
        break;
    case LayoutRule::SkipLayout:
        break;
    }
}

void QDialogButtonBoxPrivate::createStandardButtons(QDialogButtonBox::StandardButtons buttons)
{
    uint i = QDialogButtonBox::FirstButton;
    while (i <= QDialogButtonBox::LastButton) {
        if (i & buttons)
            createButton(QDialogButtonBox::StandardButton(i), LayoutRule::SkipLayout);
        i = i << 1;
    }
    layoutButtons();
}

void QDialogButtonBoxPrivate::retranslateStrings()
{
    for (auto &&[key, value] : std::as_const(standardButtonHash).asKeyValueRange()) {
        const QString text = QGuiApplicationPrivate::platformTheme()->standardButtonText(value);
        if (!text.isEmpty())
            key->setText(text);
    }
}

/*!
    Constructs an empty, horizontal button box with the given \a parent.

    \sa orientation, addButton()
*/
QDialogButtonBox::QDialogButtonBox(QWidget *parent)
    : QDialogButtonBox(Qt::Horizontal, parent)
{
}

/*!
    Constructs an empty button box with the given \a orientation and \a parent.

    \sa orientation, addButton()
*/
QDialogButtonBox::QDialogButtonBox(Qt::Orientation orientation, QWidget *parent)
    : QWidget(*new QDialogButtonBoxPrivate(orientation), parent, { })
{
    d_func()->initLayout();
}

/*!
    \since 5.2

    Constructs a horizontal button box with the given \a parent, containing
    the standard buttons specified by \a buttons.

    \sa orientation, addButton()
*/
QDialogButtonBox::QDialogButtonBox(StandardButtons buttons, QWidget *parent)
    : QDialogButtonBox(buttons, Qt::Horizontal, parent)
{
}

/*!
    Constructs a button box with the given \a orientation and \a parent, containing
    the standard buttons specified by \a buttons.

    \sa orientation, addButton()
*/
QDialogButtonBox::QDialogButtonBox(StandardButtons buttons, Qt::Orientation orientation,
                                   QWidget *parent)
    : QDialogButtonBox(orientation, parent)
{
    d_func()->createStandardButtons(buttons);
}

/*!
    Destroys the button box.
*/
QDialogButtonBox::~QDialogButtonBox()
{
    Q_D(QDialogButtonBox);

    d->ignoreShowAndHide = true;

    // QObjectPrivate::connect requires explicit disconnect in destructor
    // otherwise the connection may kick in on child destruction and reach
    // the parent's destroyed private object
    d->disconnectAll();
}

/*!
    \enum QDialogButtonBox::ButtonRole

//! [buttonrole-enum]
    This enum describes the roles that can be used to describe buttons in
    the button box. Combinations of these roles are as flags used to
    describe different aspects of their behavior.

    \value InvalidRole The button is invalid.
    \value AcceptRole Clicking the button causes the dialog to be accepted
           (e.g. OK).
    \value RejectRole Clicking the button causes the dialog to be rejected
           (e.g. Cancel).
    \value DestructiveRole Clicking the button causes a destructive change
           (e.g. for Discarding Changes) and closes the dialog.
    \value ActionRole Clicking the button causes changes to the elements within
           the dialog.
    \value HelpRole The button can be clicked to request help.
    \value YesRole The button is a "Yes"-like button.
    \value NoRole The button is a "No"-like button.
    \value ApplyRole The button applies current changes.
    \value ResetRole The button resets the dialog's fields to default values.

    \omitvalue NRoles

    \sa StandardButton
//! [buttonrole-enum]
*/

/*!
    \enum QDialogButtonBox::StandardButton

    These enums describe flags for standard buttons. Each button has a
    defined \l ButtonRole.

    \value Ok An "OK" button defined with the \l AcceptRole.
    \value Open An "Open" button defined with the \l AcceptRole.
    \value Save A "Save" button defined with the \l AcceptRole.
    \value Cancel A "Cancel" button defined with the \l RejectRole.
    \value Close A "Close" button defined with the \l RejectRole.
    \value Discard A "Discard" or "Don't Save" button, depending on the platform,
                    defined with the \l DestructiveRole.
    \value Apply An "Apply" button defined with the \l ApplyRole.
    \value Reset A "Reset" button defined with the \l ResetRole.
    \value RestoreDefaults A "Restore Defaults" button defined with the \l ResetRole.
    \value Help A "Help" button defined with the \l HelpRole.
    \value SaveAll A "Save All" button defined with the \l AcceptRole.
    \value Yes A "Yes" button defined with the \l YesRole.
    \value YesToAll A "Yes to All" button defined with the \l YesRole.
    \value No A "No" button defined with the \l NoRole.
    \value NoToAll A "No to All" button defined with the \l NoRole.
    \value Abort An "Abort" button defined with the \l RejectRole.
    \value Retry A "Retry" button defined with the \l AcceptRole.
    \value Ignore An "Ignore" button defined with the \l AcceptRole.

    \value NoButton An invalid button.

    \omitvalue FirstButton
    \omitvalue LastButton

    \sa ButtonRole, standardButtons
*/

/*!
    \enum QDialogButtonBox::ButtonLayout

    This enum describes the layout policy to be used when arranging the buttons
    contained in the button box.

    \value WinLayout Use a policy appropriate for applications on Windows.
    \value MacLayout Use a policy appropriate for applications on \macos.
    \value KdeLayout Use a policy appropriate for applications on KDE.
    \value GnomeLayout Use a policy appropriate for applications on GNOME.
    \value AndroidLayout Use a policy appropriate for applications on Android.
                            This enum value was added in Qt 5.10.

    The button layout is specified by the \l{style()}{current style}. However,
    on the X11 platform, it may be influenced by the desktop environment.
*/

/*!
    \fn void QDialogButtonBox::clicked(QAbstractButton *button)

    This signal is emitted when a button inside the button box is clicked. The
    specific button that was pressed is specified by \a button.

    \sa accepted(), rejected(), helpRequested()
*/

/*!
    \fn void QDialogButtonBox::accepted()

    This signal is emitted when a button inside the button box is clicked, as long
    as it was defined with the \l AcceptRole or \l YesRole.

    \sa rejected(), clicked(), helpRequested()
*/

/*!
    \fn void QDialogButtonBox::rejected()

    This signal is emitted when a button inside the button box is clicked, as long
    as it was defined with the \l RejectRole or \l NoRole.

    \sa accepted(), helpRequested(), clicked()
*/

/*!
    \fn void QDialogButtonBox::helpRequested()

    This signal is emitted when a button inside the button box is clicked, as long
    as it was defined with the \l HelpRole.

    \sa accepted(), rejected(), clicked()
*/

/*!
    \property QDialogButtonBox::orientation
    \brief the orientation of the button box

    By default, the orientation is horizontal (i.e. the buttons are laid out
    side by side). The possible orientations are Qt::Horizontal and
    Qt::Vertical.
*/
Qt::Orientation QDialogButtonBox::orientation() const
{
    return d_func()->orientation;
}

void QDialogButtonBox::setOrientation(Qt::Orientation orientation)
{
    Q_D(QDialogButtonBox);
    if (orientation == d->orientation)
        return;

    d->orientation = orientation;
    d->resetLayout();
}

/*!
    Clears the button box, deleting all buttons within it.

    \sa removeButton(), addButton()
*/
void QDialogButtonBox::clear()
{
    Q_D(QDialogButtonBox);
    // Remove the created standard buttons, they should be in the other lists, which will
    // do the deletion
    d->standardButtonHash.clear();
    for (int i = 0; i < NRoles; ++i) {
        QList<QAbstractButton *> &list = d->buttonLists[i];
        while (list.size()) {
            QAbstractButton *button = list.takeAt(0);
            QObjectPrivate::disconnect(button, &QAbstractButton::destroyed,
                                       d, &QDialogButtonBoxPrivate::handleButtonDestroyed);
            delete button;
        }
    }
}

/*!
    Returns a list of all buttons that have been added to the button box.

    \sa buttonRole(), addButton(), removeButton()
*/
QList<QAbstractButton *> QDialogButtonBox::buttons() const
{
    Q_D(const QDialogButtonBox);
    return d->allButtons();
}

QList<QAbstractButton *> QDialogButtonBoxPrivate::visibleButtons() const
{
    QList<QAbstractButton *> finalList;
    for (int i = 0; i < QDialogButtonBox::NRoles; ++i) {
        const QList<QAbstractButton *> &list = buttonLists[i];
        for (int j = 0; j < list.size(); ++j)
            finalList.append(list.at(j));
    }
    return finalList;
}

QList<QAbstractButton *> QDialogButtonBoxPrivate::allButtons() const
{
    return visibleButtons() << hiddenButtons.keys();
}

/*!
    Returns the button role for the specified \a button. This function returns
    \l InvalidRole if \a button is \nullptr or has not been added to the button box.

    \sa buttons(), addButton()
*/
QDialogButtonBox::ButtonRole QDialogButtonBox::buttonRole(QAbstractButton *button) const
{
    Q_D(const QDialogButtonBox);
    for (int i = 0; i < NRoles; ++i) {
        const QList<QAbstractButton *> &list = d->buttonLists[i];
        for (int j = 0; j < list.size(); ++j) {
            if (list.at(j) == button)
                return ButtonRole(i);
        }
    }
    return d->hiddenButtons.value(button, InvalidRole);
}

/*!
    Removes \a button from the button box without deleting it and sets its parent to zero.

    \sa clear(), buttons(), addButton()
*/
void QDialogButtonBox::removeButton(QAbstractButton *button)
{
    Q_D(QDialogButtonBox);
    d->removeButton(button, QDialogButtonBoxPrivate::RemoveReason::ManualRemove);
}

/*!
   \internal
   Removes \param button.
   \param reason determines the behavior following the removal:
   \list
   \li \c ManualRemove disconnects all signals and removes the button from standardButtonHash.
   \li \c HideEvent keeps connections alive, standard buttons remain in standardButtonHash.
   \li \c Destroyed removes the button from standardButtonHash. Signals remain untouched, because
          the button might already be only a QObject, the destructor of which handles disconnecting.
   \endlist
 */
void QDialogButtonBoxPrivate::removeButton(QAbstractButton *button, RemoveReason reason)
{
    if (!button)
        return;

    // Remove button from hidden buttons and roles
    hiddenButtons.remove(button);
    for (int i = 0; i < QDialogButtonBox::NRoles; ++i)
        buttonLists[i].removeOne(button);

    switch (reason) {
    case RemoveReason::ManualRemove:
        button->setParent(nullptr);
        QObjectPrivate::disconnect(button, &QAbstractButton::clicked,
                                   this, &QDialogButtonBoxPrivate::handleButtonClicked);
        QObjectPrivate::disconnect(button, &QAbstractButton::destroyed,
                                   this, &QDialogButtonBoxPrivate::handleButtonDestroyed);
        button->removeEventFilter(filter.get());
        Q_FALLTHROUGH();
    case RemoveReason::Destroyed:
        standardButtonHash.remove(reinterpret_cast<QPushButton *>(button));
        break;
    case RemoveReason::HideEvent:
        break;
    }
}

/*!
    Adds the given \a button to the button box with the specified \a role.
    If the role is invalid, the button is not added.

    If the button has already been added, it is removed and added again with the
    new role.

    \note The button box takes ownership of the button.

    \sa removeButton(), clear()
*/
void QDialogButtonBox::addButton(QAbstractButton *button, ButtonRole role)
{
    Q_D(QDialogButtonBox);
    if (Q_UNLIKELY(role <= InvalidRole || role >= NRoles)) {
        qWarning("QDialogButtonBox::addButton: Invalid ButtonRole, button not added");
        return;
    }
    removeButton(button);
    button->setParent(this);
    d->addButton(button, role);
}

/*!
    Creates a push button with the given \a text, adds it to the button box for the
    specified \a role, and returns the corresponding push button. If \a role is
    invalid, no button is created, and zero is returned.

    \sa removeButton(), clear()
*/
QPushButton *QDialogButtonBox::addButton(const QString &text, ButtonRole role)
{
    Q_D(QDialogButtonBox);
    if (Q_UNLIKELY(role <= InvalidRole || role >= NRoles)) {
        qWarning("QDialogButtonBox::addButton: Invalid ButtonRole, button not added");
        return nullptr;
    }
    QPushButton *button = new QPushButton(text, this);
    d->addButton(button, role);
    return button;
}

/*!
    Adds a standard \a button to the button box if it is valid to do so, and returns
    a push button. If \a button is invalid, it is not added to the button box, and
    zero is returned.

    \sa removeButton(), clear()
*/
QPushButton *QDialogButtonBox::addButton(StandardButton button)
{
    Q_D(QDialogButtonBox);
    return d->createButton(button);
}

/*!
    \property QDialogButtonBox::standardButtons
    \brief collection of standard buttons in the button box

    This property controls which standard buttons are used by the button box.

    \sa addButton()
*/
void QDialogButtonBox::setStandardButtons(StandardButtons buttons)
{
    Q_D(QDialogButtonBox);
    // Clear out all the old standard buttons, then recreate them.
    qDeleteAll(d->standardButtonHash.keyBegin(), d->standardButtonHash.keyEnd());
    d->standardButtonHash.clear();

    d->createStandardButtons(buttons);
}

QDialogButtonBox::StandardButtons QDialogButtonBox::standardButtons() const
{
    Q_D(const QDialogButtonBox);
    StandardButtons standardButtons = NoButton;
    QHash<QPushButton *, StandardButton>::const_iterator it = d->standardButtonHash.constBegin();
    while (it != d->standardButtonHash.constEnd()) {
        standardButtons |= it.value();
        ++it;
    }
    return standardButtons;
}

/*!
    Returns the QPushButton corresponding to the standard button \a which,
    or \nullptr if the standard button doesn't exist in this button box.

    \sa standardButton(), standardButtons(), buttons()
*/
QPushButton *QDialogButtonBox::button(StandardButton which) const
{
    Q_D(const QDialogButtonBox);
    return d->standardButtonHash.key(which);
}

/*!
    Returns the standard button enum value corresponding to the given \a button,
    or NoButton if the given \a button isn't a standard button.

    \sa button(), buttons(), standardButtons()
*/
QDialogButtonBox::StandardButton QDialogButtonBox::standardButton(QAbstractButton *button) const
{
    Q_D(const QDialogButtonBox);
    return d->standardButtonHash.value(static_cast<QPushButton *>(button));
}

void QDialogButtonBoxPrivate::handleButtonClicked()
{
    Q_Q(QDialogButtonBox);
    if (QAbstractButton *button = qobject_cast<QAbstractButton *>(q->sender())) {
        // Can't fetch this *after* emitting clicked, as clicked may destroy the button
        // or change its role. Now changing the role is not possible yet, but arguably
        // both clicked and accepted/rejected/etc. should be emitted "atomically"
        // depending on whatever role the button had at the time of the click.
        const QDialogButtonBox::ButtonRole buttonRole = q->buttonRole(button);
        QPointer<QDialogButtonBox> guard(q);

        emit q->clicked(button);

        if (!guard)
            return;

        switch (QPlatformDialogHelper::ButtonRole(buttonRole)) {
        case QPlatformDialogHelper::AcceptRole:
        case QPlatformDialogHelper::YesRole:
            emit q->accepted();
            break;
        case QPlatformDialogHelper::RejectRole:
        case QPlatformDialogHelper::NoRole:
            emit q->rejected();
            break;
        case QPlatformDialogHelper::HelpRole:
            emit q->helpRequested();
            break;
        default:
            break;
        }
    }
}

void QDialogButtonBoxPrivate::handleButtonDestroyed()
{
    Q_Q(QDialogButtonBox);
    if (QObject *object = q->sender())
        removeButton(reinterpret_cast<QAbstractButton *>(object), RemoveReason::Destroyed);
}

bool QDialogButtonBoxPrivate::handleButtonShowAndHide(QAbstractButton *button, QEvent *event)
{
    Q_Q(QDialogButtonBox);

    const QEvent::Type type = event->type();

    if ((type != QEvent::HideToParent && type != QEvent::ShowToParent) || ignoreShowAndHide)
        return false;

    switch (type) {
    case QEvent::HideToParent: {
        const QDialogButtonBox::ButtonRole role = q->buttonRole(button);
        if (role != QDialogButtonBox::ButtonRole::InvalidRole) {
            removeButton(button, RemoveReason::HideEvent);
            hiddenButtons.insert(button, role);
            layoutButtons();
        }
        break;
    }
    case QEvent::ShowToParent:
        if (hiddenButtons.contains(button)) {
            const auto role = hiddenButtons.take(button);
            addButton(button, role, LayoutRule::DoLayout, AddRule::SkipConnect);
            if (role == QDialogButtonBox::AcceptRole)
                ensureFirstAcceptIsDefault();
        }
        break;
    default: break;
    }

    return false;
}

/*!
    \property QDialogButtonBox::centerButtons
    \brief whether the buttons in the button box are centered

    By default, this property is \c false. This behavior is appropriate
    for most types of dialogs. A notable exception is message boxes
    on most platforms (e.g. Windows), where the button box is
    centered horizontally.

    \sa QMessageBox
*/
void QDialogButtonBox::setCenterButtons(bool center)
{
    Q_D(QDialogButtonBox);
    if (d->center != center) {
        d->center = center;
        d->resetLayout();
    }
}

bool QDialogButtonBox::centerButtons() const
{
    Q_D(const QDialogButtonBox);
    return d->center;
}

/*!
    \reimp
*/
void QDialogButtonBox::changeEvent(QEvent *event)
{
    typedef QHash<QPushButton *, QDialogButtonBox::StandardButton> StandardButtonHash;

    Q_D(QDialogButtonBox);
    switch (event->type()) {
    case QEvent::StyleChange:  // Propagate style
        if (!d->standardButtonHash.empty()) {
            QStyle *newStyle = style();
            const StandardButtonHash::iterator end = d->standardButtonHash.end();
            for (StandardButtonHash::iterator it = d->standardButtonHash.begin(); it != end; ++it)
                it.key()->setStyle(newStyle);
        }
#ifdef Q_OS_MAC
        Q_FALLTHROUGH();
    case QEvent::MacSizeChange:
#endif
        d->resetLayout();
        QWidget::changeEvent(event);
        break;
    default:
        QWidget::changeEvent(event);
        break;
    }
}

void QDialogButtonBoxPrivate::ensureFirstAcceptIsDefault()
{
    Q_Q(QDialogButtonBox);
    const QList<QAbstractButton *> &acceptRoleList = buttonLists[QDialogButtonBox::AcceptRole];
    QPushButton *firstAcceptButton = acceptRoleList.isEmpty()
                                   ? nullptr
                                   : qobject_cast<QPushButton *>(acceptRoleList.at(0));

    if (!firstAcceptButton)
        return;

    bool hasDefault = false;
    QWidget *dialog = nullptr;
    QWidget *p = q;
    while (p && !p->isWindow()) {
        p = p->parentWidget();
        if ((dialog = qobject_cast<QDialog *>(p)))
            break;
    }

    QWidget *parent = dialog ? dialog : q;
    Q_ASSERT(parent);

    const auto pushButtons = parent->findChildren<QPushButton *>();
    for (QPushButton *pushButton : pushButtons) {
        if (pushButton->isDefault() && pushButton != firstAcceptButton) {
            hasDefault = true;
            break;
        }
    }
    if (!hasDefault && firstAcceptButton) {
        firstAcceptButton->setDefault(true);
        // When the QDialogButtonBox is focused, and it doesn't have an
        // explicit focus widget, it will transfer focus to its focus
        // proxy, which is the first button in the layout. This behavior,
        // combined with the behavior that QPushButtons in a QDialog will
        // by default have their autoDefault set to true, results in the
        // focus proxy/first button stealing the default button status
        // immediately when the button box is focused, which is not what
        // we want. Account for this by explicitly making the firstAcceptButton
        // focused as well, unless an explicit focus widget has been set.
        if (dialog && !dialog->focusWidget())
            firstAcceptButton->setFocus();
    }
}

void QDialogButtonBoxPrivate::disconnectAll()
{
    Q_Q(QDialogButtonBox);
    const auto buttons = q->findChildren<QAbstractButton *>();
    for (auto *button : buttons)
        button->disconnect(q);
}

/*!
    \reimp
*/
bool QDialogButtonBox::event(QEvent *event)
{
    Q_D(QDialogButtonBox);
    switch (event->type()) {
    case QEvent::Show:
        d->ensureFirstAcceptIsDefault();
        break;

    case QEvent::LanguageChange:
        d->retranslateStrings();
        break;

    default: break;
    }

    return QWidget::event(event);
}

QT_END_NAMESPACE

#include "moc_qdialogbuttonbox.cpp"
