// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "browser.h"
#include "qsqlconnectiondialog.h"
#include <ui_browserwidget.h>

#include <QAction>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextEdit>
#include <QTimer>

Browser::Browser(QWidget *parent)
    : QWidget(parent)
    , m_ui{std::make_unique<Ui::Browser>()}
{
    m_ui->setupUi(this);

    m_ui->table->addAction(m_ui->insertRowAction);
    m_ui->table->addAction(m_ui->deleteRowAction);
    m_ui->table->addAction(m_ui->fieldStrategyAction);
    m_ui->table->addAction(m_ui->rowStrategyAction);
    m_ui->table->addAction(m_ui->manualStrategyAction);
    m_ui->table->addAction(m_ui->submitAction);
    m_ui->table->addAction(m_ui->revertAction);
    m_ui->table->addAction(m_ui->selectAction);

    connect(m_ui->insertRowAction, &QAction::triggered, this, &Browser::insertRow);
    connect(m_ui->deleteRowAction, &QAction::triggered, this, &Browser::deleteRow);
    connect(m_ui->fieldStrategyAction, &QAction::triggered, this, &Browser::onFieldStrategyAction);
    connect(m_ui->rowStrategyAction, &QAction::triggered, this, &Browser::onRowStrategyAction);
    connect(m_ui->sqlEdit, &QTextEdit::textChanged, this, &Browser::updateActions);

    connect(m_ui->connectionWidget, &ConnectionWidget::tableActivated,
            this, &Browser::showTable);
    connect(m_ui->connectionWidget, &ConnectionWidget::metaDataRequested,
            this, &Browser::showMetaData);

    connect(m_ui->submitButton, &QPushButton::clicked,
            this, &Browser::onSubmitButton);
    connect(m_ui->clearButton, &QPushButton::clicked,
            this, &Browser::onClearButton);

    if (QSqlDatabase::drivers().isEmpty())
        QMessageBox::information(this, tr("No database drivers found"),
                                 tr("This demo requires at least one Qt database driver. "
                                    "Please check the documentation how to build the "
                                    "Qt SQL plugins."));

    QTimer::singleShot(0, this, [this]() {
        updateActions();
        emit statusMessage(tr("Ready."));
    });
}

Browser::~Browser()
    = default;

void Browser::exec()
{
    QSqlQueryModel *model = new QSqlQueryModel(m_ui->table);
    model->setQuery(QSqlQuery(m_ui->sqlEdit->toPlainText(), m_ui->connectionWidget->currentDatabase()));
    m_ui->table->setModel(model);

    if (model->lastError().type() != QSqlError::NoError)
        emit statusMessage(model->lastError().text());
    else if (model->query().isSelect())
        emit statusMessage(tr("Query OK."));
    else
        emit statusMessage(tr("Query OK, number of affected rows: %1").arg(
                           model->query().numRowsAffected()));

    updateActions();
}

QSqlError Browser::addConnection(const QString &driver, const QString &dbName, const QString &host,
                                 const QString &user, const QString &passwd, int port)
{
    static int cCount = 0;

    QSqlError err;
    QSqlDatabase db = QSqlDatabase::addDatabase(driver, QString("Browser%1").arg(++cCount));
    db.setDatabaseName(dbName);
    db.setHostName(host);
    db.setPort(port);
    if (!db.open(user, passwd)) {
        err = db.lastError();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QString("Browser%1").arg(cCount));
    }
    m_ui->connectionWidget->refresh();

    return err;
}

