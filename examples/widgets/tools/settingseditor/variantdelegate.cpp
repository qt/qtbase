/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "variantdelegate.h"

#include <QCheckBox>
#include <QDateTime>
#include <QLineEdit>
#include <QSpinBox>
#include <QRegularExpressionValidator>
#include <QTextStream>

#include <algorithm>

static bool isPrintableChar(char c)
{
    return uchar(c) >= 32 && uchar(c) < 128;
}

static bool isPrintable(const QByteArray &ba)
{
    return std::all_of(ba.cbegin(), ba.cend(), isPrintableChar);
}

static QString byteArrayToString(const QByteArray &ba)
{
    if (isPrintable(ba))
        return QString::fromLatin1(ba);
    QString result;
    for (char c : ba) {
        if (isPrintableChar(c)) {
            if (c == '\\')
                result += QLatin1Char(c);
            result += QLatin1Char(c);
        } else {
            const uint uc = uchar(c);
            result += "\\x";
            if (uc < 16)
                result += '0';
            result += QString::number(uc, 16);
        }
    }
    return result;
}

TypeChecker::TypeChecker()
{
    boolExp.setPattern("^(true)|(false)$");
    boolExp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    Q_ASSERT(boolExp.isValid());

    byteArrayExp.setPattern(R"RX(^[\x00-\xff]*$)RX");
    charExp.setPattern("^.$");
    Q_ASSERT(charExp.isValid());
    colorExp.setPattern(R"RX(^\(([0-9]*),([0-9]*),([0-9]*),([0-9]*)\)$)RX");
    Q_ASSERT(colorExp.isValid());
    doubleExp.setPattern("");
    pointExp.setPattern(R"RX(^\((-?[0-9]*),(-?[0-9]*)\)$)RX");
    Q_ASSERT(pointExp.isValid());
    rectExp.setPattern(R"RX(^\((-?[0-9]*),(-?[0-9]*),(-?[0-9]*),(-?[0-9]*)\)$)RX");
    Q_ASSERT(rectExp.isValid());
    signedIntegerExp.setPattern("^-?[0-9]*$");
    Q_ASSERT(signedIntegerExp.isValid());
    sizeExp = pointExp;
    unsignedIntegerExp.setPattern("^[0-9]+$");
    Q_ASSERT(unsignedIntegerExp.isValid());

    const QString datePattern = "([0-9]{,4})-([0-9]{,2})-([0-9]{,2})";
    dateExp.setPattern('^' + datePattern + '$');
    Q_ASSERT(dateExp.isValid());
    const QString timePattern = "([0-9]{,2}):([0-9]{,2}):([0-9]{,2})";
    timeExp.setPattern('^' + timePattern + '$');
    Q_ASSERT(timeExp.isValid());
    dateTimeExp.setPattern('^' + datePattern + 'T' + timePattern + '$');
    Q_ASSERT(dateTimeExp.isValid());
}

VariantDelegate::VariantDelegate(const QSharedPointer<TypeChecker> &typeChecker,
                                 QObject *parent)
    : QStyledItemDelegate(parent),
      m_typeChecker(typeChecker)
{
}

void VariantDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    if (index.column() == 2) {
        QVariant value = index.model()->data(index, Qt::UserRole);
        if (!isSupportedType(value.userType())) {
            QStyleOptionViewItem myOption = option;
            myOption.state &= ~QStyle::State_Enabled;
            QStyledItemDelegate::paint(painter, myOption, index);
            return;
        }
    }

    QStyledItemDelegate::paint(painter, option, index);
}

