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
#include "interactivewidget.h"
#include <QtWidgets>

InteractiveWidget::InteractiveWidget()
{
    m_onScreenWidget = new OnScreenWidget<QWidget>("");
    m_onScreenWidget->setMinimumSize(320, 240);

    setCentralWidget(m_onScreenWidget);

    ui_textEdit = new QTextEdit();
    ui_textEdit->installEventFilter(this);

    QWidget *panelContent = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout(panelContent);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    // create and populate the command toolbox
    m_commandsToolBox = new QToolBox();
    QListWidget *currentListWidget = 0;
    foreach (PaintCommands::PaintCommandInfos paintCommandInfo, PaintCommands::s_commandInfoTable) {
        if (paintCommandInfo.isSectionHeader()) {
            currentListWidget = new QListWidget();
            m_commandsToolBox->addItem(currentListWidget, QIcon(":/icons/tools.png"), "commands - "+paintCommandInfo.identifier);
            connect(currentListWidget, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(cmdSelected(QListWidgetItem*)));
        } else {
            (new QListWidgetItem(paintCommandInfo.identifier, currentListWidget))->setToolTip(paintCommandInfo.syntax);
        }
    }

    // create and populate the enumerations toolbox
    m_enumsToolBox = new QToolBox();
    typedef QPair<QString,QStringList> EnumListType;
    foreach (EnumListType enumInfos, PaintCommands::s_enumsTable) {
        currentListWidget = new QListWidget();
        m_commandsToolBox->addItem(currentListWidget, QIcon(":/icons/enum.png"), "enums - "+enumInfos.first);
        connect(currentListWidget, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(enumSelected(QListWidgetItem*)));
        foreach (QString enumItem, enumInfos.second)
            new QListWidgetItem(enumItem, currentListWidget);
    }

    // add other widgets and layout
    vlayout->addWidget(m_commandsToolBox);
    vlayout->addWidget(m_enumsToolBox);

    QPushButton *run = new QPushButton("&Run");
    QPushButton *load = new QPushButton("&Load");
    QPushButton *save = new QPushButton("&Save");
    run->setFocusPolicy(Qt::NoFocus);

    vlayout->addSpacing(20);
    vlayout->addWidget(run);
    vlayout->addWidget(load);
    vlayout->addWidget(save);

    QDockWidget *panel = new QDockWidget("Commands");
    panel->setWidget(panelContent);
    addDockWidget(Qt::LeftDockWidgetArea, panel);

    QDockWidget *editor = new QDockWidget("Editor");
    editor->setWidget(ui_textEdit);
    addDockWidget(Qt::RightDockWidgetArea, editor);

    // connect gui signals
    connect(run, SIGNAL(clicked()), SLOT(run()));
    connect(load, SIGNAL(clicked()), SLOT(load()));
    connect(save, SIGNAL(clicked()), SLOT(save()));
}

/***************************************************************************************************/
void InteractiveWidget::run()
{
    m_onScreenWidget->m_commands.clear();
    QString script = ui_textEdit->toPlainText();
    QStringList lines = script.split("\n");
    for (int i = 0; i < lines.size(); ++i)
        m_onScreenWidget->m_commands.append(lines.at(i));
    m_onScreenWidget->repaint();
}

/***************************************************************************************************/
void InteractiveWidget::cmdSelected(QListWidgetItem *item)
{
    if (ui_textEdit->textCursor().atBlockStart()) {
        ui_textEdit->insertPlainText(PaintCommands::findCommandById(item->text())->sample + "\n");
    } else {
        ui_textEdit->moveCursor(QTextCursor::EndOfLine);
        ui_textEdit->insertPlainText("\n" + PaintCommands::findCommandById(item->text())->sample);
    }
    ui_textEdit->setFocus();
}

/***************************************************************************************************/
void InteractiveWidget::enumSelected(QListWidgetItem *item)
{
    ui_textEdit->insertPlainText(item->text());
    ui_textEdit->setFocus();
}

/***************************************************************************************************/
void InteractiveWidget::load()
{
    QString fname = QFileDialog::getOpenFileName(
        this,
        QString("Load QPaintEngine Script"),
        QFileInfo(m_filename).absoluteFilePath(),
        QString("QPaintEngine Script (*.qps);;All files (*.*)"));

    load(fname);
}

/***************************************************************************************************/
void InteractiveWidget::load(const QString &fname)
{
    if (!fname.isEmpty()) {
        m_filename = fname;
        ui_textEdit->clear();
        QFile file(fname);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream textFile(&file);
        QString script = textFile.readAll();
        ui_textEdit->setPlainText(script);
        m_onScreenWidget->m_filename = fname;
    }
}

/***************************************************************************************************/
void InteractiveWidget::save()
{
    QString script = ui_textEdit->toPlainText();
    if (!script.endsWith("\n"))
        script += QString("\n");
    QString fname = QFileDialog::getSaveFileName(this,
                            QString("Save QPaintEngine Script"),
                            QFileInfo(m_filename).absoluteFilePath(),
                            QString("QPaintEngine Script (*.qps);;All files (*.*)"));
    if (!fname.isEmpty()) {
        m_filename = fname;
        QFile file(fname);
        file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
        QTextStream textFile(&file);
        textFile << script;
        m_onScreenWidget->m_filename = fname;
    }
}

/***************************************************************************************************/
bool InteractiveWidget::eventFilter(QObject *o, QEvent *e)
{
    if (qobject_cast<QTextEdit *>(o) && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Tab) {
            m_commandsToolBox->currentWidget()->setFocus();
            return true;
        } else if (ke->key() == Qt::Key_Return && ke->modifiers() == Qt::ControlModifier) {
            run();
            return true;
        }
    }
    return false;
}
