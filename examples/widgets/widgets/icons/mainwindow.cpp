/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

#include "iconpreviewarea.h"
#include "iconsizespinbox.h"
#include "imagedelegate.h"
#include "mainwindow.h"

//! [40]
enum { OtherSize = QStyle::PM_CustomBase };
//! [40]

//! [0]
MainWindow::MainWindow()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    createActions();

    QGridLayout *mainLayout = new QGridLayout(centralWidget);

    QGroupBox *previewGroupBox = new QGroupBox(tr("Preview"));
    previewArea = new IconPreviewArea(previewGroupBox);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroupBox);
    previewLayout->addWidget(previewArea);

    mainLayout->addWidget(previewGroupBox, 0, 0, 1, 2);
    mainLayout->addWidget(createImagesGroupBox(), 1, 0);
    QVBoxLayout *vBox = new QVBoxLayout;
    vBox->addWidget(createIconSizeGroupBox());
    vBox->addWidget(createHighDpiIconSizeGroupBox());
    vBox->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    mainLayout->addLayout(vBox, 1, 1);
    createContextMenu();

    setWindowTitle(tr("Icons"));
    checkCurrentStyle();
    sizeButtonGroup->button(OtherSize)->click();
}
//! [0]

//! [44]
void MainWindow::show()
{
    QMainWindow::show();
    connect(windowHandle(), &QWindow::screenChanged, this, &MainWindow::screenChanged);
    screenChanged();
}
//! [44]

//! [1]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About Icons"),
            tr("The <b>Icons</b> example illustrates how Qt renders an icon in "
               "different modes (active, normal, disabled, and selected) and "
               "states (on and off) based on a set of images."));
}
//! [1]

//! [2]
void MainWindow::changeStyle(bool checked)
{
    if (!checked)
        return;

    const QAction *action = qobject_cast<QAction *>(sender());
//! [2] //! [3]
    QStyle *style = QStyleFactory::create(action->data().toString());
//! [3] //! [4]
    Q_ASSERT(style);
    QApplication::setStyle(style);

    foreach (QAbstractButton *button, sizeButtonGroup->buttons()) {
        const QStyle::PixelMetric metric = static_cast<QStyle::PixelMetric>(sizeButtonGroup->id(button));
        const int value = style->pixelMetric(metric);
        switch (metric) {
        case QStyle::PM_SmallIconSize:
            button->setText(tr("Small (%1 x %1)").arg(value));
            break;
        case QStyle::PM_LargeIconSize:
            button->setText(tr("Large (%1 x %1)").arg(value));
            break;
        case QStyle::PM_ToolBarIconSize:
            button->setText(tr("Toolbars (%1 x %1)").arg(value));
            break;
        case QStyle::PM_ListViewIconSize:
            button->setText(tr("List views (%1 x %1)").arg(value));
            break;
        case QStyle::PM_IconViewIconSize:
            button->setText(tr("Icon views (%1 x %1)").arg(value));
            break;
        case QStyle::PM_TabBarIconSize:
            button->setText(tr("Tab bars (%1 x %1)").arg(value));
            break;
        default:
            break;
        }
    }

    triggerChangeSize();
}
//! [4]

//! [5]
void MainWindow::changeSize(int id, bool checked)
{
    if (!checked)
        return;

    const bool other = id == int(OtherSize);
    const int extent = other
        ? otherSpinBox->value()
        : QApplication::style()->pixelMetric(static_cast<QStyle::PixelMetric>(id));

    previewArea->setSize(QSize(extent, extent));
    otherSpinBox->setEnabled(other);
}

void MainWindow::triggerChangeSize()
{
    changeSize(sizeButtonGroup->checkedId(), true);
}
//! [5]

