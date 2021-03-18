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

#include "qabstractitemdelegate.h"

#include <qabstractitemmodel.h>
#include <qabstractitemview.h>
#include <qfontmetrics.h>
#if QT_CONFIG(whatsthis)
#include <qwhatsthis.h>
#endif
#include <qtooltip.h>
#include <qevent.h>
#include <qstring.h>
#include <qdebug.h>
#if QT_CONFIG(lineedit)
#include <qlineedit.h>
#endif
#if QT_CONFIG(textedit)
#include <qtextedit.h>
#include <qplaintextedit.h>
#endif
#include <qapplication.h>
#include <qvalidator.h>
#include <qjsonvalue.h>
#include <private/qtextengine_p.h>
#include <private/qabstractitemdelegate_p.h>

#include <qpa/qplatformintegration.h>
#if QT_CONFIG(draganddrop)
#include <qpa/qplatformdrag.h>
#include <private/qdnd_p.h>
#endif
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractItemDelegate

    \brief The QAbstractItemDelegate class is used to display and edit
    data items from a model.

    \ingroup model-view
    \inmodule QtWidgets

    A QAbstractItemDelegate provides the interface and common functionality
    for delegates in the model/view architecture. Delegates display
    individual items in views, and handle the editing of model data.

    The QAbstractItemDelegate class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    To render an item in a custom way, you must implement paint() and
    sizeHint(). The QStyledItemDelegate class provides default implementations for
    these functions; if you do not need custom rendering, subclass that
    class instead.

    We give an example of drawing a progress bar in items; in our case
    for a package management program.

    \image widgetdelegate.png

    We create the \c WidgetDelegate class, which inherits from
    QStyledItemDelegate. We do the drawing in the paint() function:

    \snippet widgetdelegate.cpp 0

    Notice that we use a QStyleOptionProgressBar and initialize its
    members. We can then use the current QStyle to draw it.

    To provide custom editing, there are two approaches that can be
    used. The first approach is to create an editor widget and display
    it directly on top of the item. To do this you must reimplement
    createEditor() to provide an editor widget, setEditorData() to populate
    the editor with the data from the model, and setModelData() so that the
    delegate can update the model with data from the editor.

    The second approach is to handle user events directly by reimplementing
    editorEvent().

    \sa {model-view-programming}{Model/View Programming}, QStyledItemDelegate,
        {Pixelator Example}, QStyledItemDelegate, QStyle
*/

/*!
    \enum QAbstractItemDelegate::EndEditHint

    This enum describes the different hints that the delegate can give to the
    model and view components to make editing data in a model a comfortable
    experience for the user.

    \value NoHint           There is no recommended action to be performed.

    These hints let the delegate influence the behavior of the view:

    \value EditNextItem     The view should use the delegate to open an
                            editor on the next item in the view.
    \value EditPreviousItem The view should use the delegate to open an
                            editor on the previous item in the view.

    Note that custom views may interpret the concepts of next and previous
    differently.

    The following hints are most useful when models are used that cache
    data, such as those that manipulate data locally in order to increase
    performance or conserve network bandwidth.

    \value SubmitModelCache If the model caches data, it should write out
                            cached data to the underlying data store.
    \value RevertModelCache If the model caches data, it should discard
                            cached data and replace it with data from the
                            underlying data store.

    Although models and views should respond to these hints in appropriate
    ways, custom components may ignore any or all of them if they are not
    relevant.
*/

/*!
  \fn void QAbstractItemDelegate::commitData(QWidget *editor)

  This signal must be emitted when the \a editor widget has completed
  editing the data, and wants to write it back into the model.
*/

/*!
    \fn void QAbstractItemDelegate::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)

    This signal is emitted when the user has finished editing an item using
    the specified \a editor.

    The \a hint provides a way for the delegate to influence how the model and
    view behave after editing is completed. It indicates to these components
    what action should be performed next to provide a comfortable editing
    experience for the user. For example, if \c EditNextItem is specified,
    the view should use a delegate to open an editor on the next item in the
    model.

    \sa EndEditHint
*/

/*!
    \fn void QAbstractItemDelegate::sizeHintChanged(const QModelIndex &index)
    \since 4.4

    This signal must be emitted when the sizeHint() of \a index changed.

    Views automatically connect to this signal and relayout items as necessary.
*/


/*!
    Creates a new abstract item delegate with the given \a parent.
*/
QAbstractItemDelegate::QAbstractItemDelegate(QObject *parent)
    : QObject(*new QAbstractItemDelegatePrivate, parent)
{

}

