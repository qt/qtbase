/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    m_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
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
    qDebug("screen destroyed");
}
