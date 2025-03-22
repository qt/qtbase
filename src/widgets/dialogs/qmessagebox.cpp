// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/qmessagebox.h>

#include <QtWidgets/qdialogbuttonbox.h>
#include "private/qlabel_p.h"
#include "private/qapplication_p.h"
#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qcheckbox.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qicon.h>
#include <QtGui/qtextdocument.h>
#include <QtWidgets/qapplication.h>
#if QT_CONFIG(textedit)
#include <QtWidgets/qtextedit.h>
#endif
#if QT_CONFIG(menu)
#include <QtWidgets/qmenu.h>
#endif
#include "qdialog_p.h"
#include <QtGui/qfont.h>
#include <QtGui/qfontmetrics.h>
#include <QtGui/qclipboard.h>
#include "private/qabstractbutton_p.h"
#include <QtGui/qpa/qplatformtheme.h>

#include <QtCore/qanystringview.h>
#include <QtCore/qdebug.h>
#include <QtCore/qpointer.h>
#include <QtCore/qversionnumber.h>

#ifdef Q_OS_WIN
#    include <QtCore/qt_windows.h>
#include <qpa/qplatformnativeinterface.h>
#endif

#include <optional>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if defined(Q_OS_WIN)
HMENU qt_getWindowsSystemMenu(const QWidget *w)
{
    if (QWindow *window = QApplicationPrivate::windowForWidget(w))
        if (void *handle = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", window))
            return GetSystemMenu(reinterpret_cast<HWND>(handle), false);
    return 0;
}
#endif

static_assert(qToUnderlying(QMessageBox::ButtonRole::NRoles) ==
              qToUnderlying(QDialogButtonBox::ButtonRole::NRoles),
              "QMessageBox::ButtonRole and QDialogButtonBox::ButtonRole out of sync!");

static_assert(std::is_same_v<std::underlying_type_t<QMessageBox::ButtonRole>,
                             std::underlying_type_t<QDialogButtonBox::ButtonRole>>);

// StandardButton enums have different underlying types
// => qToUnderlying and std::is_same_v won't work
// ### Qt 7: make them have the same underlying type
static_assert(static_cast<int>(QMessageBox::StandardButton::LastButton) ==
              static_cast<int>(QDialogButtonBox::StandardButton::LastButton),
              "QMessageBox::StandardButton and QDialogButtonBox::StandardButton out of sync!");

enum Button { Old_Ok = 1, Old_Cancel = 2, Old_Yes = 3, Old_No = 4, Old_Abort = 5, Old_Retry = 6,
              Old_Ignore = 7, Old_YesAll = 8, Old_NoAll = 9, Old_ButtonMask = 0xFF,
              NewButtonMask = 0xFFFFFC00 };

enum DetailButtonLabel { ShowLabel = 0, HideLabel = 1 };
#if QT_CONFIG(textedit)
class QMessageBoxDetailsText : public QWidget
{
    Q_OBJECT
public:
    class TextEdit : public QTextEdit
    {
    public:
        TextEdit(QWidget *parent=nullptr) : QTextEdit(parent) { }
#ifndef QT_NO_CONTEXTMENU
        void contextMenuEvent(QContextMenuEvent * e) override
        {
            if (QMenu *menu = createStandardContextMenu()) {
                menu->setAttribute(Qt::WA_DeleteOnClose);
                menu->popup(e->globalPos());
            }
        }
#endif // QT_NO_CONTEXTMENU
    };

    QMessageBoxDetailsText(QWidget *parent=nullptr)
        : QWidget(parent)
        , copyAvailable(false)
    {
        QVBoxLayout *layout = new QVBoxLayout;
        layout->setContentsMargins(QMargins());
        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);
        textEdit = new TextEdit();
        textEdit->setFixedHeight(100);
        textEdit->setFocusPolicy(Qt::NoFocus);
        textEdit->setReadOnly(true);
        layout->addWidget(textEdit);
        setLayout(layout);

        connect(textEdit, &TextEdit::copyAvailable,
                this, &QMessageBoxDetailsText::textCopyAvailable);
    }
    void setText(const QString &text) { textEdit->setPlainText(text); }
    QString text() const { return textEdit->toPlainText(); }

    bool copy()
    {
#ifdef QT_NO_CLIPBOARD
        return false;
#else
        if (!copyAvailable)
            return false;
        textEdit->copy();
        return true;
#endif
    }

    void selectAll()
    {
        textEdit->selectAll();
    }

private slots:
    void textCopyAvailable(bool available)
    {
        copyAvailable = available;
    }

private:
    bool copyAvailable;
    TextEdit *textEdit;
};
#endif // QT_CONFIG(textedit)

class DetailButton : public QPushButton
{
public:
    DetailButton(QWidget *parent) : QPushButton(label(ShowLabel), parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    QString label(DetailButtonLabel label) const
    { return label == ShowLabel ? QMessageBox::tr("Show Details...") : QMessageBox::tr("Hide Details..."); }

    void setLabel(DetailButtonLabel lbl)
    { setText(label(lbl)); }

    QSize sizeHint() const override
    {
        ensurePolished();
        QStyleOptionButton opt;
        initStyleOption(&opt);
        const QFontMetrics fm = fontMetrics();
        opt.text = label(ShowLabel);
        QSize sz = fm.size(Qt::TextShowMnemonic, opt.text);
        QSize ret = style()->sizeFromContents(QStyle::CT_PushButton, &opt, sz, this);
        opt.text = label(HideLabel);
        sz = fm.size(Qt::TextShowMnemonic, opt.text);
        ret = ret.expandedTo(style()->sizeFromContents(QStyle::CT_PushButton, &opt, sz, this));
        return ret;
    }
};

class QMessageBoxPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QMessageBox)

public:
    QMessageBoxPrivate() : escapeButton(nullptr), defaultButton(nullptr), checkbox(nullptr), clickedButton(nullptr), detailsButton(nullptr),
#if QT_CONFIG(textedit)
                           detailsText(nullptr),
#endif
                           compatMode(false), autoAddOkButton(true),
                           detectedEscapeButton(nullptr), informativeLabel(nullptr),
                           options(QMessageDialogOptions::create()) { }

    void init(const QString &title = QString(), const QString &text = QString());
    void setupLayout();
    void buttonClicked(QAbstractButton *);
    void helperClicked(QPlatformDialogHelper::StandardButton button, QPlatformDialogHelper::ButtonRole role);
    void setClickedButton(QAbstractButton *button);

    QAbstractButton *findButton(int button0, int button1, int button2, int flags);
    void addOldButtons(int button0, int button1, int button2);

    QAbstractButton *abstractButtonForId(int id) const;
    int execReturnCode(QAbstractButton *button);

    void detectEscapeButton();
    void updateSize();
    int layoutMinimumWidth();
    void retranslateStrings();

    void setVisible(bool visible) override;
    bool canBeNativeDialog() const override;

    static int showOldMessageBox(QWidget *parent, QMessageBox::Icon icon,
                                 const QString &title, const QString &text,
                                 int button0, int button1, int button2);
    static int showOldMessageBox(QWidget *parent, QMessageBox::Icon icon,
                                 const QString &title, const QString &text,
                                 const QString &button0Text,
                                 const QString &button1Text,
                                 const QString &button2Text,
                                 int defaultButtonNumber,
                                 int escapeButtonNumber);

    static QMessageBox::StandardButton showNewMessageBox(QWidget *parent,
                QMessageBox::Icon icon, const QString& title, const QString& text,
                QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);

    static QPixmap standardIcon(QMessageBox::Icon icon, QMessageBox *mb);
    static QMessageBox::StandardButton standardButtonForRole(QMessageBox::ButtonRole role);

    QLabel *label;
    QMessageBox::Icon icon;
    QLabel *iconLabel;
    QDialogButtonBox *buttonBox;
    QList<QAbstractButton *> customButtonList;
    QAbstractButton *escapeButton;
    QPushButton *defaultButton;
    QCheckBox *checkbox;
    QAbstractButton *clickedButton;
    DetailButton *detailsButton;
#if QT_CONFIG(textedit)
    QMessageBoxDetailsText *detailsText;
#endif
    bool compatMode;
    bool autoAddOkButton;
    QAbstractButton *detectedEscapeButton;
    QLabel *informativeLabel;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
    QByteArray signalToDisconnectOnClose;
    QSharedPointer<QMessageDialogOptions> options;
private:
    void initHelper(QPlatformDialogHelper *) override;
    void helperPrepareShow(QPlatformDialogHelper *) override;
    int dialogCode() const override;
};

void QMessageBoxPrivate::init(const QString &title, const QString &text)
{
    Q_Q(QMessageBox);

    label = new QLabel;
    label->setObjectName("qt_msgbox_label"_L1);
    label->setTextInteractionFlags(Qt::TextInteractionFlags(q->style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, nullptr, q)));
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    label->setOpenExternalLinks(true);
    iconLabel = new QLabel(q);
    iconLabel->setObjectName("qt_msgboxex_icon_label"_L1);
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    buttonBox = new QDialogButtonBox;
    buttonBox->setObjectName("qt_msgbox_buttonbox"_L1);
    buttonBox->setCenterButtons(q->style()->styleHint(QStyle::SH_MessageBox_CenterButtons, nullptr, q));
    QObjectPrivate::connect(buttonBox, &QDialogButtonBox::clicked,
                            this, &QMessageBoxPrivate::buttonClicked);
    setupLayout();
    if (!title.isEmpty() || !text.isEmpty()) {
        q->setWindowTitle(title);
        q->setText(text);
    }
    q->setModal(true);
#ifdef Q_OS_MAC
    QFont f = q->font();
    f.setBold(true);
    label->setFont(f);
#endif
    icon = QMessageBox::NoIcon;
}

void QMessageBoxPrivate::setupLayout()
{
    Q_Q(QMessageBox);
    delete q->layout();
    QGridLayout *grid = new QGridLayout;
    const bool hasIcon = !iconLabel->pixmap().isNull();

    if (hasIcon)
        grid->addWidget(iconLabel, 0, 0, 2, 1, Qt::AlignTop);
    iconLabel->setVisible(hasIcon);
#ifdef Q_OS_MAC
    QSpacerItem *indentSpacer = new QSpacerItem(14, 1, QSizePolicy::Fixed, QSizePolicy::Fixed);
#else
    QSpacerItem *indentSpacer = new QSpacerItem(hasIcon ? 7 : 15, 1, QSizePolicy::Fixed, QSizePolicy::Fixed);
#endif
    grid->addItem(indentSpacer, 0, hasIcon ? 1 : 0, 2, 1);
    grid->addWidget(label, 0, hasIcon ? 2 : 1, 1, 1);
    if (informativeLabel) {
#ifndef Q_OS_MAC
        informativeLabel->setContentsMargins(0, 7, 0, 7);
#endif
        grid->addWidget(informativeLabel, 1, hasIcon ? 2 : 1, 1, 1);
    }
    if (checkbox) {
        grid->addWidget(checkbox, informativeLabel ? 2 : 1, hasIcon ? 2 : 1, 1, 1, Qt::AlignLeft);
#ifdef Q_OS_MAC
        grid->addItem(new QSpacerItem(1, 15, QSizePolicy::Fixed, QSizePolicy::Fixed), grid->rowCount(), 0);
#else
        grid->addItem(new QSpacerItem(1, 7, QSizePolicy::Fixed, QSizePolicy::Fixed), grid->rowCount(), 0);
#endif
    }
#ifdef Q_OS_MAC
    grid->addWidget(buttonBox, grid->rowCount(), hasIcon ? 2 : 1, 1, 1);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setVerticalSpacing(8);
    grid->setHorizontalSpacing(0);
    q->setContentsMargins(24, 15, 24, 20);
    grid->setRowStretch(1, 100);
    grid->setRowMinimumHeight(2, 6);
#else
    grid->addWidget(buttonBox, grid->rowCount(), 0, 1, grid->columnCount());
#endif
#if QT_CONFIG(textedit)
    if (detailsText)
        grid->addWidget(detailsText, grid->rowCount(), 0, 1, grid->columnCount());
#endif
    grid->setSizeConstraint(QLayout::SetNoConstraint);
    q->setLayout(grid);

    retranslateStrings();
    updateSize();
}

