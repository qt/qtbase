/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qitemdelegate.h"

#include <qabstractitemmodel.h>
#include <qapplication.h>
#include <qbrush.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsize.h>
#include <qstyle.h>
#include <qdatetime.h>
#include <qstyleoption.h>
#include <qevent.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qpixmapcache.h>
#include <qitemeditorfactory.h>
#include <qmetaobject.h>
#include <qtextlayout.h>
#include <private/qabstractitemdelegate_p.h>
#include <private/qtextengine_p.h>
#include <qdebug.h>
#include <qlocale.h>
#include <qmath.h>

#include <limits.h>

// keep in sync with QAbstractItemDelegate::helpEvent()
#ifndef DBL_DIG
#  define DBL_DIG 10
#endif

QT_BEGIN_NAMESPACE

class QItemDelegatePrivate : public QAbstractItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QItemDelegate)

public:
    QItemDelegatePrivate() : f(nullptr), clipPainting(true) {}

    inline const QItemEditorFactory *editorFactory() const
        { return f ? f : QItemEditorFactory::defaultFactory(); }

    inline QIcon::Mode iconMode(QStyle::State state) const
        {
            if (!(state & QStyle::State_Enabled)) return QIcon::Disabled;
            if (state & QStyle::State_Selected) return QIcon::Selected;
            return QIcon::Normal;
        }

    inline QIcon::State iconState(QStyle::State state) const
        { return state & QStyle::State_Open ? QIcon::On : QIcon::Off; }

    inline static QString replaceNewLine(QString text)
        {
            text.replace(QLatin1Char('\n'), QChar::LineSeparator);
            return text;
        }

    QString valueToText(const QVariant &value, const QStyleOptionViewItem &option) const;

    QItemEditorFactory *f;
    bool clipPainting;

    QRect displayRect(const QModelIndex &index, const QStyleOptionViewItem &option,
                       const QRect &decorationRect, const QRect &checkRect) const;
    QRect textLayoutBounds(const QStyleOptionViewItem &option,
                           const QRect &decorationRect, const QRect &checkRect) const;
    QSizeF doTextLayout(int lineWidth) const;
    mutable QTextLayout textLayout;
    mutable QTextOption textOption;

    const QWidget *widget(const QStyleOptionViewItem &option) const
    {
        return option.widget;
    }

    // ### temporary hack until we have QStandardItemDelegate
    mutable struct Icon {
        QIcon icon;
        QIcon::Mode mode;
        QIcon::State state;
    } tmp;
};

QRect QItemDelegatePrivate::displayRect(const QModelIndex &index, const QStyleOptionViewItem &option,
                                        const QRect &decorationRect, const QRect &checkRect) const
{
    Q_Q(const QItemDelegate);
    const QVariant value = index.data(Qt::DisplayRole);
    if (!value.isValid() || value.isNull())
        return QRect();

    const QString text = valueToText(value, option);
    const QVariant fontVal = index.data(Qt::FontRole);
    const QFont fnt = qvariant_cast<QFont>(fontVal).resolve(option.font);
    return q->textRectangle(nullptr,
                            textLayoutBounds(option, decorationRect, checkRect),
                            fnt, text);
}

// similar to QCommonStylePrivate::viewItemSize(Qt::DisplayRole)
QRect QItemDelegatePrivate::textLayoutBounds(const QStyleOptionViewItem &option,
                                             const QRect &decorationRect, const QRect &checkRect) const
{
    QRect rect = option.rect;
    const QWidget *w = widget(option);
    QStyle *style = w ? w->style() : QApplication::style();
    const bool wrapText = option.features & QStyleOptionViewItem::WrapText;
    // see QItemDelegate::drawDisplay
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, w) + 1;
    switch (option.decorationPosition) {
    case QStyleOptionViewItem::Left:
    case QStyleOptionViewItem::Right:
        rect.setWidth(wrapText && rect.isValid() ? rect.width() - 2 * textMargin : (QFIXED_MAX));
        break;
    case QStyleOptionViewItem::Top:
    case QStyleOptionViewItem::Bottom:
        rect.setWidth(wrapText ? option.decorationSize.width() - 2 * textMargin : (QFIXED_MAX));
        break;
    }

    if (wrapText) {
        if (!decorationRect.isNull())
            rect.setWidth(rect.width() - decorationRect.width() - 2 * textMargin);
        if (!checkRect.isNull())
            rect.setWidth(rect.width() - checkRect.width() - 2 * textMargin);
        // adjust height to be sure that the text fits
        const QSizeF size = doTextLayout(rect.width());
        rect.setHeight(qCeil(size.height()));
    }

    return rect;
}

QSizeF QItemDelegatePrivate::doTextLayout(int lineWidth) const
{
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
}

