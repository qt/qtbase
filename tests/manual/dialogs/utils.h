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

#ifndef UTILS_H
#define UTILS_H

#include <QComboBox>
#include <QGroupBox>
#include <QVariant>
#include <QPair>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QCheckBox)

// Associate enum/flag value with a description.
struct FlagData
{
    const char *description;
    int value;
};

// Helpers for creating combo boxes representing enumeration values from flag data.
QComboBox *createCombo(QWidget *parent, const FlagData *d, size_t size);
void populateCombo(QComboBox *combo, const FlagData *d, size_t size);

template <class Enum>
Enum comboBoxValue(const QComboBox *c)
{
    return static_cast<Enum>(c->itemData(c->currentIndex()).toInt());
}

void setComboBoxValue(QComboBox *c, int v);

// A group box with check boxes for option flags.
class OptionsControl : public QGroupBox {
public:
    OptionsControl(QWidget *parent);
    explicit OptionsControl(const QString &title, const FlagData *data, size_t count, QWidget *parent);

    void populateOptions(const FlagData *data, size_t count);

    void setValue(int flags);
    template <class Enum>
    Enum value() const { return static_cast<Enum>(intValue()); }

private:
    typedef QPair<QCheckBox *, int> CheckBoxFlagPair;

    int intValue() const;

    QList<CheckBoxFlagPair> m_checkBoxes;
};

#endif // UTILS_H