int QMessageBoxPrivate::layoutMinimumWidth()
{
    layout->activate();
    return layout->totalMinimumSize().width();
}

void QMessageBoxPrivate::updateSize()
{
    Q_Q(QMessageBox);

    if (!q->isVisible())
        return;

    const QSize screenSize = q->screen()->availableGeometry().size();
    int hardLimit = qMin(screenSize.width() - 480, 1000); // can never get bigger than this
    // on small screens allows the messagebox be the same size as the screen
    if (screenSize.width() <= 1024)
        hardLimit = screenSize.width();
#ifdef Q_OS_MAC
    int softLimit = qMin(screenSize.width()/2, 420);
#else
    // note: ideally on windows, hard and soft limits but it breaks compat
    int softLimit = qMin(screenSize.width()/2, 500);
#endif

    if (informativeLabel)
        informativeLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    label->setWordWrap(false); // makes the label return min size
    int width = layoutMinimumWidth();

    if (width > softLimit) {
        label->setWordWrap(true);
        width = qMax(softLimit, layoutMinimumWidth());

        if (width > hardLimit) {
            label->d_func()->ensureTextControl();
            if (QWidgetTextControl *control = label->d_func()->control) {
                QTextOption opt = control->document()->defaultTextOption();
                opt.setWrapMode(QTextOption::WrapAnywhere);
                control->document()->setDefaultTextOption(opt);
            }
            width = hardLimit;
        }
    }

    if (informativeLabel) {
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        QSizePolicy policy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        informativeLabel->setSizePolicy(policy);
        width = qMax(width, layoutMinimumWidth());
        if (width > hardLimit) { // longest word is really big, so wrap anywhere
            informativeLabel->d_func()->ensureTextControl();
            if (QWidgetTextControl *control = informativeLabel->d_func()->control) {
                QTextOption opt = control->document()->defaultTextOption();
                opt.setWrapMode(QTextOption::WrapAnywhere);
                control->document()->setDefaultTextOption(opt);
            }
            width = hardLimit;
        }
        policy.setHeightForWidth(label->wordWrap());
        label->setSizePolicy(policy);
    }

    QFontMetrics fm(QApplication::font("QMdiSubWindowTitleBar"));
    int windowTitleWidth = qMin(fm.horizontalAdvance(q->windowTitle()) + 50, hardLimit);
    if (windowTitleWidth > width)
        width = windowTitleWidth;

    layout->activate();
    int height = (layout->hasHeightForWidth())
                     ? layout->totalHeightForWidth(width)
                     : layout->totalMinimumSize().height();

    q->setFixedSize(width, height);
    QCoreApplication::removePostedEvents(q, QEvent::LayoutRequest);
}

static int oldButton(int button)
{
    switch (button & QMessageBox::ButtonMask) {
    case QMessageBox::Ok:
        return Old_Ok;
    case QMessageBox::Cancel:
        return Old_Cancel;
    case QMessageBox::Yes:
        return Old_Yes;
    case QMessageBox::No:
        return Old_No;
    case QMessageBox::Abort:
        return Old_Abort;
    case QMessageBox::Retry:
        return Old_Retry;
    case QMessageBox::Ignore:
        return Old_Ignore;
    case QMessageBox::YesToAll:
        return Old_YesAll;
    case QMessageBox::NoToAll:
        return Old_NoAll;
    default:
        return 0;
    }
}

int QMessageBoxPrivate::execReturnCode(QAbstractButton *button)
{
    if (int standardButton = buttonBox->standardButton(button)) {
        // When using a QMessageBox with standard buttons, the return
        // code is a StandardButton value indicating the standard button
        // that was clicked.
        if (compatMode)
            return oldButton(standardButton);
        else
            return standardButton;
    } else {
        // When using QMessageBox with custom buttons, the return code
        // is an opaque value, and the user is expected to use clickedButton()
        // to determine which button was clicked. We make sure to keep the opaque
        // value out of the QDialog::DialogCode range, so we can distinguish them.
        auto customButtonIndex = customButtonList.indexOf(button);
        if (customButtonIndex >= 0)
            return QDialog::DialogCode::Accepted + customButtonIndex + 1;
        else
            return customButtonIndex; // Not found, return -1
    }
}

int QMessageBoxPrivate::dialogCode() const
{
    Q_Q(const QMessageBox);

    if (rescode <= QDialog::Accepted) {
        return rescode;
    } else if (clickedButton) {
        switch (q->buttonRole(clickedButton)) {
        case QMessageBox::AcceptRole:
        case QMessageBox::YesRole:
            return QDialog::Accepted;
        case QMessageBox::RejectRole:
        case QMessageBox::NoRole:
            return QDialog::Rejected;
        default:
            ;
        }
    }

    return QDialogPrivate::dialogCode();
}

void QMessageBoxPrivate::buttonClicked(QAbstractButton *button)
{
    Q_Q(QMessageBox);
#if QT_CONFIG(textedit)
    if (detailsButton && detailsText && button == detailsButton) {
        detailsButton->setLabel(detailsText->isHidden() ? HideLabel : ShowLabel);
        detailsText->setHidden(!detailsText->isHidden());
        updateSize();
    } else
#endif
    {
        setClickedButton(button);

        if (receiverToDisconnectOnClose) {
            QObject::disconnect(q, signalToDisconnectOnClose, receiverToDisconnectOnClose,
                                memberToDisconnectOnClose);
            receiverToDisconnectOnClose = nullptr;
        }
        signalToDisconnectOnClose.clear();
        memberToDisconnectOnClose.clear();
    }
}

void QMessageBoxPrivate::setClickedButton(QAbstractButton *button)
{
    Q_Q(QMessageBox);

    clickedButton = button;
    emit q->buttonClicked(clickedButton);

    auto resultCode = execReturnCode(button);
    q->done(resultCode);
}

void QMessageBoxPrivate::helperClicked(QPlatformDialogHelper::StandardButton helperButton, QPlatformDialogHelper::ButtonRole role)
{
    Q_UNUSED(role);
    Q_Q(QMessageBox);

    // Map back to QAbstractButton, so that the message box behaves the same from
    // the outside, regardless of whether it's backed by a native helper or not.
    QAbstractButton *dialogButton = helperButton > QPlatformDialogHelper::LastButton ?
        static_cast<QAbstractButton *>(options->customButton(helperButton)->button) :
        q->button(QMessageBox::StandardButton(helperButton));

    Q_ASSERT(dialogButton);

    // Simulate click by explicitly clicking the button. This will ensure that
    // any logic of the button that responds to the click is respected, including
    // plumbing back to buttonClicked above based on the clicked() signal.
    dialogButton->click();
}

/*!
    \class QMessageBox

    \brief The QMessageBox class provides a modal dialog for informing
    the user or for asking the user a question and receiving an answer.

    \ingroup standard-dialogs
    \inmodule QtWidgets

    A message box displays a primary \l{QMessageBox::text}{text} to
    alert the user to a situation, an \l{QMessageBox::informativeText}
    {informative text} to further explain the situation, and an optional
    \l{QMessageBox::detailedText} {detailed text} to provide even more data
    if the user requests it.

    A message box can also display an \l{QMessageBox::icon} {icon}
    and \l{QMessageBox::standardButtons} {standard buttons} for
    accepting a user response.

    Two APIs for using QMessageBox are provided, the property-based
    API, and the static functions. Calling one of the static functions
    is the simpler approach, but it is less flexible than using the
    property-based API, and the result is less informative. Using the
    property-based API is recommended.

    \section1 The Property-based API

    To use the property-based API, construct an instance of
    QMessageBox, set the desired properties, and call exec() to show
    the message. The simplest configuration is to set only the
    \l{QMessageBox::text} {message text} property.

    \snippet code/src_gui_dialogs_qmessagebox.cpp 5

    The user must click the \uicontrol{OK} button to dismiss the message
    box. The rest of the GUI is blocked until the message box is
    dismissed.

    \image msgbox1.png

    A better approach than just alerting the user to an event is to
    also ask the user what to do about it.

    Set the \l{QMessageBox::standardButtons} {standard buttons}
    property to the set of buttons you want as the set of user
    responses. The buttons are specified by combining values from
    StandardButtons using the bitwise OR operator. The display order
    for the buttons is platform-dependent. For example, on Windows,
    \uicontrol{Save} is displayed to the left of \uicontrol{Cancel}, whereas on
    \macos, the order is reversed. Mark one of your standard buttons to be your
    \l{QMessageBox::defaultButton()} {default button}.

    The \l{QMessageBox::informativeText} {informative text} property can
    be used to add additional context to help the user choose the appropriate action.

    \snippet code/src_gui_dialogs_qmessagebox.cpp 6

    \image msgbox2.png

    The exec() slot returns the StandardButtons value of the button
    that was clicked.

    \snippet code/src_gui_dialogs_qmessagebox.cpp 7

    To give the user more information to help them choose the appropriate,
    action, set the \l{QMessageBox::detailedText} {detailed text} property.
    Depending on the platform the \l{QMessageBox::detailedText} {detailed text},
    may require the user to click a \uicontrol{Show Details...} button to be shown.

    \image msgbox3.png

    Clicking the \uicontrol{Show Details...} button displays the detailed text.

    \image msgbox4.png

    \section2 Rich Text and the Text Format Property

    The \l{QMessageBox::detailedText} {detailed text} property is
    always interpreted as plain text. The \l{QMessageBox::text} {main
    text} and \l{QMessageBox::informativeText} {informative text}
    properties can be either plain text or rich text. These strings
    are interpreted according to the setting of the
    \l{QMessageBox::textFormat} {text format} property. The default
    setting is \l{Qt::AutoText} {auto-text}.

    Note that for some plain text strings containing XML
    meta-characters, the auto-text \l{Qt::mightBeRichText()} {rich
    text detection test} may fail causing your plain text string to be
    interpreted incorrectly as rich text. In these rare cases, use
    Qt::convertFromPlainText() to convert your plain text string to a
    visually equivalent rich text string, or set the
    \l{QMessageBox::textFormat} {text format} property explicitly with
    setTextFormat().

    \section2 Severity Levels and the Icon and Pixmap Properties

    QMessageBox supports four predefined message severity levels, or message
    types, which really only differ in the predefined icon they each show.
    Specify one of the four predefined message types by setting the
    \l{QMessageBox::icon}{icon} property to one of the
    \l{QMessageBox::Icon}{predefined icons}. The following rules are
    guidelines:

    \table
    \row
    \li \image qmessagebox-quest.png
    \li \l Question
    \li For asking a question during normal operations.
    \row
    \li \image qmessagebox-info.png
    \li \l Information
    \li For reporting information about normal operations.
    \row
    \li \image qmessagebox-warn.png
    \li \l Warning
    \li For reporting non-critical errors.
    \row
    \li \image qmessagebox-crit.png
    \li \l Critical
    \li For reporting critical errors.
    \endtable

    \l{QMessageBox::Icon}{Predefined icons} are not defined by QMessageBox, but
    provided by the style. The default value is \l{QMessageBox::NoIcon}
    {No Icon}. The message boxes are otherwise the same for all cases. When
    using a standard icon, use the one recommended in the table, or use the
    one recommended by the style guidelines for your platform. If none of the
    standard icons is right for your message box, you can use a custom icon by
    setting the \l{QMessageBox::iconPixmap}{icon pixmap} property instead of
    setting the \l{QMessageBox::icon}{icon} property.

    In summary, to set an icon, use \e{either} setIcon() for one of the
    standard icons, \e{or} setIconPixmap() for a custom icon.

    \section1 The Static Functions API

    Building message boxes with the static functions API, although
    convenient, is less flexible than using the property-based API,
    because the static function signatures lack parameters for setting
    the \l{QMessageBox::informativeText} {informative text} and
    \l{QMessageBox::detailedText} {detailed text} properties. One
    work-around for this has been to use the \c{title} parameter as
    the message box main text and the \c{text} parameter as the
    message box informative text. Because this has the obvious
    drawback of making a less readable message box, platform
    guidelines do not recommend it. The \e{Microsoft Windows User
    Interface Guidelines} recommend using the
    \l{QCoreApplication::applicationName} {application name} as the
    \l{QMessageBox::setWindowTitle()} {window's title}, which means
    that if you have an informative text in addition to your main
    text, you must concatenate it to the \c{text} parameter.

    Note that the static function signatures have changed with respect
    to their button parameters, which are now used to set the
    \l{QMessageBox::standardButtons} {standard buttons} and the
    \l{QMessageBox::defaultButton()} {default button}.

    Static functions are available for creating information(),
    question(), warning(), and critical() message boxes.

    \snippet code/src_gui_dialogs_qmessagebox.cpp 0

    The \l{dialogs/standarddialogs}{Standard Dialogs} example shows
    how to use QMessageBox and the other built-in Qt dialogs.

    \section1 Advanced Usage

    If the \l{QMessageBox::StandardButtons} {standard buttons} are not
    flexible enough for your message box, you can use the addButton()
    overload that takes a text and a ButtonRole to add custom
    buttons. The ButtonRole is used by QMessageBox to determine the
    ordering of the buttons on screen (which varies according to the
    platform). You can test the value of clickedButton() after calling
    exec(). For example,

    \snippet code/src_gui_dialogs_qmessagebox.cpp 2

    \section1 Default and Escape Keys

    The default button (i.e., the button activated when \uicontrol Enter is
    pressed) can be specified using setDefaultButton(). If a default
    button is not specified, QMessageBox tries to find one based on
    the \l{ButtonRole} {button roles} of the buttons used in the
    message box.

    The escape button (the button activated when \uicontrol Esc is pressed)
    can be specified using setEscapeButton().  If an escape button is
    not specified, QMessageBox tries to find one using these rules:

    \list 1

    \li If there is only one button, it is the button activated when
    \uicontrol Esc is pressed.

    \li If there is a \l Cancel button, it is the button activated when
    \uicontrol Esc is pressed.

    \li If there is exactly one button having either
       \l{QMessageBox::RejectRole} {the Reject role} or the
       \l{QMessageBox::NoRole} {the No role}, it is the button
       activated when \uicontrol Esc is pressed.

    \endlist

    When an escape button can't be determined using these rules,
    pressing \uicontrol Esc has no effect.

    \sa QDialogButtonBox, {Standard Dialogs Example}
*/