QWidget *VariantDelegate::createEditor(QWidget *parent,
        const QStyleOptionViewItem & /* option */,
        const QModelIndex &index) const
{
    if (index.column() != 2)
        return nullptr;

    QVariant originalValue = index.model()->data(index, Qt::UserRole);
    if (!isSupportedType(originalValue.userType()))
        return nullptr;

    switch (originalValue.userType()) {
    case QMetaType::Bool:
        return new QCheckBox(parent);
        break;
    case QMetaType::Int:
    case QMetaType::LongLong: {
        auto spinBox = new QSpinBox(parent);
        spinBox->setRange(-32767, 32767);
        return spinBox;
    }
    case QMetaType::UInt:
    case QMetaType::ULongLong: {
        auto spinBox = new QSpinBox(parent);
        spinBox->setRange(0, 63335);
        return spinBox;
    }
    default:
        break;
    }

    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setFrame(false);

    QRegularExpression regExp;

    switch (originalValue.userType()) {
    case QMetaType::Bool:
        regExp = m_typeChecker->boolExp;
        break;
    case QMetaType::QByteArray:
        regExp = m_typeChecker->byteArrayExp;
        break;
    case QMetaType::QChar:
        regExp = m_typeChecker->charExp;
        break;
    case QMetaType::QColor:
        regExp = m_typeChecker->colorExp;
        break;
    case QMetaType::QDate:
        regExp = m_typeChecker->dateExp;
        break;
    case QMetaType::QDateTime:
        regExp = m_typeChecker->dateTimeExp;
        break;
    case QMetaType::Double:
        regExp = m_typeChecker->doubleExp;
        break;
    case QMetaType::Int:
    case QMetaType::LongLong:
        regExp = m_typeChecker->signedIntegerExp;
        break;
    case QMetaType::QPoint:
        regExp = m_typeChecker->pointExp;
        break;
    case QMetaType::QRect:
        regExp = m_typeChecker->rectExp;
        break;
    case QMetaType::QSize:
        regExp = m_typeChecker->sizeExp;
        break;
    case QMetaType::QTime:
        regExp = m_typeChecker->timeExp;
        break;
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        regExp = m_typeChecker->unsignedIntegerExp;
        break;
    default:
        break;
    }

    if (regExp.isValid()) {
        QValidator *validator = new QRegularExpressionValidator(regExp, lineEdit);
        lineEdit->setValidator(validator);
    }

    return lineEdit;
}

void VariantDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    QVariant value = index.model()->data(index, Qt::UserRole);
    if (auto spinBox = qobject_cast<QSpinBox *>(editor)) {
        const auto userType = value.userType();
        if (userType == QMetaType::UInt || userType == QMetaType::ULongLong)
            spinBox->setValue(value.toUInt());
        else
            spinBox->setValue(value.toInt());
    } else if (auto checkBox = qobject_cast<QCheckBox *>(editor)) {
        checkBox->setChecked(value.toBool());
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor)) {
        if (value.userType() == QMetaType::QByteArray
            && !isPrintable(value.toByteArray())) {
            lineEdit->setReadOnly(true);
          }
        lineEdit->setText(displayText(value));
    }
}

void VariantDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    const QVariant originalValue = index.model()->data(index, Qt::UserRole);
    QVariant value;

    if (auto spinBox = qobject_cast<QSpinBox *>(editor)) {
        value.setValue(spinBox->value());
    } else if (auto checkBox = qobject_cast<QCheckBox *>(editor)) {
        value.setValue(checkBox->isChecked());
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor)) {
        if (!lineEdit->isModified())
            return;

        QString text = lineEdit->text();
        const QValidator *validator = lineEdit->validator();
        if (validator) {
            int pos;
            if (validator->validate(text, pos) != QValidator::Acceptable)
                return;
        }

        QRegularExpressionMatch match;

        switch (originalValue.userType()) {
        case QMetaType::QChar:
            value = text.at(0);
            break;
        case QMetaType::QColor:
            match = m_typeChecker->colorExp.match(text);
            value = QColor(qMin(match.captured(1).toInt(), 255),
                           qMin(match.captured(2).toInt(), 255),
                           qMin(match.captured(3).toInt(), 255),
                           qMin(match.captured(4).toInt(), 255));
            break;
        case QMetaType::QDate:
            {
                QDate date = QDate::fromString(text, Qt::ISODate);
                if (!date.isValid())
                    return;
                value = date;
            }
            break;
        case QMetaType::QDateTime:
            {
                QDateTime dateTime = QDateTime::fromString(text, Qt::ISODate);
                if (!dateTime.isValid())
                    return;
                value = dateTime;
            }
            break;
        case QMetaType::QPoint:
            match = m_typeChecker->pointExp.match(text);
            value = QPoint(match.captured(1).toInt(), match.captured(2).toInt());
            break;
        case QMetaType::QRect:
            match = m_typeChecker->rectExp.match(text);
            value = QRect(match.captured(1).toInt(), match.captured(2).toInt(),
                          match.captured(3).toInt(), match.captured(4).toInt());
            break;
        case QMetaType::QSize:
            match = m_typeChecker->sizeExp.match(text);
            value = QSize(match.captured(1).toInt(), match.captured(2).toInt());
            break;
        case QMetaType::QStringList:
            value = text.split(',');
            break;
        case QMetaType::QTime:
            {
                QTime time = QTime::fromString(text, Qt::ISODate);
                if (!time.isValid())
                    return;
                value = time;
            }
            break;
        default:
            value = text;
            value.convert(originalValue.userType());
        }
    }

    model->setData(index, displayText(value), Qt::DisplayRole);
    model->setData(index, value, Qt::UserRole);
}

bool VariantDelegate::isSupportedType(int type)
{
    switch (type) {
    case QMetaType::Bool:
    case QMetaType::QByteArray:
    case QMetaType::QChar:
    case QMetaType::QColor:
    case QMetaType::QDate:
    case QMetaType::QDateTime:
    case QMetaType::Double:
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::QPoint:
    case QMetaType::QRect:
    case QMetaType::QSize:
    case QMetaType::QString:
    case QMetaType::QStringList:
    case QMetaType::QTime:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        return true;
    default:
        return false;
    }
}

QString VariantDelegate::displayText(const QVariant &value)
{
    static const ushort unicodeBallotBox = 0x2610;
    static const ushort unicodeCheckmark = 0x2713;

    switch (value.userType()) {
    case QMetaType::Bool:
        return value.toBool()
            ? QString(QChar(unicodeCheckmark))
            : QString(QChar(unicodeBallotBox));
    case QMetaType::QByteArray:
        return byteArrayToString(value.toByteArray());
    case QMetaType::QChar:
    case QMetaType::Double:
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::QString:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        return value.toString();
    case QMetaType::QColor:
        {
            QColor color = qvariant_cast<QColor>(value);
            return QString("(%1,%2,%3,%4)")
                   .arg(color.red()).arg(color.green())
                   .arg(color.blue()).arg(color.alpha());
        }
    case QMetaType::QDate:
        return value.toDate().toString(Qt::ISODate);
    case QMetaType::QDateTime:
        return value.toDateTime().toString(Qt::ISODate);
    case QMetaType::UnknownType:
        return "<Invalid>";
    case QMetaType::QPoint:
        {
            QPoint point = value.toPoint();
            return QString("(%1,%2)").arg(point.x()).arg(point.y());
        }
    case QMetaType::QRect:
        {
            QRect rect = value.toRect();
            return QString("(%1,%2,%3,%4)")
                   .arg(rect.x()).arg(rect.y())
                   .arg(rect.width()).arg(rect.height());
        }
    case QMetaType::QSize:
        {
            QSize size = value.toSize();
            return QString("(%1,%2)").arg(size.width()).arg(size.height());
        }
    case QMetaType::QStringList:
        return value.toStringList().join(',');
    case QMetaType::QTime:
        return value.toTime().toString(Qt::ISODate);
    default:
        break;
    }
    return QString("<%1>").arg(value.typeName());
}
