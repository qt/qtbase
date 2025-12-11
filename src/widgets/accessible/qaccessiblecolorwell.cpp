// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qaccessiblecolorwell_p.h"

#include <QtCore/qcoreapplication.h>

QT_REQUIRE_CONFIG(accessibility);

#if QT_CONFIG(colordialog)

#include "private/qcolorwell_p.h"

QT_BEGIN_NAMESPACE
class QAccessibleColorWellItem : public QAccessibleInterface
{
    QAccessibleColorWell *m_parent;

    Q_DECLARE_TR_FUNCTIONS(QAccessibleColorWellItem)
public:
    QAccessibleColorWellItem(QAccessibleColorWell *parent);

    virtual bool isValid() const override;
    virtual QObject *object() const override;

    virtual QAccessibleInterface *parent() const override;
    virtual QAccessibleInterface *child(int index) const override;
    virtual QAccessibleInterface *childAt(int x, int y) const override;
    virtual int childCount() const override;
    virtual int indexOfChild(const QAccessibleInterface *) const override;

    virtual QString text(QAccessible::Text t) const override;
    virtual void setText(QAccessible::Text t, const QString &text) override;
    virtual QRect rect() const override;
    virtual QAccessible::Role role() const override;
    virtual QAccessible::State state() const override;
};

QAccessibleColorWellItem::QAccessibleColorWellItem(QAccessibleColorWell *parent)
    : m_parent(parent)
{
}

bool QAccessibleColorWellItem::isValid() const
{
    return m_parent->isValid();
}

QObject *QAccessibleColorWellItem::object() const
{
    return nullptr;
}

QAccessibleInterface *QAccessibleColorWellItem::parent() const
{
    return m_parent;
}

QAccessibleInterface *QAccessibleColorWellItem::child(int) const
{
    return nullptr;
}

QAccessibleInterface *QAccessibleColorWellItem::childAt(int, int) const
{
    return nullptr;
}

int QAccessibleColorWellItem::childCount() const
{
    return 0;
}

int QAccessibleColorWellItem::indexOfChild(const QAccessibleInterface *) const
{
    return -1;
}

QString QAccessibleColorWellItem::text(QAccessible::Text t) const
{

    if (t == QAccessible::Name) {
        QRgb color = m_parent->colorWell()->rgbValues()[m_parent->indexOfChild(this)];
        //: Color specified via its 3 RGB components (red, green, blue)
        return tr("RGB %1, %2, %3")
                .arg(QString::number(qRed(color)), QString::number(qGreen(color)),
                     QString::number(qBlue(color)));
    }

    return QString();
}

void QAccessibleColorWellItem::setText(QAccessible::Text, const QString &) { }

QRect QAccessibleColorWellItem::rect() const
{
    const QColorWell *well = m_parent->colorWell();
    const int childIndex = m_parent->indexOfChild(this);
    // QColorWell layout is column-based, i.e. color indices 0 to QColorWell::numRows() - 1
    // are in first column, see QColorWell::paintCellContents
    const int col = childIndex / well->numRows();
    const int row = childIndex % well->numRows();

    return m_parent->colorWell()->cellGeometry(row, col).translated(m_parent->rect().topLeft());
}

QAccessible::Role QAccessibleColorWellItem::role() const
{
    return QAccessible::ListItem;
}

QAccessible::State QAccessibleColorWellItem::state() const
{
    QAccessible::State state;

    state.invisible = m_parent->state().invisible;

    state.focusable = true;
    const int childIndex = m_parent->indexOfChild(this);
    Q_ASSERT(childIndex >= 0);
    QColorWell *well = m_parent->colorWell();
    if (well->hasFocus() && well->index(well->currentRow(), well->currentColumn()) == childIndex)
        state.focused = true;

    state.selectable = true;
    if (well->index(well->selectedRow(), well->selectedColumn() == childIndex))
        state.selected = true;

    return state;
}

QAccessibleColorWell::QAccessibleColorWell(QWidget *widget)
    : QAccessibleWidgetV2(widget, QAccessible::List)
{
    m_childIds.resize(childCount());
}

QAccessibleColorWell::~QAccessibleColorWell()
{
    for (const std::optional<QAccessible::Id> &childId : m_childIds) {
        if (childId.has_value())
            QAccessible::deleteAccessibleInterface(childId.value());
    }
}

QAccessibleInterface *QAccessibleColorWell::child(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= m_childIds.size())
        return nullptr;

    std::optional<QAccessible::Id> childId = m_childIds.at(index);
    if (childId.has_value())
        return QAccessible::accessibleInterface(childId.value());

    QAccessibleInterface *child =
            new QAccessibleColorWellItem(const_cast<QAccessibleColorWell *>(this));
    const QAccessible::Id id = QAccessible::registerAccessibleInterface(child);
    m_childIds[index] = id;
    return child;
}

int QAccessibleColorWell::indexOfChild(const QAccessibleInterface *child) const
{
    auto it = std::find_if(m_childIds.cbegin(), m_childIds.cend(),
                           [child](const std::optional<QAccessible::Id> &id) {
                               return id.has_value()
                                       && QAccessible::accessibleInterface(id.value()) == child;
                           });
    if (it != m_childIds.cend())
        return std::distance(m_childIds.cbegin(), it);

    return -1;
}

int QAccessibleColorWell::childCount() const
{
    const QColorWell *well = colorWell();
    return well->numCols() * well->numRows();
}

QColorWell *QAccessibleColorWell::colorWell() const
{
    return qobject_cast<QColorWell *>(object());
}

QT_END_NAMESPACE

#endif // QT_CONFIG(colordialog)