/*!
    \class QItemDelegate

    \brief The QItemDelegate class provides display and editing facilities for
    data items from a model.

    \ingroup model-view
    \inmodule QtWidgets

    QItemDelegate can be used to provide custom display features and editor
    widgets for item views based on QAbstractItemView subclasses. Using a
    delegate for this purpose allows the display and editing mechanisms to be
    customized and developed independently from the model and view.

    The QItemDelegate class is one of the \l{Model/View Classes} and
    is part of Qt's \l{Model/View Programming}{model/view framework}.
    Note that QStyledItemDelegate has taken over the job of drawing
    Qt's item views. We recommend the use of QStyledItemDelegate when
    creating new delegates.

    When displaying items from a custom model in a standard view, it is
    often sufficient to simply ensure that the model returns appropriate
    data for each of the \l{Qt::ItemDataRole}{roles} that determine the
    appearance of items in views. The default delegate used by Qt's
    standard views uses this role information to display items in most
    of the common forms expected by users. However, it is sometimes
    necessary to have even more control over the appearance of items than
    the default delegate can provide.

    This class provides default implementations of the functions for
    painting item data in a view and editing data from item models.
    Default implementations of the paint() and sizeHint() virtual
    functions, defined in QAbstractItemDelegate, are provided to
    ensure that the delegate implements the correct basic behavior
    expected by views. You can reimplement these functions in
    subclasses to customize the appearance of items.

    When editing data in an item view, QItemDelegate provides an
    editor widget, which is a widget that is placed on top of the view
    while editing takes place. Editors are created with a
    QItemEditorFactory; a default static instance provided by
    QItemEditorFactory is installed on all item delegates. You can set
    a custom factory using setItemEditorFactory() or set a new default
    factory with QItemEditorFactory::setDefaultFactory(). It is the
    data stored in the item model with the Qt::EditRole that is edited.

    Only the standard editing functions for widget-based delegates are
    reimplemented here:

    \list
        \li createEditor() returns the widget used to change data from the model
           and can be reimplemented to customize editing behavior.
        \li setEditorData() provides the widget with data to manipulate.
        \li updateEditorGeometry() ensures that the editor is displayed correctly
           with respect to the item view.
        \li setModelData() returns updated data to the model.
    \endlist

    The closeEditor() signal indicates that the user has completed editing the data,
    and that the editor widget can be destroyed.

    \section1 Standard Roles and Data Types

    The default delegate used by the standard views supplied with Qt
    associates each standard role (defined by Qt::ItemDataRole) with certain
    data types. Models that return data in these types can influence the
    appearance of the delegate as described in the following table.

    \table
    \header \li Role \li Accepted Types
    \omit
    \row    \li \l Qt::AccessibleDescriptionRole \li QString
    \row    \li \l Qt::AccessibleTextRole \li QString
    \endomit
    \row    \li \l Qt::BackgroundRole \li QBrush (\since 4.2)
    \row    \li \l Qt::BackgroundColorRole \li QColor (obsolete; use Qt::BackgroundRole instead)
    \row    \li \l Qt::CheckStateRole \li Qt::CheckState
    \row    \li \l Qt::DecorationRole \li QIcon, QPixmap and QColor
    \row    \li \l Qt::DisplayRole \li QString and types with a string representation
    \row    \li \l Qt::EditRole \li See QItemEditorFactory for details
    \row    \li \l Qt::FontRole \li QFont
    \row    \li \l Qt::SizeHintRole \li QSize
    \omit
    \row    \li \l Qt::StatusTipRole \li
    \endomit
    \row    \li \l Qt::TextAlignmentRole \li Qt::Alignment
    \row    \li \l Qt::ForegroundRole \li QBrush (\since 4.2)
    \row    \li \l Qt::TextColorRole \li QColor (obsolete; use Qt::ForegroundRole instead)
    \omit
    \row    \li \l Qt::ToolTipRole
    \row    \li \l Qt::WhatsThisRole
    \endomit
    \endtable

    If the default delegate does not allow the level of customization that
    you need, either for display purposes or for editing data, it is possible to
    subclass QItemDelegate to implement the desired behavior.

    \section1 Subclassing

    When subclassing QItemDelegate to create a delegate that displays items
    using a custom renderer, it is important to ensure that the delegate can
    render items suitably for all the required states; e.g. selected,
    disabled, checked. The documentation for the paint() function contains
    some hints to show how this can be achieved.

    You can provide custom editors by using a QItemEditorFactory. The
    \l{Color Editor Factory Example} shows how a custom editor can be
    made available to delegates with the default item editor
    factory. This way, there is no need to subclass QItemDelegate.  An
    alternative is to reimplement createEditor(), setEditorData(),
    setModelData(), and updateEditorGeometry(). This process is
    described in the \l{Spin Box Delegate Example}.

    \section1 QStyledItemDelegate vs. QItemDelegate

    Since Qt 4.4, there are two delegate classes: QItemDelegate and
    QStyledItemDelegate. However, the default delegate is QStyledItemDelegate.
    These two classes are independent alternatives to painting and providing
    editors for items in views. The difference between them is that
    QStyledItemDelegate uses the current style to paint its items. We therefore
    recommend using QStyledItemDelegate as the base class when implementing
    custom delegates or when working with Qt style sheets. The code required
    for either class should be equal unless the custom delegate needs to use
    the style for drawing.

    \sa {Delegate Classes}, QStyledItemDelegate, QAbstractItemDelegate,
        {Spin Box Delegate Example}, {Settings Editor Example},
        {Icons Example}
*/

