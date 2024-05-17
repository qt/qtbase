// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef UTILS_H
#define UTILS_H

#include <QComboBox>
#include <QGroupBox>
#include <QVariant>
#include <QPair>
#include <QList>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QGridLayout)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QPushButton)

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

QPushButton *addButton(const QString &description, QGridLayout *layout, int &row, int column,
                       QObject *receiver, const char *slotFunc);

QPushButton *addButton(const QString &description, QGridLayout *layout, int &row, int column,
                       std::function<void()> fn);

QPushButton *addButton(const QString &description, QVBoxLayout *layout, QObject *receiver,
                       const char *slotFunc);

QPushButton *addButton(const QString &description, QVBoxLayout *layout, std::function<void()> fn);

#endif // UTILS_H