/*!
    \enum QMessageBox::ButtonRole

    \include qdialogbuttonbox.cpp buttonrole-enum
*/

/*!
    \enum QMessageBox::StandardButton

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

    The following values are obsolete:

    \value YesAll Use YesToAll instead.
    \value NoAll Use NoToAll instead.
    \value Default Use the \c defaultButton argument of
           information(), warning(), etc. instead, or call
           setDefaultButton().
    \value Escape Call setEscapeButton() instead.
    \value FlagMask
    \value ButtonMask

    \sa ButtonRole, standardButtons
*/

/*!
    \enum QMessageBox::Option
    \since 6.6
    \value DontUseNativeDialog Don't use the native message dialog.
*/

/*!
    \fn void QMessageBox::buttonClicked(QAbstractButton *button)

    This signal is emitted whenever a button is clicked inside the QMessageBox.
    The button that was clicked in returned in \a button.
*/

/*!
    Constructs an \l{Qt::ApplicationModal} {application modal} message box with no text and no buttons.
    \a parent is passed to the QDialog constructor.

    The window modality can be overridden via setWindowModality() before calling show().

    \note Using open() or exec() to show the message box affects the window modality.
    Please see the detailed documentation for each function for more information.

    On \macos, if you want your message box to appear
    as a Qt::Sheet of its \a parent, set the message box's
    \l{setWindowModality()} {window modality} to Qt::WindowModal or use open().
    Otherwise, the message box will be a standard dialog.

    \sa setWindowTitle(), setText(), setIcon(), setStandardButtons(), setWindowModality()

*/
QMessageBox::QMessageBox(QWidget *parent)
    : QDialog(*new QMessageBoxPrivate, parent, Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
    Q_D(QMessageBox);
    d->init();
}

/*!
    Constructs an \l{Qt::ApplicationModal} {application modal} message box with the given \a icon,
    \a title, \a text, and standard \a buttons. Standard or custom buttons can be
    added at any time using addButton(). The \a parent and \a f
    arguments are passed to the QDialog constructor.

    The window modality can be overridden via setWindowModality() before calling show().

    \note Using open() or exec() to show the message box affects the window modality.
    Please see the detailed documentation for each function for more information.

    On \macos, if \a parent is not \nullptr and you want your message box
    to appear as a Qt::Sheet of that parent, set the message box's
    \l{setWindowModality()} {window modality} to Qt::WindowModal
    (default). Otherwise, the message box will be a standard dialog.

    \sa setWindowTitle(), setText(), setIcon(), setStandardButtons(), setWindowModality()
*/
QMessageBox::QMessageBox(Icon icon, const QString &title, const QString &text,
                         StandardButtons buttons, QWidget *parent,
                         Qt::WindowFlags f)
: QDialog(*new QMessageBoxPrivate, parent, f | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
    Q_D(QMessageBox);
    d->init(title, text);
    setIcon(icon);
    if (buttons != NoButton)
        setStandardButtons(buttons);
}

/*!
    Destroys the message box.
*/
QMessageBox::~QMessageBox()
{
}

/*!
    Adds the given \a button to the message box with the specified \a
    role.

    \sa removeButton(), button(), setStandardButtons()
*/
void QMessageBox::addButton(QAbstractButton *button, ButtonRole role)
{
    Q_D(QMessageBox);
    if (!button)
        return;
    removeButton(button);

    if (button->text().isEmpty()) {
        if (auto *platformTheme = QGuiApplicationPrivate::platformTheme()) {
            if (auto standardButton = QMessageBoxPrivate::standardButtonForRole(role))
                button->setText(platformTheme->standardButtonText(standardButton));
        }

        if (button->text().isEmpty()) {
            qWarning() << "Cannot add" << button << "without title";
            return;
        }
    }

    d->buttonBox->addButton(button, (QDialogButtonBox::ButtonRole)role);
    d->customButtonList.append(button);
    d->autoAddOkButton = false;
}

QMessageBox::StandardButton QMessageBoxPrivate::standardButtonForRole(QMessageBox::ButtonRole role)
{
    switch (role) {
    case QMessageBox::AcceptRole: return QMessageBox::Ok;
    case QMessageBox::RejectRole: return QMessageBox::Cancel;
    case QMessageBox::DestructiveRole: return QMessageBox::Discard;
    case QMessageBox::HelpRole: return QMessageBox::Help;
    case QMessageBox::ApplyRole: return QMessageBox::Apply;
    case QMessageBox::YesRole: return QMessageBox::Yes;
    case QMessageBox::NoRole: return QMessageBox::No;
    case QMessageBox::ResetRole: return QMessageBox::Reset;
    default: return QMessageBox::NoButton;
    }
}

/*!
    \overload

    Creates a button with the given \a text, adds it to the message box for the
    specified \a role, and returns it.
*/
QPushButton *QMessageBox::addButton(const QString& text, ButtonRole role)
{
    Q_D(QMessageBox);
    QPushButton *pushButton = new QPushButton(text);
    addButton(pushButton, role);
    d->updateSize();
    return pushButton;
}

/*!
    \overload

    Adds a standard \a button to the message box if it is valid to do so, and
    returns the push button.

    \sa setStandardButtons()
*/
QPushButton *QMessageBox::addButton(StandardButton button)
{
    Q_D(QMessageBox);
    QPushButton *pushButton = d->buttonBox->addButton((QDialogButtonBox::StandardButton)button);
    if (pushButton)
        d->autoAddOkButton = false;
    return pushButton;
}

/*!
    Removes \a button from the button box without deleting it.

    \sa addButton(), setStandardButtons()
*/
void QMessageBox::removeButton(QAbstractButton *button)
{
    Q_D(QMessageBox);
    d->customButtonList.removeAll(button);
    if (d->escapeButton == button)
        d->escapeButton = nullptr;
    if (d->defaultButton == button)
        d->defaultButton = nullptr;
    d->buttonBox->removeButton(button);
    d->updateSize();
}

/*!
    \property QMessageBox::standardButtons
    \brief collection of standard buttons in the message box

    This property controls which standard buttons are used by the message box.

    By default, this property contains no standard buttons.

    \sa addButton()
*/
void QMessageBox::setStandardButtons(StandardButtons buttons)
{
    Q_D(QMessageBox);
    d->buttonBox->setStandardButtons(QDialogButtonBox::StandardButtons(int(buttons)));

    QList<QAbstractButton *> buttonList = d->buttonBox->buttons();
    if (!buttonList.contains(d->escapeButton))
        d->escapeButton = nullptr;
    if (!buttonList.contains(d->defaultButton))
        d->defaultButton = nullptr;
    d->autoAddOkButton = false;
    d->updateSize();
}

QMessageBox::StandardButtons QMessageBox::standardButtons() const
{
    Q_D(const QMessageBox);
    return QMessageBox::StandardButtons(int(d->buttonBox->standardButtons()));
}

/*!
    Returns the standard button enum value corresponding to the given \a button,
    or NoButton if the given \a button isn't a standard button.

    \sa button(), standardButtons()
*/
QMessageBox::StandardButton QMessageBox::standardButton(QAbstractButton *button) const
{
    Q_D(const QMessageBox);
    return (QMessageBox::StandardButton)d->buttonBox->standardButton(button);
}

/*!
    Returns a pointer corresponding to the standard button \a which,
    or \nullptr if the standard button doesn't exist in this message box.

    \note Modifying the properties of the returned button may not be reflected
    in native implementations of the message dialog. To customize dialog
    buttons add a \l{addButton(QAbstractButton *button, QMessageBox::ButtonRole role)}
    {custom button} or \l{addButton(const QString &text, QMessageBox::ButtonRole role)}
    {button title} instead, or set the \l Option::DontUseNativeDialog option.

    \sa standardButtons, standardButton()
*/
QAbstractButton *QMessageBox::button(StandardButton which) const
{
    Q_D(const QMessageBox);
    return d->buttonBox->button(QDialogButtonBox::StandardButton(which));
}

/*!
    Returns the button that is activated when escape is pressed.

    By default, QMessageBox attempts to automatically detect an
    escape button as follows:

    \list 1
    \li If there is only one button, it is made the escape button.
    \li If there is a \l Cancel button, it is made the escape button.
    \li On \macos only, if there is exactly one button with the role
       QMessageBox::RejectRole, it is made the escape button.
    \endlist

    When an escape button could not be automatically detected, pressing
    \uicontrol Esc has no effect.

    \sa addButton()
*/
QAbstractButton *QMessageBox::escapeButton() const
{
    Q_D(const QMessageBox);
    return d->escapeButton;
}

/*!
    Sets the button that gets activated when the \uicontrol Escape key is
    pressed to \a button.

    \sa addButton(), clickedButton()
*/
void QMessageBox::setEscapeButton(QAbstractButton *button)
{
    Q_D(QMessageBox);
    if (d->buttonBox->buttons().contains(button))
        d->escapeButton = button;
}

/*!
    Sets the buttons that gets activated when the \uicontrol Escape key is
    pressed to \a button.

    \sa addButton(), clickedButton()
*/
void QMessageBox::setEscapeButton(QMessageBox::StandardButton button)
{
    Q_D(QMessageBox);
    setEscapeButton(d->buttonBox->button(QDialogButtonBox::StandardButton(button)));
}

