// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>
#include <QVBoxLayout>

QComboBox *createCombo(QWidget *parent, const FlagData *d, size_t size)
{
    QComboBox *c = new QComboBox(parent);
    for (size_t i = 0; i < size; ++i)
        c->addItem(QLatin1String(d[i].description), QVariant(d[i].value));
    return c;
}

void populateCombo(QComboBox *combo, const FlagData *d, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        combo->addItem(QLatin1String(d[i].description), QVariant(d[i].value));
}

void setComboBoxValue(QComboBox *c, int v)
{
    c->setCurrentIndex(c->findData(QVariant(v)));
}

OptionsControl::OptionsControl(QWidget *parent)
    : QGroupBox(parent)
{
    setLayout(new QVBoxLayout(this));
}

OptionsControl::OptionsControl(const QString &title, const FlagData *data, size_t count, QWidget *parent)
    : QGroupBox(title, parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    for (size_t i = 0; i < count; ++i) {
        QCheckBox *box = new QCheckBox(QString::fromLatin1(data[i].description));
        m_checkBoxes.push_back(CheckBoxFlagPair(box, data[i].value));
        layout->addWidget(box);
    }
}

void OptionsControl::populateOptions(const FlagData *data, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        QCheckBox *box = new QCheckBox(QString::fromLatin1(data[i].description));
        m_checkBoxes.push_back(CheckBoxFlagPair(box, data[i].value));
        layout()->addWidget(box);
    }
}

void OptionsControl::setValue(int flags)
{
    foreach (const CheckBoxFlagPair &cf, m_checkBoxes)
        cf.first->setChecked(cf.second & flags);
}

int OptionsControl::intValue() const
{
    int result = 0;
    foreach (const CheckBoxFlagPair &cf, m_checkBoxes) {
        if (cf.first->isChecked())
            result |= cf.second;
    }
    return result;
}

QPushButton *addButton(const QString &description, QGridLayout *layout, int &row, int column,
                       QObject *receiver, const char *slotFunc)
{
    QPushButton *button = new QPushButton(description);
    QObject::connect(button, SIGNAL(clicked()), receiver, slotFunc);
    layout->addWidget(button, row++, column);
    return button;
}

QPushButton *addButton(const QString &description, QGridLayout *layout, int &row, int column,
                       std::function<void()> fn)
{
    QPushButton *button = new QPushButton(description);
    QObject::connect(button, &QPushButton::clicked, fn);
    layout->addWidget(button, row++, column);
    return button;
}

QPushButton *addButton(const QString &description, QVBoxLayout *layout, QObject *receiver,
                       const char *slotFunc)
{
    QPushButton *button = new QPushButton(description);
    QObject::connect(button, SIGNAL(clicked()), receiver, slotFunc);
    layout->addWidget(button);
    return button;
}

QPushButton *addButton(const QString &description, QVBoxLayout *layout, std::function<void()> fn)
{
    QPushButton *button = new QPushButton(description);
    QObject::connect(button, &QPushButton::clicked, fn);
    layout->addWidget(button);
    return button;
}
