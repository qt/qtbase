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

#include "propertywatcher.h"
#include <QMetaProperty>
#include <QFormLayout>
#include <QPushButton>
#include "propertyfield.h"

PropertyWatcher::PropertyWatcher(QObject *subject, QString annotation, QWidget *parent)
    : QWidget(parent), m_subject(subject), m_layout(new QFormLayout)
{
    setWindowTitle(QString("Properties of %1 %2 %3")
        .arg(subject->metaObject()->className()).arg(subject->objectName()).arg(annotation));
    setMinimumSize(450, 300);
    const QMetaObject* meta = m_subject->metaObject();

    for (int i = 0; i < meta->propertyCount(); ++i) {
        QMetaProperty prop = meta->property(i);
        if (prop.isReadable()) {
            PropertyField* field = new PropertyField(m_subject, prop);
            m_layout->addRow(prop.name(), field);
        }
    }
    QPushButton *updateButton = new QPushButton("update");
    connect(updateButton, &QPushButton::clicked, this, &PropertyWatcher::updateAllFields);
    m_layout->addRow("", updateButton);
    setLayout(m_layout);
    connect(subject, &QObject::destroyed, this, &PropertyWatcher::subjectDestroyed);
}

PropertyWatcher::~PropertyWatcher()
{
}

void PropertyWatcher::updateAllFields()
{
    QList<PropertyField *> fields = findChildren<PropertyField*>();
    foreach (PropertyField *field, fields)
        field->propertyChanged();
    emit updatedAllFields(this);
}

void PropertyWatcher::subjectDestroyed()
{
    hide();
    deleteLater();
}