void QMessageBoxPrivate::detectEscapeButton()
{
    if (escapeButton) { // escape button explicitly set
        detectedEscapeButton = escapeButton;
        return;
    }

    // Cancel button automatically becomes escape button
    detectedEscapeButton = buttonBox->button(QDialogButtonBox::Cancel);
    if (detectedEscapeButton)
        return;

    // If there is only one button, make it the escape button
    const QList<QAbstractButton *> buttons = buttonBox->buttons();
    if (buttons.size() == 1) {
        detectedEscapeButton = buttons.first();
        return;
    }

    // If there are two buttons and one of them is the "Show Details..."
    // button, then make the other one the escape button
    if (buttons.size() == 2 && detailsButton) {
        auto idx = buttons.indexOf(detailsButton);
        if (idx != -1) {
            detectedEscapeButton = buttons.at(1 - idx);
            return;
        }
    }

    // if the message box has one RejectRole button, make it the escape button
    for (auto *button : buttons) {
        if (buttonBox->buttonRole(button) == QDialogButtonBox::RejectRole) {
            if (detectedEscapeButton) { // already detected!
                detectedEscapeButton = nullptr;
                break;
            }
            detectedEscapeButton = button;
        }
    }
    if (detectedEscapeButton)
        return;

    // if the message box has one NoRole button, make it the escape button
    for (auto *button : buttons) {
        if (buttonBox->buttonRole(button) == QDialogButtonBox::NoRole) {
            if (detectedEscapeButton) { // already detected!
                detectedEscapeButton = nullptr;
                break;
            }
            detectedEscapeButton = button;
        }
    }
}

/*!
    Returns the button that was clicked by the user,
    or \nullptr if the user hit the \uicontrol Esc key and
    no \l{setEscapeButton()}{escape button} was set.

    If exec() hasn't been called yet, returns nullptr.

    Example:

    \snippet code/src_gui_dialogs_qmessagebox.cpp 3

    \sa standardButton(), button()
*/
QAbstractButton *QMessageBox::clickedButton() const
{
    Q_D(const QMessageBox);
    return d->clickedButton;
}

/*!
    Returns the button that should be the message box's
    \l{QPushButton::setDefault()}{default button}. Returns nullptr
    if no default button was set.

    \sa addButton(), QPushButton::setDefault()
*/
QPushButton *QMessageBox::defaultButton() const
{
    Q_D(const QMessageBox);
    return d->defaultButton;
}

/*!
    Sets the message box's \l{QPushButton::setDefault()}{default button}
    to \a button.

    \sa addButton(), QPushButton::setDefault()
*/
void QMessageBox::setDefaultButton(QPushButton *button)
{
    Q_D(QMessageBox);
    if (!d->buttonBox->buttons().contains(button))
        return;
    d->defaultButton = button;
    button->setDefault(true);
    button->setFocus();
}

/*!
    Sets the message box's \l{QPushButton::setDefault()}{default button}
    to \a button.

    \sa addButton(), QPushButton::setDefault()
*/
void QMessageBox::setDefaultButton(QMessageBox::StandardButton button)
{
    Q_D(QMessageBox);
    setDefaultButton(d->buttonBox->button(QDialogButtonBox::StandardButton(button)));
}

/*! \since 5.2

    Sets the checkbox \a cb on the message dialog. The message box takes ownership of the checkbox.
    The argument \a cb can be \nullptr to remove an existing checkbox from the message box.

    \sa checkBox()
*/

void QMessageBox::setCheckBox(QCheckBox *cb)
{
    Q_D(QMessageBox);

    if (cb == d->checkbox)
        return;

    if (d->checkbox) {
        d->checkbox->hide();
        layout()->removeWidget(d->checkbox);
        if (d->checkbox->parentWidget() == this) {
            d->checkbox->setParent(nullptr);
            d->checkbox->deleteLater();
        }
    }
    d->checkbox = cb;
    if (d->checkbox) {
        QSizePolicy sp = d->checkbox->sizePolicy();
        sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
        d->checkbox->setSizePolicy(sp);
    }
    d->setupLayout();
}


/*! \since 5.2

    Returns the checkbox shown on the dialog. This is \nullptr if no checkbox is set.
    \sa setCheckBox()
*/

QCheckBox* QMessageBox::checkBox() const
{
    Q_D(const QMessageBox);
    return d->checkbox;
}

/*!
    \since 6.6
    Sets the given \a option to be enabled if \a on is true; otherwise,
    clears the given \a option.

    Options (particularly the \l Option::DontUseNativeDialog option) should be set
    before showing the dialog.

    Setting options while the dialog is visible is not guaranteed to have
    an immediate effect on the dialog.

    Setting options after changing other properties may cause these
    values to have no effect.

    \sa options, testOption()
*/
void QMessageBox::setOption(QMessageBox::Option option, bool on)
{
    const QMessageBox::Options previousOptions = options();
    if (!(previousOptions & option) != !on)
        setOptions(previousOptions ^ option);
}

/*!
    \since 6.6

    Returns \c true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool QMessageBox::testOption(QMessageBox::Option option) const
{
    Q_D(const QMessageBox);
    return d->options->testOption(static_cast<QMessageDialogOptions::Option>(option));
}

void QMessageBox::setOptions(QMessageBox::Options options)
{
    Q_D(QMessageBox);

    if (QMessageBox::options() == options)
        return;

    d->options->setOptions(QMessageDialogOptions::Option(int(options)));
}

QMessageBox::Options QMessageBox::options() const
{
    Q_D(const QMessageBox);
    return QMessageBox::Options(int(d->options->options()));
}

/*!
    \property QMessageBox::options
    \brief Options that affect the look and feel of the dialog.
    \since 6.6

    By default, these options are disabled.

    The option \l Option::DontUseNativeDialog should be set
    before changing dialog properties or showing the dialog.

    Setting options while the dialog is visible is not guaranteed to have
    an immediate effect on the dialog.

    Setting options after changing other properties may cause these
    values to have no effect.

    \sa setOption(), testOption()
*/

/*!
  \property QMessageBox::text
  \brief the message box text to be displayed.

  The text should be a brief sentence or phrase that describes the situation,
  ideally formulated as a neutral statement, or a call-to-action question.

  The text will be interpreted either as a plain text or as rich text,
  depending on the text format setting (\l QMessageBox::textFormat).
  The default setting is Qt::AutoText, i.e., the message box will try
  to auto-detect the format of the text.

  The default value of this property is an empty string.

  \sa textFormat, QMessageBox::informativeText, QMessageBox::detailedText
*/
QString QMessageBox::text() const
{
    Q_D(const QMessageBox);
    return d->label->text();
}

void QMessageBox::setText(const QString &text)
{
    Q_D(QMessageBox);
    d->label->setText(text);
    d->label->setWordWrap(d->label->textFormat() == Qt::RichText
        || (d->label->textFormat() == Qt::AutoText && Qt::mightBeRichText(text)));
    d->updateSize();
}

/*!
    \enum QMessageBox::Icon

    This enum has the following values:

    \value NoIcon the message box does not have any icon.

    \value Question an icon indicating that
    the message is asking a question.

    \value Information an icon indicating that
    the message is nothing out of the ordinary.

    \value Warning an icon indicating that the
    message is a warning, but can be dealt with.

    \value Critical an icon indicating that
    the message represents a critical problem.

*/

/*!
    \property QMessageBox::icon
    \brief the message box's icon

    The icon of the message box can be specified with one of the
    values:

    \list
    \li QMessageBox::NoIcon
    \li QMessageBox::Question
    \li QMessageBox::Information
    \li QMessageBox::Warning
    \li QMessageBox::Critical
    \endlist

    The default is QMessageBox::NoIcon.

    The pixmap used to display the actual icon depends on the current
    \l{QWidget::style()} {GUI style}. You can also set a custom pixmap
    for the icon by setting the \l{QMessageBox::iconPixmap} {icon
    pixmap} property.

    \sa iconPixmap
*/
QMessageBox::Icon QMessageBox::icon() const
{
    Q_D(const QMessageBox);
    return d->icon;
}

void QMessageBox::setIcon(Icon icon)
{
    Q_D(QMessageBox);
    setIconPixmap(QMessageBoxPrivate::standardIcon((QMessageBox::Icon)icon,
                                                   this));
    d->icon = icon;
}

/*!
    \property QMessageBox::iconPixmap
    \brief the current icon

    The icon currently used by the message box. Note that it's often
    hard to draw one pixmap that looks appropriate in all GUI styles;
    you may want to supply a different pixmap for each platform.

    By default, this property is undefined.

    \sa icon
*/
QPixmap QMessageBox::iconPixmap() const
{
    Q_D(const QMessageBox);
    return d->iconLabel->pixmap();
}

void QMessageBox::setIconPixmap(const QPixmap &pixmap)
{
    Q_D(QMessageBox);
    d->iconLabel->setPixmap(pixmap);
    d->icon = NoIcon;
    d->setupLayout();
}

/*!
    \property QMessageBox::textFormat
    \brief the format of the text displayed by the message box

    The current text format used by the message box. See the \l
    Qt::TextFormat enum for an explanation of the possible options.

    The default format is Qt::AutoText.

    \sa setText()
*/
Qt::TextFormat QMessageBox::textFormat() const
{
    Q_D(const QMessageBox);
    return d->label->textFormat();
}

void QMessageBox::setTextFormat(Qt::TextFormat format)
{
    Q_D(QMessageBox);
    d->label->setTextFormat(format);
    d->label->setWordWrap(format == Qt::RichText
                    || (format == Qt::AutoText && Qt::mightBeRichText(d->label->text())));
    if (d->informativeLabel)
        d->informativeLabel->setTextFormat(format);
    d->updateSize();
}

/*!
    \property QMessageBox::textInteractionFlags
    \since 5.1

    Specifies how the label of the message box should interact with user
    input.

    The default value depends on the style.

    \sa QStyle::SH_MessageBox_TextInteractionFlags
*/

Qt::TextInteractionFlags QMessageBox::textInteractionFlags() const
{
    Q_D(const QMessageBox);
    return d->label->textInteractionFlags();
}

void QMessageBox::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    Q_D(QMessageBox);
    d->label->setTextInteractionFlags(flags);
}

/*!
    \reimp
*/
bool QMessageBox::event(QEvent *e)
{
    bool result =QDialog::event(e);
    switch (e->type()) {
        case QEvent::LayoutRequest:
            d_func()->updateSize();
            break;
        case QEvent::LanguageChange:
            d_func()->retranslateStrings();
            break;
        default:
            break;
    }
    return result;
}

/*!
    \reimp
*/
void QMessageBox::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
}

/*!
    \reimp
*/
void QMessageBox::closeEvent(QCloseEvent *e)
{
    Q_D(QMessageBox);
    if (!d->detectedEscapeButton) {
        e->ignore();
        return;
    }
    QDialog::closeEvent(e);
    if (!d->clickedButton) {
        d->clickedButton = d->detectedEscapeButton;
        setResult(d->execReturnCode(d->detectedEscapeButton));
    }
}

/*!
    \reimp
*/
void QMessageBox::changeEvent(QEvent *ev)
{
    Q_D(QMessageBox);
    switch (ev->type()) {
    case QEvent::StyleChange:
    {
        if (d->icon != NoIcon)
            setIcon(d->icon);
        Qt::TextInteractionFlags flags(style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, nullptr, this));
        d->label->setTextInteractionFlags(flags);
        d->buttonBox->setCenterButtons(style()->styleHint(QStyle::SH_MessageBox_CenterButtons, nullptr, this));
        if (d->informativeLabel)
            d->informativeLabel->setTextInteractionFlags(flags);
        Q_FALLTHROUGH();
    }
    case QEvent::FontChange:
    case QEvent::ApplicationFontChange:
#ifdef Q_OS_MAC
    {
        QFont f = font();
        f.setBold(true);
        d->label->setFont(f);
    }
#endif
        Q_FALLTHROUGH();
    default:
        break;
    }
    QDialog::changeEvent(ev);
}

