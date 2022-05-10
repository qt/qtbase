// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QComboBox>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMimeData>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

namespace dropevents {
class Window : public QWidget
{

public:
    explicit Window(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QComboBox *mimeTypeCombo = nullptr;
    QTextBrowser *textBrowser = nullptr;
    QString oldText;
    QStringList oldMimeTypes;
};

//! [0]
Window::Window(QWidget *parent)
    : QWidget(parent)
{
//! [0]
    QLabel *textLabel = new QLabel(tr("Data:"), this);
    textBrowser = new QTextBrowser(this);

    QLabel *mimeTypeLabel = new QLabel(tr("MIME types:"), this);
    mimeTypeCombo = new QComboBox(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(textLabel);
    layout->addWidget(textBrowser);
    layout->addWidget(mimeTypeLabel);
    layout->addWidget(mimeTypeCombo);

//! [1]
    setAcceptDrops(true);
//! [1]
    setWindowTitle(tr("Drop Events"));
//! [2]
}
//! [2]

//! [3]
void Window::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain"))
        event->acceptProposedAction();
}
//! [3]

//! [4]
void Window::dropEvent(QDropEvent *event)
{
    textBrowser->setPlainText(event->mimeData()->text());
    mimeTypeCombo->clear();
    mimeTypeCombo->addItems(event->mimeData()->formats());

    event->acceptProposedAction();
}
//! [4]

} // dropevents
