// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QITEMEDITORFACTORY_P_H
#define QITEMEDITORFACTORY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#include <QtWidgets/private/qtwidgetsglobal_p.h>

#if QT_CONFIG(lineedit)
#include <qlineedit.h>

QT_REQUIRE_CONFIG(itemviews);

QT_BEGIN_NAMESPACE

class QExpandingLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    QExpandingLineEdit(QWidget *parent);

    void setWidgetOwnsGeometry(bool value)
    {
        widgetOwnsGeometry = value;
    }

protected:
    void changeEvent(QEvent *e) override;

public Q_SLOTS:
    void resizeToContents();

private:
    void updateMinimumWidth();

    int originalWidth;
    bool widgetOwnsGeometry;
};


QT_END_NAMESPACE

#endif // QT_CONFIG(lineedit)

#endif //QITEMEDITORFACTORY_P_H
