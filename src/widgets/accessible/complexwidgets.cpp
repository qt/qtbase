// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "complexwidgets_p.h"

#include <qaccessible.h>
#include <qapplication.h>
#include <qevent.h>
#if QT_CONFIG(itemviews)
#include <qheaderview.h>
#endif
#if QT_CONFIG(tabbar)
#include <qtabbar.h>
#include <private/qtabbar_p.h>
#endif
#if QT_CONFIG(combobox)
#include <qcombobox.h>
#endif
#if QT_CONFIG(lineedit)
#include <qlineedit.h>
#endif
#include <qstyle.h>
#include <qstyleoption.h>
#if QT_CONFIG(tooltip)
#include <qtooltip.h>
#endif
#if QT_CONFIG(whatsthis)
#include <qwhatsthis.h>
#endif
#include <QAbstractScrollArea>
#if QT_CONFIG(scrollarea)
#include <QScrollArea>
#endif
#if QT_CONFIG(scrollbar)
#include <QScrollBar>
#endif
#include <QDebug>

#if QT_CONFIG(accessibility)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QString qt_accStripAmp(const QString &text);
QString qt_accHotKey(const QString &text);

#if QT_CONFIG(tabbar)
/*!
  \class QAccessibleTabBar
  \brief The QAccessibleTabBar class implements the QAccessibleInterface for tab bars.
  \internal

  \ingroup accessibility
*/

class QAccessibleTabButton: public QAccessibleInterface, public QAccessibleActionInterface
{
public:
    QAccessibleTabButton(QTabBar *parent, int index)
        : m_parent(parent), m_index(index)
    {}

    void *interface_cast(QAccessible::InterfaceType t) override {
        if (t == QAccessible::ActionInterface) {
            return static_cast<QAccessibleActionInterface*>(this);
        }
        return nullptr;
    }

    QObject *object() const override { return nullptr; }
    QAccessible::Role role() const override { return QAccessible::PageTab; }
    QAccessible::State state() const override {
        if (!isValid()) {
            QAccessible::State s;
            s.invalid = true;
            return s;
        }

        QAccessible::State s = parent()->state();
        s.focused = (m_index == m_parent->currentIndex());
        return s;
    }
    QRect rect() const override {
        if (!isValid())
            return QRect();

        QPoint tp = m_parent->mapToGlobal(QPoint(0,0));
        QRect rec = m_parent->tabRect(m_index);
        rec = QRect(tp.x() + rec.x(), tp.y() + rec.y(), rec.width(), rec.height());
        return rec;
    }

    bool isValid() const override {
        if (m_parent) {
            if (static_cast<QWidget *>(m_parent.data())->d_func()->data.in_destructor)
                return false;
            return m_parent->count() > m_index;
        }
        return false;
    }

    QAccessibleInterface *childAt(int, int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface *) const override  { return -1; }

    QString text(QAccessible::Text t) const override
    {
        if (!isValid())
            return QString();
        QString str;
        switch (t) {
        case QAccessible::Name:
            str = m_parent->accessibleTabName(m_index);
            if (str.isEmpty())
                str = qt_accStripAmp(m_parent->tabText(m_index));
            break;
        case QAccessible::Accelerator:
            str = qt_accHotKey(m_parent->tabText(m_index));
            break;
#if QT_CONFIG(tooltip)
        case QAccessible::Description:
            str = m_parent->tabToolTip(m_index);
            break;
#endif
#if QT_CONFIG(whatsthis)
        case QAccessible::Help:
            str = m_parent->tabWhatsThis(m_index);
            break;
#endif
        default:
            break;
        }
        return str;
    }

    void setText(QAccessible::Text, const QString &) override {}

    QAccessibleInterface *parent() const override {
        return QAccessible::queryAccessibleInterface(m_parent.data());
    }
    QAccessibleInterface *child(int) const override { return nullptr; }

    // action interface
    QStringList actionNames() const override
    {
        return QStringList(pressAction());
    }

    void doAction(const QString &actionName) override
    {
        if (isValid() && actionName == pressAction())
            m_parent->setCurrentIndex(m_index);
    }

