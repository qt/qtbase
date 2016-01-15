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

#include "utils.h"

#include <QCheckBox>
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
