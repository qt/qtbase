/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
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
};

#endif // PROPERTYFIELD_H
