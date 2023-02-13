// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QMap>
#include <QTabBar>
#include <QLabel>
#include <QLayout>
#include <QTabWidget>
#include <QProxyStyle>
#include <qdebug.h>
#include "tabbarform.h"

class TabBarProxyStyle : public QProxyStyle
{
public:
    TabBarProxyStyle() : QProxyStyle(), alignment(Qt::AlignLeft)
    { }

    int styleHint(StyleHint hint, const QStyleOption *option = 0,
                  const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const
    {
        if (hint == QStyle::SH_TabBar_Alignment)
            return alignment;

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    Qt::Alignment alignment;
};

const int TabCount = 5;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    auto *proxyStyle = new TabBarProxyStyle;
    app.setStyle(proxyStyle);

    QWidget widget;
    QStackedWidget stackedWidget;
    QTabBar tabBar;
    tabBar.setDocumentMode(true);
    tabBar.setTabsClosable(true);
    tabBar.setMovable(true);
    tabBar.setExpanding(false);

    // top
    tabBar.setShape(QTabBar::RoundedNorth);
    // bottom
//    tabBar.setShape(QTabBar::RoundedSouth);
    // left
//    tabBar.setShape(QTabBar::RoundedWest);
    // right
//    tabBar.setShape(QTabBar::RoundedEast);

    const auto shortLabel = QStringLiteral("Tab %1");
    const auto longLabel = QStringLiteral("An Extremely Long Tab Label %1");

    QMap<int, QWidget*> tabs;
    for (int i = 0; i < TabCount; i++) {
        QString tabNumberString = QString::number(i);
        QLabel *label = new QLabel(QStringLiteral("Tab %1 content").arg(tabNumberString));
        tabs[i] = label;
        label->setAlignment(Qt::AlignCenter);
        stackedWidget.addWidget(label);
        tabBar.addTab(shortLabel.arg(tabNumberString));
    }

    QObject::connect(&tabBar, &QTabBar::tabMoved, [&tabs](int from, int to) {
        QWidget *thisWidget = tabs[from];
        QWidget *thatWidget = tabs[to];
        tabs[from] = thatWidget;
        tabs[to] = thisWidget;
    });

    QObject::connect(&tabBar, &QTabBar::currentChanged, [&stackedWidget, &tabs](int index) {
        if (index >= 0)
            stackedWidget.setCurrentWidget(tabs[index]);
    });

    QObject::connect(&tabBar, &QTabBar::tabCloseRequested, [&stackedWidget, &tabBar, &tabs](int index) {
        QWidget *widget = tabs[index];
        tabBar.removeTab(index);
        for (int i = index + 1; i < TabCount; i++)
            tabs[i-1] = tabs[i];
        int currentIndex = tabBar.currentIndex();
        if (currentIndex >= 0)
            stackedWidget.setCurrentWidget(tabs[currentIndex]);
        delete widget;
    });

    QLayout *layout;
    switch (tabBar.shape()) {
    case QTabBar::RoundedEast:
    case QTabBar::TriangularEast:
        tabBar.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        layout = new QHBoxLayout(&widget);
        layout->addWidget(&stackedWidget);
        layout->addWidget(&tabBar);
        break;
    case QTabBar::RoundedWest:
    case QTabBar::TriangularWest:
        tabBar.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        layout = new QHBoxLayout(&widget);
        layout->addWidget(&tabBar);
        layout->addWidget(&stackedWidget);
        break;
    case QTabBar::RoundedNorth:
    case QTabBar::TriangularNorth:
        tabBar.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        layout = new QVBoxLayout(&widget);
        layout->addWidget(&tabBar);
        layout->addWidget(&stackedWidget);
        break;
    case QTabBar::RoundedSouth:
    case QTabBar::TriangularSouth:
        tabBar.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        layout = new QVBoxLayout(&widget);
        layout->addWidget(&stackedWidget);
        layout->addWidget(&tabBar);
        break;
    }

    TabBarForm form;
    layout->addWidget(&form);
    layout->setAlignment(&form, Qt::AlignHCenter);

    form.ui->documentModeButton->setChecked(tabBar.documentMode());
    QObject::connect(form.ui->documentModeButton, &QCheckBox::toggled, [&] {
        tabBar.setDocumentMode(form.ui->documentModeButton->isChecked());
        // QMacStyle (and maybe other styles) requires a re-polish to get the right font
        QApplication::sendEvent(&tabBar, new QEvent(QEvent::ThemeChange));
    });

    form.ui->movableTabsButton->setChecked(tabBar.isMovable());
    QObject::connect(form.ui->movableTabsButton, &QCheckBox::toggled, [&] {
        tabBar.setMovable(form.ui->movableTabsButton->isChecked());
        tabBar.update();
    });

    form.ui->closableTabsButton->setChecked(tabBar.tabsClosable());
    QObject::connect(form.ui->closableTabsButton, &QCheckBox::toggled, [&] {
        tabBar.setTabsClosable(form.ui->closableTabsButton->isChecked());
        tabBar.update();
    });

    form.ui->expandingTabsButton->setChecked(tabBar.expanding());
    QObject::connect(form.ui->expandingTabsButton, &QCheckBox::toggled, [&] {
        tabBar.setExpanding(form.ui->expandingTabsButton->isChecked());
        tabBar.update();
    });

    form.ui->displayIconButton->setChecked(!tabBar.tabIcon(0).isNull());
    QObject::connect(form.ui->displayIconButton, &QCheckBox::toggled, [&] {
        const auto icon = form.ui->displayIconButton->isChecked() ?
                          tabBar.style()->standardIcon(QStyle::SP_ComputerIcon) : QIcon();
        for (int i = 0; i < tabBar.count(); i++)
            tabBar.setTabIcon(i, icon);
    });

    form.ui->longLabelButton->setChecked(false);
    QObject::connect(form.ui->longLabelButton, &QCheckBox::toggled, [&] {
        const auto &label = form.ui->longLabelButton->isChecked() ? longLabel : shortLabel;
        for (int i = 0; i < tabBar.count(); i++)
            tabBar.setTabText(i, label.arg(i));
    });

    QObject::connect(form.ui->shapeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
        Q_UNUSED(index);
        // TODO
    });

    if (proxyStyle->alignment == Qt::AlignLeft)
        form.ui->leftAlignedButton->setChecked(true);
    else if (proxyStyle->alignment == Qt::AlignRight)
        form.ui->rightAlignedButton->setChecked(true);
    else
        form.ui->centeredButton->setChecked(true);
    QObject::connect(form.ui->textAlignmentGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton *b) {
        proxyStyle->alignment = b == form.ui->leftAlignedButton ? Qt::AlignLeft :
                                b == form.ui->rightAlignedButton ? Qt::AlignRight : Qt::AlignCenter;
        QApplication::sendEvent(&tabBar, new QEvent(QEvent::StyleChange));
    });

    layout->setContentsMargins(12, 12, 12, 12);
    widget.show();

    return app.exec();
}
