/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PROPERTYFIELD_H
#define PROPERTYFIELD_H

#include <QLineEdit>
#include <QMetaProperty>
#include <QTime>

/*!
    A QLineEdit for viewing the text form of a property on an object.
    Automatically stays up-to-date when the property changes.
    (This is rather like a QML TextField bound to a property.)
 */
class PropertyField : public QLineEdit
{
    Q_OBJECT
public:
    explicit PropertyField(QObject* subject, const QMetaProperty& prop, QWidget *parent = 0);

signals:

public slots:
    void propertyChanged();

protected:
    QString valueToString(QVariant val);

private:
    QObject* m_subject;
    QString m_lastText;
    QString m_lastTextShowing;
    QTime m_lastChangeTime;
    const QMetaProperty m_prop;
    QBrush m_defaultBrush;
};

#endif // PROPERTYFIELD_H