    QStringList keyBindingsForAction(const QString &) const override
    {
        return QStringList();
    }

    int index() const { return m_index; }

private:
    QPointer<QTabBar> m_parent;
    int m_index;

};

/*!
  Constructs a QAccessibleTabBar object for \a w.
*/
QAccessibleTabBar::QAccessibleTabBar(QWidget *w)
: QAccessibleWidget(w, QAccessible::PageTabList)
{
    Q_ASSERT(tabBar());
}

QAccessibleTabBar::~QAccessibleTabBar()
{
    for (QAccessible::Id id : std::as_const(m_childInterfaces))
        QAccessible::deleteAccessibleInterface(id);
}

/*! Returns the QTabBar. */
QTabBar *QAccessibleTabBar::tabBar() const
{
    return qobject_cast<QTabBar*>(object());
}

QAccessibleInterface* QAccessibleTabBar::focusChild() const
{
    for (int i = 0; i < childCount(); ++i) {
        if (child(i)->state().focused)
            return child(i);
    }

    return nullptr;
}

QAccessibleInterface* QAccessibleTabBar::child(int index) const
{
    if (QAccessible::Id id = m_childInterfaces.value(index))
        return QAccessible::accessibleInterface(id);

    // first the tabs, then 2 buttons
    if (index < tabBar()->count()) {
        QAccessibleTabButton *button = new QAccessibleTabButton(tabBar(), index);
        QAccessible::registerAccessibleInterface(button);
        m_childInterfaces.insert(index, QAccessible::uniqueId(button));
        return button;
    } else if (index >= tabBar()->count()) {
        // left button
        if (index - tabBar()->count() == 0) {
            return QAccessible::queryAccessibleInterface(tabBar()->d_func()->leftB);
        }
        // right button
        if (index - tabBar()->count() == 1) {
            return QAccessible::queryAccessibleInterface(tabBar()->d_func()->rightB);
        }
    }
    return nullptr;
}

int QAccessibleTabBar::indexOfChild(const QAccessibleInterface *child) const
{
    if (child->object() && child->object() == tabBar()->d_func()->leftB)
        return tabBar()->count();
    if (child->object() && child->object() == tabBar()->d_func()->rightB)
        return tabBar()->count() + 1;
    if (child->role() == QAccessible::PageTab) {
        QAccessibleInterface *parent = child->parent();
        if (parent == this) {
            const QAccessibleTabButton *tabButton = static_cast<const QAccessibleTabButton *>(child);
            return tabButton->index();
        }
    }
    return -1;
}

int QAccessibleTabBar::childCount() const
{
    // tabs + scroll buttons
    return tabBar()->count() + 2;
}

QString QAccessibleTabBar::text(QAccessible::Text t) const
{
    if (t == QAccessible::Name) {
        const QTabBar *tBar = tabBar();
        int idx = tBar->currentIndex();
        QString str = tBar->accessibleTabName(idx);
        if (str.isEmpty())
            str = qt_accStripAmp(tBar->tabText(idx));
        return str;
    } else if (t == QAccessible::Accelerator) {
        return qt_accHotKey(tabBar()->tabText(tabBar()->currentIndex()));
    }
    return QString();
}

#endif // QT_CONFIG(tabbar)

#if QT_CONFIG(combobox)
/*!
  \class QAccessibleComboBox
  \brief The QAccessibleComboBox class implements the QAccessibleInterface for editable and read-only combo boxes.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleComboBox object for \a w.
*/
QAccessibleComboBox::QAccessibleComboBox(QWidget *w)
: QAccessibleWidget(w, QAccessible::ComboBox)
{
    Q_ASSERT(comboBox());
}

/*!
  Returns the combobox.
*/
QComboBox *QAccessibleComboBox::comboBox() const
{
    return qobject_cast<QComboBox *>(object());
}