/*!
    \internal

    Creates a new abstract item delegate with the given \a parent.
*/
QAbstractItemDelegate::QAbstractItemDelegate(QObjectPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{

}

/*!
    Destroys the abstract item delegate.
*/
QAbstractItemDelegate::~QAbstractItemDelegate()
{

}

/*!
    \fn void QAbstractItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const = 0;

    This pure abstract function must be reimplemented if you want to
    provide custom rendering. Use the \a painter and style \a option to
    render the item specified by the item \a index.

    If you reimplement this you must also reimplement sizeHint().
*/

/*!
    \fn QSize QAbstractItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const = 0

    This pure abstract function must be reimplemented if you want to
    provide custom rendering. The options are specified by \a option
    and the model item by \a index.

    If you reimplement this you must also reimplement paint().
*/

/*!
    Returns the editor to be used for editing the data item with the
    given \a index. Note that the index contains information about the
    model being used. The editor's parent widget is specified by \a parent,
    and the item options by \a option.

    The base implementation returns \nullptr. If you want custom editing you
    will need to reimplement this function.

    The returned editor widget should have Qt::StrongFocus;
    otherwise, \l{QMouseEvent}s received by the widget will propagate
    to the view. The view's background will shine through unless the
    editor paints its own background (e.g., with
    \l{QWidget::}{setAutoFillBackground()}).

    \sa destroyEditor(), setModelData(), setEditorData()
*/
QWidget *QAbstractItemDelegate::createEditor(QWidget *,
                                             const QStyleOptionViewItem &,
                                             const QModelIndex &) const
{
    return nullptr;
}


/*!
    Called when the \a editor is no longer needed for editing the data item
    with the given \a index and should be destroyed. The default behavior is a
    call to deleteLater on the editor. It is possible e.g. to avoid this delete by
    reimplementing this function.

    \since 5.0
    \sa createEditor()
*/
void QAbstractItemDelegate::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->deleteLater();
}

/*!
    Sets the contents of the given \a editor to the data for the item
    at the given \a index. Note that the index contains information
    about the model being used.

    The base implementation does nothing. If you want custom editing
    you will need to reimplement this function.

    \sa setModelData()
*/
void QAbstractItemDelegate::setEditorData(QWidget *,
                                          const QModelIndex &) const
{
    // do nothing
}

/*!
    Sets the data for the item at the given \a index in the \a model
    to the contents of the given \a editor.

    The base implementation does nothing. If you want custom editing
    you will need to reimplement this function.

    \sa setEditorData()
*/
void QAbstractItemDelegate::setModelData(QWidget *,
                                         QAbstractItemModel *,
                                         const QModelIndex &) const
{
    // do nothing
}

/*!
    Updates the geometry of the \a editor for the item with the given
    \a index, according to the rectangle specified in the \a option.
    If the item has an internal layout, the editor will be laid out
    accordingly. Note that the index contains information about the
    model being used.

    The base implementation does nothing. If you want custom editing
    you must reimplement this function.
*/
void QAbstractItemDelegate::updateEditorGeometry(QWidget *,
                                                 const QStyleOptionViewItem &,
                                                 const QModelIndex &) const
{
    // do nothing
}

/*!
    When editing of an item starts, this function is called with the
    \a event that triggered the editing, the \a model, the \a index of
    the item, and the \a option used for rendering the item.

    Mouse events are sent to editorEvent() even if they don't start
    editing of the item. This can, for instance, be useful if you wish
    to open a context menu when the right mouse button is pressed on
    an item.

    The base implementation returns \c false (indicating that it has not
    handled the event).
*/
bool QAbstractItemDelegate::editorEvent(QEvent *,
                                        QAbstractItemModel *,
                                        const QStyleOptionViewItem &,
                                        const QModelIndex &)
{
    // do nothing
    return false;
}

#if QT_DEPRECATED_SINCE(5, 13)
/*!
    \obsolete

    Use QFontMetrics::elidedText() instead.

    \oldcode
        QFontMetrics fm = ...
        QString str = QAbstractItemDelegate::elidedText(fm, width, mode, text);
    \newcode
        QFontMetrics fm = ...
        QString str = fm.elidedText(text, mode, width);
    \endcode
*/

QString QAbstractItemDelegate::elidedText(const QFontMetrics &fontMetrics, int width,
                                          Qt::TextElideMode mode, const QString &text)
{
    return fontMetrics.elidedText(text, mode, width);
}
#endif