/*!
    Constructs an item delegate with the given \a parent.
*/

QItemDelegate::QItemDelegate(QObject *parent)
    : QAbstractItemDelegate(*new QItemDelegatePrivate(), parent)
{

}

/*!
    Destroys the item delegate.
*/

QItemDelegate::~QItemDelegate()
{
}

/*!
  \property QItemDelegate::clipping
  \brief if the delegate should clip the paint events
  \since 4.2

  This property will set the paint clip to the size of the item.
  The default value is on. It is useful for cases such
  as when images are larger than the size of the item.
*/

bool QItemDelegate::hasClipping() const
{
    Q_D(const QItemDelegate);
    return d->clipPainting;
}

void QItemDelegate::setClipping(bool clip)
{
    Q_D(QItemDelegate);
    d->clipPainting = clip;
}

QString QItemDelegatePrivate::valueToText(const QVariant &value, const QStyleOptionViewItem &option) const
{
    return textForRole(Qt::DisplayRole, value, option.locale, DBL_DIG);
}

/*!
    Renders the delegate using the given \a painter and style \a option for
    the item specified by \a index.

    When reimplementing this function in a subclass, you should update the area
    held by the option's \l{QStyleOption::rect}{rect} variable, using the
    option's \l{QStyleOption::state}{state} variable to determine the state of
    the item to be displayed, and adjust the way it is painted accordingly.

    For example, a selected item may need to be displayed differently to
    unselected items, as shown in the following code:

    \snippet itemviews/pixelator/pixeldelegate.cpp 2
    \dots

    After painting, you should ensure that the painter is returned to its
    the state it was supplied in when this function was called. For example,
    it may be useful to call QPainter::save() before painting and
    QPainter::restore() afterwards.

    \sa QStyle::State
*/
void QItemDelegate::paint(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    Q_D(const QItemDelegate);
    Q_ASSERT(index.isValid());

    QStyleOptionViewItem opt = setOptions(index, option);

    // prepare
    painter->save();
    if (d->clipPainting)
        painter->setClipRect(opt.rect);

    // get the data and the rectangles

    QVariant value;

    QPixmap pixmap;
    QRect decorationRect;
    value = index.data(Qt::DecorationRole);
    if (value.isValid()) {
        // ### we need the pixmap to call the virtual function
        pixmap = decoration(opt, value);
        if (value.userType() == QMetaType::QIcon) {
            d->tmp.icon = qvariant_cast<QIcon>(value);
            d->tmp.mode = d->iconMode(option.state);
            d->tmp.state = d->iconState(option.state);
            const QSize size = d->tmp.icon.actualSize(option.decorationSize,
                                                      d->tmp.mode, d->tmp.state);
            decorationRect = QRect(QPoint(0, 0), size);
        } else {
            d->tmp.icon = QIcon();
            decorationRect = QRect(QPoint(0, 0), pixmap.size());
        }
    } else {
        d->tmp.icon = QIcon();
        decorationRect = QRect();
    }

    QRect checkRect;
    Qt::CheckState checkState = Qt::Unchecked;
    value = index.data(Qt::CheckStateRole);
    if (value.isValid()) {
        checkState = static_cast<Qt::CheckState>(value.toInt());
        checkRect = doCheck(opt, opt.rect, value);
    }

    QString text;
    QRect displayRect;
    value = index.data(Qt::DisplayRole);
    if (value.isValid() && !value.isNull()) {
        text = d->valueToText(value, opt);
        displayRect = d->displayRect(index, opt, decorationRect, checkRect);
    }

    // do the layout

    doLayout(opt, &checkRect, &decorationRect, &displayRect, false);

    // draw the item

    drawBackground(painter, opt, index);
    drawCheck(painter, opt, checkRect, checkState);
    drawDecoration(painter, opt, decorationRect, pixmap);
    drawDisplay(painter, opt, displayRect, text);
    drawFocus(painter, opt, displayRect);

    // done
    painter->restore();
}

/*!
    Returns the size needed by the delegate to display the item
    specified by \a index, taking into account the style information
    provided by \a option.

    When reimplementing this function, note that in case of text
    items, QItemDelegate adds a margin (i.e. 2 *
    QStyle::PM_FocusFrameHMargin) to the length of the text.
*/