QAccessibleInterface *QAccessibleComboBox::child(int index) const
{
    if (QComboBox *cBox = comboBox()) {
        if (index == 0) {
            QAbstractItemView *view = cBox->view();
            //QWidget *parent = view ? view->parentWidget() : 0;
            return QAccessible::queryAccessibleInterface(view);
        } else if (index == 1 && cBox->isEditable()) {
            return QAccessible::queryAccessibleInterface(cBox->lineEdit());
        }
    }
    return nullptr;
}

int QAccessibleComboBox::childCount() const
{
    // list and text edit
    if (QComboBox *cBox = comboBox())
        return (cBox->isEditable()) ? 2 : 1;
    return 0;
}

QAccessibleInterface *QAccessibleComboBox::childAt(int x, int y) const
{
    if (QComboBox *cBox = comboBox()) {
        if (cBox->isEditable() && cBox->lineEdit()->rect().contains(x, y))
            return child(1);
    }
    return nullptr;
}

int QAccessibleComboBox::indexOfChild(const QAccessibleInterface *child) const
{
    if (QComboBox *cBox = comboBox()) {
        if (cBox->view() == child->object())
            return 0;
        if (cBox->isEditable() && cBox->lineEdit() == child->object())
            return 1;
    }
    return -1;
}

QAccessibleInterface *QAccessibleComboBox::focusChild() const
{
    // The editable combobox is the focus proxy of its lineedit, so the
    // lineedit itself never gets focus. But it is the accessible focus
    // child of an editable combobox.
    if (QComboBox *cBox = comboBox()) {
        if (cBox->isEditable())
            return child(1);
    }
    return nullptr;
}

/*! \reimp */
QString QAccessibleComboBox::text(QAccessible::Text t) const
{
    QString str;
    if (QComboBox *cBox = comboBox()) {
        switch (t) {
        case QAccessible::Name:
#ifndef Q_OS_UNIX // on Linux we use relations for this, name is text (fall through to Value)
        str = QAccessibleWidget::text(t);
        break;
#endif
        case QAccessible::Value:
            if (cBox->isEditable())
                str = cBox->lineEdit()->text();
            else
                str = cBox->currentText();
            break;
#ifndef QT_NO_SHORTCUT
        case QAccessible::Accelerator:
            str = QKeySequence(Qt::Key_Down).toString(QKeySequence::NativeText);
            break;
#endif
        default:
            break;
        }
        if (str.isEmpty())
            str = QAccessibleWidget::text(t);
    }
    return str;
}

QAccessible::State QAccessibleComboBox::state() const
{
    QAccessible::State s = QAccessibleWidget::state();

    if (QComboBox *cBox = comboBox()) {
        s.expandable = true;
        s.expanded = isValid() && cBox->view()->isVisible();
        s.editable = cBox->isEditable();
    }
    return s;
}

QStringList QAccessibleComboBox::actionNames() const
{
    return QStringList() << showMenuAction() << pressAction();
}

QString QAccessibleComboBox::localizedActionDescription(const QString &actionName) const
{
    if (actionName == showMenuAction() || actionName == pressAction())
        return QComboBox::tr("Open the combo box selection popup");
    return QString();
}

void QAccessibleComboBox::doAction(const QString &actionName)
{
    if (QComboBox *cBox = comboBox()) {
        if (actionName == showMenuAction() || actionName == pressAction()) {
            if (cBox->view()->isVisible()) {
#if defined(Q_OS_ANDROID)
                const auto list = child(0)->tableInterface();
                if (list && list->selectedRowCount() > 0) {
                    cBox->setCurrentIndex(list->selectedRows().at(0));
                }
                cBox->setFocus();
#endif
                cBox->hidePopup();
            } else {
                cBox->showPopup();
#if defined(Q_OS_ANDROID)
                const auto list = child(0)->tableInterface();
                if (list && list->selectedRowCount() > 0) {
                    auto selectedCells = list->selectedCells();
                    QAccessibleEvent ev(selectedCells.at(0),QAccessible::Focus);
                    QAccessible::updateAccessibility(&ev);
                }
#endif
            }
        }
    }
}

QStringList QAccessibleComboBox::keyBindingsForAction(const QString &/*actionName*/) const
{
    return QStringList();
}

#endif // QT_CONFIG(combobox)