/*!
    \reimp
*/
void QMessageBox::keyPressEvent(QKeyEvent *e)
{
#if QT_CONFIG(shortcut)
        Q_D(QMessageBox);
        if (e->matches(QKeySequence::Cancel)) {
            if (d->detectedEscapeButton) {
#ifdef Q_OS_MAC
                d->detectedEscapeButton->animateClick();
#else
                d->detectedEscapeButton->click();
#endif
            }
            return;
        }
#endif // QT_CONFIG(shortcut)

#if !defined(QT_NO_CLIPBOARD) && !defined(QT_NO_SHORTCUT)

#if QT_CONFIG(textedit)
        if (e == QKeySequence::Copy) {
            if (d->detailsText && d->detailsText->isVisible() && d->detailsText->copy()) {
                e->setAccepted(true);
                return;
            }
        } else if (e == QKeySequence::SelectAll && d->detailsText && d->detailsText->isVisible()) {
            d->detailsText->selectAll();
            e->setAccepted(true);
            return;
        }
#endif // QT_CONFIG(textedit)

#if defined(Q_OS_WIN)
        if (e == QKeySequence::Copy) {
            const auto separator = "---------------------------\n"_L1;
            QString textToCopy;
            textToCopy += separator + windowTitle() + u'\n' + separator // title
                          + d->label->text() + u'\n' + separator;       // text

            if (d->informativeLabel)
                textToCopy += d->informativeLabel->text() + u'\n' + separator;

            const QList<QAbstractButton *> buttons = d->buttonBox->buttons();
            for (const auto *button : buttons)
                textToCopy += button->text() + "   "_L1;
            textToCopy += u'\n' + separator;
#if QT_CONFIG(textedit)
            if (d->detailsText)
                textToCopy += d->detailsText->text() + u'\n' + separator;
#endif
            QGuiApplication::clipboard()->setText(textToCopy);
            return;
        }
#endif // Q_OS_WIN

#endif // !QT_NO_CLIPBOARD && !QT_NO_SHORTCUT

#ifndef QT_NO_SHORTCUT
    if (!(e->modifiers() & (Qt::AltModifier | Qt::ControlModifier | Qt::MetaModifier))) {
        int key = e->key() & ~Qt::MODIFIER_MASK;
        if (key) {
            const QList<QAbstractButton *> buttons = d->buttonBox->buttons();
            for (auto *pb : buttons) {
                QKeySequence shortcut = pb->shortcut();
                if (!shortcut.isEmpty() && key == shortcut[0].key()) {
                    pb->animateClick();
                    return;
                }
            }
        }
    }
#endif
    QDialog::keyPressEvent(e);
}

/*!
    Opens the dialog and connects its finished() or buttonClicked() signal to
    the slot specified by \a receiver and \a member. If the slot in \a member
    has a pointer for its first parameter the connection is to buttonClicked(),
    otherwise the connection is to finished().

    The signal will be disconnected from the slot when the dialog is closed.
*/
void QMessageBox::open(QObject *receiver, const char *member)
{
    Q_D(QMessageBox);
    const char *signal = member && strchr(member, '*') ? SIGNAL(buttonClicked(QAbstractButton*))
                                                       : SIGNAL(finished(int));
    connect(this, signal, receiver, member);
    d->signalToDisconnectOnClose = signal;
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;
    QDialog::open();
}

void QMessageBoxPrivate::setVisible(bool visible)
{
    // Last minute setup
    if (visible) {
        Q_Q(QMessageBox);
        if (autoAddOkButton)
            q->addButton(QMessageBox::Ok);
        detectEscapeButton();
    }
    // On hide, we may be called from ~QDialog(), so prevent accessing
    // q_ptr as a QMessageBox!

    if (canBeNativeDialog())
        setNativeDialogVisible(visible);

    // Update WA_DontShowOnScreen based on whether the native dialog was shown,
    // so that QDialog::setVisible(visible) below updates the QWidget state correctly,
    // but skips showing the non-native version.
    static_cast<QWidget*>(q_ptr)->setAttribute(Qt::WA_DontShowOnScreen, nativeDialogInUse);

    QDialogPrivate::setVisible(visible);
}

/*!
    Returns a list of all the buttons that have been added to the message box.

    \sa buttonRole(), addButton(), removeButton()
*/
QList<QAbstractButton *> QMessageBox::buttons() const
{
    Q_D(const QMessageBox);
    return d->buttonBox->buttons();
}

/*!
    Returns the button role for the specified \a button. This function returns
    \l InvalidRole if \a button is \nullptr or has not been added to the message box.

    \sa buttons(), addButton()
*/
QMessageBox::ButtonRole QMessageBox::buttonRole(QAbstractButton *button) const
{
    Q_D(const QMessageBox);
    return QMessageBox::ButtonRole(d->buttonBox->buttonRole(button));
}