QSize QItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    Q_D(const QItemDelegate);
    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);
    QRect decorationRect = rect(option, index, Qt::DecorationRole);
    QRect checkRect = rect(option, index, Qt::CheckStateRole);
    QRect displayRect = d->displayRect(index, option, decorationRect, checkRect);

    doLayout(option, &checkRect, &decorationRect, &displayRect, true);

    return (decorationRect|displayRect|checkRect).size();
}

/*!
    Returns the widget used to edit the item specified by \a index
    for editing. The \a parent widget and style \a option are used to
    control how the editor widget appears.

    \sa QAbstractItemDelegate::createEditor()
*/

QWidget *QItemDelegate::createEditor(QWidget *parent,
                                     const QStyleOptionViewItem &,
                                     const QModelIndex &index) const
{
    Q_D(const QItemDelegate);
    if (!index.isValid())
        return nullptr;
    const QItemEditorFactory *factory = d->f;
    if (factory == nullptr)
        factory = QItemEditorFactory::defaultFactory();
    QWidget *w = factory->createEditor(index.data(Qt::EditRole).userType(), parent);
    if (w)
        w->setFocusPolicy(Qt::WheelFocus);
    return w;
}

/*!
    Sets the data to be displayed and edited by the \a editor from the
    data model item specified by the model \a index.

    The default implementation stores the data in the \a editor
    widget's \l {Qt's Property System} {user property}.

    \sa QMetaProperty::isUser()
*/

void QItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(editor);
    Q_UNUSED(index);
#else
    QVariant v = index.data(Qt::EditRole);
    QByteArray n = editor->metaObject()->userProperty().name();

    if (!n.isEmpty()) {
        if (!v.isValid())
            v = QVariant(editor->property(n).userType(), (const void *)nullptr);
        editor->setProperty(n, v);
    }
#endif
}

/*!
    Gets data from the \a editor widget and stores it in the specified
    \a model at the item \a index.

    The default implementation gets the value to be stored in the data
    model from the \a editor widget's \l {Qt's Property System} {user
    property}.

    \sa QMetaProperty::isUser()
*/

void QItemDelegate::setModelData(QWidget *editor,
                                 QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(model);
    Q_UNUSED(editor);
    Q_UNUSED(index);
#else
    Q_D(const QItemDelegate);
    Q_ASSERT(model);
    Q_ASSERT(editor);
    QByteArray n = editor->metaObject()->userProperty().name();
    if (n.isEmpty())
        n = d->editorFactory()->valuePropertyName(
            model->data(index, Qt::EditRole).userType());
    if (!n.isEmpty())
        model->setData(index, editor->property(n), Qt::EditRole);
#endif
}

/*!
    Updates the \a editor for the item specified by \a index
    according to the style \a option given.
*/

void QItemDelegate::updateEditorGeometry(QWidget *editor,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    if (!editor)
        return;
    Q_ASSERT(index.isValid());
    QPixmap pixmap = decoration(option, index.data(Qt::DecorationRole));
    QString text = QItemDelegatePrivate::replaceNewLine(index.data(Qt::DisplayRole).toString());
    QRect pixmapRect = QRect(QPoint(0, 0), option.decorationSize).intersected(pixmap.rect());
    QRect textRect = textRectangle(nullptr, option.rect, option.font, text);
    QRect checkRect = doCheck(option, textRect, index.data(Qt::CheckStateRole));
    QStyleOptionViewItem opt = option;
    opt.showDecorationSelected = true; // let the editor take up all available space
    doLayout(opt, &checkRect, &pixmapRect, &textRect, false);
    editor->setGeometry(textRect);
}

/*!
  Returns the editor factory used by the item delegate.
  If no editor factory is set, the function will return null.

  \sa setItemEditorFactory()
*/
QItemEditorFactory *QItemDelegate::itemEditorFactory() const
{
    Q_D(const QItemDelegate);
    return d->f;
}

/*!
  Sets the editor factory to be used by the item delegate to be the \a factory
  specified. If no editor factory is set, the item delegate will use the
  default editor factory.

  \sa itemEditorFactory()
*/
void QItemDelegate::setItemEditorFactory(QItemEditorFactory *factory)
{
    Q_D(QItemDelegate);
    d->f = factory;
}

/*!
   Renders the item view \a text within the rectangle specified by \a rect
   using the given \a painter and style \a option.
*/

void QItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                                const QRect &rect, const QString &text) const
{
    Q_D(const QItemDelegate);

    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                              ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, option.palette.brush(cg, QPalette::Highlight));
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    if (text.isEmpty())
        return;

    if (option.state & QStyle::State_Editing) {
        painter->save();
        painter->setPen(option.palette.color(cg, QPalette::Text));
        painter->drawRect(rect.adjusted(0, 0, -1, -1));
        painter->restore();
    }

    const QStyleOptionViewItem opt = option;

    const QWidget *widget = d->widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, widget) + 1;
    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = opt.features & QStyleOptionViewItem::WrapText;
    d->textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
    d->textOption.setTextDirection(option.direction);
    d->textOption.setAlignment(QStyle::visualAlignment(option.direction, option.displayAlignment));
    d->textLayout.setTextOption(d->textOption);
    d->textLayout.setFont(option.font);
    d->textLayout.setText(QItemDelegatePrivate::replaceNewLine(text));

    QSizeF textLayoutSize = d->doTextLayout(textRect.width());

    if (textRect.width() < textLayoutSize.width()
        || textRect.height() < textLayoutSize.height()) {
        QString elided;
        int start = 0;
        int end = text.indexOf(QChar::LineSeparator, start);
        if (end == -1) {
            elided += option.fontMetrics.elidedText(text, option.textElideMode, textRect.width());
        } else {
            while (end != -1) {
                elided += option.fontMetrics.elidedText(text.mid(start, end - start),
                                                        option.textElideMode, textRect.width());
                elided += QChar::LineSeparator;
                start = end + 1;
                end = text.indexOf(QChar::LineSeparator, start);
            }
            //let's add the last line (after the last QChar::LineSeparator)
            elided += option.fontMetrics.elidedText(text.mid(start),
                                                    option.textElideMode, textRect.width());
        }
        d->textLayout.setText(elided);
        textLayoutSize = d->doTextLayout(textRect.width());
    }

    const QSize layoutSize(textRect.width(), int(textLayoutSize.height()));
    const QRect layoutRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                                  layoutSize, textRect);
    // if we still overflow even after eliding the text, enable clipping
    if (!hasClipping() && (textRect.width() < textLayoutSize.width()
                           || textRect.height() < textLayoutSize.height())) {
        painter->save();
        painter->setClipRect(layoutRect);
        d->textLayout.draw(painter, layoutRect.topLeft(), QVector<QTextLayout::FormatRange>(), layoutRect);
        painter->restore();
    } else {
        d->textLayout.draw(painter, layoutRect.topLeft(), QVector<QTextLayout::FormatRange>(), layoutRect);
    }
}

/*!
    Renders the decoration \a pixmap within the rectangle specified by
    \a rect using the given \a painter and style \a option.
*/
void QItemDelegate::drawDecoration(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QRect &rect, const QPixmap &pixmap) const
{
    Q_D(const QItemDelegate);
    // if we have an icon, we ignore the pixmap
    if (!d->tmp.icon.isNull()) {
        d->tmp.icon.paint(painter, rect, option.decorationAlignment,
                          d->tmp.mode, d->tmp.state);
        return;
    }

    if (pixmap.isNull() || !rect.isValid())
        return;
    QPoint p = QStyle::alignedRect(option.direction, option.decorationAlignment,
                                   pixmap.size(), rect).topLeft();
    if (option.state & QStyle::State_Selected) {
        const QPixmap pm = selectedPixmap(pixmap, option.palette, option.state & QStyle::State_Enabled);
        painter->drawPixmap(p, pm);
    } else {
        painter->drawPixmap(p, pixmap);
    }
}

/*!
    Renders the region within the rectangle specified by \a rect, indicating
    that it has the focus, using the given \a painter and style \a option.
*/

void QItemDelegate::drawFocus(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QRect &rect) const
{
    Q_D(const QItemDelegate);
    if ((option.state & QStyle::State_HasFocus) == 0 || !rect.isValid())
        return;
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(option);
    o.rect = rect;
    o.state |= QStyle::State_KeyboardFocusChange;
    o.state |= QStyle::State_Item;
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                              ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected)
                                             ? QPalette::Highlight : QPalette::Window);
    const QWidget *widget = d->widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter, widget);
}

/*!
    Renders a check indicator within the rectangle specified by \a
    rect, using the given \a painter and style \a option, using the
    given \a state.
*/

void QItemDelegate::drawCheck(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QRect &rect, Qt::CheckState state) const
{
    Q_D(const QItemDelegate);
    if (!rect.isValid())
        return;

    QStyleOptionViewItem opt(option);
    opt.rect = rect;
    opt.state = opt.state & ~QStyle::State_HasFocus;

    switch (state) {
    case Qt::Unchecked:
        opt.state |= QStyle::State_Off;
        break;
    case Qt::PartiallyChecked:
        opt.state |= QStyle::State_NoChange;
        break;
    case Qt::Checked:
        opt.state |= QStyle::State_On;
        break;
    }

    const QWidget *widget = d->widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &opt, painter, widget);
}

/*!
    \since 4.2

    Renders the item background for the given \a index,
    using the given \a painter and style \a option.
*/

void QItemDelegate::drawBackground(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
    } else {
        QVariant value = index.data(Qt::BackgroundRole);
        if (value.canConvert<QBrush>()) {
            QPointF oldBO = painter->brushOrigin();
            painter->setBrushOrigin(option.rect.topLeft());
            painter->fillRect(option.rect, qvariant_cast<QBrush>(value));
            painter->setBrushOrigin(oldBO);
        }
    }
}


