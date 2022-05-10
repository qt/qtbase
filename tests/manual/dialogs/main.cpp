// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filedialogpanel.h"
#include "colordialogpanel.h"
#include "fontdialogpanel.h"
#include "printdialogpanel.h"
#include "wizardpanel.h"
#include "messageboxpanel.h"

#include <QLibraryInfo>
#include <QDialogButtonBox>
#include <QMainWindow>
#include <QApplication>
#include <QMenuBar>
#include <QTabWidget>
#include <QFormLayout>
#include <QMenu>
#include <QAction>
#include <QKeySequence>

static bool optNoPrinter = false;

// Test for dialogs, allowing to play with all dialog options for implementing native dialogs.
// Compiles with Qt 4.8 and Qt 5.

class AboutDialog : public QDialog
{
public:
    explicit AboutDialog(QWidget *parent = nullptr);
};

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QFormLayout *mainLayout = new QFormLayout(this);
    mainLayout->addRow(new QLabel(QLibraryInfo::build()));
    mainLayout->addRow("Style:", new QLabel(qApp->style()->objectName()));
    mainLayout->addRow("DPR:", new QLabel(QString::number(qApp->devicePixelRatio())));
    const QString resolution = QString::number(logicalDpiX()) + QLatin1Char(',')
                               + QString::number(logicalDpiY()) + QLatin1String("dpi");
    mainLayout->addRow("Resolution:", new QLabel(resolution));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal);
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addRow(buttonBox);
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void aboutDialog();
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("Dialogs Qt %1").arg(QLatin1String(QT_VERSION_STR)));
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    QAction *quitAction = fileMenu->addAction(tr("Quit"));
    quitAction->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *action = editMenu->addAction(tr("Cut"));
    action->setShortcut(QKeySequence(QKeySequence::Cut));
    action = editMenu->addAction(tr("Copy"));
    action->setShortcut(QKeySequence(QKeySequence::Copy));
    action = editMenu->addAction(tr("Paste"));
    action->setShortcut(QKeySequence(QKeySequence::Paste));
    action = editMenu->addAction(tr("Select All"));
    action->setShortcut(QKeySequence(QKeySequence::SelectAll));
    QMenu *aboutMenu = menuBar()->addMenu(tr("&About"));
    QAction *aboutAction = aboutMenu->addAction(tr("About..."), this, SLOT(aboutDialog()));
    aboutAction->setShortcut(Qt::Key_F1);
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->addTab(new FileDialogPanel, tr("QFileDialog"));
    tabWidget->addTab(new ColorDialogPanel, tr("QColorDialog"));
    tabWidget->addTab(new FontDialogPanel, tr("QFontDialog"));
    tabWidget->addTab(new WizardPanel, tr("QWizard"));
    tabWidget->addTab(new MessageBoxPanel, tr("QMessageBox"));
#ifndef QT_NO_PRINTER
    if (!optNoPrinter)
        tabWidget->addTab(new PrintDialogPanel, tr("QPrintDialog"));
#endif
    setCentralWidget(tabWidget);
}

void MainWindow::aboutDialog()
{
    AboutDialog dialog(this);
    dialog.setWindowTitle(tr("About Dialogs"));
    dialog.exec();
}

int main(int argc, char *argv[])
{
    for (int a = 1; a < argc; ++a) {
        if (!qstrcmp(argv[a], "-n")) {
            qDebug("AA_DontUseNativeDialogs");
            QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        } else if (!qstrcmp(argv[a], "-p")) {
            optNoPrinter = true; // Avoid startup slowdown by printer code
        }
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.move(500, 200);
    w.show();
    return a.exec();
}

#include "main.moc"