/*!
    \reimp
*/
void QMessageBox::showEvent(QShowEvent *e)
{
    Q_D(QMessageBox);
    d->clickedButton = nullptr;
    d->updateSize();

#if QT_CONFIG(accessibility)
    QAccessibleEvent event(this, QAccessible::Alert);
    QAccessible::updateAccessibility(&event);
#endif
#if defined(Q_OS_WIN)
    if (const HMENU systemMenu = qt_getWindowsSystemMenu(this)) {
        EnableMenuItem(systemMenu, SC_CLOSE, d->detectedEscapeButton ?
                       MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
    }
#endif
    QDialog::showEvent(e);
}


static QMessageBox::StandardButton showNewMessageBox(QWidget *parent,
    QMessageBox::Icon icon,
    const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    // necessary for source compatibility with Qt 4.0 and 4.1
    // handles (Yes, No) and (Yes|Default, No)
    if (defaultButton && !(buttons & defaultButton)) {
        const int defaultButtons = defaultButton | QMessageBox::Default;
        const int otherButtons = buttons.toInt();
        const int ret = QMessageBoxPrivate::showOldMessageBox(parent, icon, title,
                                                              text, otherButtons,
                                                              defaultButtons, 0);
        return static_cast<QMessageBox::StandardButton>(ret);
    }

    QMessageBox msgBox(icon, title, text, QMessageBox::NoButton, parent);
    QDialogButtonBox *buttonBox = msgBox.findChild<QDialogButtonBox*>();
    Q_ASSERT(buttonBox != nullptr);

    uint mask = QMessageBox::FirstButton;
    while (mask <= QMessageBox::LastButton) {
        uint sb = buttons & mask;
        mask <<= 1;
        if (!sb)
            continue;
        QPushButton *button = msgBox.addButton((QMessageBox::StandardButton)sb);
        // Choose the first accept role as the default
        if (msgBox.defaultButton())
            continue;
        if ((defaultButton == QMessageBox::NoButton && buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
            || (defaultButton != QMessageBox::NoButton && sb == uint(defaultButton)))
            msgBox.setDefaultButton(button);
    }
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}

/*!
    Opens an information message box with the given \a title and
    \a text in front of the specified \a parent widget.

    The standard \a buttons are added to the message box.
    \a defaultButton specifies the button used when \uicontrol Enter is pressed.
    \a defaultButton must refer to a button that was given in \a buttons.
    If \a defaultButton is QMessageBox::NoButton, QMessageBox
    chooses a suitable default automatically.

    Returns the identity of the standard button that was clicked. If
    \uicontrol Esc was pressed instead, the \l{Default and Escape Keys}
    {escape button} is returned.

    The message box is an \l{Qt::ApplicationModal}{application modal}
    dialog box.

    \warning Do not delete \a parent during the execution of the dialog.
             If you want to do this, you should create the dialog
             yourself using one of the QMessageBox constructors.

    \sa question(), warning(), critical()
*/
QMessageBox::StandardButton QMessageBox::information(QWidget *parent, const QString &title,
                               const QString& text, StandardButtons buttons,
                               StandardButton defaultButton)
{
    return showNewMessageBox(parent, Information, title, text, buttons,
                             defaultButton);
}


/*!
    Opens a question message box with the given \a title and \a
    text in front of the specified \a parent widget.

    The standard \a buttons are added to the message box. \a
    defaultButton specifies the button used when \uicontrol Enter is
    pressed. \a defaultButton must refer to a button that was given in \a buttons.
    If \a defaultButton is QMessageBox::NoButton, QMessageBox
    chooses a suitable default automatically.

    Returns the identity of the standard button that was clicked. If
    \uicontrol Esc was pressed instead, the \l{Default and Escape Keys}
    {escape button} is returned.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

    \warning Do not delete \a parent during the execution of the dialog.
             If you want to do this, you should create the dialog
             yourself using one of the QMessageBox constructors.

    \sa information(), warning(), critical()
*/
QMessageBox::StandardButton QMessageBox::question(QWidget *parent, const QString &title,
                            const QString& text, StandardButtons buttons,
                            StandardButton defaultButton)
{
    return showNewMessageBox(parent, Question, title, text, buttons, defaultButton);
}

/*!
    Opens a warning message box with the given \a title and \a
    text in front of the specified \a parent widget.

    The standard \a buttons are added to the message box. \a
    defaultButton specifies the button used when \uicontrol Enter is
    pressed. \a defaultButton must refer to a button that was given in \a buttons.
    If \a defaultButton is QMessageBox::NoButton, QMessageBox
    chooses a suitable default automatically.

    Returns the identity of the standard button that was clicked. If
    \uicontrol Esc was pressed instead, the \l{Default and Escape Keys}
    {escape button} is returned.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

    \warning Do not delete \a parent during the execution of the dialog.
             If you want to do this, you should create the dialog
             yourself using one of the QMessageBox constructors.

    \sa question(), information(), critical()
*/
QMessageBox::StandardButton QMessageBox::warning(QWidget *parent, const QString &title,
                        const QString& text, StandardButtons buttons,
                        StandardButton defaultButton)
{
    return showNewMessageBox(parent, Warning, title, text, buttons, defaultButton);
}

/*!
    Opens a critical message box with the given \a title and \a
    text in front of the specified \a parent widget.

    The standard \a buttons are added to the message box. \a
    defaultButton specifies the button used when \uicontrol Enter is
    pressed. \a defaultButton must refer to a button that was given in \a buttons.
    If \a defaultButton is QMessageBox::NoButton, QMessageBox
    chooses a suitable default automatically.

    Returns the identity of the standard button that was clicked. If
    \uicontrol Esc was pressed instead, the \l{Default and Escape Keys}
    {escape button} is returned.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

    \warning Do not delete \a parent during the execution of the dialog.
             If you want to do this, you should create the dialog
             yourself using one of the QMessageBox constructors.

    \sa question(), warning(), information()
*/
QMessageBox::StandardButton QMessageBox::critical(QWidget *parent, const QString &title,
                         const QString& text, StandardButtons buttons,
                         StandardButton defaultButton)
{
    return showNewMessageBox(parent, Critical, title, text, buttons, defaultButton);
}

/*!
    Displays a simple about box with title \a title and text \a
    text. The about box's parent is \a parent.

    about() looks for a suitable icon in four locations:

    \list 1
    \li It prefers \l{QWidget::windowIcon()}{parent->icon()}
    if that exists.
    \li If not, it tries the top-level widget containing \a parent.
    \li If that fails, it tries the \l{QApplication::activeWindow()}{active window.}
    \li As a last resort it uses the Information icon.
    \endlist

    The about box has a single button labelled "OK".

    On \macos, the about box is popped up as a modeless window; on
    other platforms, it is currently application modal.

    \sa QWidget::windowIcon(), QApplication::activeWindow()
*/
void QMessageBox::about(QWidget *parent, const QString &title, const QString &text)
{
#ifdef Q_OS_MAC
    static QPointer<QMessageBox> oldMsgBox;

    if (oldMsgBox && oldMsgBox->text() == text) {
        oldMsgBox->show();
        oldMsgBox->raise();
        oldMsgBox->activateWindow();
        return;
    }
#endif

    QMessageBox *msgBox = new QMessageBox(Information, title, text, NoButton, parent
#ifdef Q_OS_MAC
                                          , Qt::WindowTitleHint | Qt::WindowSystemMenuHint
#endif
    );
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    QIcon icon = msgBox->windowIcon();
    msgBox->setIconPixmap(icon.pixmap(QSize(64, 64), msgBox->devicePixelRatio()));

    // should perhaps be a style hint
#ifdef Q_OS_MAC
    oldMsgBox = msgBox;
    auto *d = msgBox->d_func();
    d->buttonBox->setCenterButtons(true);
#ifdef Q_OS_IOS
    msgBox->setModal(true);
#else
    msgBox->setModal(false);
#endif
    msgBox->show();
#else
    msgBox->exec();
#endif
}

/*!
    Displays a simple message box about Qt, with the given \a title
    and centered over \a parent (if \a parent is not \nullptr). The message
    includes the version number of Qt being used by the application.

    This is useful for inclusion in the \uicontrol Help menu of an application,
    as shown in the \l{mainwindows/menus}{Menus} example.

    QApplication provides this functionality as a slot.

    On \macos, the aboutQt box is popped up as a modeless window; on
    other platforms, it is currently application modal.

    \sa QApplication::aboutQt()
*/
void QMessageBox::aboutQt(QWidget *parent, const QString &title)
{
#ifdef Q_OS_MAC
    static QPointer<QMessageBox> oldMsgBox;

    if (oldMsgBox) {
        oldMsgBox->show();
        oldMsgBox->raise();
        oldMsgBox->activateWindow();
        return;
    }
#endif

    QString translatedTextAboutQtCaption;
    translatedTextAboutQtCaption = QMessageBox::tr(
        "<h3>About Qt</h3>"
        "<p>This program uses Qt version %1.</p>"
        ).arg(QT_VERSION_STR ""_L1);
    //: Leave this text untranslated or include a verbatim copy of it below
    //: and note that it is the authoritative version in case of doubt.
    const QString translatedTextAboutQtText = QMessageBox::tr(
        "<p>Qt is a C++ toolkit for cross-platform application "
        "development.</p>"
        "<p>Qt provides single-source portability across all major desktop "
        "operating systems. It is also available for embedded Linux and other "
        "embedded and mobile operating systems.</p>"
        "<p>Qt is available under multiple licensing options designed "
        "to accommodate the needs of our various users.</p>"
        "<p>Qt licensed under our commercial license agreement is appropriate "
        "for development of proprietary/commercial software where you do not "
        "want to share any source code with third parties or otherwise cannot "
        "comply with the terms of GNU (L)GPL.</p>"
        "<p>Qt licensed under GNU (L)GPL is appropriate for the "
        "development of Qt&nbsp;applications provided you can comply with the terms "
        "and conditions of the respective licenses.</p>"
        "<p>Please see <a href=\"https://%2/\">%2</a> "
        "for an overview of Qt licensing.</p>"
        "<p>Copyright (C) The Qt Company Ltd. and other "
        "contributors.</p>"
        "<p>Qt and the Qt logo are trademarks of The Qt Company Ltd.</p>"
        "<p>Qt is The Qt Company Ltd. product developed as an open source "
        "project. See <a href=\"https://%3/\">%3</a> for more information.</p>"
        ).arg(QStringLiteral("qt.io/licensing"),
              QStringLiteral("qt.io"));
    QMessageBox *msgBox = new QMessageBox(parent);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowTitle(title.isEmpty() ? tr("About Qt") : title);
    msgBox->setText(translatedTextAboutQtCaption);
    msgBox->setInformativeText(translatedTextAboutQtText);

    QPixmap pm(":/qt-project.org/qmessagebox/images/qtlogo-64.png"_L1);
    if (!pm.isNull())
        msgBox->setIconPixmap(pm);

    // should perhaps be a style hint
#ifdef Q_OS_MAC
    oldMsgBox = msgBox;
    auto *d = msgBox->d_func();
    d->buttonBox->setCenterButtons(true);
#ifdef Q_OS_IOS
    msgBox->setModal(true);
#else
    msgBox->setModal(false);
#endif
    msgBox->show();
#else
    msgBox->exec();
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
// Source and binary compatibility routines for 4.0 and 4.1

static QMessageBox::StandardButton newButton(int button)
{
    // this is needed for source compatibility with Qt 4.0 and 4.1
    if (button == QMessageBox::NoButton || (button & NewButtonMask))
        return QMessageBox::StandardButton(button & QMessageBox::ButtonMask);

    return QMessageBox::NoButton;
}

static bool detectedCompat(int button0, int button1, int button2)
{
    if (button0 != 0 && !(button0 & NewButtonMask))
        return true;
    if (button1 != 0 && !(button1 & NewButtonMask))
        return true;
    if (button2 != 0 && !(button2 & NewButtonMask))
        return true;
    return false;
}

QAbstractButton *QMessageBoxPrivate::findButton(int button0, int button1, int button2, int flags)
{
    Q_Q(QMessageBox);
    int button = 0;

    if (button0 & flags) {
        button = button0;
    } else if (button1 & flags) {
        button = button1;
    } else if (button2 & flags) {
        button = button2;
    }
    return q->button(newButton(button));
}

void QMessageBoxPrivate::addOldButtons(int button0, int button1, int button2)
{
    Q_Q(QMessageBox);
    q->addButton(newButton(button0));
    q->addButton(newButton(button1));
    q->addButton(newButton(button2));
    q->setDefaultButton(
        static_cast<QPushButton *>(findButton(button0, button1, button2, QMessageBox::Default)));
    q->setEscapeButton(findButton(button0, button1, button2, QMessageBox::Escape));
    compatMode = detectedCompat(button0, button1, button2);
}

QAbstractButton *QMessageBoxPrivate::abstractButtonForId(int id) const
{
    Q_Q(const QMessageBox);
    QAbstractButton *result = customButtonList.value(id);
    if (result)
        return result;
    if (id & QMessageBox::FlagMask)    // for compatibility with Qt 4.0/4.1 (even if it is silly)
        return nullptr;
    return q->button(newButton(id));
}

int QMessageBoxPrivate::showOldMessageBox(QWidget *parent, QMessageBox::Icon icon,
                                          const QString &title, const QString &text,
                                          int button0, int button1, int button2)
{
    QMessageBox messageBox(icon, title, text, QMessageBox::NoButton, parent);
    messageBox.d_func()->addOldButtons(button0, button1, button2);
    return messageBox.exec();
}

int QMessageBoxPrivate::showOldMessageBox(QWidget *parent, QMessageBox::Icon icon,
                                            const QString &title, const QString &text,
                                            const QString &button0Text,
                                            const QString &button1Text,
                                            const QString &button2Text,
                                            int defaultButtonNumber,
                                            int escapeButtonNumber)
{
    QMessageBox messageBox(icon, title, text, QMessageBox::NoButton, parent);
    QString myButton0Text = button0Text;
    if (myButton0Text.isEmpty())
        myButton0Text = QDialogButtonBox::tr("OK");
    messageBox.addButton(myButton0Text, QMessageBox::ActionRole);
    if (!button1Text.isEmpty())
        messageBox.addButton(button1Text, QMessageBox::ActionRole);
    if (!button2Text.isEmpty())
        messageBox.addButton(button2Text, QMessageBox::ActionRole);

    const QList<QAbstractButton *> &buttonList = messageBox.d_func()->customButtonList;
    messageBox.setDefaultButton(static_cast<QPushButton *>(buttonList.value(defaultButtonNumber)));
    messageBox.setEscapeButton(buttonList.value(escapeButtonNumber));

    messageBox.exec();

    // Ignore exec return value and use button index instead,
    // as that's what the documentation promises.
    return buttonList.indexOf(messageBox.clickedButton());
}

void QMessageBoxPrivate::retranslateStrings()
{
#if QT_CONFIG(textedit)
    if (detailsButton && detailsText)
        detailsButton->setLabel(detailsText->isHidden() ? ShowLabel : HideLabel);
#endif
}

#if QT_DEPRECATED_SINCE(6,2)
/*!
    \deprecated

    Constructs a message box with a \a title, a \a text, an \a icon,
    and up to three buttons.

    The \a icon must be one of the following:
    \list
    \li QMessageBox::NoIcon
    \li QMessageBox::Question
    \li QMessageBox::Information
    \li QMessageBox::Warning
    \li QMessageBox::Critical
    \endlist

    Each button, \a button0, \a button1 and \a button2, can have one
    of the following values:
    \list
    \li QMessageBox::NoButton
    \li QMessageBox::Ok
    \li QMessageBox::Cancel
    \li QMessageBox::Yes
    \li QMessageBox::No
    \li QMessageBox::Abort
    \li QMessageBox::Retry
    \li QMessageBox::Ignore
    \li QMessageBox::YesAll
    \li QMessageBox::NoAll
    \endlist

    Use QMessageBox::NoButton for the later parameters to have fewer
    than three buttons in your message box. If you don't specify any
    buttons at all, QMessageBox will provide an Ok button.

    One of the buttons can be OR-ed with the QMessageBox::Default
    flag to make it the default button (clicked when Enter is
    pressed).

    One of the buttons can be OR-ed with the QMessageBox::Escape flag
    to make it the cancel or close button (clicked when \uicontrol Esc is
    pressed).

    \snippet dialogs/dialogs.cpp 2

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

    The \a parent and \a f arguments are passed to
    the QDialog constructor.

    \sa setWindowTitle(), setText(), setIcon()
*/
QMessageBox::QMessageBox(const QString &title, const QString &text, Icon icon,
                         int button0, int button1, int button2, QWidget *parent,
                         Qt::WindowFlags f)
    : QDialog(*new QMessageBoxPrivate, parent,
              f /*| Qt::MSWindowsFixedSizeDialogHint #### */| Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
    Q_D(QMessageBox);
    d->init(title, text);
    setIcon(icon);
    d->addOldButtons(button0, button1, button2);
}

/*!
    \deprecated [6.2]. Use the overload taking StandardButtons instead.

    Opens an information message box with the given \a title and the
    \a text. The dialog may have up to three buttons. Each of the
    buttons, \a button0, \a button1 and \a button2 may be set to one
    of the following values:

    \list
    \li QMessageBox::NoButton
    \li QMessageBox::Ok
    \li QMessageBox::Cancel
    \li QMessageBox::Yes
    \li QMessageBox::No
    \li QMessageBox::Abort
    \li QMessageBox::Retry
    \li QMessageBox::Ignore
    \li QMessageBox::YesAll
    \li QMessageBox::NoAll
    \endlist

    If you don't want all three buttons, set the last button, or last
    two buttons to QMessageBox::NoButton.

    One button can be OR-ed with QMessageBox::Default, and one
    button can be OR-ed with QMessageBox::Escape.

    Returns the identity (QMessageBox::Ok, or QMessageBox::No, etc.)
    of the button that was clicked.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

  \warning Do not delete \a parent during the execution of the dialog.
           If you want to do this, you should create the dialog
           yourself using one of the QMessageBox constructors.

    \sa question(), warning(), critical()
*/
int QMessageBox::information(QWidget *parent, const QString &title, const QString& text,
                               int button0, int button1, int button2)
{
    return QMessageBoxPrivate::showOldMessageBox(parent, Information, title, text,
                                                   button0, button1, button2);
}

/*!
    \deprecated [6.2] Use the overload taking StandardButtons instead.
    \overload

    Displays an information message box with the given \a title and
    \a text, as well as one, two or three buttons. Returns the index
    of the button that was clicked (0, 1 or 2).

    \a button0Text is the text of the first button, and is optional.
    If \a button0Text is not supplied, "OK" (translated) will be
    used. \a button1Text is the text of the second button, and is
    optional. \a button2Text is the text of the third button, and is
    optional. \a defaultButtonNumber (0, 1 or 2) is the index of the
    default button; pressing Return or Enter is the same as clicking
    the default button. It defaults to 0 (the first button). \a
    escapeButtonNumber is the index of the escape button; pressing
    \uicontrol Esc is the same as clicking this button. It defaults to -1;
    supply 0, 1 or 2 to make pressing \uicontrol Esc equivalent to clicking
    the relevant button.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

  \warning Do not delete \a parent during the execution of the dialog.
           If you want to do this, you should create the dialog
           yourself using one of the QMessageBox constructors.

    \sa question(), warning(), critical()
*/

int QMessageBox::information(QWidget *parent, const QString &title, const QString& text,
                               const QString& button0Text, const QString& button1Text,
                               const QString& button2Text, int defaultButtonNumber,
                               int escapeButtonNumber)
{
    return QMessageBoxPrivate::showOldMessageBox(parent, Information, title, text,
                                                   button0Text, button1Text, button2Text,
                                                   defaultButtonNumber, escapeButtonNumber);
}

/*!
    \deprecated [6.2]. Use the overload taking StandardButtons
    instead.

    Opens a question message box with the given \a title and \a text.
    The dialog may have up to three buttons. Each of the buttons, \a
    button0, \a button1 and \a button2 may be set to one of the
    following values:

    \list
    \li QMessageBox::NoButton
    \li QMessageBox::Ok
    \li QMessageBox::Cancel
    \li QMessageBox::Yes
    \li QMessageBox::No
    \li QMessageBox::Abort
    \li QMessageBox::Retry
    \li QMessageBox::Ignore
    \li QMessageBox::YesAll
    \li QMessageBox::NoAll
    \endlist

    If you don't want all three buttons, set the last button, or last
    two buttons to QMessageBox::NoButton.

    One button can be OR-ed with QMessageBox::Default, and one
    button can be OR-ed with QMessageBox::Escape.

    Returns the identity (QMessageBox::Yes, or QMessageBox::No, etc.)
    of the button that was clicked.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

  \warning Do not delete \a parent during the execution of the dialog.
           If you want to do this, you should create the dialog
           yourself using one of the QMessageBox constructors.

    \sa information(), warning(), critical()
*/
int QMessageBox::question(QWidget *parent, const QString &title, const QString& text,
                            int button0, int button1, int button2)
{
    return QMessageBoxPrivate::showOldMessageBox(parent, Question, title, text,
                                                   button0, button1, button2);
}

/*!
    \deprecated [6.2]. Use the overload taking StandardButtons instead.
    \overload

    Displays a question message box with the given \a title and \a
    text, as well as one, two or three buttons. Returns the index of
    the button that was clicked (0, 1 or 2).

    \a button0Text is the text of the first button, and is optional.
    If \a button0Text is not supplied, "OK" (translated) will be used.
    \a button1Text is the text of the second button, and is optional.
    \a button2Text is the text of the third button, and is optional.
    \a defaultButtonNumber (0, 1 or 2) is the index of the default
    button; pressing Return or Enter is the same as clicking the
    default button. It defaults to 0 (the first button). \a
    escapeButtonNumber is the index of the Escape button; pressing
    Escape is the same as clicking this button. It defaults to -1;
    supply 0, 1 or 2 to make pressing Escape equivalent to clicking
    the relevant button.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

  \warning Do not delete \a parent during the execution of the dialog.
           If you want to do this, you should create the dialog
           yourself using one of the QMessageBox constructors.

    \sa information(), warning(), critical()
*/
int QMessageBox::question(QWidget *parent, const QString &title, const QString& text,
                            const QString& button0Text, const QString& button1Text,
                            const QString& button2Text, int defaultButtonNumber,
                            int escapeButtonNumber)
{
    return QMessageBoxPrivate::showOldMessageBox(parent, Question, title, text,
                                                   button0Text, button1Text, button2Text,
                                                   defaultButtonNumber, escapeButtonNumber);
}


/*!
    \deprecated [6.2]. Use the overload taking StandardButtons instead.

    Opens a warning message box with the given \a title and \a text.
    The dialog may have up to three buttons. Each of the button
    parameters, \a button0, \a button1 and \a button2 may be set to
    one of the following values:

    \list
    \li QMessageBox::NoButton
    \li QMessageBox::Ok
    \li QMessageBox::Cancel
    \li QMessageBox::Yes
    \li QMessageBox::No
    \li QMessageBox::Abort
    \li QMessageBox::Retry
    \li QMessageBox::Ignore
    \li QMessageBox::YesAll
    \li QMessageBox::NoAll
    \endlist

    If you don't want all three buttons, set the last button, or last
    two buttons to QMessageBox::NoButton.

    One button can be OR-ed with QMessageBox::Default, and one
    button can be OR-ed with QMessageBox::Escape.

    Returns the identity (QMessageBox::Ok or QMessageBox::No or ...)
    of the button that was clicked.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

  \warning Do not delete \a parent during the execution of the dialog.
           If you want to do this, you should create the dialog
           yourself using one of the QMessageBox constructors.

    \sa information(), question(), critical()
*/
int QMessageBox::warning(QWidget *parent, const QString &title, const QString& text,
                           int button0, int button1, int button2)
{
    return QMessageBoxPrivate::showOldMessageBox(parent, Warning, title, text,
                                                   button0, button1, button2);
}

/*!
    \deprecated [6.2]. Use the overload taking StandardButtons instead.
    \overload

    Displays a warning message box with the given \a title and \a
    text, as well as one, two, or three buttons. Returns the number
    of the button that was clicked (0, 1, or 2).

    \a button0Text is the text of the first button, and is optional.
    If \a button0Text is not supplied, "OK" (translated) will be used.
    \a button1Text is the text of the second button, and is optional,
    and \a button2Text is the text of the third button, and is
    optional. \a defaultButtonNumber (0, 1 or 2) is the index of the
    default button; pressing Return or Enter is the same as clicking
    the default button. It defaults to 0 (the first button). \a
    escapeButtonNumber is the index of the Escape button; pressing
    Escape is the same as clicking this button. It defaults to -1;
    supply 0, 1, or 2 to make pressing Escape equivalent to clicking
    the relevant button.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

  \warning Do not delete \a parent during the execution of the dialog.
           If you want to do this, you should create the dialog
           yourself using one of the QMessageBox constructors.

    \sa information(), question(), critical()
*/
int QMessageBox::warning(QWidget *parent, const QString &title, const QString& text,
                           const QString& button0Text, const QString& button1Text,
                           const QString& button2Text, int defaultButtonNumber,
                           int escapeButtonNumber)
{
    return QMessageBoxPrivate::showOldMessageBox(parent, Warning, title, text,
                                                   button0Text, button1Text, button2Text,
                                                   defaultButtonNumber, escapeButtonNumber);
}

/*!
    \deprecated [6.2]. Use the overload taking StandardButtons instead.

    Opens a critical message box with the given \a title and \a text.
    The dialog may have up to three buttons. Each of the button
    parameters, \a button0, \a button1 and \a button2 may be set to
    one of the following values:

    \list
    \li QMessageBox::NoButton
    \li QMessageBox::Ok
    \li QMessageBox::Cancel
    \li QMessageBox::Yes
    \li QMessageBox::No
    \li QMessageBox::Abort
    \li QMessageBox::Retry
    \li QMessageBox::Ignore
    \li QMessageBox::YesAll
    \li QMessageBox::NoAll
    \endlist

    If you don't want all three buttons, set the last button, or last
    two buttons to QMessageBox::NoButton.

    One button can be OR-ed with QMessageBox::Default, and one
    button can be OR-ed with QMessageBox::Escape.

    Returns the identity (QMessageBox::Ok, or QMessageBox::No, etc.)
    of the button that was clicked.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

  \warning Do not delete \a parent during the execution of the dialog.
           If you want to do this, you should create the dialog
           yourself using one of the QMessageBox constructors.

    \sa information(), question(), warning()
*/

int QMessageBox::critical(QWidget *parent, const QString &title, const QString& text,
                          int button0, int button1, int button2)
{
    return QMessageBoxPrivate::showOldMessageBox(parent, Critical, title, text,
                                                 button0, button1, button2);
}

/*!
    \deprecated [6.2]. Use the overload taking StandardButtons instead.
    \overload

    Displays a critical error message box with the given \a title and
    \a text, as well as one, two, or three buttons. Returns the
    number of the button that was clicked (0, 1 or 2).

    \a button0Text is the text of the first button, and is optional.
    If \a button0Text is not supplied, "OK" (translated) will be used.
    \a button1Text is the text of the second button, and is optional,
    and \a button2Text is the text of the third button, and is
    optional. \a defaultButtonNumber (0, 1 or 2) is the index of the
    default button; pressing Return or Enter is the same as clicking
    the default button. It defaults to 0 (the first button). \a
    escapeButtonNumber is the index of the Escape button; pressing
    Escape is the same as clicking this button. It defaults to -1;
    supply 0, 1, or 2 to make pressing Escape equivalent to clicking
    the relevant button.

    The message box is an \l{Qt::ApplicationModal} {application modal}
    dialog box.

  \warning Do not delete \a parent during the execution of the dialog.
           If you want to do this, you should create the dialog
           yourself using one of the QMessageBox constructors.

    \sa information(), question(), warning()
*/
int QMessageBox::critical(QWidget *parent, const QString &title, const QString& text,
                            const QString& button0Text, const QString& button1Text,
                            const QString& button2Text, int defaultButtonNumber,
                            int escapeButtonNumber)
{
    return QMessageBoxPrivate::showOldMessageBox(parent, Critical, title, text,
                                                   button0Text, button1Text, button2Text,
                                                   defaultButtonNumber, escapeButtonNumber);
}


/*!
    \deprecated [6.2]

    Returns the text of the message box button \a button, or
    an empty string if the message box does not contain the button.

    Use button() and QPushButton::text() instead.
*/
QString QMessageBox::buttonText(int button) const
{
    Q_D(const QMessageBox);

    if (QAbstractButton *abstractButton = d->abstractButtonForId(button)) {
        return abstractButton->text();
    } else if (d->buttonBox->buttons().isEmpty() && (button == Ok || button == Old_Ok)) {
        // for compatibility with Qt 4.0/4.1
        return QDialogButtonBox::tr("OK");
    }
    return QString();
}

/*!
    \deprecated [6.2]

    Sets the text of the message box button \a button to \a text.
    Setting the text of a button that is not in the message box is
    silently ignored.

    Use addButton() instead.
*/
void QMessageBox::setButtonText(int button, const QString &text)
{
    Q_D(QMessageBox);
    if (QAbstractButton *abstractButton = d->abstractButtonForId(button)) {
        abstractButton->setText(text);
    } else if (d->buttonBox->buttons().isEmpty() && (button == Ok || button == Old_Ok)) {
        // for compatibility with Qt 4.0/4.1
        addButton(QMessageBox::Ok)->setText(text);
    }
}
#endif // QT_DEPRECATED_SINCE(6,2)


#if QT_CONFIG(textedit)
/*!
  \property QMessageBox::detailedText
  \brief the text to be displayed in the details area.

  The text will be interpreted as a plain text.

  By default, this property contains an empty string.

  \sa QMessageBox::text, QMessageBox::informativeText
*/
QString QMessageBox::detailedText() const
{
    Q_D(const QMessageBox);
    return d->detailsText ? d->detailsText->text() : QString();
}

void QMessageBox::setDetailedText(const QString &text)
{
    Q_D(QMessageBox);
    if (text.isEmpty()) {
        if (d->detailsText) {
            d->detailsText->hide();
            d->detailsText->deleteLater();
        }
        d->detailsText = nullptr;
        removeButton(d->detailsButton);
        if (d->detailsButton) {
            d->detailsButton->hide();
            d->detailsButton->deleteLater();
        }
        d->detailsButton = nullptr;
    } else {
        if (!d->detailsText) {
            d->detailsText = new QMessageBoxDetailsText(this);
            d->detailsText->hide();
        }
        if (!d->detailsButton) {
            const bool autoAddOkButton = d->autoAddOkButton; // QTBUG-39334, addButton() clears the flag.
            d->detailsButton = new DetailButton(this);
            addButton(d->detailsButton, QMessageBox::ActionRole);
            d->autoAddOkButton = autoAddOkButton;
        }
        d->detailsText->setText(text);
    }
    d->setupLayout();
}
#endif // QT_CONFIG(textedit)

/*!
  \property QMessageBox::informativeText

  \brief the informative text that provides a fuller description for
  the message

  Informative text can be used to expand upon the text() to give more
  information to the user, for example describing the consequences of
  the situation, or suggestion alternative solutions.

  The text will be interpreted either as a plain text or as rich text,
  depending on the text format setting (\l QMessageBox::textFormat).
  The default setting is Qt::AutoText, i.e., the message box will try
  to auto-detect the format of the text.

  By default, this property contains an empty string.

  \sa textFormat, QMessageBox::text, QMessageBox::detailedText
*/
QString QMessageBox::informativeText() const
{
    Q_D(const QMessageBox);
    return d->informativeLabel ? d->informativeLabel->text() : QString();
}

void QMessageBox::setInformativeText(const QString &text)
{
    Q_D(QMessageBox);
    if (text.isEmpty()) {
        if (d->informativeLabel) {
            d->informativeLabel->hide();
            d->informativeLabel->deleteLater();
        }
        d->informativeLabel = nullptr;
    } else {
        if (!d->informativeLabel) {
            QLabel *label = new QLabel;
            label->setObjectName("qt_msgbox_informativelabel"_L1);
            label->setTextInteractionFlags(Qt::TextInteractionFlags(style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, nullptr, this)));
            label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            label->setOpenExternalLinks(true);
#ifdef Q_OS_MAC
            // apply a smaller font the information label on the mac
            label->setFont(qt_app_fonts_hash()->value("QTipLabel"));
#endif
            label->setWordWrap(true);
            label->setTextFormat(d->label->textFormat());
            d->informativeLabel = label;
        }
        d->informativeLabel->setText(text);
    }
    d->setupLayout();
}

/*!
    This function shadows QWidget::setWindowTitle().

    Sets the title of the message box to \a title. On \macos,
    the window title is ignored (as required by the \macos
    Guidelines).
*/
void QMessageBox::setWindowTitle(const QString &title)
{
    // Message boxes on the mac do not have a title
#ifndef Q_OS_MAC
    QDialog::setWindowTitle(title);
#else
    Q_UNUSED(title);
#endif
}


/*!
    This function shadows QWidget::setWindowModality().

    Sets the modality of the message box to \a windowModality.

    On \macos, if the modality is set to Qt::WindowModal and the message box
    has a parent, then the message box will be a Qt::Sheet, otherwise the
    message box will be a standard dialog.
*/
void QMessageBox::setWindowModality(Qt::WindowModality windowModality)
{
    QDialog::setWindowModality(windowModality);

    if (parentWidget() && windowModality == Qt::WindowModal)
        setParent(parentWidget(), Qt::Sheet);
    else
        setParent(parentWidget(), Qt::Dialog);
    setDefaultButton(d_func()->defaultButton);
}


QPixmap QMessageBoxPrivate::standardIcon(QMessageBox::Icon icon, QMessageBox *mb)
{
    QStyle *style = mb ? mb->style() : QApplication::style();
    int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, mb);
    QIcon tmpIcon;
    switch (icon) {
    case QMessageBox::Information:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxInformation, nullptr, mb);
        break;
    case QMessageBox::Warning:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, mb);
        break;
    case QMessageBox::Critical:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxCritical, nullptr, mb);
        break;
    case QMessageBox::Question:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxQuestion, nullptr, mb);
        break;
    default:
        break;
    }
    if (!tmpIcon.isNull()) {
        qreal dpr = mb ? mb->devicePixelRatio() : qApp->devicePixelRatio();
        return tmpIcon.pixmap(QSize(iconSize, iconSize), dpr);
    }
    return QPixmap();
}

