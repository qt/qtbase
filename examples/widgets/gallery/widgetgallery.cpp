/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

#include "widgetgallery.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDateTimeEdit>
#include <QDial>
#include <QDialogButtonBox>
#include <QFileSystemModel>
#include <QGridLayout>
#include <QGroupBox>
#include <QMenu>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QShortcut>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStyle>
#include <QStyleFactory>
#include <QTextBrowser>
#include <QTreeView>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolBox>
#include <QToolButton>

#include <QIcon>
#include <QDesktopServices>
#include <QScreen>
#include <QWindow>

#include <QDebug>
#include <QLibraryInfo>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>

static inline QString className(const QObject *o)
{
     return QString::fromUtf8(o->metaObject()->className());
}

static inline void setClassNameToolTip(QWidget *w)
{
    w->setToolTip(className(w));
}

static QString helpUrl(const QString &page)
{
    QString result;
    QTextStream(&result) << "https://doc.qt.io/qt-" << QT_VERSION_MAJOR
        << '/' << page << ".html";
    return result;
}

static inline QString helpUrl(const QWidget *w)
{
    return helpUrl(className(w).toLower());
}

static void launchHelp(const QWidget *w)
{
     QDesktopServices::openUrl(helpUrl(w));
}

static void launchModuleHelp()
{
    QDesktopServices::openUrl(helpUrl(QLatin1String("qtwidgets-index")));
}

template <class Widget>
Widget *createWidget(const char *name, QWidget *parent = nullptr)
{
    auto result = new Widget(parent);
    result->setObjectName(QLatin1String(name));
    setClassNameToolTip(result);
    return result;
}

template <class Widget, class Parameter>
Widget *createWidget1(const Parameter &p1, const char *name, QWidget *parent = nullptr)
{
    auto result = new Widget(p1, parent);
    result->setObjectName(QLatin1String(name));
    setClassNameToolTip(result);
    return result;
}

QTextStream &operator<<(QTextStream &str, const QRect &r)
{
    str << r.width() << 'x' << r.height() << Qt::forcesign << r.x() << r.y()
        << Qt::noforcesign;
    return str;
}

static QString highDpiScaleFactorRoundingPolicy()
{
    QString result;
    QDebug(&result) << QGuiApplication::highDpiScaleFactorRoundingPolicy();
    if (result.endsWith(QLatin1Char(')')))
        result.chop(1);
    const int lastSep = result.lastIndexOf(QLatin1String("::"));
    if (lastSep != -1)
        result.remove(0, lastSep + 2);
    return result;
}

