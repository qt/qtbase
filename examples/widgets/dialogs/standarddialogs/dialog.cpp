/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

#include <QtWidgets>

#include "dialog.h"

#define MESSAGE \
    Dialog::tr("<p>Message boxes have a caption, a text, " \
               "and any number of buttons, each with standard or custom texts." \
               "<p>Click a button to close the message box. Pressing the Esc button " \
               "will activate the detected escape button (if any).")
#define MESSAGE_DETAILS \
    Dialog::tr("If a message box has detailed text, the user can reveal it " \
               "by pressing the Show Details... button.")


class DialogOptionsWidget : public QGroupBox
{
public:
    explicit DialogOptionsWidget(QWidget *parent = 0);

    void addCheckBox(const QString &text, int value);
    void addSpacer();
    int value() const;

private:
    typedef QPair<QCheckBox *, int> CheckBoxEntry;
    QVBoxLayout *layout;
    QList<CheckBoxEntry> checkBoxEntries;
};

DialogOptionsWidget::DialogOptionsWidget(QWidget *parent) :
    QGroupBox(parent) , layout(new QVBoxLayout)
{
    setTitle(Dialog::tr("Options"));
    setLayout(layout);
}

void DialogOptionsWidget::addCheckBox(const QString &text, int value)
{
    QCheckBox *checkBox = new QCheckBox(text);
    layout->addWidget(checkBox);
    checkBoxEntries.append(CheckBoxEntry(checkBox, value));
}

void DialogOptionsWidget::addSpacer()
{
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
}

