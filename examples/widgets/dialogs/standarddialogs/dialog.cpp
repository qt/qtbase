// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "dialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QErrorMessage>
#include <QFileDialog>
#include <QFontDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QToolBox>

#include <QGuiApplication>
#include <QStyleHints>

using namespace Qt::StringLiterals;

class DialogOptionsWidget : public QGroupBox
{
public:
    explicit DialogOptionsWidget(QWidget *parent = nullptr);

    void addCheckBox(const QString &text, int value);
    void addSpacer();
    int value() const;

private:
    using CheckBoxEntry = QPair<QCheckBox *, int>;
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
    auto *checkBox = new QCheckBox(text);
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
    for (const CheckBoxEntry &checkboxEntry : std::as_const(checkBoxEntries)) {
        if (checkboxEntry.first->isChecked())
            result |= checkboxEntry.second;
    }
    return result;
}

Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *verticalLayout{};
    if (QGuiApplication::styleHints()->showIsFullScreen() || QGuiApplication::styleHints()->showIsMaximized()) {
        auto *horizontalLayout = new QHBoxLayout(this);
        auto *groupBox = new QGroupBox(QGuiApplication::applicationDisplayName(), this);
        horizontalLayout->addWidget(groupBox);
        verticalLayout = new QVBoxLayout(groupBox);
    } else {
        verticalLayout = new QVBoxLayout(this);
    }

    auto *toolbox = new QToolBox;
    verticalLayout->addWidget(toolbox);

    errorMessageDialog = new QErrorMessage(this);

    int frameStyle = QFrame::Sunken | QFrame::Panel;

    integerLabel = new QLabel;
    integerLabel->setFrameStyle(frameStyle);
    auto *integerButton = new QPushButton(tr("QInputDialog::get&Int()"));

    doubleLabel = new QLabel;
    doubleLabel->setFrameStyle(frameStyle);
    auto *doubleButton = new QPushButton(tr("QInputDialog::get&Double()"));

    itemLabel = new QLabel;
    itemLabel->setFrameStyle(frameStyle);
    auto *itemButton = new QPushButton(tr("QInputDialog::getIte&m()"));

    textLabel = new QLabel;
    textLabel->setFrameStyle(frameStyle);
    auto *textButton = new QPushButton(tr("QInputDialog::get&Text()"));

    multiLineTextLabel = new QLabel;
    multiLineTextLabel->setFrameStyle(frameStyle);
    auto *multiLineTextButton = new QPushButton(tr("QInputDialog::get&MultiLineText()"));

    colorLabel = new QLabel;
    colorLabel->setFrameStyle(frameStyle);
    auto *colorButton = new QPushButton(tr("QColorDialog::get&Color()"));

    fontLabel = new QLabel;
    fontLabel->setFrameStyle(frameStyle);
    auto *fontButton = new QPushButton(tr("QFontDialog::get&Font()"));

    directoryLabel = new QLabel;
    directoryLabel->setFrameStyle(frameStyle);
    auto *directoryButton = new QPushButton(tr("QFileDialog::getE&xistingDirectory()"));

    openFileNameLabel = new QLabel;
    openFileNameLabel->setFrameStyle(frameStyle);
    auto *openFileNameButton = new QPushButton(tr("QFileDialog::get&OpenFileName()"));

    openFileNamesLabel = new QLabel;
    openFileNamesLabel->setFrameStyle(frameStyle);
    auto *openFileNamesButton = new QPushButton(tr("QFileDialog::&getOpenFileNames()"));

    saveFileNameLabel = new QLabel;
    saveFileNameLabel->setFrameStyle(frameStyle);
    auto *saveFileNameButton = new QPushButton(tr("QFileDialog::get&SaveFileName()"));

    criticalLabel = new QLabel;
    criticalLabel->setFrameStyle(frameStyle);
    auto *criticalButton = new QPushButton(tr("QMessageBox::critica&l()"));

    informationLabel = new QLabel;
    informationLabel->setFrameStyle(frameStyle);
    QPushButton *informationButton =
            new QPushButton(tr("QMessageBox::i&nformation()"));

    questionLabel = new QLabel;
    questionLabel->setFrameStyle(frameStyle);
    auto *questionButton = new QPushButton(tr("QMessageBox::&question()"));

    warningLabel = new QLabel;
    warningLabel->setFrameStyle(frameStyle);
    auto *warningButton = new QPushButton(tr("QMessageBox::&warning()"));

    auto *errorButton = new QPushButton(tr("QErrorMessage::showM&essage()"));

    connect(integerButton, &QAbstractButton::clicked, this, &Dialog::setInteger);
    connect(doubleButton, &QAbstractButton::clicked, this, &Dialog::setDouble);
    connect(itemButton, &QAbstractButton::clicked, this, &Dialog::setItem);
    connect(textButton, &QAbstractButton::clicked, this, &Dialog::setText);
    connect(multiLineTextButton, &QAbstractButton::clicked, this, &Dialog::setMultiLineText);
    connect(colorButton, &QAbstractButton::clicked, this, &Dialog::setColor);
    connect(fontButton, &QAbstractButton::clicked, this, &Dialog::setFont);
    connect(directoryButton, &QAbstractButton::clicked,
            this, &Dialog::setExistingDirectory);
    connect(openFileNameButton, &QAbstractButton::clicked,
            this, &Dialog::setOpenFileName);
    connect(openFileNamesButton, &QAbstractButton::clicked,
            this, &Dialog::setOpenFileNames);
    connect(saveFileNameButton, &QAbstractButton::clicked,
            this, &Dialog::setSaveFileName);
    connect(criticalButton, &QAbstractButton::clicked, this, &Dialog::criticalMessage);
    connect(informationButton, &QAbstractButton::clicked,
            this, &Dialog::informationMessage);
    connect(questionButton, &QAbstractButton::clicked, this, &Dialog::questionMessage);
    connect(warningButton, &QAbstractButton::clicked, this, &Dialog::warningMessage);
    connect(errorButton, &QAbstractButton::clicked, this, &Dialog::errorMessage);

    auto *page = new QWidget;
    auto *layout = new QGridLayout(page);
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
    colorDialogOptionsWidget->addCheckBox(tr("Hide Color Picker") , QColorDialog::NoEyeDropperButton);
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
    fontDialogOptionsWidget->addCheckBox(tr("Show scalable fonts"), QFontDialog::ScalableFonts);
    fontDialogOptionsWidget->addCheckBox(tr("Show non scalable fonts"), QFontDialog::NonScalableFonts);
    fontDialogOptionsWidget->addCheckBox(tr("Show monospaced fonts"), QFontDialog::MonospacedFonts);
    fontDialogOptionsWidget->addCheckBox(tr("Show proportional fonts"), QFontDialog::ProportionalFonts);
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
    fileDialogOptionsWidget->addCheckBox(tr("Readonly"), QFileDialog::ReadOnly);
    fileDialogOptionsWidget->addCheckBox(tr("Hide name filter details"), QFileDialog::HideNameFilterDetails);
    fileDialogOptionsWidget->addCheckBox(tr("Do not use custom directory icons (Windows)"), QFileDialog::DontUseCustomDirectoryIcons);
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
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding), 5, 0);
    toolbox->addItem(page, tr("Message Boxes"));

    setWindowTitle(QGuiApplication::applicationDisplayName());
}