WidgetGallery::WidgetGallery(QWidget *parent)
    : QDialog(parent)
    , progressBar(createProgressBar())
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto styleComboBox = createWidget<QComboBox>("styleComboBox");
    const QString defaultStyleName = QApplication::style()->objectName();
    QStringList styleNames = QStyleFactory::keys();
    for (int i = 1, size = styleNames.size(); i < size; ++i) {
        if (defaultStyleName.compare(styleNames.at(i), Qt::CaseInsensitive) == 0) {
            styleNames.swapItemsAt(0, i);
            break;
        }
    }
    styleComboBox->addItems(styleNames);

    auto styleLabel = createWidget1<QLabel>(tr("&Style:"), "styleLabel");
    styleLabel->setBuddy(styleComboBox);

    auto helpLabel = createWidget1<QLabel>(tr("Press F1 over a widget to see Documentation"), "helpLabel");

    auto disableWidgetsCheckBox = createWidget1<QCheckBox>(tr("&Disable widgets"), "disableWidgetsCheckBox");

    auto buttonsGroupBox = createButtonsGroupBox();
    auto itemViewTabWidget = createItemViewTabWidget();
    auto simpleInputWidgetsGroupBox = createSimpleInputWidgetsGroupBox();
    auto textToolBox = createTextToolBox();

    connect(styleComboBox, &QComboBox::textActivated,
            this, &WidgetGallery::changeStyle);
    connect(disableWidgetsCheckBox, &QCheckBox::toggled,
            buttonsGroupBox, &QWidget::setDisabled);
    connect(disableWidgetsCheckBox, &QCheckBox::toggled,
            textToolBox, &QWidget::setDisabled);
    connect(disableWidgetsCheckBox, &QCheckBox::toggled,
            itemViewTabWidget, &QWidget::setDisabled);
    connect(disableWidgetsCheckBox, &QCheckBox::toggled,
            simpleInputWidgetsGroupBox, &QWidget::setDisabled);

    auto topLayout = new QHBoxLayout;
    topLayout->addWidget(styleLabel);
    topLayout->addWidget(styleComboBox);
    topLayout->addStretch(1);
    topLayout->addWidget(helpLabel);
    topLayout->addStretch(1);
    topLayout->addWidget(disableWidgetsCheckBox);

    auto dialogButtonBox = createWidget1<QDialogButtonBox>(QDialogButtonBox::Help | QDialogButtonBox::Close,
                                                           "dialogButtonBox");
    connect(dialogButtonBox, &QDialogButtonBox::helpRequested, this, launchModuleHelp);
    connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto mainLayout = new QGridLayout(this);
    mainLayout->addLayout(topLayout, 0, 0, 1, 2);
    mainLayout->addWidget(buttonsGroupBox, 1, 0);
    mainLayout->addWidget(simpleInputWidgetsGroupBox, 1, 1);
    mainLayout->addWidget(itemViewTabWidget, 2, 0);
    mainLayout->addWidget(textToolBox, 2, 1);
    mainLayout->addWidget(progressBar, 3, 0, 1, 2);
    mainLayout->addWidget(dialogButtonBox, 4, 0, 1, 2);

    setWindowTitle(tr("Widget Gallery Qt %1").arg(QT_VERSION_STR));

    new QShortcut(QKeySequence::HelpContents, this, this, &WidgetGallery::helpOnCurrentWidget);
}

void  WidgetGallery::setVisible(bool visible)
{
    QDialog::setVisible(visible);
    if (visible) {
        connect(windowHandle(), &QWindow::screenChanged, this, &WidgetGallery::updateSystemInfo);
        updateSystemInfo();
    }
}

void WidgetGallery::changeStyle(const QString &styleName)
{
    QApplication::setStyle(QStyleFactory::create(styleName));
}

void WidgetGallery::advanceProgressBar()
{
    int curVal = progressBar->value();
    int maxVal = progressBar->maximum();
    progressBar->setValue(curVal + (maxVal - curVal) / 100);
}

QGroupBox *WidgetGallery::createButtonsGroupBox()
{
    auto result = createWidget1<QGroupBox>(tr("Buttons"), "buttonsGroupBox");

    auto defaultPushButton = createWidget1<QPushButton>(tr("Default Push Button"), "defaultPushButton");
    defaultPushButton->setDefault(true);

    auto togglePushButton = createWidget1<QPushButton>(tr("Toggle Push Button"), "togglePushButton");
    togglePushButton->setCheckable(true);
    togglePushButton->setChecked(true);

    auto flatPushButton = createWidget1<QPushButton>(tr("Flat Push Button"), "flatPushButton");
    flatPushButton->setFlat(true);

    auto toolButton = createWidget<QToolButton>("toolButton");
    toolButton->setText(tr("Tool Button"));

    auto menuToolButton = createWidget<QToolButton>("menuButton");
    menuToolButton->setText(tr("Menu Button"));
    auto toolMenu = new QMenu(menuToolButton);
    menuToolButton->setPopupMode(QToolButton::InstantPopup);
    toolMenu->addAction("Option");
    toolMenu->addSeparator();
    auto action = toolMenu->addAction("Checkable Option");
    action->setCheckable(true);
    menuToolButton->setMenu(toolMenu);
    auto toolLayout = new QHBoxLayout;
    toolLayout->addWidget(toolButton);
    toolLayout->addWidget(menuToolButton);

    auto commandLinkButton = createWidget1<QCommandLinkButton>(tr("Command Link Button"), "commandLinkButton");
    commandLinkButton->setDescription(tr("Description"));

    auto buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(defaultPushButton);
    buttonLayout->addWidget(togglePushButton);
    buttonLayout->addWidget(flatPushButton);
    buttonLayout->addLayout(toolLayout);
    buttonLayout->addWidget(commandLinkButton);
    buttonLayout->addStretch(1);

    auto radioButton1 = createWidget1<QRadioButton>(tr("Radio button 1"), "radioButton1");
    auto radioButton2 = createWidget1<QRadioButton>(tr("Radio button 2"), "radioButton2");
    auto radioButton3 = createWidget1<QRadioButton>(tr("Radio button 3"), "radioButton3");
    radioButton1->setChecked(true);

    auto checkBox =  createWidget1<QCheckBox>(tr("Tri-state check box"), "checkBox");
    checkBox->setTristate(true);
    checkBox->setCheckState(Qt::PartiallyChecked);

    auto checkableLayout = new QVBoxLayout;
    checkableLayout->addWidget(radioButton1);
    checkableLayout->addWidget(radioButton2);
    checkableLayout->addWidget(radioButton3);
    checkableLayout->addWidget(checkBox);
    checkableLayout->addStretch(1);

    auto mainLayout = new QHBoxLayout(result);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(checkableLayout);
    mainLayout->addStretch();
    return result;
}

