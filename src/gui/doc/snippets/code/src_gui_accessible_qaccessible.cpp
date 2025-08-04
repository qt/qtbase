// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui/qtguiglobal.h>

#if QT_CONFIG(accessibility)
#include <QAccessible>

namespace src_gui_accessible_qaccessible {
class MyWidget
{
    void setFocus(Qt::FocusReason reason);
};
QAccessibleInterface *f = nullptr;

//! [1]
typedef QAccessibleInterface *myFactoryFunction(const QString &key, QObject *);
//! [1]

//! [2]
void MyWidget::setFocus(Qt::FocusReason reason)
{
    // handle custom focus setting...
    QAccessibleEvent event(f, QAccessible::Focus);
    QAccessible::updateAccessibility(&event);
}
//! [2]

} // src_gui_accessible_qaccessible

#endif // QT_CONFIG(accessibility)