void Dialog::setInteger()
{
//! [0]
    bool ok;
    int i = QInputDialog::getInt(this, tr("QInputDialog::getInt()"),
                                 tr("Percentage:"), 25, 0, 100, 1, &ok);
    if (ok)
        integerLabel->setText(tr("%1%").arg(i));
//! [0]
}

void Dialog::setDouble()
{
//! [1]
    bool ok{};
    double d = QInputDialog::getDouble(this, tr("QInputDialog::getDouble()"),
                                       tr("Amount:"), 37.56, -10000, 10000, 2, &ok,
                                       Qt::WindowFlags(), 1);
    if (ok)
        doubleLabel->setText(QStringLiteral("$%1").arg(d));
//! [1]
}

void Dialog::setItem()
{
//! [2]
    const QStringList items{tr("Spring"), tr("Summer"), tr("Fall"), tr("Winter")};

    bool ok{};
    QString item = QInputDialog::getItem(this, tr("QInputDialog::getItem()"),
                                         tr("Season:"), items, 0, false, &ok);
    if (ok && !item.isEmpty())
        itemLabel->setText(item);
//! [2]
}

void Dialog::setText()
{
//! [3]
    bool ok{};
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
    bool ok{};
    QString text = QInputDialog::getMultiLineText(this, tr("QInputDialog::getMultiLineText()"),
                                                  tr("Address:"), "John Doe\nFreedom Street"_L1, &ok);
    if (ok && !text.isEmpty())
        multiLineTextLabel->setText(text);
//! [4]
}