static QWidget *embedIntoHBoxLayout(QWidget *w, int margin = 5)
{
    auto result = new QWidget;
    auto layout = new QHBoxLayout(result);
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->addWidget(w);
    return result;
}

QToolBox *WidgetGallery::createTextToolBox()
{
    auto result = createWidget<QToolBox>("toolBox");

    const QString plainText = tr("Twinkle, twinkle, little star,\n"
                            "How I wonder what you are.\n"
                            "Up above the world so high,\n"
                            "Like a diamond in the sky.\n"
                            "Twinkle, twinkle, little star,\n"
                            "How I wonder what you are!\n");
    // Create centered/italic HTML rich text
    QString richText = QLatin1String("<html><head/><body><i>");
    for (const auto &line : plainText.splitRef(QLatin1Char('\n')))
        richText += QLatin1String("<center>") + line + QLatin1String("</center>");
    richText += QLatin1String("</i></body></html>");

    auto textEdit = createWidget1<QTextEdit>(richText, "textEdit");
    auto plainTextEdit = createWidget1<QPlainTextEdit>(plainText, "plainTextEdit");

    systemInfoTextBrowser = createWidget<QTextBrowser>("systemInfoTextBrowser");

    result->addItem(embedIntoHBoxLayout(textEdit), tr("Text Edit"));
    result->addItem(embedIntoHBoxLayout(plainTextEdit), tr("Plain Text Edit"));
    result->addItem(embedIntoHBoxLayout(systemInfoTextBrowser), tr("Text Browser"));
    return result;
}

QTabWidget *WidgetGallery::createItemViewTabWidget()
{
    auto result = createWidget<QTabWidget>("bottomLeftTabWidget");
    result->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);

    auto treeView = createWidget<QTreeView>("treeView");
    auto fileSystemModel = new QFileSystemModel(treeView);
    fileSystemModel->setRootPath(QDir::rootPath());
    treeView->setModel(fileSystemModel);

    auto tableWidget = createWidget<QTableWidget>("tableWidget");
    tableWidget->setRowCount(10);
    tableWidget->setColumnCount(10);

    auto listModel = new QStandardItemModel(0, 1, result);
    listModel->appendRow(new QStandardItem(QIcon(QLatin1String(":/qt-project.org/styles/commonstyle/images/diropen-128.png")),
                                           tr("Directory")));
    listModel->appendRow(new QStandardItem(QIcon(QLatin1String(":/qt-project.org/styles/commonstyle/images/computer-32.png")),
                                           tr("Computer")));

    auto listView = createWidget<QListView>("listView");
    listView->setModel(listModel);

    auto iconModeListView = createWidget<QListView>("iconModeListView");
    iconModeListView->setViewMode(QListView::IconMode);
    iconModeListView->setModel(listModel);

    result->addTab(embedIntoHBoxLayout(treeView), tr("&Tree View"));
    result->addTab(embedIntoHBoxLayout(tableWidget), tr("T&able"));
    result->addTab(embedIntoHBoxLayout(listView), tr("&List"));
    result->addTab(embedIntoHBoxLayout(iconModeListView), tr("&Icon Mode List"));
    return result;
}