int DialogOptionsWidget::value() const
{
    int result = 0;
    foreach (const CheckBoxEntry &checkboxEntry, checkBoxEntries)
        if (checkboxEntry.first->isChecked())
            result |= checkboxEntry.second;
    return result;
}

Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QToolBox *toolbox = new QToolBox;
    mainLayout->addWidget(toolbox);

    errorMessageDialog = new QErrorMessage(this);

    int frameStyle = QFrame::Sunken | QFrame::Panel;

    integerLabel = new QLabel;
    integerLabel->setFrameStyle(frameStyle);
    QPushButton *integerButton =
            new QPushButton(tr("QInputDialog::get&Int()"));

    doubleLabel = new QLabel;
    doubleLabel->setFrameStyle(frameStyle);
    QPushButton *doubleButton =
            new QPushButton(tr("QInputDialog::get&Double()"));

    itemLabel = new QLabel;
    itemLabel->setFrameStyle(frameStyle);
    QPushButton *itemButton = new QPushButton(tr("QInputDialog::getIte&m()"));

    textLabel = new QLabel;
    textLabel->setFrameStyle(frameStyle);
    QPushButton *textButton = new QPushButton(tr("QInputDialog::get&Text()"));

    multiLineTextLabel = new QLabel;
    multiLineTextLabel->setFrameStyle(frameStyle);
    QPushButton *multiLineTextButton = new QPushButton(tr("QInputDialog::get&MultiLineText()"));

    colorLabel = new QLabel;
    colorLabel->setFrameStyle(frameStyle);
    QPushButton *colorButton = new QPushButton(tr("QColorDialog::get&Color()"));

    fontLabel = new QLabel;
    fontLabel->setFrameStyle(frameStyle);
    QPushButton *fontButton = new QPushButton(tr("QFontDialog::get&Font()"));

    directoryLabel = new QLabel;
    directoryLabel->setFrameStyle(frameStyle);
    QPushButton *directoryButton =
            new QPushButton(tr("QFileDialog::getE&xistingDirectory()"));

    openFileNameLabel = new QLabel;
    openFileNameLabel->setFrameStyle(frameStyle);
    QPushButton *openFileNameButton =
            new QPushButton(tr("QFileDialog::get&OpenFileName()"));

    openFileNamesLabel = new QLabel;
    openFileNamesLabel->setFrameStyle(frameStyle);
    QPushButton *openFileNamesButton =
            new QPushButton(tr("QFileDialog::&getOpenFileNames()"));

    saveFileNameLabel = new QLabel;
    saveFileNameLabel->setFrameStyle(frameStyle);
    QPushButton *saveFileNameButton =
            new QPushButton(tr("QFileDialog::get&SaveFileName()"));

    criticalLabel = new QLabel;
    criticalLabel->setFrameStyle(frameStyle);
    QPushButton *criticalButton =
            new QPushButton(tr("QMessageBox::critica&l()"));

    informationLabel = new QLabel;
    informationLabel->setFrameStyle(frameStyle);
    QPushButton *informationButton =
            new QPushButton(tr("QMessageBox::i&nformation()"));

    questionLabel = new QLabel;
    questionLabel->setFrameStyle(frameStyle);
    QPushButton *questionButton =
            new QPushButton(tr("QMessageBox::&question()"));

    warningLabel = new QLabel;
    warningLabel->setFrameStyle(frameStyle);
    QPushButton *warningButton = new QPushButton(tr("QMessageBox::&warning()"));

    errorLabel = new QLabel;
    errorLabel->setFrameStyle(frameStyle);
    QPushButton *errorButton =
            new QPushButton(tr("QErrorMessage::showM&essage()"));

    connect(integerButton, SIGNAL(clicked()), this, SLOT(setInteger()));
    connect(doubleButton, SIGNAL(clicked()), this, SLOT(setDouble()));
    connect(itemButton, SIGNAL(clicked()), this, SLOT(setItem()));
    connect(textButton, SIGNAL(clicked()), this, SLOT(setText()));
    connect(multiLineTextButton, SIGNAL(clicked()), this, SLOT(setMultiLineText()));
    connect(colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
    connect(fontButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(directoryButton, SIGNAL(clicked()),
            this, SLOT(setExistingDirectory()));
    connect(openFileNameButton, SIGNAL(clicked()),
            this, SLOT(setOpenFileName()));
    connect(openFileNamesButton, SIGNAL(clicked()),
            this, SLOT(setOpenFileNames()));
    connect(saveFileNameButton, SIGNAL(clicked()),
            this, SLOT(setSaveFileName()));
    connect(criticalButton, SIGNAL(clicked()), this, SLOT(criticalMessage()));
    connect(informationButton, SIGNAL(clicked()),
            this, SLOT(informationMessage()));
    connect(questionButton, SIGNAL(clicked()), this, SLOT(questionMessage()));
    connect(warningButton, SIGNAL(clicked()), this, SLOT(warningMessage()));
    connect(errorButton, SIGNAL(clicked()), this, SLOT(errorMessage()));

    QWidget *page = new QWidget;
    QGridLayout *layout = new QGridLayout(page);
    layout->setColumnStretch(1, 1);
    layout->setColumnMinimumWidth(1, 250);
    layout->addWidget(integerButton, 0, 0);
    layout->addWidget(integerLabel, 0, 1);
    layout->addWidget(doubleButton, 1, 0);
    layout->addWidget(doubleLabel, 1, 1);
    layout->addWidget(itemButton, 2, 0);
    layout->addWidget(itemLabel, 2, 1);
    layout->addWidget(textButton, 3, 0);
    layout->addWidget(textLabel, 3, 1);
    layout->addWidget(multiLineTextButton, 4, 0);
    layout->addWidget(multiLineTextLabel, 4, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding), 5, 0);
    toolbox->addItem(page, tr("Input Dialogs"));

    const QString doNotUseNativeDialog = tr("Do not use native dialog");

    page = new QWidget;
    layout = new QGridLayout(page);
    layout->setColumnStretch(1, 1);
    layout->addWidget(colorButton, 0, 0);
    layout->addWidget(colorLabel, 0, 1);
    colorDialogOptionsWidget = new DialogOptionsWidget;
    colorDialogOptionsWidget->addCheckBox(doNotUseNativeDialog, QColorDialog::DontUseNativeDialog);
    colorDialogOptionsWidget->addCheckBox(tr("Show alpha channel") , QColorDialog::ShowAlphaChannel);
    colorDialogOptionsWidget->addCheckBox(tr("No buttons") , QColorDialog::NoButtons);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding), 1, 0);
    layout->addWidget(colorDialogOptionsWidget, 2, 0, 1 ,2);

    toolbox->addItem(page, tr("Color Dialog"));

    page = new QWidget;
    layout = new QGridLayout(page);
    layout->setColumnStretch(1, 1);
    layout->addWidget(fontButton, 0, 0);
    layout->addWidget(fontLabel, 0, 1);
    fontDialogOptionsWidget = new DialogOptionsWidget;
    fontDialogOptionsWidget->addCheckBox(doNotUseNativeDialog, QFontDialog::DontUseNativeDialog);
    fontDialogOptionsWidget->addCheckBox(tr("No buttons") , QFontDialog::NoButtons);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding), 1, 0);
    layout->addWidget(fontDialogOptionsWidget, 2, 0, 1 ,2);
    toolbox->addItem(page, tr("Font Dialog"));

    page = new QWidget;
    layout = new QGridLayout(page);
    layout->setColumnStretch(1, 1);
    layout->addWidget(directoryButton, 0, 0);
    layout->addWidget(directoryLabel, 0, 1);
    layout->addWidget(openFileNameButton, 1, 0);
    layout->addWidget(openFileNameLabel, 1, 1);
    layout->addWidget(openFileNamesButton, 2, 0);
    layout->addWidget(openFileNamesLabel, 2, 1);
    layout->addWidget(saveFileNameButton, 3, 0);
    layout->addWidget(saveFileNameLabel, 3, 1);
    fileDialogOptionsWidget = new DialogOptionsWidget;
    fileDialogOptionsWidget->addCheckBox(doNotUseNativeDialog, QFileDialog::DontUseNativeDialog);
    fileDialogOptionsWidget->addCheckBox(tr("Show directories only"), QFileDialog::ShowDirsOnly);
    fileDialogOptionsWidget->addCheckBox(tr("Do not resolve symlinks"), QFileDialog::DontResolveSymlinks);
    fileDialogOptionsWidget->addCheckBox(tr("Do not confirm overwrite"), QFileDialog::DontConfirmOverwrite);
    fileDialogOptionsWidget->addCheckBox(tr("Do not use sheet"), QFileDialog::DontUseSheet);
    fileDialogOptionsWidget->addCheckBox(tr("Readonly"), QFileDialog::ReadOnly);
    fileDialogOptionsWidget->addCheckBox(tr("Hide name filter details"), QFileDialog::HideNameFilterDetails);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding), 4, 0);
    layout->addWidget(fileDialogOptionsWidget, 5, 0, 1 ,2);
    toolbox->addItem(page, tr("File Dialogs"));

    page = new QWidget;
    layout = new QGridLayout(page);
    layout->setColumnStretch(1, 1);
    layout->addWidget(criticalButton, 0, 0);
    layout->addWidget(criticalLabel, 0, 1);
    layout->addWidget(informationButton, 1, 0);
    layout->addWidget(informationLabel, 1, 1);
    layout->addWidget(questionButton, 2, 0);
    layout->addWidget(questionLabel, 2, 1);
    layout->addWidget(warningButton, 3, 0);
    layout->addWidget(warningLabel, 3, 1);
    layout->addWidget(errorButton, 4, 0);
    layout->addWidget(errorLabel, 4, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding), 5, 0);
    toolbox->addItem(page, tr("Message Boxes"));

    setWindowTitle(tr("Standard Dialogs"));
}