//! [6]
void MainWindow::changeIcon()
{
    QIcon icon;

    for (int row = 0; row < imagesTable->rowCount(); ++row) {
        const QTableWidgetItem *fileItem = imagesTable->item(row, 0);
        const QTableWidgetItem *modeItem = imagesTable->item(row, 1);
        const QTableWidgetItem *stateItem = imagesTable->item(row, 2);

        if (fileItem->checkState() == Qt::Checked) {
            const int modeIndex = IconPreviewArea::iconModeNames().indexOf(modeItem->text());
            Q_ASSERT(modeIndex >= 0);
            const int stateIndex = IconPreviewArea::iconStateNames().indexOf(stateItem->text());
            Q_ASSERT(stateIndex >= 0);
            const QIcon::Mode mode = IconPreviewArea::iconModes().at(modeIndex);
            const QIcon::State state = IconPreviewArea::iconStates().at(stateIndex);
//! [6]

//! [8]
            const QString fileName = fileItem->data(Qt::UserRole).toString();
            QImage image(fileName);
            if (!image.isNull())
                icon.addPixmap(QPixmap::fromImage(image), mode, state);
//! [8] //! [9]
        }
//! [9] //! [10]
    }
//! [10]

//! [11]
    previewArea->setIcon(icon);
}
//! [11]

void MainWindow::addSampleImages()
{
    addImages(QLatin1String(SRCDIR) + QLatin1String("/images"));
}

void MainWindow::addOtherImages()
{
    static bool firstInvocation = true;
    QString directory;
    if (firstInvocation) {
        firstInvocation = false;
        directory = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0, QString());
    }
    addImages(directory);
}

//! [12]
void MainWindow::addImages(const QString &directory)
{
    QFileDialog fileDialog(this, tr("Open Images"), directory);
    QStringList mimeTypeFilters;
    foreach (const QByteArray &mimeTypeName, QImageReader::supportedMimeTypes())
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    fileDialog.setMimeTypeFilters(mimeTypeFilters);
    fileDialog.selectMimeTypeFilter(QLatin1String("image/png"));
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    if (!nativeFileDialogAct->isChecked())
        fileDialog.setOption(QFileDialog::DontUseNativeDialog);
    if (fileDialog.exec() == QDialog::Accepted)
        loadImages(fileDialog.selectedFiles());
//! [12]
}