/*!
    \since 4.3
    Whenever a help event occurs, this function is called with the \a event
    \a view \a option and the \a index that corresponds to the item where the
    event occurs.

    Returns \c true if the delegate can handle the event; otherwise returns \c false.
    A return value of true indicates that the data obtained using the index had
    the required role.

    For QEvent::ToolTip and QEvent::WhatsThis events that were handled successfully,
    the relevant popup may be shown depending on the user's system configuration.

    \sa QHelpEvent
*/
bool QAbstractItemDelegate::helpEvent(QHelpEvent *event,
                                      QAbstractItemView *view,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index)
{
    if (!event || !view)
        return false;
    Q_D(QAbstractItemDelegate);
    switch (event->type()) {
#ifndef QT_NO_TOOLTIP
    case QEvent::ToolTip: {
        QHelpEvent *he = static_cast<QHelpEvent*>(event);
        const int precision = inherits("QItemDelegate") ? 10 : 6; // keep in sync with DBL_DIG in qitemdelegate.cpp
        const QString tooltip = index.isValid() ?
              d->textForRole(Qt::ToolTipRole, index.data(Qt::ToolTipRole), option.locale, precision) :
              QString();
        QRect rect;
        if (index.isValid()) {
            const QRect r = view->visualRect(index);
            rect = QRect(view->mapToGlobal(r.topLeft()), r.size());
        }
        QToolTip::showText(he->globalPos(), tooltip, view, rect);
        event->setAccepted(!tooltip.isEmpty());
        break;
        }
#endif
#if QT_CONFIG(whatsthis)
    case QEvent::QueryWhatsThis:
        event->setAccepted(index.data(Qt::WhatsThisRole).isValid());
        break;
    case QEvent::WhatsThis: {
        QHelpEvent *he = static_cast<QHelpEvent*>(event);
        const int precision = inherits("QItemDelegate") ? 10 : 6; // keep in sync with DBL_DIG in qitemdelegate.cpp
        const QString whatsthis = index.isValid() ?
              d->textForRole(Qt::WhatsThisRole, index.data(Qt::WhatsThisRole), option.locale, precision) :
              QString();
        QWhatsThis::showText(he->globalPos(), whatsthis, view);
        event->setAccepted(!whatsthis.isEmpty());
        break;
        }
#endif
    default:
        break;
    }
    return event->isAccepted();
}

/*!
    \internal

    This virtual method is reserved and will be used in Qt 5.1.
*/
QVector<int> QAbstractItemDelegate::paintingRoles() const
{
    return QVector<int>();
}

QAbstractItemDelegatePrivate::QAbstractItemDelegatePrivate()
    : QObjectPrivate()
{
}

static bool editorHandlesKeyEvent(QWidget *editor, const QKeyEvent *event)
{
#if QT_CONFIG(textedit)
    // do not filter enter / return / tab / backtab for QTextEdit or QPlainTextEdit
    if (qobject_cast<QTextEdit *>(editor) || qobject_cast<QPlainTextEdit *>(editor)) {
        switch (event->key()) {
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
        case Qt::Key_Enter:
        case Qt::Key_Return:
            return true;

        default:
            break;
        }
    }
#endif // QT_CONFIG(textedit)

    Q_UNUSED(editor);
    Q_UNUSED(event);
    return false;
}