void Dialog::setInteger()
{
//! [0]
    bool ok;
    int i = QInputDialog::getInt(this, tr("QInputDialog::getInteger()"),
                                 tr("Percentage:"), 25, 0, 100, 1, &ok);
    if (ok)
        integerLabel->setText(tr("%1%").arg(i));
//! [0]
}

void Dialog::setDouble()
{
//! [1]
    bool ok;
    double d = QInputDialog::getDouble(this, tr("QInputDialog::getDouble()"),
                                       tr("Amount:"), 37.56, -10000, 10000, 2, &ok);
    if (ok)
        doubleLabel->setText(QString("$%1").arg(d));
//! [1]
}

void Dialog::setItem()
{
//! [2]
    QStringList items;
    items << tr("Spring") << tr("Summer") << tr("Fall") << tr("Winter");

    bool ok;
    QString item = QInputDialog::getItem(this, tr("QInputDialog::getItem()"),
                                         tr("Season:"), items, 0, false, &ok);
    if (ok && !item.isEmpty())
        itemLabel->setText(item);
//! [2]
}

void Dialog::setText()
{
//! [3]
    bool ok;
    QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                         tr("User name:"), QLineEdit::Normal,
                                         QDir::home().dirName(), &ok);
    if (ok && !text.isEmpty())
        textLabel->setText(text);
//! [3]
}

void Dialog::setMultiLineText()
{
//! [4]
    bool ok;
    QString text = QInputDialog::getMultiLineText(this, tr("QInputDialog::getMultiLineText()"),
                                                  tr("Address:"), "John Doe\nFreedom Street", &ok);
    if (ok && !text.isEmpty())
        multiLineTextLabel->setText(text);
//! [4]
}