void Browser::openNewConnectionDialog()
{
    QSqlConnectionDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.useInMemoryDatabase()) {
        QSqlDatabase::database("in_mem_db", false).close();
        QSqlDatabase::removeDatabase("in_mem_db");
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "in_mem_db");
        db.setDatabaseName(":memory:");
        if (!db.open()) {
            QMessageBox::warning(this, tr("Unable to open database"),
                                 tr("An error occurred while "
                                    "opening the connection: %1") .arg(db.lastError().text()));
            return;
        }
        db.transaction();
        QSqlQuery q(db);
        q.exec("drop table Movies");
        q.exec("drop table Names");
        q.exec("create table Movies (id integer primary key, Title varchar, Director varchar, Rating number)");
        q.exec("insert into Movies values (0, 'Metropolis', 'Fritz Lang', '8.4')");
        q.exec("insert into Movies values (1, 'Nosferatu, eine Symphonie des Grauens', 'F.W. Murnau', '8.1')");
        q.exec("insert into Movies values (2, 'Bis ans Ende der Welt', 'Wim Wenders', '6.5')");
        q.exec("insert into Movies values (3, 'Hardware', 'Richard Stanley', '5.2')");
        q.exec("insert into Movies values (4, 'Mitchell', 'Andrew V. McLaglen', '2.1')");
        q.exec("create table Names (id integer primary key, FirstName varchar, LastName varchar, City varchar)");
        q.exec("insert into Names values (0, 'Sala', 'Palmer', 'Morristown')");
        q.exec("insert into Names values (1, 'Christopher', 'Walker', 'Morristown')");
        q.exec("insert into Names values (2, 'Donald', 'Duck', 'Andeby')");
        q.exec("insert into Names values (3, 'Buck', 'Rogers', 'Paris')");
        q.exec("insert into Names values (4, 'Sherlock', 'Holmes', 'London')");
        db.commit();
        m_ui->connectionWidget->refresh();
    } else {
        QSqlError err = addConnection(dialog.driverName(), dialog.databaseName(), dialog.hostName(),
                                      dialog.userName(), dialog.password(), dialog.port());
        if (err.type() != QSqlError::NoError)
            QMessageBox::warning(this, tr("Unable to open database"),
                                 tr("An error occurred while "
                                    "opening the connection: %1").arg(err.text()));
    }
}

void Browser::showTable(const QString &t)
{
    QSqlTableModel *model = new CustomModel(m_ui->table, m_ui->connectionWidget->currentDatabase());
    model->setEditStrategy(QSqlTableModel::OnRowChange);
    model->setTable(m_ui->connectionWidget->currentDatabase().driver()->escapeIdentifier(t, QSqlDriver::TableName));
    model->select();
    if (model->lastError().type() != QSqlError::NoError)
        emit statusMessage(model->lastError().text());

    m_ui->table->setModel(model);
    m_ui->table->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);
    connect(m_ui->table->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &Browser::updateActions);

    connect(m_ui->submitAction, &QAction::triggered, model, &QSqlTableModel::submitAll);
    connect(m_ui->revertAction, &QAction::triggered, model, &QSqlTableModel::revertAll);
    connect(m_ui->selectAction, &QAction::triggered, model, &QSqlTableModel::select);

    updateActions();
}

void Browser::showMetaData(const QString &t)
{
    QSqlRecord rec = m_ui->connectionWidget->currentDatabase().record(t);
    QStandardItemModel *model = new QStandardItemModel(m_ui->table);

    model->insertRows(0, rec.count());
    model->insertColumns(0, 7);

    model->setHeaderData(0, Qt::Horizontal, "Fieldname");
    model->setHeaderData(1, Qt::Horizontal, "Type");
    model->setHeaderData(2, Qt::Horizontal, "Length");
    model->setHeaderData(3, Qt::Horizontal, "Precision");
    model->setHeaderData(4, Qt::Horizontal, "Required");
    model->setHeaderData(5, Qt::Horizontal, "AutoValue");
    model->setHeaderData(6, Qt::Horizontal, "DefaultValue");

    for (int i = 0; i < rec.count(); ++i) {
        QSqlField fld = rec.field(i);
        model->setData(model->index(i, 0), fld.name());
        model->setData(model->index(i, 1),  QString::fromUtf8(fld.metaType().name()));
        model->setData(model->index(i, 2), fld.length());
        model->setData(model->index(i, 3), fld.precision());
        model->setData(model->index(i, 4), fld.requiredStatus() == -1 ? QVariant("?")
                : QVariant(bool(fld.requiredStatus())));
        model->setData(model->index(i, 5), fld.isAutoValue());
        model->setData(model->index(i, 6), fld.defaultValue());
    }

    m_ui->table->setModel(model);
    m_ui->table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    updateActions();
}

