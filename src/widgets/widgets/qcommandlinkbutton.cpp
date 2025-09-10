// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcommandlinkbutton.h"
#include "qstylepainter.h"
#include "qstyleoption.h"
#include "qtextdocument.h"
#include "qtextlayout.h"
#include "qcolor.h"
#include "qfont.h"
#include <qmath.h>

#include "private/qpushbutton_p.h"
#include "private/qstylesheetstyle_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCommandLinkButton
    \since 4.4
    \brief The QCommandLinkButton widget provides a Vista style command link button.

    \ingroup basicwidgets
    \inmodule QtWidgets

    The command link is a new control that was introduced by Windows Vista. Its
    intended use is similar to that of a radio button in that it is used to choose
    between a set of mutually exclusive options. Command link buttons should not
    be used by themselves but rather as an alternative to radio buttons in
    Wizards and dialogs and makes pressing the "next" button redundant.
    The appearance is generally similar to that of a flat pushbutton, but
    it allows for a descriptive text in addition to the normal button text.
    By default it will also carry an arrow icon, indicating that pressing the
    control will open another window or page.

    \sa QPushButton, QRadioButton
*/

/*!
    \property QCommandLinkButton::description
    \brief A descriptive label to complement the button text

    Setting this property will set a descriptive text on the
    button, complementing the text label. This will usually
    be displayed in a smaller font than the primary text.
*/

/*!
    \property QCommandLinkButton::flat
    \brief This property determines whether the button is displayed as a flat
    panel or with a border.

    By default, this property is set to false.

    \sa QPushButton::flat
*/

class QCommandLinkButtonPrivate : public QPushButtonPrivate
{
    Q_DECLARE_PUBLIC(QCommandLinkButton)

public:
    QCommandLinkButtonPrivate()
        : QPushButtonPrivate(){}

    void init();
    qreal titleSize() const;
    bool usingVistaStyle() const;

    QFont titleFont() const;
    QFont descriptionFont() const;

    QRect titleRect() const;
    QRect descriptionRect() const;

    int textOffset() const;
    int descriptionOffset() const;
    int descriptionHeight(int width) const;
    QColor mergedColors(const QColor &a, const QColor &b, int value) const;

    int topMargin() const { return 10; }
    int leftMargin() const { return 7; }
    int rightMargin() const { return 4; }
    int bottomMargin() const { return 10; }

    QString description;
    QColor currentColor;
};

// Mix colors a and b with a ratio in the range [0-255]
QColor QCommandLinkButtonPrivate::mergedColors(const QColor &a, const QColor &b, int value = 50) const
{
    Q_ASSERT(value >= 0);
    Q_ASSERT(value <= 255);
    QColor tmp = a;
    tmp.setRed((tmp.red() * value) / 255 + (b.red() * (255 - value)) / 255);
    tmp.setGreen((tmp.green() * value) / 255 + (b.green() * (255 - value)) / 255);
    tmp.setBlue((tmp.blue() * value) / 255 + (b.blue() * (255 - value)) / 255);
    return tmp;
}

QFont QCommandLinkButtonPrivate::titleFont() const
{
    Q_Q(const QCommandLinkButton);
    QFont font = q->font();
    if (usingVistaStyle()) {
        font.setPointSizeF(12.0);
    } else {
        font.setBold(true);
        font.setPointSizeF(9.0);
    }

    // Note the font will be resolved against
    // QPainters font, so we need to restore the mask
    int resolve_mask = font.resolve_mask;
    QFont modifiedFont = q->font().resolve(font);
    modifiedFont.detach();
    modifiedFont.resolve_mask = resolve_mask;
    return modifiedFont;
}

QFont QCommandLinkButtonPrivate::descriptionFont() const
{
    Q_Q(const QCommandLinkButton);
    QFont font = q->font();
    font.setPointSizeF(9.0);

    // Note the font will be resolved against
    // QPainters font, so we need to restore the mask
    int resolve_mask = font.resolve_mask;
    QFont modifiedFont = q->font().resolve(font);
    modifiedFont.detach();
    modifiedFont.resolve_mask = resolve_mask;
    return modifiedFont;
}