void Dialog::setColor()
{
    const QColorDialog::ColorDialogOptions options = QFlag(colorDialogOptionsWidget->value());
    const QColor color = QColorDialog::getColor(Qt::green, this, tr("Select Color"), options);

    if (color.isValid()) {
        colorLabel->setText(color.name());
        colorLabel->setPalette(QPalette(color));
        colorLabel->setAutoFillBackground(true);
    }
}

void Dialog::setFont()
{
    const QFontDialog::FontDialogOptions options = QFlag(fontDialogOptionsWidget->value());

    const QString &description = fontLabel->text();
    QFont defaultFont;
    if (!description.isEmpty())
        defaultFont.fromString(description);

    bool ok{};
    QFont font = QFontDialog::getFont(&ok, defaultFont, this, tr("Select Font"), options);
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
    if (!files.isEmpty()) {
        openFilesPath = files.constFirst();
        openFileNamesLabel->setText(u'[' + files.join(", "_L1) + u']');
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
    QMessageBox msgBox(QMessageBox::Critical, tr("QMessageBox::critical()"),
                              tr("Houston, we have a problem"), { }, this);
    msgBox.setInformativeText(tr("Activating the liquid oxygen stirring fans caused an explosion in one of the tanks. " \
                                 "Liquid oxygen levels are getting low. This may jeopardize the moon landing mission."));
    msgBox.addButton(QMessageBox::Abort);
    msgBox.addButton(QMessageBox::Retry);
    msgBox.addButton(QMessageBox::Ignore);
    int reply = msgBox.exec();
    if (reply == QMessageBox::Abort)
        criticalLabel->setText(tr("Abort"));
    else if (reply == QMessageBox::Retry)
        criticalLabel->setText(tr("Retry"));
    else
        criticalLabel->setText(tr("Ignore"));
}

void Dialog::informationMessage()
{
    QMessageBox msgBox(QMessageBox::Information, tr("QMessageBox::information()"),
                              tr("Elvis has left the building."), { }, this);
    msgBox.setInformativeText(tr("This phrase was often used by public address announcers at the conclusion " \
                                 "of Elvis Presley concerts in order to disperse audiences who lingered in " \
                                 "hopes of an encore. It has since become a catchphrase and punchline."));
    if (msgBox.exec() == QMessageBox::Ok)
        informationLabel->setText(tr("OK"));
    else
        informationLabel->setText(tr("Escape"));
}

void Dialog::questionMessage()
{
    QMessageBox msgBox(QMessageBox::Question, tr("QMessageBox::question()"),
                              tr("Would you like cheese with that?"), { }, this);
    msgBox.setInformativeText(tr("A cheeseburger is a hamburger topped with cheese. Traditionally, the slice of " \
                                 "cheese is placed on top of the meat patty. The cheese is usually added to the " \
                                 "cooking hamburger patty shortly before serving, which allows the cheese to melt."));
    msgBox.addButton(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    msgBox.addButton(QMessageBox::Cancel);
    int reply = msgBox.exec();
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
                              tr("Delete the only copy of your movie manuscript?"), { }, this);
    msgBox.setInformativeText(tr("You've been working on this manuscript for 738 days now. Hang in there!"));
    msgBox.setDetailedText("\"A long time ago in a galaxy far, far away....\""_L1);
    auto *keepButton = msgBox.addButton(tr("&Keep"), QMessageBox::AcceptRole);
    auto *deleteButton = msgBox.addButton(tr("Delete"), QMessageBox::DestructiveRole);
    msgBox.exec();
    if (msgBox.clickedButton() == keepButton)
        warningLabel->setText(tr("Keep"));
    else if (msgBox.clickedButton() == deleteButton)
        warningLabel->setText(tr("Delete"));
    else
        warningLabel->setText({});
}

void Dialog::errorMessage()
{
    errorMessageDialog->showMessage(
            tr("This dialog shows and remembers error messages. "
               "If the user chooses to not show the dialog again, the dialog "
               "will not appear again if QErrorMessage::showMessage() "
               "is called with the same message."));
    errorMessageDialog->showMessage(
            tr("You can queue up error messages, and they will be "
               "shown one after each other. Each message maintains "
               "its own state for whether it will be shown again "
               "the next time QErrorMessage::showMessage() is called "
               "with the same message."));
}