/*!
    \internal

    Code duplicated in QCommonStylePrivate::viewItemLayout
*/

void QItemDelegate::doLayout(const QStyleOptionViewItem &option,
                             QRect *checkRect, QRect *pixmapRect, QRect *textRect,
                             bool hint) const
{
    Q_ASSERT(checkRect && pixmapRect && textRect);
    Q_D(const QItemDelegate);
    const QWidget *widget = d->widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();
    const bool hasCheck = checkRect->isValid();
    const bool hasPixmap = pixmapRect->isValid();
    const bool hasText = textRect->isValid();
    const bool hasMargin = (hasText | hasPixmap | hasCheck);
    const int frameHMargin = hasMargin ?
                style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, widget) + 1 : 0;
    const int textMargin = hasText ? frameHMargin : 0;
    const int pixmapMargin = hasPixmap ? frameHMargin : 0;
    const int checkMargin = hasCheck ? frameHMargin : 0;
    const int x = option.rect.left();
    const int y = option.rect.top();
    int w, h;

    textRect->adjust(-textMargin, 0, textMargin, 0); // add width padding
    if (textRect->height() == 0 && (!hasPixmap || !hint)) {
        //if there is no text, we still want to have a decent height for the item sizeHint and the editor size
        textRect->setHeight(option.fontMetrics.height());
    }

    QSize pm(0, 0);
    if (hasPixmap) {
        pm = pixmapRect->size();
        pm.rwidth() += 2 * pixmapMargin;
    }
    if (hint) {
        h = qMax(checkRect->height(), qMax(textRect->height(), pm.height()));
        if (option.decorationPosition == QStyleOptionViewItem::Left
            || option.decorationPosition == QStyleOptionViewItem::Right) {
            w = textRect->width() + pm.width();
        } else {
            w = qMax(textRect->width(), pm.width());
        }
    } else {
        w = option.rect.width();
        h = option.rect.height();
    }

    int cw = 0;
    QRect check;
    if (hasCheck) {
        cw = checkRect->width() + 2 * checkMargin;
        if (hint) w += cw;
        if (option.direction == Qt::RightToLeft) {
            check.setRect(x + w - cw, y, cw, h);
        } else {
            check.setRect(x, y, cw, h);
        }
    }

    // at this point w should be the *total* width

    QRect display;
    QRect decoration;
    switch (option.decorationPosition) {
    case QStyleOptionViewItem::Top: {
        if (hasPixmap)
            pm.setHeight(pm.height() + pixmapMargin); // add space
        h = hint ? textRect->height() : h - pm.height();

        if (option.direction == Qt::RightToLeft) {
            decoration.setRect(x, y, w - cw, pm.height());
            display.setRect(x, y + pm.height(), w - cw, h);
        } else {
            decoration.setRect(x + cw, y, w - cw, pm.height());
            display.setRect(x + cw, y + pm.height(), w - cw, h);
        }
        break; }
    case QStyleOptionViewItem::Bottom: {
        if (hasText)
            textRect->setHeight(textRect->height() + textMargin); // add space
        h = hint ? textRect->height() + pm.height() : h;

        if (option.direction == Qt::RightToLeft) {
            display.setRect(x, y, w - cw, textRect->height());
            decoration.setRect(x, y + textRect->height(), w - cw, h - textRect->height());
        } else {
            display.setRect(x + cw, y, w - cw, textRect->height());
            decoration.setRect(x + cw, y + textRect->height(), w - cw, h - textRect->height());
        }
        break; }
    case QStyleOptionViewItem::Left: {
        if (option.direction == Qt::LeftToRight) {
            decoration.setRect(x + cw, y, pm.width(), h);
            display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
        } else {
            display.setRect(x, y, w - pm.width() - cw, h);
            decoration.setRect(display.right() + 1, y, pm.width(), h);
        }
        break; }
    case QStyleOptionViewItem::Right: {
        if (option.direction == Qt::LeftToRight) {
            display.setRect(x + cw, y, w - pm.width() - cw, h);
            decoration.setRect(display.right() + 1, y, pm.width(), h);
        } else {
            decoration.setRect(x, y, pm.width(), h);
            display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
        }
        break; }
    default:
        qWarning("doLayout: decoration position is invalid");
        decoration = *pixmapRect;
        break;
    }

    if (!hint) { // we only need to do the internal layout if we are going to paint
        *checkRect = QStyle::alignedRect(option.direction, Qt::AlignCenter,
                                         checkRect->size(), check);
        *pixmapRect = QStyle::alignedRect(option.direction, option.decorationAlignment,
                                          pixmapRect->size(), decoration);
        // the text takes up all available space, unless the decoration is not shown as selected
        if (option.showDecorationSelected)
            *textRect = display;
        else
            *textRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                            textRect->size().boundedTo(display.size()), display);
    } else {
        *checkRect = check;
        *pixmapRect = decoration;
        *textRect = display;
    }
}

