/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
#include <qtooltip.h>
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

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

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
    for (QAccessible::Id id : qAsConst(m_childInterfaces))
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
    return qobject_cast<QComboBox*>(object());
}

QAccessibleInterface *QAccessibleComboBox::child(int index) const
{
    if (index == 0) {
        QAbstractItemView *view = comboBox()->view();
        //QWidget *parent = view ? view->parentWidget() : 0;
        return QAccessible::queryAccessibleInterface(view);
    } else if (index == 1 && comboBox()->isEditable()) {
        return QAccessible::queryAccessibleInterface(comboBox()->lineEdit());
    }
    return nullptr;
}

int QAccessibleComboBox::childCount() const
{
    // list and text edit
    return comboBox()->isEditable() ? 2 : 1;
}

QAccessibleInterface *QAccessibleComboBox::childAt(int x, int y) const
{
    if (comboBox()->isEditable() && comboBox()->lineEdit()->rect().contains(x, y))
        return child(1);
    return nullptr;
}

int QAccessibleComboBox::indexOfChild(const QAccessibleInterface *child) const
{
    if (comboBox()->view() == child->object())
        return 0;
    if (comboBox()->isEditable() && comboBox()->lineEdit() == child->object())
        return 1;
    return -1;
}

/*! \reimp */
QString QAccessibleComboBox::text(QAccessible::Text t) const
{
    QString str;

    switch (t) {
    case QAccessible::Name:
#ifndef Q_OS_UNIX // on Linux we use relations for this, name is text (fall through to Value)
        str = QAccessibleWidget::text(t);
        break;
#endif
    case QAccessible::Value:
        if (comboBox()->isEditable())
            str = comboBox()->lineEdit()->text();
        else
            str = comboBox()->currentText();
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
    return str;
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
    if (actionName == showMenuAction() || actionName == pressAction()) {
        if (comboBox()->view()->isVisible()) {
            comboBox()->hidePopup();
        } else {
            comboBox()->showPopup();
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
    return accessibleChildren().count();
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
        children.append(horizontalScrollBar->parentWidget());
    }

    // Vertical scrollBar container.
    QScrollBar *verticalScrollBar = abstractScrollArea()->verticalScrollBar();
    if (verticalScrollBar && verticalScrollBar->isVisible()) {
        children.append(verticalScrollBar->parentWidget());
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
    if (widget->objectName() == QLatin1String("qt_scrollarea_hcontainer"))
        return HorizontalContainer;
    if (widget->objectName() == QLatin1String("qt_scrollarea_vcontainer"))
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

#endif // QT_NO_ACCESSIBILITY