QRect QCommandLinkButtonPrivate::titleRect() const
{
    Q_Q(const QCommandLinkButton);
    QRect r = q->rect().adjusted(textOffset(), topMargin(), -rightMargin(), 0);
    if (description.isEmpty())
    {
        QFontMetrics fm(titleFont());
        r.setTop(r.top() + qMax(0, (q->icon().actualSize(q->iconSize()).height()
                 - fm.height()) / 2));
    }

    return r;
}

QRect QCommandLinkButtonPrivate::descriptionRect() const
{
    Q_Q(const QCommandLinkButton);
    return q->rect().adjusted(textOffset(), descriptionOffset(),
                              -rightMargin(), -bottomMargin());
}

int QCommandLinkButtonPrivate::textOffset() const
{
    Q_Q(const QCommandLinkButton);
    return q->icon().actualSize(q->iconSize()).width() + leftMargin() + 6;
}

int QCommandLinkButtonPrivate::descriptionOffset() const
{
    QFontMetrics fm(titleFont());
    return topMargin() + fm.height();
}

bool QCommandLinkButtonPrivate::usingVistaStyle() const
{
    Q_Q(const QCommandLinkButton);
    //### This is a hack to detect if we are indeed running Vista style themed and not in classic
    // When we add api to query for this, we should change this implementation to use it.
    return q->property("_qt_usingVistaStyle").toBool()
        && q->style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, nullptr, q) == 0;
}

void QCommandLinkButtonPrivate::init()
{
    Q_Q(QCommandLinkButton);
    QPushButtonPrivate::init();
    q->setAttribute(Qt::WA_Hover);
    q->setAttribute(Qt::WA_MacShowFocusRect, false);

    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::PushButton);
    policy.setHeightForWidth(true);
    q->setSizePolicy(policy);

    q->setIconSize(QSize(20, 20));
    QStyleOptionButton opt;
    q->initStyleOption(&opt);
    q->setIcon(q->style()->standardIcon(QStyle::SP_CommandLink, &opt, q));
}

// Calculates the height of the description text based on widget width
int QCommandLinkButtonPrivate::descriptionHeight(int widgetWidth) const
{
    // Calc width of actual paragraph
    int lineWidth = widgetWidth - textOffset() - rightMargin();

    qreal descriptionheight = 0;
    if (!description.isEmpty()) {
        QTextLayout layout(description);
        layout.setFont(descriptionFont());
        layout.beginLayout();
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(lineWidth);
            line.setPosition(QPointF(0, descriptionheight));
            descriptionheight += line.height();
        }
        layout.endLayout();
    }
    return qCeil(descriptionheight);
}

/*!
    \reimp
 */
QSize QCommandLinkButton::minimumSizeHint() const
{
    Q_D(const QCommandLinkButton);
    QSize size = sizeHint();
    int minimumHeight = qMax(d->descriptionOffset() + d->bottomMargin(),
                             icon().actualSize(iconSize()).height() + d->topMargin());
    size.setHeight(minimumHeight);
    return size;
}

void QCommandLinkButton::initStyleOption(QStyleOptionButton *option) const
{
    QPushButton::initStyleOption(option);
    option->features |= QStyleOptionButton::CommandLinkButton;
}

/*!
    Constructs a command link with no text and a \a parent.
*/

QCommandLinkButton::QCommandLinkButton(QWidget *parent)
: QPushButton(*new QCommandLinkButtonPrivate, parent)
{
    Q_D(QCommandLinkButton);
    d->init();
}

/*!
    Constructs a command link with the parent \a parent and the text \a
    text.
*/

QCommandLinkButton::QCommandLinkButton(const QString &text, QWidget *parent)
    : QCommandLinkButton(parent)
{
    setText(text);
}

/*!
    Constructs a command link with a \a text, a \a description, and a \a parent.
*/
QCommandLinkButton::QCommandLinkButton(const QString &text, const QString &description, QWidget *parent)
    : QCommandLinkButton(text, parent)
{
    setDescription(description);
}

/*!
    Destructor.
*/
QCommandLinkButton::~QCommandLinkButton()
{
}

