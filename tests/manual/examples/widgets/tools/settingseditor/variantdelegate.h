// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef VARIANTDELEGATE_H
#define VARIANTDELEGATE_H

#include <QStyledItemDelegate>
#include <QRegularExpression>
#include <QSharedPointer>

struct TypeChecker
{
    TypeChecker();

    QRegularExpression boolExp;
    QRegularExpression byteArrayExp;
    QRegularExpression charExp;
    QRegularExpression colorExp;
    QRegularExpression dateExp;
    QRegularExpression dateTimeExp;
    QRegularExpression doubleExp;
    QRegularExpression pointExp;
    QRegularExpression rectExp;
    QRegularExpression signedIntegerExp;
    QRegularExpression sizeExp;
    QRegularExpression timeExp;
    QRegularExpression unsignedIntegerExp;
};

class VariantDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit VariantDelegate(const QSharedPointer<TypeChecker> &typeChecker,
                             QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    static bool isSupportedType(int type);
    static QString displayText(const QVariant &value);

private:
    QSharedPointer<TypeChecker> m_typeChecker;
};

#endif