/*!
    \internal

    Returns the pixmap used to decorate the root of the item view.
    The style \a option controls the appearance of the root; the \a variant
    refers to the data associated with an item.
*/

QPixmap QItemDelegate::decoration(const QStyleOptionViewItem &option, const QVariant &variant) const
{
    Q_D(const QItemDelegate);
    switch (variant.userType()) {
    case QMetaType::QIcon: {
        QIcon::Mode mode = d->iconMode(option.state);
        QIcon::State state = d->iconState(option.state);
        return qvariant_cast<QIcon>(variant).pixmap(option.decorationSize, mode, state); }
    case QMetaType::QColor: {
        static QPixmap pixmap(option.decorationSize);
        pixmap.fill(qvariant_cast<QColor>(variant));
        return pixmap; }
    default:
        break;
    }

    return qvariant_cast<QPixmap>(variant);
}

// hacky but faster version of "QString::asprintf("%d-%d", i, enabled)"
static QString qPixmapSerial(quint64 i, bool enabled)
{
    ushort arr[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', ushort('0' + enabled) };
    ushort *ptr = &arr[16];

    while (i > 0) {
        // hey - it's our internal representation, so use the ascii character after '9'
        // instead of 'a' for hex
        *(--ptr) = '0' + i % 16;
        i >>= 4;
    }

    return QString((const QChar *)ptr, int(&arr[sizeof(arr) / sizeof(ushort)] - ptr));
}

#if QT_DEPRECATED_SINCE(5, 13)
QPixmap *QItemDelegate::selected(const QPixmap &pixmap, const QPalette &palette, bool enabled) const
{
    const QString key = qPixmapSerial(pixmap.cacheKey(), enabled);
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QPixmap *pm = QPixmapCache::find(key);
    if (pm)
        return pm;
    selectedPixmap(pixmap, palette, enabled);
    return QPixmapCache::find(key);
QT_WARNING_POP
}
#endif

/*!
  \internal
  Returns the selected version of the given \a pixmap using the given \a palette.
  The \a enabled argument decides whether the normal or disabled highlight color of
  the palette is used.
*/
QPixmap QItemDelegate::selectedPixmap(const QPixmap &pixmap, const QPalette &palette, bool enabled)
{
    const QString key = qPixmapSerial(pixmap.cacheKey(), enabled);
    QPixmap pm;
    if (!QPixmapCache::find(key, &pm)) {
        QImage img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);

        QColor color = palette.color(enabled ? QPalette::Normal : QPalette::Disabled,
                                     QPalette::Highlight);
        color.setAlphaF((qreal)0.3);

        QPainter painter(&img);
        painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        painter.fillRect(0, 0, img.width(), img.height(), color);
        painter.end();

        pm = QPixmap(QPixmap::fromImage(img));
        const int n = (img.sizeInBytes() >> 10) + 1;
        if (QPixmapCache::cacheLimit() < n)
            QPixmapCache::setCacheLimit(n);

        QPixmapCache::insert(key, pm);
    }
    return pm;
}

/*!
  \internal
  Only used (and usable) for Qt::DecorationRole and Qt::CheckStateRole
*/
QRect QItemDelegate::rect(const QStyleOptionViewItem &option,
                          const QModelIndex &index, int role) const
{
    Q_D(const QItemDelegate);
    QVariant value = index.data(role);
    if (role == Qt::CheckStateRole)
        return doCheck(option, option.rect, value);
    if (value.isValid() && !value.isNull()) {
        switch (value.userType()) {
        case QMetaType::UnknownType:
            break;
        case QMetaType::QPixmap: {
            const QPixmap &pixmap = qvariant_cast<QPixmap>(value);
            return QRect(QPoint(0, 0), pixmap.size() / pixmap.devicePixelRatio() ); }
        case QMetaType::QImage: {
            const QImage &image = qvariant_cast<QImage>(value);
            return QRect(QPoint(0, 0), image.size() /  image.devicePixelRatio() ); }
        case QMetaType::QIcon: {
            QIcon::Mode mode = d->iconMode(option.state);
            QIcon::State state = d->iconState(option.state);
            QIcon icon = qvariant_cast<QIcon>(value);
            QSize size = icon.actualSize(option.decorationSize, mode, state);
            return QRect(QPoint(0, 0), size); }
        case QMetaType::QColor:
            return QRect(QPoint(0, 0), option.decorationSize);
        case QMetaType::QString:
        default: {
            const QString text = d->valueToText(value, option);
            value = index.data(Qt::FontRole);
            QFont fnt = qvariant_cast<QFont>(value).resolve(option.font);
            return textRectangle(nullptr,
                                 d->textLayoutBounds(option, QRect(), QRect()),
                                 fnt, text); }
        }
    }
    return QRect();
}

