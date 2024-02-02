// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "propertywatcher.h"
#include <QMetaProperty>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include "propertyfield.h"

PropertyWatcher::PropertyWatcher(QObject *subject, QString annotation, QWidget *parent)
    : QWidget(parent), m_subject(nullptr), m_formLayout(new QFormLayout(this))
{
    setMinimumSize(450, 300);
    m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setSubject(subject, annotation);
}

class UpdatesEnabledBlocker
{
    Q_DISABLE_COPY(UpdatesEnabledBlocker);
public:
    explicit UpdatesEnabledBlocker(QWidget *w) : m_widget(w)
    {
        m_widget->setUpdatesEnabled(false);
    }
    ~UpdatesEnabledBlocker()
    {
        m_widget->setUpdatesEnabled(true);
        m_widget->update();
    }

private:
    QWidget *m_widget;
};

void  PropertyWatcher::setSubject(QObject *s, const QString &annotation)
{
    if (s == m_subject)
        return;

    UpdatesEnabledBlocker blocker(this);

    if (m_subject) {
        disconnect(m_subject, &QObject::destroyed, this, &PropertyWatcher::subjectDestroyed);
        for (int i = m_formLayout->count() - 1; i >= 0; --i) {
            QLayoutItem *item = m_formLayout->takeAt(i);
            delete item->widget();
            delete item;
        }
        window()->setWindowTitle(QString());
        window()->setWindowIconText(QString());
    }

    m_subject = s;
    if (!m_subject)
        return;

    const QMetaObject* meta = m_subject->metaObject();
    QString title = QLatin1String("Properties ") + QLatin1String(meta->className());
    if (!m_subject->objectName().isEmpty())
        title += QLatin1Char(' ') + m_subject->objectName();
    if (!annotation.isEmpty())
        title += QLatin1Char(' ') + annotation;
    window()->setWindowTitle(title);

    for (int i = 0, count = meta->propertyCount(); i < count; ++i) {
        const QMetaProperty prop = meta->property(i);
        if (prop.isReadable()) {
            QLabel *label = new QLabel(prop.name(), this);
            PropertyField *field = new PropertyField(m_subject, prop, this);
            m_formLayout->addRow(label, field);
            if (!qstrcmp(prop.name(), "name"))
                window()->setWindowIconText(prop.read(m_subject).toString());
            label->setVisible(true);
            field->setVisible(true);
        }
    }
    connect(m_subject, &QObject::destroyed, this, &PropertyWatcher::subjectDestroyed);

    QPushButton *updateButton = new QPushButton(QLatin1String("Update"), this);
    connect(updateButton, &QPushButton::clicked, this, &PropertyWatcher::updateAllFields);
    m_formLayout->addRow(QString(), updateButton);
}

void PropertyWatcher::updateAllFields()
{
    const QList<PropertyField *> fields = findChildren<PropertyField*>();
    for (PropertyField *field : fields)
        field->propertyChanged();
    emit updatedAllFields(this);
}

void PropertyWatcher::subjectDestroyed()
{
    qDebug("screen destroyed");
}