#if QT_CONFIG(scrollarea)
// ======================= QAccessibleAbstractScrollArea =======================
QAccessibleAbstractScrollArea::QAccessibleAbstractScrollArea(QWidget *widget)
    : QAccessibleWidget(widget, QAccessible::Client)
{
    Q_ASSERT(qobject_cast<QAbstractScrollArea *>(widget));
}

QAccessibleInterface *QAccessibleAbstractScrollArea::child(int index) const
{
    return QAccessible::queryAccessibleInterface(accessibleChildren().at(index));
}

int QAccessibleAbstractScrollArea::childCount() const
{
    return accessibleChildren().size();
}

int QAccessibleAbstractScrollArea::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child || !child->object())
        return -1;
    return accessibleChildren().indexOf(qobject_cast<QWidget *>(child->object()));
}

bool QAccessibleAbstractScrollArea::isValid() const
{
    return (QAccessibleWidget::isValid() && abstractScrollArea() && abstractScrollArea()->viewport());
}

QAccessibleInterface *QAccessibleAbstractScrollArea::childAt(int x, int y) const
{
    if (!abstractScrollArea()->isVisible())
        return nullptr;

    for (int i = 0; i < childCount(); ++i) {
        QPoint wpos = accessibleChildren().at(i)->mapToGlobal(QPoint(0, 0));
        QRect rect = QRect(wpos, accessibleChildren().at(i)->size());
        if (rect.contains(x, y))
            return child(i);
    }
    return nullptr;
}

QAbstractScrollArea *QAccessibleAbstractScrollArea::abstractScrollArea() const
{
    return static_cast<QAbstractScrollArea *>(object());
}

QWidgetList QAccessibleAbstractScrollArea::accessibleChildren() const
{
    QWidgetList children;

    // Viewport.
    QWidget * viewport = abstractScrollArea()->viewport();
    if (viewport)
        children.append(viewport);

    // Horizontal scrollBar container.
    QScrollBar *horizontalScrollBar = abstractScrollArea()->horizontalScrollBar();
    if (horizontalScrollBar && horizontalScrollBar->isVisible()) {
        QWidget *scrollBarParent = horizontalScrollBar->parentWidget();
        // Add container only if scroll bar is in the container
        if (elementType(scrollBarParent) == HorizontalContainer)
            children.append(scrollBarParent);
    }

    // Vertical scrollBar container.
    QScrollBar *verticalScrollBar = abstractScrollArea()->verticalScrollBar();
    if (verticalScrollBar && verticalScrollBar->isVisible()) {
        QWidget *scrollBarParent = verticalScrollBar->parentWidget();
        // Add container only if scroll bar is in the container
        if (elementType(scrollBarParent) == VerticalContainer)
            children.append(scrollBarParent);
    }

    // CornerWidget.
    QWidget *cornerWidget = abstractScrollArea()->cornerWidget();
    if (cornerWidget && cornerWidget->isVisible())
        children.append(cornerWidget);

    return children;
}

QAccessibleAbstractScrollArea::AbstractScrollAreaElement
QAccessibleAbstractScrollArea::elementType(QWidget *widget) const
{
    if (!widget)
        return Undefined;

    if (widget == abstractScrollArea())
        return Self;
    if (widget == abstractScrollArea()->viewport())
        return Viewport;
    if (widget->objectName() == "qt_scrollarea_hcontainer"_L1)
        return HorizontalContainer;
    if (widget->objectName() == "qt_scrollarea_vcontainer"_L1)
        return VerticalContainer;
    if (widget == abstractScrollArea()->cornerWidget())
        return CornerWidget;

    return Undefined;
}

bool QAccessibleAbstractScrollArea::isLeftToRight() const
{
    return abstractScrollArea()->isLeftToRight();
}

// ======================= QAccessibleScrollArea ===========================
QAccessibleScrollArea::QAccessibleScrollArea(QWidget *widget)
    : QAccessibleAbstractScrollArea(widget)
{
    Q_ASSERT(qobject_cast<QScrollArea *>(widget));
}
#endif // QT_CONFIG(scrollarea)

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
