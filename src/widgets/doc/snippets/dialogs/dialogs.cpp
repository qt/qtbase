// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

typedef QDialog WordCountDialog;
typedef QDialog FindDialog;

#define this 0
#define setWordCount(x) isVisible()

QString tr(const char *text)
{
    return QApplication::translate(text, text);
}

class EditorWindow : public QMainWindow
{
public:
    void find();
    void countWords();

private:
    FindDialog *findDialog;
};

//! [0]
void EditorWindow::find()
{
    if (!findDialog) {
        findDialog = new FindDialog(this);
        connect(findDialog, &FindDialog::findNext,
                this, &EditorWindow::findNext);
    }

    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}
//! [0]

//! [1]
void EditorWindow::countWords()
{
    WordCountDialog dialog(this);
    dialog.setWordCount(document().wordCount());
    dialog.exec();
}
//! [1]

inline bool boo()
{
    QMessageBox::information(this, "Application name",
                             "Unable to find the user preferences file.\n"
                             "The factory default will be used instead.");

    QString filename;
    if (QFile::exists(filename) &&
        QMessageBox::question(
            this,
            tr("Overwrite File? -- Application Name"),
            tr("A file called %1 already exists."
               "Do you want to overwrite it?")
            .arg(filename),
            tr("&Yes"), tr("&No"),
            QString(), 0, 1))
        return false;

    switch(QMessageBox::warning(this, "Application name",
                                "Could not connect to the <mumble> server.\n"
                                "This program can't function correctly "
                                "without the server.\n\n",
                                "Retry",
                                "Quit", 0, 0, 1)) {
    case 0: // The user clicked the Retry again button or pressed Enter
        // try again
        break;
    case 1: // The user clicked the Quit or pressed Escape
        // exit
        break;
    }

    switch(QMessageBox::information(this, "Application name here",
                                    "The document contains unsaved changes\n"
                                    "Do you want to save the changes before exiting?",
                                    "&Save", "&Discard", "Cancel",
                                    0,      // Enter == button 0
                                    2)) { // Escape == button 2
    case 0: // Save clicked or Alt+S pressed or Enter pressed.
        // save
        break;
    case 1: // Discard clicked or Alt+D pressed
        // don't save but exit
        break;
    case 2: // Cancel clicked or Escape pressed
        // don't exit
        break;
    }

    switch(QMessageBox::warning(this, "Application name here",
                                "Could not save the user preferences,\n"
                                "because the disk is full. You can delete\n"
                                "some files and press Retry, or you can\n"
                                "abort the Save Preferences operation.",
                                QMessageBox::Retry | QMessageBox::Default,
                                QMessageBox::Abort | QMessageBox::Escape)) {
    case QMessageBox::Retry: // Retry clicked or Enter pressed
        // try again
        break;
    case QMessageBox::Abort: // Abort clicked or Escape pressed
        // abort
        break;
    }

    QString errorDetails;
    QMessageBox::critical(0, "Application name here",
                          QString("An internal error occurred. Please ") +
                          "call technical support at 1234-56789 and report\n"+
                          "these numbers:\n\n" + errorDetails +
                          "\n\nApplication will now exit.");

    QMessageBox::about(this, "About <Application>",
                       "<Application> is a <one-paragraph blurb>\n\n"
                       "Copyright 1991-2003 Such-and-such. "
                       "<License words here.>\n\n"
                       "For technical support, call 1234-56789 or see\n"
                       "http://www.such-and-such.com/Application/\n");

    {
        // saving the file
        QMessageBox mb("Application name here",
                       "Saving the file will overwrite the original file on the disk.\n"
                       "Do you really want to save?",
                       QMessageBox::Information,
                       QMessageBox::Yes | QMessageBox::Default,
                       QMessageBox::No,
                       QMessageBox::Cancel | QMessageBox::Escape);
        mb.setButtonText(QMessageBox::Yes, "Save");
        mb.setButtonText(QMessageBox::No, "Discard");
        switch(mb.exec()) {
        case QMessageBox::Yes:
            // save and exit
            break;
        case QMessageBox::No:
            // exit without saving
            break;
        case QMessageBox::Cancel:
            // don't save and don't exit
            break;
        }
    }

    {
        // hardware failure
//! [2]
        QMessageBox mb("Application Name",
                       "Hardware failure.\n\nDisk error detected\nDo you want to stop?",
                       QMessageBox::Question,
                       QMessageBox::Yes | QMessageBox::Default,
                       QMessageBox::No | QMessageBox::Escape,
                       QMessageBox::NoButton);
        if (mb.exec() == QMessageBox::No) {
            // try again
//! [2]
        }
    }
}

inline void moo()
{
    int numFiles;
//! [3]
    QProgressDialog progress("Copying files...", "Abort Copy", 0, numFiles, this);
    progress.setWindowModality(Qt::WindowModal);

    for (int i = 0; i < numFiles; i++) {
        progress.setValue(i);

        if (progress.wasCanceled())
            break;
        //... copy one file
    }
    progress.setValue(numFiles);
//! [3]
}

class Operation : public QObject
{
public:
    Operation(QObject *parent);
    void perform();
    void cancel();

private:
    int steps;
    QProgressDialog *pd;
    QTimer *t;
};

//! [4]
// Operation constructor
Operation::Operation(QObject *parent)
    : QObject(parent), steps(0)
{
    pd = new QProgressDialog("Operation in progress.", "Cancel", 0, 100);
    connect(pd, &QProgressDialog::canceled, this, &Operation::cancel);
    t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &Operation::perform);
    t->start(0);
}
//! [4] //! [5]

void Operation::perform()
{
    pd->setValue(steps);
    //... perform one percent of the operation
    steps++;
    if (steps > pd->maximum())
        t->stop();
}
//! [5] //! [6]

void Operation::cancel()
{
    t->stop();
    //... cleanup
}
//! [6]

void extension()
{
    using ExtendedControls = QWidget;
    QPushButton *findButton;
    QPushButton *moreButton;
    QWidget *extension;
    QVBoxLayout *mainLayout;

//! [extension]
    findButton = new QPushButton(tr("&Find"));
    moreButton = new QPushButton(tr("&More..."));
    moreButton->setCheckable(true);

    extension = new ExtendedControls;
    mainLayout->addWidget(extension);
    extension->hide();

    connect(moreButton, &QAbstractButton::toggled, extension, &QWidget::setVisible);
//! [extension]

//! [buttonbox]
    QDialogButtonBox *buttonBox = new QDialogButtonBox(Qt::Vertical);
    buttonBox->addButton(findButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(moreButton, QDialogButtonBox::ActionRole);
//! [buttonbox]
}

int main()
{
}