void MainWindow::loadImages(const QStringList &fileNames)
{
    foreach (const QString &fileName, fileNames) {
        const int row = imagesTable->rowCount();
        imagesTable->setRowCount(row + 1);
//! [13]
        const QFileInfo fileInfo(fileName);
        const QString imageName = fileInfo.baseName();
        const QString fileName2x = fileInfo.absolutePath()
            + QLatin1Char('/') + imageName + QLatin1String("@2x.") + fileInfo.suffix();
        const QFileInfo fileInfo2x(fileName2x);
        const QImage image(fileName);
        const QString toolTip =
            tr("Directory: %1\nFile: %2\nFile@2x: %3\nSize: %4x%5")
               .arg(QDir::toNativeSeparators(fileInfo.absolutePath()), fileInfo.fileName())
               .arg(fileInfo2x.exists() ? fileInfo2x.fileName() : tr("<None>"))
               .arg(image.width()).arg(image.height());
//! [13] //! [14]
        QTableWidgetItem *fileItem = new QTableWidgetItem(imageName);
        fileItem->setData(Qt::UserRole, fileName);
        fileItem->setIcon(QPixmap::fromImage(image));
        fileItem->setFlags((fileItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        fileItem->setToolTip(toolTip);
//! [14]

//! [15]
        QIcon::Mode mode = QIcon::Normal;
//! [15] //! [16]
        QIcon::State state = QIcon::Off;
        if (guessModeStateAct->isChecked()) {
            if (imageName.contains(QLatin1String("_act"), Qt::CaseInsensitive))
                mode = QIcon::Active;
            else if (imageName.contains(QLatin1String("_dis"), Qt::CaseInsensitive))
                mode = QIcon::Disabled;
            else if (imageName.contains(QLatin1String("_sel"), Qt::CaseInsensitive))
                mode = QIcon::Selected;

            if (imageName.contains(QLatin1String("_on"), Qt::CaseInsensitive))
                state = QIcon::On;
//! [16] //! [17]
        }
//! [17]

//! [18]
        imagesTable->setItem(row, 0, fileItem);
//! [18] //! [19]
        QTableWidgetItem *modeItem =
            new QTableWidgetItem(IconPreviewArea::iconModeNames().at(IconPreviewArea::iconModes().indexOf(mode)));
        modeItem->setToolTip(toolTip);
        imagesTable->setItem(row, 1, modeItem);
        QTableWidgetItem *stateItem =
            new QTableWidgetItem(IconPreviewArea::iconStateNames().at(IconPreviewArea::iconStates().indexOf(state)));
        stateItem->setToolTip(toolTip);
        imagesTable->setItem(row, 2, stateItem);
        imagesTable->openPersistentEditor(modeItem);
        imagesTable->openPersistentEditor(stateItem);

        fileItem->setCheckState(Qt::Checked);
    }
}
//! [19]

void MainWindow::useHighDpiPixmapsChanged(int checkState)
{
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, checkState == Qt::Checked);
    changeIcon();
}

//! [20]
void MainWindow::removeAllImages()
{
    imagesTable->setRowCount(0);
    changeIcon();
}
//! [20]

//! [21]
QWidget *MainWindow::createImagesGroupBox()
{
    QGroupBox *imagesGroupBox = new QGroupBox(tr("Images"));

    imagesTable = new QTableWidget;
    imagesTable->setSelectionMode(QAbstractItemView::NoSelection);
    imagesTable->setItemDelegate(new ImageDelegate(this));
//! [21]

//! [22]
    QStringList labels;
//! [22] //! [23]
    labels << tr("Image") << tr("Mode") << tr("State");

    imagesTable->horizontalHeader()->setDefaultSectionSize(90);
    imagesTable->setColumnCount(3);
    imagesTable->setHorizontalHeaderLabels(labels);
    imagesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    imagesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    imagesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    imagesTable->verticalHeader()->hide();
//! [23]

//! [24]
    connect(imagesTable, &QTableWidget::itemChanged,
//! [24] //! [25]
            this, &MainWindow::changeIcon);

    QVBoxLayout *layout = new QVBoxLayout(imagesGroupBox);
    layout->addWidget(imagesTable);
    return imagesGroupBox;
}
//! [25]

//! [26]
QWidget *MainWindow::createIconSizeGroupBox()
{
    QGroupBox *iconSizeGroupBox = new QGroupBox(tr("Icon Size"));

    sizeButtonGroup = new QButtonGroup(this);
    sizeButtonGroup->setExclusive(true);

    typedef void (QButtonGroup::*QButtonGroupIntBoolSignal)(int, bool);
    connect(sizeButtonGroup, static_cast<QButtonGroupIntBoolSignal>(&QButtonGroup::buttonToggled),
            this, &MainWindow::changeSize);

    QRadioButton *smallRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(smallRadioButton, QStyle::PM_SmallIconSize);
    QRadioButton *largeRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(largeRadioButton, QStyle::PM_LargeIconSize);
    QRadioButton *toolBarRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(toolBarRadioButton, QStyle::PM_ToolBarIconSize);
    QRadioButton *listViewRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(listViewRadioButton, QStyle::PM_ListViewIconSize);
    QRadioButton *iconViewRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(iconViewRadioButton, QStyle::PM_IconViewIconSize);
    QRadioButton *tabBarRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(tabBarRadioButton, QStyle::PM_TabBarIconSize);
    QRadioButton *otherRadioButton = new QRadioButton(tr("Other:"));
    sizeButtonGroup->addButton(otherRadioButton, OtherSize);
    otherSpinBox = new IconSizeSpinBox;
    otherSpinBox->setRange(8, 256);
    const QString spinBoxToolTip =
        tr("Enter a custom size within %1..%2")
           .arg(otherSpinBox->minimum()).arg(otherSpinBox->maximum());
    otherSpinBox->setValue(64);
    otherSpinBox->setToolTip(spinBoxToolTip);
    otherRadioButton->setToolTip(spinBoxToolTip);
//! [26]

//! [27]
    typedef void (QSpinBox::*QSpinBoxIntSignal)(int);
    connect(otherSpinBox, static_cast<QSpinBoxIntSignal>(&QSpinBox::valueChanged),
            this, &MainWindow::triggerChangeSize);

    QHBoxLayout *otherSizeLayout = new QHBoxLayout;
    otherSizeLayout->addWidget(otherRadioButton);
    otherSizeLayout->addWidget(otherSpinBox);
    otherSizeLayout->addStretch();

    QGridLayout *layout = new QGridLayout(iconSizeGroupBox);
    layout->addWidget(smallRadioButton, 0, 0);
    layout->addWidget(largeRadioButton, 1, 0);
    layout->addWidget(toolBarRadioButton, 2, 0);
    layout->addWidget(listViewRadioButton, 0, 1);
    layout->addWidget(iconViewRadioButton, 1, 1);
    layout->addWidget(tabBarRadioButton, 2, 1);
    layout->addLayout(otherSizeLayout, 3, 0, 1, 2);
    layout->setRowStretch(4, 1);
    return iconSizeGroupBox;
}
//! [27]

void MainWindow::screenChanged()
{
    devicePixelRatioLabel->setText(QString::number(devicePixelRatioF()));
    if (const QWindow *window = windowHandle()) {
        const QScreen *screen = window->screen();
        const QString screenDescription =
            tr("\"%1\" (%2x%3)").arg(screen->name())
               .arg(screen->geometry().width()).arg(screen->geometry().height());
        screenNameLabel->setText(screenDescription);
    }
    changeIcon();
}

QWidget *MainWindow::createHighDpiIconSizeGroupBox()
{
    QGroupBox *highDpiGroupBox = new QGroupBox(tr("High DPI Scaling"));
    QFormLayout *layout = new QFormLayout(highDpiGroupBox);
    devicePixelRatioLabel = new QLabel(highDpiGroupBox);
    screenNameLabel = new QLabel(highDpiGroupBox);
    layout->addRow(tr("Screen:"), screenNameLabel);
    layout->addRow(tr("Device pixel ratio:"), devicePixelRatioLabel);
    QCheckBox *highDpiPixmapsCheckBox = new QCheckBox(QLatin1String("Qt::AA_UseHighDpiPixmaps"));
    highDpiPixmapsCheckBox->setChecked(QCoreApplication::testAttribute(Qt::AA_UseHighDpiPixmaps));
    connect(highDpiPixmapsCheckBox, &QCheckBox::stateChanged, this, &MainWindow::useHighDpiPixmapsChanged);
    layout->addRow(highDpiPixmapsCheckBox);
    return highDpiGroupBox;
}

//! [28]
void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    addSampleImagesAct = new QAction(tr("Add &Sample Images..."), this);
    addSampleImagesAct->setShortcut(tr("Ctrl+A"));
    connect(addSampleImagesAct, &QAction::triggered, this, &MainWindow::addSampleImages);
    fileMenu->addAction(addSampleImagesAct);

    addOtherImagesAct = new QAction(tr("&Add Images..."), this);
    addOtherImagesAct->setShortcut(QKeySequence::Open);
    connect(addOtherImagesAct, &QAction::triggered, this, &MainWindow::addOtherImages);
    fileMenu->addAction(addOtherImagesAct);

    removeAllImagesAct = new QAction(tr("&Remove All Images"), this);
    removeAllImagesAct->setShortcut(tr("Ctrl+R"));
    connect(removeAllImagesAct, &QAction::triggered,
            this, &MainWindow::removeAllImages);
    fileMenu->addAction(removeAllImagesAct);

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("&Quit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    styleActionGroup = new QActionGroup(this);
    foreach (const QString &styleName, QStyleFactory::keys()) {
        QAction *action = new QAction(tr("%1 Style").arg(styleName), styleActionGroup);
        action->setData(styleName);
        action->setCheckable(true);
        connect(action, &QAction::triggered, this, &MainWindow::changeStyle);
        viewMenu->addAction(action);
    }

    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));

    guessModeStateAct = new QAction(tr("&Guess Image Mode/State"), this);
    guessModeStateAct->setCheckable(true);
    guessModeStateAct->setChecked(true);
    settingsMenu->addAction(guessModeStateAct);

    nativeFileDialogAct = new QAction(tr("&Use Native File Dialog"), this);
    nativeFileDialogAct->setCheckable(true);
    nativeFileDialogAct->setChecked(true);
    settingsMenu->addAction(nativeFileDialogAct);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}
//! [28]

//! [30]
void MainWindow::createContextMenu()
{
    imagesTable->setContextMenuPolicy(Qt::ActionsContextMenu);
    imagesTable->addAction(addSampleImagesAct);
    imagesTable->addAction(addOtherImagesAct);
    imagesTable->addAction(removeAllImagesAct);
}
//! [30]

//! [31]
void MainWindow::checkCurrentStyle()
{
    foreach (QAction *action, styleActionGroup->actions()) {
        QString styleName = action->data().toString();
        QScopedPointer<QStyle> candidate(QStyleFactory::create(styleName));
        Q_ASSERT(!candidate.isNull());
        if (candidate->metaObject()->className()
                == QApplication::style()->metaObject()->className()) {
            action->trigger();
            return;
        }
    }
}
//! [31]
