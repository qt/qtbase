// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QPushButton>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <QClipboard>
#include <QGuiApplication>

#include <QMimeData>

#include "droparea.h"
#include "dropsitewindow.h"

using namespace Qt::StringLiterals;

//! [constructor part1]
DropSiteWindow::DropSiteWindow()
{
    abstractLabel = new QLabel(tr("This example accepts drags from other "
                                  "applications and displays the MIME types "
                                  "provided by the drag object."));
    abstractLabel->setWordWrap(true);
    abstractLabel->adjustSize();
//! [constructor part1]

//! [constructor part2]
    dropArea = new DropArea;
    connect(dropArea, &DropArea::changed,
            this, &DropSiteWindow::updateFormatsTable);
//! [constructor part2]

//! [constructor part3]
    formatsTable = new QTableWidget;
    formatsTable->setColumnCount(2);
    formatsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    formatsTable->setHorizontalHeaderLabels({tr("Format"),  tr("Content")});
    formatsTable->horizontalHeader()->setStretchLastSection(true);
//! [constructor part3]

//! [constructor part4]
    clearButton = new QPushButton(tr("Clear"));
    copyButton = new QPushButton(tr("Copy"));
    quitButton = new QPushButton(tr("Quit"));

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(clearButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(copyButton, QDialogButtonBox::ActionRole);
#if !QT_CONFIG(clipboard)
    copyButton->setVisible(false);
#endif

    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    connect(quitButton, &QAbstractButton::clicked, this, &QWidget::close);
    connect(clearButton, &QAbstractButton::clicked, dropArea, &DropArea::clear);
    connect(copyButton, &QAbstractButton::clicked, this, &DropSiteWindow::copy);
//! [constructor part4]

//! [constructor part5]
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(abstractLabel);
    mainLayout->addWidget(dropArea);
    mainLayout->addWidget(formatsTable);
    mainLayout->addWidget(buttonBox);

    setWindowTitle(tr("Drop Site"));
    resize(700, 500);
}
//! [constructor part5]

//! [updateFormatsTable() part1]
void DropSiteWindow::updateFormatsTable(const QMimeData *mimeData)
{
    formatsTable->setRowCount(0);
    copyButton->setEnabled(false);
    if (!mimeData)
        return;
//! [updateFormatsTable() part1]

//! [updateFormatsTable() part2]
    const QStringList formats = mimeData->formats();
    for (const QString &format : formats) {
        QTableWidgetItem *formatItem = new QTableWidgetItem(format);
        formatItem->setFlags(Qt::ItemIsEnabled);
        formatItem->setTextAlignment(Qt::AlignTop | Qt::AlignLeft);
//! [updateFormatsTable() part2]

//! [updateFormatsTable() part3]
        QString text;
        if (format == u"text/plain") {
            text = mimeData->text().simplified();
        } else if (format == u"text/markdown") {
            text = QString::fromUtf8(mimeData->data(u"text/markdown"_s));
        } else if (format == u"text/html") {
            text = mimeData->html().simplified();
        } else if (format == u"text/uri-list") {
            QList<QUrl> urlList = mimeData->urls();
            for (qsizetype i = 0, count = qMin(urlList.size(), qsizetype(32)); i < count; ++i)
                text.append(urlList.at(i).toString() + u' ');
        } else {
            QByteArray data = mimeData->data(format);
            if (data.size() > 32)
                data.truncate(32);
            text = QString::fromLatin1(data.toHex(' ')).toUpper();
        }
//! [updateFormatsTable() part3]

//! [updateFormatsTable() part4]
        int row = formatsTable->rowCount();
        formatsTable->insertRow(row);
        formatsTable->setItem(row, 0, new QTableWidgetItem(format));
        formatsTable->setItem(row, 1, new QTableWidgetItem(text));
    }

    formatsTable->resizeColumnToContents(0);
#if QT_CONFIG(clipboard)
    copyButton->setEnabled(formatsTable->rowCount() > 0);
#endif
}
//! [updateFormatsTable() part4]

void DropSiteWindow::copy()
{
#if QT_CONFIG(clipboard)
    QString text;
    for (int row = 0, rowCount = formatsTable->rowCount(); row < rowCount; ++row)
        text += formatsTable->item(row, 0)->text() + ": " + formatsTable->item(row, 1)->text() + '\n';
    QGuiApplication::clipboard()->setText(text);
#endif
}