bool QAbstractItemDelegatePrivate::editorEventFilter(QObject *object, QEvent *event)
{
    Q_Q(QAbstractItemDelegate);

    QWidget *editor = qobject_cast<QWidget*>(object);
    if (!editor)
        return false;
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (editorHandlesKeyEvent(editor, keyEvent))
            return false;

#ifndef QT_NO_SHORTCUT
        if (keyEvent->matches(QKeySequence::Cancel)) {
            // don't commit data
            emit q->closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
            return true;
        }
#endif

        switch (keyEvent->key()) {
        case Qt::Key_Tab:
            if (tryFixup(editor)) {
                emit q->commitData(editor);
                emit q->closeEditor(editor, QAbstractItemDelegate::EditNextItem);
            }
            return true;
        case Qt::Key_Backtab:
            if (tryFixup(editor)) {
                emit q->commitData(editor);
                emit q->closeEditor(editor, QAbstractItemDelegate::EditPreviousItem);
            }
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            // We want the editor to be able to process the key press
            // before committing the data (e.g. so it can do
            // validation/fixup of the input).
            if (!tryFixup(editor))
                return true;

            QMetaObject::invokeMethod(q, "_q_commitDataAndCloseEditor",
                                      Qt::QueuedConnection, Q_ARG(QWidget*, editor));
            return false;
        default:
            return false;
        }
    } else if (event->type() == QEvent::FocusOut || (event->type() == QEvent::Hide && editor->isWindow())) {
        //the Hide event will take care of he editors that are in fact complete dialogs
        if (!editor->isActiveWindow() || (QApplication::focusWidget() != editor)) {
            QWidget *w = QApplication::focusWidget();
            while (w) { // don't worry about focus changes internally in the editor
                if (w == editor)
                    return false;
                w = w->parentWidget();
            }
#if QT_CONFIG(draganddrop)
            // The window may lose focus during an drag operation.
            // i.e when dragging involves the taskbar on Windows.
            QPlatformDrag *platformDrag = QGuiApplicationPrivate::instance()->platformIntegration()->drag();
            if (platformDrag && platformDrag->currentDrag()) {
                return false;
            }
#endif
            if (tryFixup(editor))
                emit q->commitData(editor);

            // If the application loses focus while editing, then the focus needs to go back
            // to the itemview when the editor closes. This ensures that when the application
            // is active again it will have the focus on the itemview as expected.
            const bool manuallyFixFocus = (event->type() == QEvent::FocusOut) && !editor->hasFocus() &&
                    editor->parentWidget() &&
                    (static_cast<QFocusEvent *>(event)->reason() == Qt::ActiveWindowFocusReason);
            emit q->closeEditor(editor, QAbstractItemDelegate::NoHint);
            if (manuallyFixFocus)
                editor->parentWidget()->setFocus();
        }
#ifndef QT_NO_SHORTCUT
    } else if (event->type() == QEvent::ShortcutOverride) {
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel)) {
            event->accept();
            return true;
        }
#endif
    }
    return false;
}

bool QAbstractItemDelegatePrivate::tryFixup(QWidget *editor)
{
#if QT_CONFIG(lineedit)
    if (QLineEdit *e = qobject_cast<QLineEdit*>(editor)) {
        if (!e->hasAcceptableInput()) {
#if QT_CONFIG(validator)
            if (const QValidator *validator = e->validator()) {
                QString text = e->text();
                validator->fixup(text);
                e->setText(text);
            }
#endif
            return e->hasAcceptableInput();
        }
    }
#else
    Q_UNUSED(editor)
#endif // QT_CONFIG(lineedit)

    return true;
}

QString QAbstractItemDelegatePrivate::textForRole(Qt::ItemDataRole role, const QVariant &value, const QLocale &locale, int precision) const
{
    const QLocale::FormatType formatType = (role == Qt::DisplayRole) ? QLocale::ShortFormat : QLocale::LongFormat;
    QString text;
    switch (value.userType()) {
    case QMetaType::Float:
        text = locale.toString(value.toFloat());
        break;
    case QMetaType::Double:
        text = locale.toString(value.toDouble(), 'g', precision);
        break;
    case QMetaType::Int:
    case QMetaType::LongLong:
        text = locale.toString(value.toLongLong());
        break;
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        text = locale.toString(value.toULongLong());
        break;
    case QMetaType::QDate:
        text = locale.toString(value.toDate(), formatType);
        break;
    case QMetaType::QTime:
        text = locale.toString(value.toTime(), formatType);
        break;
    case QMetaType::QDateTime:
        text = locale.toString(value.toDateTime(), formatType);
        break;
    case QMetaType::QJsonValue: {
        const QJsonValue val = value.toJsonValue();
        if (val.isBool()) {
            text = QVariant(val.toBool()).toString();
            break;
        }
        if (val.isDouble()) {
            text = locale.toString(val.toDouble(), 'g', precision);
            break;
        }
        // val is a string (or null) here
        Q_FALLTHROUGH();
    }
    default: {
        text = value.toString();
        if (role == Qt::DisplayRole)
            text.replace(QLatin1Char('\n'), QChar::LineSeparator);
        break;
    }
    }
    return text;
}

void QAbstractItemDelegatePrivate::_q_commitDataAndCloseEditor(QWidget *editor)
{
    Q_Q(QAbstractItemDelegate);
    emit q->commitData(editor);
    emit q->closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
}

QT_END_NAMESPACE

#include "moc_qabstractitemdelegate.cpp"