void Browser::insertRow()
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel *>(m_ui->table->model());
    if (!model)
        return;

    QModelIndex insertIndex = m_ui->table->currentIndex();
    int row = insertIndex.row() == -1 ? 0 : insertIndex.row();
    model->insertRow(row);
    insertIndex = model->index(row, 0);
    m_ui->table->setCurrentIndex(insertIndex);
    m_ui->table->edit(insertIndex);
}

void Browser::deleteRow()
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel *>(m_ui->table->model());
    if (!model)
        return;

    const QModelIndexList currentSelection = m_ui->table->selectionModel()->selectedIndexes();
    for (const auto &idx : currentSelection) {
        if (idx.column() != 0)
            continue;
        model->removeRow(idx.row());
    }

    updateActions();
}

void Browser::updateActions()
{
    QSqlTableModel *tm = qobject_cast<QSqlTableModel *>(m_ui->table->model());
    bool enableIns = tm != nullptr;
    bool enableDel = enableIns && m_ui->table->currentIndex().isValid();

    m_ui->insertRowAction->setEnabled(enableIns);
    m_ui->deleteRowAction->setEnabled(enableDel);

    m_ui->submitAction->setEnabled(tm);
    m_ui->revertAction->setEnabled(tm);
    m_ui->selectAction->setEnabled(tm);

    const bool isEmpty = m_ui->sqlEdit->toPlainText().isEmpty();
    m_ui->submitButton->setEnabled(m_ui->connectionWidget->currentDatabase().isOpen() && !isEmpty);
    m_ui->clearButton->setEnabled(!isEmpty);

    if (tm) {
        QSqlTableModel::EditStrategy es = tm->editStrategy();
        m_ui->fieldStrategyAction->setChecked(es == QSqlTableModel::OnFieldChange);
        m_ui->rowStrategyAction->setChecked(es == QSqlTableModel::OnRowChange);
        m_ui->manualStrategyAction->setChecked(es == QSqlTableModel::OnManualSubmit);
    } else {
        m_ui->fieldStrategyAction->setEnabled(false);
        m_ui->rowStrategyAction->setEnabled(false);
        m_ui->manualStrategyAction->setEnabled(false);
    }
}

void Browser::about()
{
    QMessageBox::about(this, tr("About"),
                       tr("The SQL Browser demonstration shows how a data browser "
                          "can be used to visualize the results of SQL "
                          "statements on a live database"));
}

void Browser::onFieldStrategyAction()
{
    QSqlTableModel *tm = qobject_cast<QSqlTableModel *>(m_ui->table->model());
    if (tm)
        tm->setEditStrategy(QSqlTableModel::OnFieldChange);
}

void Browser::onRowStrategyAction()
{
    QSqlTableModel *tm = qobject_cast<QSqlTableModel *>(m_ui->table->model());
    if (tm)
        tm->setEditStrategy(QSqlTableModel::OnRowChange);
}

void Browser::onManualStrategyAction()
{
    QSqlTableModel *tm = qobject_cast<QSqlTableModel *>(m_ui->table->model());
    if (tm)
        tm->setEditStrategy(QSqlTableModel::OnManualSubmit);
}

void Browser::onSubmitButton()
{
    exec();
    m_ui->sqlEdit->setFocus();
}

void Browser::onClearButton()
{
    m_ui->sqlEdit->clear();
    m_ui->sqlEdit->setFocus();
}