void QMessageBoxPrivate::initHelper(QPlatformDialogHelper *h)
{
    auto *messageDialogHelper = static_cast<QPlatformMessageDialogHelper *>(h);
    QObjectPrivate::connect(messageDialogHelper, &QPlatformMessageDialogHelper::clicked,
                            this, &QMessageBoxPrivate::helperClicked);
    // Forward state via lambda, so that we can handle addition and removal
    // of checkbox via setCheckBox() after initializing helper.
    QObject::connect(messageDialogHelper, &QPlatformMessageDialogHelper::checkBoxStateChanged,
        q_ptr, [this](Qt::CheckState state) {
            if (checkbox)
                checkbox->setCheckState(state);
        }
    );
    messageDialogHelper->setOptions(options);
}

static QMessageDialogOptions::StandardIcon helperIcon(QMessageBox::Icon i)
{
    switch (i) {
    case QMessageBox::NoIcon:
        return QMessageDialogOptions::NoIcon;
    case QMessageBox::Information:
        return QMessageDialogOptions::Information;
    case QMessageBox::Warning:
        return QMessageDialogOptions::Warning;
    case QMessageBox::Critical:
        return QMessageDialogOptions::Critical;
    case QMessageBox::Question:
        return QMessageDialogOptions::Question;
    }
    return QMessageDialogOptions::NoIcon;
}