QGroupBox *WidgetGallery::createSimpleInputWidgetsGroupBox()
{
    auto result = createWidget1<QGroupBox>(tr("Simple Input Widgets"), "bottomRightGroupBox");
    result->setCheckable(true);
    result->setChecked(true);

    auto lineEdit = createWidget1<QLineEdit>("s3cRe7", "lineEdit");
    lineEdit->setClearButtonEnabled(true);
    lineEdit->setEchoMode(QLineEdit::Password);

    auto spinBox = createWidget<QSpinBox>("spinBox", result);
    spinBox->setValue(50);

    auto dateTimeEdit = createWidget<QDateTimeEdit>("dateTimeEdit", result);
    dateTimeEdit->setDateTime(QDateTime::currentDateTime());

    auto slider = createWidget<QSlider>("slider", result);
    slider->setOrientation(Qt::Horizontal);
    slider->setValue(40);

    auto scrollBar = createWidget<QScrollBar>("scrollBar", result);
    scrollBar->setOrientation(Qt::Horizontal);
    setClassNameToolTip(scrollBar);
    scrollBar->setValue(60);

    auto dial = createWidget<QDial>("dial", result);
    dial->setValue(30);
    dial->setNotchesVisible(true);

    auto layout = new QGridLayout(result);
    layout->addWidget(lineEdit, 0, 0, 1, 2);
    layout->addWidget(spinBox, 1, 0, 1, 2);
    layout->addWidget(dateTimeEdit, 2, 0, 1, 2);
    layout->addWidget(slider, 3, 0);
    layout->addWidget(scrollBar, 4, 0);
    layout->addWidget(dial, 3, 1, 2, 1);
    layout->setRowStretch(5, 1);
    return result;
}

QProgressBar *WidgetGallery::createProgressBar()
{
    auto result = createWidget<QProgressBar>("progressBar");
    result->setRange(0, 10000);
    result->setValue(0);

    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &WidgetGallery::advanceProgressBar);
    timer->start(1000);
    return result;
}

void WidgetGallery::updateSystemInfo()
{
    QString systemInfo;
    QTextStream str(&systemInfo);
    str << "<html><head/><body><h3>Build</h3><p>" << QLibraryInfo::build() << "</p>"
        << "<h3>Operating System</h3><p>" << QSysInfo::prettyProductName() << "</p>"
        << "<h3>Screens</h3><p>High DPI scale factor rounding policy: "
        << highDpiScaleFactorRoundingPolicy() << "</p><ol>";
    const auto screens = QGuiApplication::screens();
    for (auto screen : screens) {
        const bool current = screen == this->screen();
        str << "<li>";
        if (current)
            str << "<i>";
        str << '"' << screen->name() << "\" " << screen->geometry() << ", "
            << screen->logicalDotsPerInchX() << "DPI, DPR="
            << screen->devicePixelRatio();
        if (current)
            str << "</i>";
        str << "</li>";
    }
    str << "</ol></body></html>";
    systemInfoTextBrowser->setHtml(systemInfo);
}

void WidgetGallery::helpOnCurrentWidget()
{
    // Skip over internal widgets
    for (auto w = QApplication::widgetAt(QCursor::pos(screen())); w; w = w->parentWidget()) {
        const QString name = w->objectName();
        if (!name.isEmpty() && !name.startsWith(QLatin1String("qt_"))) {
            launchHelp(w);
            break;
        }
    }
}
