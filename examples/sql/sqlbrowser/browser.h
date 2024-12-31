// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BROWSER_H
#define BROWSER_H

#include <QWidget>
#include <QSqlTableModel>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QSqlError)

QT_BEGIN_NAMESPACE
namespace Ui
{
class Browser;
}
QT_END_NAMESPACE

class Browser : public QWidget
{
    Q_OBJECT
public:
    explicit Browser(QWidget *parent = nullptr);
    ~Browser() override;

    QSqlError addConnection(const QString &driver, const QString &dbName, const QString &host,
                            const QString &user, const QString &passwd, int port);

public slots:
    void openNewConnectionDialog();
    void about();

protected:
    void insertRow();
    void deleteRow();
    void updateActions();

protected slots:
    void exec();
    void showTable(const QString &table);
    void showMetaData(const QString &table);

    void onFieldStrategyAction();
    void onRowStrategyAction();
    void onManualStrategyAction();
    void onSubmitButton();
    void onClearButton();

signals:
    void statusMessage(const QString &message);

private:
    const std::unique_ptr<Ui::Browser> m_ui;
};

class CustomModel : public QSqlTableModel
{
    Q_OBJECT
public:
    using QSqlTableModel::QSqlTableModel;
    QVariant data(const QModelIndex &idx, int role) const override
    {
        if (role == Qt::BackgroundRole && isDirty(idx))
            return QBrush(QColor(Qt::yellow));
        return QSqlTableModel::data(idx, role);
    }
};

#endif