static QPlatformDialogHelper::StandardButtons helperStandardButtons(QMessageBox * q)
{
    QPlatformDialogHelper::StandardButtons buttons(int(q->standardButtons()));
    return buttons;
}

bool QMessageBoxPrivate::canBeNativeDialog() const
{
    // Don't use Q_Q here! This function is called from ~QDialog,
    // so Q_Q calling q_func() invokes undefined behavior (invalid cast in q_func()).
    const QDialog * const q = static_cast<const QDialog*>(q_ptr);
    if (nativeDialogInUse)
        return true;
    if (QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)
        || q->testAttribute(Qt::WA_DontShowOnScreen)
        || q->testAttribute(Qt::WA_StyleSheet)
        || (options->options() & QMessageDialogOptions::Option::DontUseNativeDialog)) {
        return false;
    }

    if (strcmp(QMessageBox::staticMetaObject.className(), q->metaObject()->className()) != 0)
        return false;

#if QT_CONFIG(menu)
    for (auto *customButton : buttonBox->buttons()) {
        if (QPushButton *pushButton = qobject_cast<QPushButton *>(customButton)) {
            // We can't support buttons with menus in native dialogs (yet)
            if (pushButton->menu())
                return false;
        }
    }
#endif

    return QDialogPrivate::canBeNativeDialog();
}

void QMessageBoxPrivate::helperPrepareShow(QPlatformDialogHelper *)
{
    Q_Q(QMessageBox);
    options->setWindowTitle(q->windowTitle());
    options->setText(q->text());
    options->setInformativeText(q->informativeText());
#if QT_CONFIG(textedit)
    options->setDetailedText(q->detailedText());
#endif
    options->setStandardIcon(helperIcon(q->icon()));
    options->setIconPixmap(q->iconPixmap());

    // Clear up front, since we might have prepared earlier
    options->clearCustomButtons();

    // Add standard buttons and resolve default/escape button
    auto standardButtons = helperStandardButtons(q);
    for (int button = QDialogButtonBox::StandardButton::FirstButton;
             button <= QDialogButtonBox::StandardButton::LastButton; button <<= 1) {
        auto *standardButton = buttonBox->button(QDialogButtonBox::StandardButton(button));
        if (!standardButton)
            continue;

        if (auto *platformTheme = QGuiApplicationPrivate::platformTheme()) {
            if (standardButton->text() != platformTheme->standardButtonText(button)) {
                // The standard button has been customized, so add it as
                // a custom button instead.
                const auto buttonRole = buttonBox->buttonRole(standardButton);
                options->addButton(standardButton->text(),
                    static_cast<QPlatformDialogHelper::ButtonRole>(buttonRole),
                    standardButton, button);
                standardButtons &= ~QPlatformDialogHelper::StandardButton(button);
            }
        }

        if (standardButton == defaultButton)
            options->setDefaultButton(button);
        else if (standardButton == detectedEscapeButton)
            options->setEscapeButton(button);
    }
    options->setStandardButtons(standardButtons);

    // Add custom buttons and resolve default/escape button
    for (auto *customButton : customButtonList) {
        // Unless it's the details button, since we don't do any
        // plumbing for the button's action in that case.
        if (customButton == detailsButton)
            continue;

        const auto buttonRole = buttonBox->buttonRole(customButton);
        const int buttonId = options->addButton(customButton->text(),
            static_cast<QPlatformDialogHelper::ButtonRole>(buttonRole),
            customButton);

        if (customButton == defaultButton)
            options->setDefaultButton(buttonId);
        else if (customButton == detectedEscapeButton)
            options->setEscapeButton(buttonId);
    }

    if (checkbox)
        options->setCheckBox(checkbox->text(), checkbox->checkState());
}

void qRequireVersion(int argc, char *argv[], QAnyStringView req)
{
    const auto required = QVersionNumber::fromString(req).normalized();
    const auto current = QVersionNumber::fromString(qVersion()).normalized();
    if (current >= required)
        return;
    std::optional<QApplication> application;
    if (!qApp)
        application.emplace(argc, argv);
    const QString message = QApplication::tr("Application \"%1\" requires Qt %2, found Qt %3.")
                                    .arg(qAppName(), required.toString(), current.toString());
    QMessageBox::critical(nullptr, QApplication::tr("Incompatible Qt Library Error"),
                          message, QMessageBox::Abort);
    qFatal("%ls", qUtf16Printable(message));
}

#if QT_DEPRECATED_SINCE(6,2)
/*!
    \deprecated [6.2]

    Returns the pixmap used for a standard icon. This allows the
    pixmaps to be used in more complex message boxes. \a icon
    specifies the required icon, e.g. QMessageBox::Question,
    QMessageBox::Information, QMessageBox::Warning or
    QMessageBox::Critical.

    Call QStyle::standardIcon() with QStyle::SP_MessageBoxInformation etc.
    instead.
*/

QPixmap QMessageBox::standardIcon(Icon icon)
{
    return QMessageBoxPrivate::standardIcon(icon, nullptr);
}
#endif

/*!
    \typedef QMessageBox::Button
    \deprecated

    Use QMessageBox::StandardButton instead.
*/

/*!
    \fn int QMessageBox::information(QWidget *parent, const QString &title,
                                     const QString& text, StandardButton button0,
                                     StandardButton button1)
    \fn int QMessageBox::warning(QWidget *parent, const QString &title,
                                 const QString& text, StandardButton button0,
                                 StandardButton button1)
    \fn int QMessageBox::critical(QWidget *parent, const QString &title,
                                  const QString& text, StandardButton button0,
                                  StandardButton button1)
    \fn int QMessageBox::question(QWidget *parent, const QString &title,
                                  const QString& text, StandardButton button0,
                                  StandardButton button1)
    \internal

    ### Needed for Qt 4 source compatibility
*/

/*!
  \fn int QMessageBox::exec()

  Shows the message box as a \l{QDialog#Modal Dialogs}{modal dialog},
  blocking until the user closes it.

  When using a QMessageBox with standard buttons, this function returns a
  \l StandardButton value indicating the standard button that was clicked.
  When using QMessageBox with custom buttons, this function returns an
  opaque value; use clickedButton() to determine which button was clicked.

  \note The result() function returns also \l StandardButton value instead
  of \l QDialog::DialogCode.

  Users cannot interact with any other window in the same
  application until they close the dialog, either by clicking a
  button or by using a mechanism provided by the window system.

  \sa show(), result()
*/

/*!
    \macro QT_REQUIRE_VERSION(int argc, char **argv, const char *version)
    \relates QMessageBox

    This macro can be used to ensure that the application is run
    with a recent enough version of Qt. This is especially useful
    if your application depends on a specific bug fix introduced in a
    bug-fix release (for example, 6.1.2).

    The \a argc and \a argv parameters are the \c main() function's
    \c argc and \c argv parameters. The \a version parameter is a
    string literal that specifies which version of Qt the application
    requires (for example, "6.1.2").

    Example:

    \snippet code/src_gui_dialogs_qmessagebox.cpp 4
*/

QT_END_NAMESPACE

#include "moc_qmessagebox.cpp"
#include "qmessagebox.moc"