/*! \reimp */
bool QCommandLinkButton::event(QEvent *e)
{
    if (e->type() == QEvent::StyleChange) {
        // If the new style is a QStyleSheetStyle, don't reset the icon, because:
        // - either it has been explicitly set, in which case we want to keep it.
        // - or it has been initialised by the previous style, which is now the base style,
        //   in which case we want to keep it as well.
        // - or it has been set in the style sheet, in which case we don't want to override it here.
        // When a style sheet with an icon is replaced by one without an icon, the old icon
        // will be reset, when baseStyle()->repolish() is called.
        if (!qobject_cast<QStyleSheetStyle *>(style())) {
            QStyleOptionButton opt;
            initStyleOption(&opt);
            setIcon(style()->standardIcon(QStyle::SP_CommandLink, &opt, this));
        }
    }

    return QPushButton::event(e);
}

/*! \reimp */
QSize QCommandLinkButton::sizeHint() const
{
//  Standard size hints from UI specs
//  Without note: 135, 41
//  With note: 135, 60
    Q_D(const QCommandLinkButton);

    QSize size = QPushButton::sizeHint();
    QFontMetrics fm(d->titleFont());
    int textWidth = qMax(fm.horizontalAdvance(text()), 135);
    int buttonWidth = textWidth + d->textOffset() + d->rightMargin();
    int heightWithoutDescription = d->descriptionOffset() + d->bottomMargin();

    size.setWidth(qMax(size.width(), buttonWidth));
    size.setHeight(qMax(d->description.isEmpty() ? 41 : 60,
                        heightWithoutDescription + d->descriptionHeight(buttonWidth)));
    return size;
}

/*! \reimp */
int QCommandLinkButton::heightForWidth(int width) const
{
    Q_D(const QCommandLinkButton);
    int heightWithoutDescription = d->descriptionOffset() + d->bottomMargin();
    // find the width available for the description area
    return qMax(heightWithoutDescription + d->descriptionHeight(width),
                icon().actualSize(iconSize()).height() + d->topMargin() +
                d->bottomMargin());
}

/*! \reimp */
void QCommandLinkButton::paintEvent(QPaintEvent *)
{
    Q_D(QCommandLinkButton);
    QStylePainter p(this);

    QStyleOptionButton option;
    initStyleOption(&option);

    option.text = QString();
    option.icon = QIcon(); //we draw this ourselves

    const int vOffset = isDown()
        ? style()->pixelMetric(QStyle::PM_ButtonShiftVertical, &option, this) : 0;
    const int hOffset = isDown()
        ? style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, &option, this) : 0;

    //Draw icon
    p.drawControl(QStyle::CE_PushButton, option);
    if (!icon().isNull()) {
        const auto size = icon().actualSize(iconSize());
        const auto mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        const auto state = isChecked() ? QIcon::On : QIcon::Off;
        const auto rect = QRect(d->leftMargin() + hOffset, d->topMargin() + vOffset,
                                size.width(), size.height());
        icon().paint(&p, rect, Qt::AlignCenter, mode, state);
    }

    //Draw title
    QColor textColor = palette().buttonText().color();
    if (isEnabled() && d->usingVistaStyle()) {
        textColor = option.palette.buttonText().color();
        if (underMouse() && !isDown())
            textColor = option.palette.brightText().color();
        //A simple text color transition
        d->currentColor = d->mergedColors(textColor, d->currentColor, 60);
        option.palette.setColor(QPalette::ButtonText, d->currentColor);
    }

    int textflags = Qt::TextShowMnemonic;
    if (!style()->styleHint(QStyle::SH_UnderlineShortcut, &option, this))
        textflags |= Qt::TextHideMnemonic;

    p.setFont(d->titleFont());
    p.drawItemText(d->titleRect().translated(hOffset, vOffset),
                    textflags, option.palette, isEnabled(), text(), QPalette::ButtonText);

    //Draw description
    textflags |= Qt::TextWordWrap | Qt::ElideRight;
    p.setFont(d->descriptionFont());
    p.drawItemText(d->descriptionRect().translated(hOffset, vOffset), textflags,
                    option.palette, isEnabled(), description(), QPalette::ButtonText);
}

void QCommandLinkButton::setDescription(const QString &description)
{
    Q_D(QCommandLinkButton);
    d->description = description;
    updateGeometry();
    update();
}

QString QCommandLinkButton::description() const
{
    Q_D(const QCommandLinkButton);
    return d->description;
}

QT_END_NAMESPACE

#include "moc_qcommandlinkbutton.cpp"