/*!
  \internal
*/
QRect QItemDelegate::doCheck(const QStyleOptionViewItem &option,
                           const QRect &bounding, const QVariant &value) const
{
    if (value.isValid()) {
        Q_D(const QItemDelegate);
        QStyleOptionButton opt;
        opt.QStyleOption::operator=(option);
        opt.rect = bounding;
        const QWidget *widget = d->widget(option); // cast
        QStyle *style = widget ? widget->style() : QApplication::style();
        return style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &opt, widget);
    }
    return QRect();
}

/*!
  \internal
*/
QRect QItemDelegate::textRectangle(QPainter * /*painter*/, const QRect &rect,
                                   const QFont &font, const QString &text) const
{
    Q_D(const QItemDelegate);
    d->textOption.setWrapMode(QTextOption::WordWrap);
    d->textLayout.setTextOption(d->textOption);
    d->textLayout.setFont(font);
    d->textLayout.setText(QItemDelegatePrivate::replaceNewLine(text));
    QSizeF fpSize = d->doTextLayout(rect.width());
    const QSize size = QSize(qCeil(fpSize.width()), qCeil(fpSize.height()));
    // ###: textRectangle should take style option as argument
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr) + 1;
    return QRect(0, 0, size.width() + 2 * textMargin, size.height());
}

/*!
    \fn bool QItemDelegate::eventFilter(QObject *editor, QEvent *event)

    Returns \c true if the given \a editor is a valid QWidget and the
    given \a event is handled; otherwise returns \c false. The following
    key press events are handled by default:

    \list
        \li \uicontrol Tab
        \li \uicontrol Backtab
        \li \uicontrol Enter
        \li \uicontrol Return
        \li \uicontrol Esc
    \endlist

    In the case of \uicontrol Tab, \uicontrol Backtab, \uicontrol Enter and \uicontrol Return
    key press events, the \a editor's data is committed to the model
    and the editor is closed. If the \a event is a \uicontrol Tab key press
    the view will open an editor on the next item in the
    view. Likewise, if the \a event is a \uicontrol Backtab key press the
    view will open an editor on the \e previous item in the view.

    If the event is a \uicontrol Esc key press event, the \a editor is
    closed \e without committing its data.

    \sa commitData(), closeEditor()
*/

bool QItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QItemDelegate);
    return d->editorEventFilter(object, event);
}

/*!
  \reimp
*/

bool QItemDelegate::editorEvent(QEvent *event,
                                QAbstractItemModel *model,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index)
{
    Q_ASSERT(event);
    Q_ASSERT(model);

    // make sure that the item is checkable
    Qt::ItemFlags flags = model->flags(index);
    if (!(flags & Qt::ItemIsUserCheckable) || !(option.state & QStyle::State_Enabled)
        || !(flags & Qt::ItemIsEnabled))
        return false;

    // make sure that we have a check state
    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid())
        return false;

    // make sure that we have the right event type
    if ((event->type() == QEvent::MouseButtonRelease)
        || (event->type() == QEvent::MouseButtonDblClick)
        || (event->type() == QEvent::MouseButtonPress)) {
        QRect checkRect = doCheck(option, option.rect, Qt::Checked);
        QRect emptyRect;
        doLayout(option, &checkRect, &emptyRect, &emptyRect, false);
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton || !checkRect.contains(me->pos()))
            return false;

        // eat the double click events inside the check rect
        if ((event->type() == QEvent::MouseButtonPress)
            || (event->type() == QEvent::MouseButtonDblClick))
            return true;

    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space
         && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
            return false;
    } else {
        return false;
    }

    Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
    if (flags & Qt::ItemIsUserTristate)
        state = ((Qt::CheckState)((state + 1) % 3));
    else
        state = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
    return model->setData(index, state, Qt::CheckStateRole);
}

/*!
  \internal
*/

QStyleOptionViewItem QItemDelegate::setOptions(const QModelIndex &index,
                                               const QStyleOptionViewItem &option) const
{
    QStyleOptionViewItem opt = option;

    // set font
    QVariant value = index.data(Qt::FontRole);
    if (value.isValid()){
        opt.font = qvariant_cast<QFont>(value).resolve(opt.font);
        opt.fontMetrics = QFontMetrics(opt.font);
    }

    // set text alignment
    value = index.data(Qt::TextAlignmentRole);
    if (value.isValid())
        opt.displayAlignment = Qt::Alignment(value.toInt());

    // set foreground brush
    value = index.data(Qt::ForegroundRole);
    if (value.canConvert<QBrush>())
        opt.palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(value));

    // disable style animations for checkboxes etc. within itemviews (QTBUG-30146)
    opt.styleObject = nullptr;

    return opt;
}

QT_END_NAMESPACE

#include "moc_qitemdelegate.cpp"