void Dialog::setColor()
{
    const QColorDialog::ColorDialogOptions options = QFlag(colorDialogOptionsWidget->value());
    const QColor color = QColorDialog::getColor(Qt::green, this, "Select Color", options);

    if (color.isValid()) {
        colorLabel->setText(color.name());
        colorLabel->setPalette(QPalette(color));
        colorLabel->setAutoFillBackground(true);
    }
}

void Dialog::setFont()
{
    const QFontDialog::FontDialogOptions options = QFlag(fontDialogOptionsWidget->value());
    bool ok;
    QFont font = QFontDialog::getFont(&ok, QFont(fontLabel->text()), this, "Select Font", options);
    if (ok) {
        fontLabel->setText(font.key());
        fontLabel->setFont(font);
    }
}

void Dialog::setExistingDirectory()
{
    QFileDialog::Options options = QFlag(fileDialogOptionsWidget->value());
    options |= QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
    QString directory = QFileDialog::getExistingDirectory(this,
                                tr("QFileDialog::getExistingDirectory()"),
                                directoryLabel->text(),
                                options);
    if (!directory.isEmpty())
        directoryLabel->setText(directory);
}

void Dialog::setOpenFileName()
{
    const QFileDialog::Options options = QFlag(fileDialogOptionsWidget->value());
    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(this,
                                tr("QFileDialog::getOpenFileName()"),
                                openFileNameLabel->text(),
                                tr("All Files (*);;Text Files (*.txt)"),
                                &selectedFilter,
                                options);
    if (!fileName.isEmpty())
        openFileNameLabel->setText(fileName);
}

void Dialog::setOpenFileNames()
{
    const QFileDialog::Options options = QFlag(fileDialogOptionsWidget->value());
    QString selectedFilter;
    QStringList files = QFileDialog::getOpenFileNames(
                                this, tr("QFileDialog::getOpenFileNames()"),
                                openFilesPath,
                                tr("All Files (*);;Text Files (*.txt)"),
                                &selectedFilter,
                                options);
    if (files.count()) {
        openFilesPath = files[0];
        openFileNamesLabel->setText(QString("[%1]").arg(files.join(", ")));
    }
}

void Dialog::setSaveFileName()
{
    const QFileDialog::Options options = QFlag(fileDialogOptionsWidget->value());
    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(this,
                                tr("QFileDialog::getSaveFileName()"),
                                saveFileNameLabel->text(),
                                tr("All Files (*);;Text Files (*.txt)"),
                                &selectedFilter,
                                options);
    if (!fileName.isEmpty())
        saveFileNameLabel->setText(fileName);
}

void Dialog::criticalMessage()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::critical(this, tr("QMessageBox::critical()"),
                                    MESSAGE,
                                    QMessageBox::Abort | QMessageBox::Retry | QMessageBox::Ignore);
    if (reply == QMessageBox::Abort)
        criticalLabel->setText(tr("Abort"));
    else if (reply == QMessageBox::Retry)
        criticalLabel->setText(tr("Retry"));
    else
        criticalLabel->setText(tr("Ignore"));
}

void Dialog::informationMessage()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::information(this, tr("QMessageBox::information()"), MESSAGE);
    if (reply == QMessageBox::Ok)
        informationLabel->setText(tr("OK"));
    else
        informationLabel->setText(tr("Escape"));
}

void Dialog::questionMessage()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("QMessageBox::question()"),
                                    MESSAGE,
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (reply == QMessageBox::Yes)
        questionLabel->setText(tr("Yes"));
    else if (reply == QMessageBox::No)
        questionLabel->setText(tr("No"));
    else
        questionLabel->setText(tr("Cancel"));
}

void Dialog::warningMessage()
{
    QMessageBox msgBox(QMessageBox::Warning, tr("QMessageBox::warning()"),
                       MESSAGE, 0, this);
    msgBox.setDetailedText(MESSAGE_DETAILS);
    msgBox.addButton(tr("Save &Again"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("&Continue"), QMessageBox::RejectRole);
    if (msgBox.exec() == QMessageBox::AcceptRole)
        warningLabel->setText(tr("Save Again"));
    else
        warningLabel->setText(tr("Continue"));

}

void Dialog::errorMessage()
{
    errorMessageDialog->showMessage(
            tr("This dialog shows and remembers error messages. "
               "If the checkbox is checked (as it is by default), "
               "the shown message will be shown again, "
               "but if the user unchecks the box the message "
               "will not appear again if QErrorMessage::showMessage() "
               "is called with the same message."));
    errorLabel->setText(tr("If the box is unchecked, the message "
                           "won't appear again."));
}
