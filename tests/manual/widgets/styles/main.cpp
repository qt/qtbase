/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QStyle>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWindow>
#include <QScreen>

// Format enumeration value and strip off the class name
// added by QDebug: "QStyle::StandardPixmap(SP_Icon)" -> "SP_Icon".
template <typename Enum>
static inline QString formatEnumValue(Enum value)
{
    QString result;
    QDebug(&result) << value;
    int index = result.indexOf(QLatin1Char('('));
    if (index > 0) { // "QStyle::StandardPixmap(..)".
        result.remove(0, index + 1);
        index = result.lastIndexOf(QLatin1Char(')'));
        if (index > 0)
            result.truncate(index);
    }
    return result;
}

static QString pixmapDescription(QStyle::StandardPixmap sp, const QPixmap &pixmap)
{
    QString description = formatEnumValue(sp);
    QTextStream str(&description);
    str << '(' << int(sp) << ") ";
    if (pixmap.isNull()) {
        str << "(null)";
    } else {
        const qreal dpr = pixmap.devicePixelRatioF();
        str << ' ' << pixmap.width() << 'x' << pixmap.height();
        if (!qFuzzyCompare(dpr, qreal(1)))
            str << " DPR=" << dpr;
    }
    return description;
}

// Display pixmaps returned by QStyle::standardPixmap() in a grid.
static QWidget *createStandardPixmapPage(QWidget *parent)
{
    QWidget *result = new QWidget(parent);
    QGridLayout *grid = new QGridLayout(result);
    int row = 0;
    int column = 0;
    const int maxColumns = 6;
    for (int i = 0; i <= int(QStyle::SP_LineEditClearButton); ++i) {
        const QStyle::StandardPixmap sp = static_cast<QStyle::StandardPixmap>(i);
        QPixmap pixmap = result->style()->standardPixmap(sp, Q_NULLPTR, result);
        QLabel *descriptionLabel = new QLabel(pixmapDescription(sp, pixmap));
        grid->addWidget(descriptionLabel, row, column++);
        QLabel *displayLabel = new QLabel;
        displayLabel->setPixmap(pixmap);
        displayLabel->setFrameShape(QFrame::Box);
        grid->addWidget(displayLabel, row, column++);
        if (column >= maxColumns) {
            ++row;
            column = 0;
        }
    }
    return result;
}

// Display values returned by QStyle::pixelMetric().
static QWidget *createMetricsPage(QWidget *parent)
{
    QPlainTextEdit *result = new QPlainTextEdit(parent);
    QString text;
    QTextStream str(&text);
    for (int i = 0; i <= int(QStyle::PM_HeaderDefaultSectionSizeVertical); ++i) {
        const QStyle::PixelMetric m = static_cast<QStyle::PixelMetric>(i);
        str << formatEnumValue(m) << '(' << int(m) << ")="
            << result->style()->pixelMetric(m, Q_NULLPTR, result) << '\n';
    }
    result->setPlainText(text);
    return result;
}

// Display values returned by QStyle::styleHint()
static QWidget *createHintsPage(QWidget *parent)
{
    QPlainTextEdit *result = new QPlainTextEdit(parent);
    QString text;
    QTextStream str(&text);
    for (int i = 0; i <= int(QStyle::SH_Menu_SubMenuDontStartSloppyOnLeave); ++i) {
        const QStyle::StyleHint h = static_cast<QStyle::StyleHint>(i);
        str << formatEnumValue(h) << '(' << int(h) << ")="
            << result->style()->styleHint(h, Q_NULLPTR, result) << '\n';
    }
    result->setPlainText(text);
    return result;
}

// Display palette colors
static QWidget *createColorsPage(QWidget *parent)
{
    QWidget *result = new QWidget(parent);
    QGridLayout *grid = new QGridLayout;
    const QPalette palette = QGuiApplication::palette();
    int row = 0;
    for (int r = 0; r < int(QPalette::NColorRoles); ++r) {
        const QPalette::ColorRole role = static_cast<QPalette::ColorRole>(r);
        const QColor color = palette.color(QPalette::Active, role);
        if (color.isValid()) {
            const QString description =
                formatEnumValue(role) + QLatin1Char('(') + QString::number(r)
                + QLatin1String(") ") + color.name(QColor::HexArgb);
            grid->addWidget(new QLabel(description), row, 0);
            QLabel *displayLabel = new QLabel;
            QPixmap pixmap(20, 20);
            pixmap.fill(color);
            displayLabel->setPixmap(pixmap);
            displayLabel->setFrameShape(QFrame::Box);
            grid->addWidget(displayLabel, row, 1);
            ++row;
        }
    }
    QHBoxLayout *hBox = new QHBoxLayout;
    hBox->addLayout(grid);
    hBox->addStretch();
    QVBoxLayout *vBox = new QVBoxLayout(result);
    vBox->addLayout(hBox);
    vBox->addStretch();
    return result;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

public slots:
    void updateDescription();

private:
    QTabWidget *m_tabWidget;
    QLabel *m_descriptionLabel;
};

MainWindow::MainWindow()
    : m_tabWidget(new QTabWidget)
    , m_descriptionLabel(new QLabel)
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *a = fileMenu->addAction("Quit", this, &QWidget::close);
    a->setShortcut(Qt::CTRL + Qt::Key_Q);

    QWidget *central = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->addWidget(m_descriptionLabel);
    mainLayout->addWidget(m_tabWidget);
    m_tabWidget->addTab(createStandardPixmapPage(m_tabWidget), "Standard Pixmaps");
    m_tabWidget->addTab(createHintsPage(m_tabWidget), "Hints");
    m_tabWidget->addTab(createMetricsPage(m_tabWidget), "Pixel Metrics");
    m_tabWidget->addTab(createColorsPage(m_tabWidget), "Colors");
    setCentralWidget(central);

    setWindowTitle(QLatin1String("Style Tester (Qt") + QLatin1String(QT_VERSION_STR)
                   + QLatin1String(", ") + style()->objectName() + QLatin1Char(')'));
}

void MainWindow::updateDescription()
{
    QString text;
    QTextStream str(&text);
    str << "Qt " << QT_VERSION_STR << ", platform: " << QGuiApplication::platformName()
        << ", Style: \"" << style()->objectName() << "\", DPR=" << devicePixelRatioF()
        << ' ' << logicalDpiX() << ',' << logicalDpiY() << "DPI";
    if (const QWindow *w = windowHandle())
        str << ", Screen: \"" << w->screen()->name() << '"';
    m_descriptionLabel->setText(text);
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    MainWindow mw;
    mw.show();
    mw.updateDescription();
    QObject::connect(mw.windowHandle(), &QWindow::screenChanged,
                     &mw, &MainWindow::updateDescription);
    return app.exec();
}

#include "main.moc"
