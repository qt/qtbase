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

#include <QMainWindow>
#include <QMenuBar>
#include <QLabel>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QApplication>
#include <QAction>
#include <QStyle>
#include <QToolBar>
#include <QPushButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QTabBar>
#include <QTextBrowser>
#include <QIcon>
#include <QPainter>
#include <QWindow>
#include <QScreen>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QFile>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTimer>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QElapsedTimer>
#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformscreen.h>

#include "dragwidget.h"

#include <utility>

static QTextStream &operator<<(QTextStream &str, const QSizeF &s)
{
    str << s.width() << 'x' << s.height();
    return str;
}

static QTextStream &operator<<(QTextStream &str, const QRect &r)
{
    str << r.width() << 'x' << r.height() << forcesign << r.x() << r.y() << noforcesign;
    return str;
}

static QString formatWindowTitle(const QString &title)
{
    QString result;
    QTextStream(&result) << title << ' ' << QT_VERSION_STR << " ("
        << QGuiApplication::platformName()
        << '/' << QApplication::style()->objectName() << ')';
    return result;
}

class DemoContainerBase
{
public:
    DemoContainerBase() = default;
    virtual ~DemoContainerBase() = default;
    QString name() { return option().names().constFirst(); }
    virtual QCommandLineOption &option() = 0;
    virtual void makeVisible(bool visible, QWidget *parent) = 0;
    QWidget *widget() const { return m_widget; }

protected:
    QWidget *m_widget = nullptr;
};

using DemoContainerList = QVector<DemoContainerBase*>;

template <class T>
class DemoContainer : public DemoContainerBase
{
public:
    DemoContainer(const QString &optionName, const QString &description)
        : m_option(optionName, description)
    {
    }
    ~DemoContainer() { delete m_widget; }

    QCommandLineOption &option() override { return m_option; }

    void makeVisible(bool visible, QWidget *parent) override
    {
        if (visible && !m_widget) {
            m_widget = new T;
            if (m_widget->windowTitle().isEmpty()) {
                QString title = m_option.description();
                if (title.startsWith("Test ", Qt::CaseInsensitive))
                    title.remove(0, 5);
                title[0] = title.at(0).toUpper();
                m_widget->setWindowTitle(formatWindowTitle(title));
            }
            m_widget->installEventFilter(parent);
        }
        if (m_widget)
            m_widget->setVisible(visible);
    }

private:
    QCommandLineOption m_option;
};

class LabelSlider : public QObject
{
Q_OBJECT
public:
    LabelSlider(QObject *parent, const QString &text, QGridLayout *layout, int row)
        : QObject(parent)
    {
        QLabel *textLabel = new QLabel(text);
        m_slider = new QSlider();
        m_slider->setOrientation(Qt::Horizontal);
        m_slider->setMinimum(1);
        m_slider->setMaximum(40);
        m_slider->setValue(10);
        m_slider->setTracking(false);
        m_slider->setTickInterval(5);
        m_slider->setTickPosition(QSlider::TicksBelow);
        m_label = new QLabel("1.0");

        // set up layouts
        layout->addWidget(textLabel, row, 0);
        layout->addWidget(m_slider, row, 1);
        layout->addWidget(m_label, row, 2);

        // handle slider position change
        connect(m_slider, &QSlider::sliderMoved, this, &LabelSlider::updateLabel);
        connect(m_slider, &QSlider::valueChanged, this, &LabelSlider::valueChanged);
    }
    void setValue(int scaleFactor)
    {
        m_slider->setValue(scaleFactor);
        updateLabel(scaleFactor);
    }

private slots:
    void updateLabel(int scaleFactor)
    {
            // slider value is scale factor times ten;
            qreal scalefactorF = qreal(scaleFactor) / 10.0;

            // update label, add ".0" if needed.
            QString number = QString::number(scalefactorF);
            if (!number.contains(QLatin1Char('.')))
                number.append(".0");
            m_label->setText(number);
    }

signals:
    void valueChanged(int scaleFactor);

private:
    QSlider *m_slider;
    QLabel *m_label;
};

static qreal getScreenFactorWithoutPixelDensity(const QScreen *screen)
{
    // this is a hack that relies on knowing the internals of QHighDpiScaling
    static const char *scaleFactorProperty = "_q_scaleFactor";
    QVariant screenFactor = screen->property(scaleFactorProperty);
    return screenFactor.isValid() ? screenFactor.toReal() : 1.0;
}

static inline qreal getGlobalScaleFactor()
{
    QScreen *noScreen = nullptr;
    return QHighDpiScaling::factor(noScreen);
}

class DemoController : public QWidget
{
    Q_OBJECT
public:
    DemoController(DemoContainerList demos, QCommandLineParser *parser);
    ~DemoController();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void closeEvent(QCloseEvent *) override { QCoreApplication::quit(); }

private slots:
    void handleButton(int id, bool toggled);

private:
    DemoContainerList m_demos;
    QButtonGroup *m_group;
};

DemoController::DemoController(DemoContainerList demos, QCommandLineParser *parser)
    : m_demos(std::move(demos))
{
    setWindowTitle(formatWindowTitle("Screen Scale Factors"));
    setObjectName("controller"); // make WindowScaleFactorSetter skip this window

    auto mainLayout = new QVBoxLayout(this);
    auto scaleLayout = new QGridLayout;
    mainLayout->addLayout(scaleLayout);

    int layoutRow = 0;
    LabelSlider *globalScaleSlider = new LabelSlider(this, "Global scale factor", scaleLayout, layoutRow++);
    globalScaleSlider->setValue(int(getGlobalScaleFactor() * 10));
     connect(globalScaleSlider, &LabelSlider::valueChanged, [](int scaleFactor){
            // slider value is scale factor times ten;
            qreal scalefactorF = qreal(scaleFactor) / 10.0;
            QHighDpiScaling::setGlobalFactor(scalefactorF);
         });

    // set up one scale control line per screen
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        // create scale control line
        QSize screenSize = screen->geometry().size();
        QString screenId = screen->name() + QLatin1Char(' ') + QString::number(screenSize.width())
                                          + QLatin1Char(' ') + QString::number(screenSize.height());
        LabelSlider *slider = new LabelSlider(this, screenId, scaleLayout, layoutRow++);
        slider->setValue(getScreenFactorWithoutPixelDensity(screen) * 10);

        // handle slider value change
        connect(slider, &LabelSlider::valueChanged, [screen](int scaleFactor){
            // slider value is scale factor times ten;
            qreal scalefactorF = qreal(scaleFactor) / 10.0;

            // set scale factor for screen
            qreal oldFactor = QHighDpiScaling::factor(screen);
            QHighDpiScaling::setScreenFactor(screen, scalefactorF);
            qreal newFactor = QHighDpiScaling::factor(screen);

            qDebug() << "factor was / is" << oldFactor << newFactor;
        });
    }

    auto demoLayout = new QFormLayout;
    mainLayout->addLayout(demoLayout);
    m_group = new QButtonGroup(this);
    m_group->setExclusive(false);

    for (int i = 0; i < m_demos.size(); ++i) {
        DemoContainerBase *demo = m_demos.at(i);
        QString name = demo->name();
        name[0] = name.at(0).toUpper();
        auto button = new QPushButton(name);
        button->setCheckable(true);
        demoLayout->addRow(demo->option().description(), button);
        m_group->addButton(button, i);

        if (parser->isSet(demo->option())) {
            demo->makeVisible(true, this);
            button->setChecked(true);
        }
    }
    connect(m_group, QOverload<int,bool>::of(&QButtonGroup::buttonToggled),
            this, &DemoController::handleButton);
}

DemoController::~DemoController()
{
    qDeleteAll(m_demos);
}

bool DemoController::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        for (int i = 0; i < m_demos.size(); ++i) {
            DemoContainerBase *demo = m_demos.at(i);
            if (demo->widget() == object) {
                m_group->button(i)->setChecked(false);
                break;
            }
        }
    }
    return false;
}

void DemoController::handleButton(int id, bool toggled)
{
    m_demos.at(id)->makeVisible(toggled, this);
}

class PixmapPainter : public QWidget
{
public:
    PixmapPainter();

    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap pixmap1X;
    QPixmap pixmap2X;
    QPixmap pixmapLarge;
    QImage image1X;
    QImage image2X;
    QImage imageLarge;
    QIcon qtIcon;
};

PixmapPainter::PixmapPainter()
{
    pixmap1X = QPixmap(":/qticon32.png");
    pixmap2X = QPixmap(":/qticon32@2x.png");
    pixmapLarge = QPixmap(":/qticon64.png");

    image1X = QImage(":/qticon32.png");
    image2X = QImage(":/qticon32@2x.png");
    imageLarge = QImage(":/qticon64.png");

    qtIcon.addFile(":/qticon32.png");
    qtIcon.addFile(":/qticon32@2x.png");
}

void PixmapPainter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(QRect(QPoint(0, 0), size()), QBrush(Qt::gray));

    int pixmapPointSize = 32;
    int y = 30;
    int dy = 90;

    int x = 10;
    int dx = 40;
    // draw at point
//          qDebug() << "paint pixmap" << pixmap1X.devicePixelRatio();
          p.drawPixmap(x, y, pixmap1X);
    x+=dx;p.drawPixmap(x, y, pixmap2X);
    x+=dx;p.drawPixmap(x, y, pixmapLarge);
  x+=dx*2;p.drawPixmap(x, y, qtIcon.pixmap(QSize(pixmapPointSize, pixmapPointSize)));
    x+=dx;p.drawImage(x, y, image1X);
    x+=dx;p.drawImage(x, y, image2X);
    x+=dx;p.drawImage(x, y, imageLarge);

    // draw at 32x32 rect
    y+=dy;
    x = 10;
          p.drawPixmap(QRect(x, y, pixmapPointSize, pixmapPointSize), pixmap1X);
    x+=dx;p.drawPixmap(QRect(x, y, pixmapPointSize, pixmapPointSize), pixmap2X);
    x+=dx;p.drawPixmap(QRect(x, y, pixmapPointSize, pixmapPointSize), pixmapLarge);
    x+=dx;p.drawPixmap(QRect(x, y, pixmapPointSize, pixmapPointSize), qtIcon.pixmap(QSize(pixmapPointSize, pixmapPointSize)));
    x+=dx;p.drawImage(QRect(x, y, pixmapPointSize, pixmapPointSize), image1X);
    x+=dx;p.drawImage(QRect(x, y, pixmapPointSize, pixmapPointSize), image2X);
    x+=dx;p.drawImage(QRect(x, y, pixmapPointSize, pixmapPointSize), imageLarge);


    // draw at 64x64 rect
    y+=dy - 50;
    x = 10;
               p.drawPixmap(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), pixmap1X);
    x+=dx * 2; p.drawPixmap(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), pixmap2X);
    x+=dx * 2; p.drawPixmap(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), pixmapLarge);
    x+=dx * 2; p.drawPixmap(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), qtIcon.pixmap(QSize(pixmapPointSize, pixmapPointSize)));
    x+=dx * 2; p.drawImage(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), image1X);
    x+=dx * 2; p.drawImage(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), image2X);
    x+=dx * 2; p.drawImage(QRect(x, y, pixmapPointSize * 2, pixmapPointSize * 2), imageLarge);
 }

class TiledPixmapPainter : public QWidget
{
public:
    TiledPixmapPainter();

    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap pixmap1X;
    QPixmap pixmap2X;
    QPixmap pixmapLarge;
};

TiledPixmapPainter::TiledPixmapPainter()
{
    pixmap1X = QPixmap(":/qticon32.png");
    pixmap2X = QPixmap(":/qticon32@2x.png");
    pixmapLarge = QPixmap(":/qticon64.png");
}

void TiledPixmapPainter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);

    int xoff = 10;
    int yoff = 10;
    int tiles = 4;
    int pixmapEdge = 32;
    int tileAreaEdge = pixmapEdge * tiles;

    // Expected behavior for both 1x and 2x dislays:
    // 1x pixmap   : 4 x 4 tiles
    // large pixmap: 2 x 2 tiles
    // 2x pixmap   : 4 x 4 tiles
    //
    // On a 2x display the 2x pixmap tiles
    // will be drawn in high resolution.
    p.drawTiledPixmap(QRect(xoff, yoff, tileAreaEdge, tileAreaEdge), pixmap1X);
    yoff += tiles * pixmapEdge + 10;
    p.drawTiledPixmap(QRect(xoff, yoff, tileAreaEdge, tileAreaEdge), pixmapLarge);
    yoff += tiles * pixmapEdge + 10;
    p.drawTiledPixmap(QRect(xoff, yoff, tileAreaEdge, tileAreaEdge), pixmap2X);

    // Again, with an offset. The offset is in
    // device-independent pixels.
    QPoint offset(40, 40); // larger than the pixmap edge size to exercise that code path
    yoff = 10;
    xoff = 20 + tiles * pixmapEdge ;
    p.drawTiledPixmap(QRect(xoff, yoff, tileAreaEdge, tileAreaEdge), pixmap1X, offset);
    yoff += tiles * pixmapEdge + 10;
    p.drawTiledPixmap(QRect(xoff, yoff, tileAreaEdge, tileAreaEdge), pixmapLarge, offset);
    yoff += tiles * pixmapEdge + 10;
    p.drawTiledPixmap(QRect(xoff, yoff, tileAreaEdge, tileAreaEdge), pixmap2X, offset);
}

class Labels : public QWidget
{
public:
    Labels();

private:
    QPixmap pixmap1X;
    QPixmap pixmap2X;
    QPixmap pixmapLarge;
    QIcon qtIcon;
};

Labels::Labels()
{
    pixmap1X = QPixmap(":/qticon32.png");
    pixmap2X = QPixmap(":/qticon32@2x.png");
    pixmapLarge = QPixmap(":/qticon64.png");

    qtIcon.addFile(":/qticon32.png");
    qtIcon.addFile(":/qticon32@2x.png");
    setWindowIcon(qtIcon);
    setWindowTitle(formatWindowTitle("Labels"));

    QLabel *label1x = new QLabel();
    label1x->setPixmap(pixmap1X);
    QLabel *label2x = new QLabel();
    label2x->setPixmap(pixmap2X);
    QLabel *labelIcon = new QLabel();
    labelIcon->setPixmap(qtIcon.pixmap(QSize(32,32)));
    QLabel *labelLarge = new QLabel();
    labelLarge->setPixmap(pixmapLarge);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(label1x);    //expected low-res on high-dpi displays
    layout->addWidget(label2x);    //expected high-res on high-dpi displays
    layout->addWidget(labelIcon);  //expected high-res on high-dpi displays
    layout->addWidget(labelLarge); // expected large size and low-res
    setLayout(layout);
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    QMenu *addNewMenu(const QString &title, int itemCount = 5);

private slots:
    void maskActionToggled(bool t);

private:
    QIcon qtIcon;
    QIcon qtIcon1x;
    QIcon qtIcon2x;

    QToolBar *fileToolBar;
    QAction *m_maskAction;
    int menuCount = 0;
};

MainWindow::MainWindow()
{
    // beware that QIcon auto-loads the @2x versions.
    qtIcon1x.addFile(":/qticon16.png");
    qtIcon2x.addFile(":/qticon32.png");
    setWindowIcon(qtIcon);
    setWindowTitle(formatWindowTitle("MainWindow"));

    fileToolBar = addToolBar(tr("File"));
//    fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    fileToolBar->addAction(new QAction(qtIcon1x, QString("1x"), this));
    fileToolBar->addAction(new QAction(qtIcon2x, QString("2x"), this));
    addNewMenu("&Edit");
    addNewMenu("&Build");
    addNewMenu("&Debug", 4);
    QMenu *menu = addNewMenu("&Transmogrify", 7);
    menu->addSeparator();
    m_maskAction = menu->addAction("Mask");
    m_maskAction->setCheckable(true);
    connect(m_maskAction, &QAction::toggled, this, &MainWindow::maskActionToggled);
    fileToolBar->addAction(m_maskAction);
    addNewMenu("T&ools");
    addNewMenu("&Help", 2);
}

QMenu *MainWindow::addNewMenu(const QString &title, int itemCount)
{
    QMenu *menu = menuBar()->addMenu(title);
    for (int i = 0; i < itemCount; i++) {
        menuCount++;
        QString s = "Menu item " + QString::number(menuCount);
        if (i == 3) {
            QMenu *subMenu = menu->addMenu(s);
            for (int j = 1; j < 4; j++)
                subMenu->addAction(QString::fromLatin1("SubMenu item %1.%2").arg(menuCount).arg(j));
        } else {
            menu->addAction(s);
        }
    }
    return menu;
}

void MainWindow::maskActionToggled(bool t)
{
    if (t) {
        QVector<QPoint> upperLeftTriangle;
        upperLeftTriangle << QPoint(0, 0) << QPoint(width(), 0) << QPoint(0, height());
        setMask(QRegion(QPolygon(upperLeftTriangle)));
    } else {
        clearMask();
    }
}

class StandardIcons : public QWidget
{
public:
    void paintEvent(QPaintEvent *) override
    {
        int x = 10;
        int y = 10;
        int dx = 50;
        int dy = 50;
        int maxX = 500;

        for (uint iconIndex = QStyle::SP_TitleBarMenuButton; iconIndex < QStyle::SP_MediaVolumeMuted; ++iconIndex) {
            QIcon icon = qApp->style()->standardIcon(QStyle::StandardPixmap(iconIndex));
            QPainter p(this);
            p.drawPixmap(x, y, icon.pixmap(dx - 5, dy - 5));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }
    }
};

class Caching : public QWidget
{
public:
    void paintEvent(QPaintEvent *) override
    {
        QSize layoutSize(75, 75);

        QPainter widgetPainter(this);
        widgetPainter.fillRect(QRect(QPoint(0, 0), this->size()), Qt::gray);

        {
            const qreal devicePixelRatio = this->windowHandle()->devicePixelRatio();
            QPixmap cache(layoutSize * devicePixelRatio);
            cache.setDevicePixelRatio(devicePixelRatio);

            QPainter cachedPainter(&cache);
            cachedPainter.fillRect(QRect(0,0, 75, 75), Qt::blue);
            cachedPainter.fillRect(QRect(10,10, 55, 55), Qt::red);
            cachedPainter.drawEllipse(QRect(10,10, 55, 55));

            QPainter widgetPainter(this);
            widgetPainter.drawPixmap(QPoint(10, 10), cache);
        }

        {
            const qreal devicePixelRatio = this->windowHandle()->devicePixelRatio();
            QImage cache = QImage(layoutSize * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
            cache.setDevicePixelRatio(devicePixelRatio);

            QPainter cachedPainter(&cache);
            cachedPainter.fillRect(QRect(0,0, 75, 75), Qt::blue);
            cachedPainter.fillRect(QRect(10,10, 55, 55), Qt::red);
            cachedPainter.drawEllipse(QRect(10,10, 55, 55));

            QPainter widgetPainter(this);
            widgetPainter.drawImage(QPoint(95, 10), cache);
        }

    }
};

class Style : public QWidget
{
public:
    Style()
    {
        row1 = new QHBoxLayout(this);

        button = new QPushButton();
        button->setText("Test Button");
        row1->addWidget(button);

        lineEdit = new QLineEdit();
        lineEdit->setText("Test Lineedit");
        row1->addWidget(lineEdit);

        slider = new QSlider();
        row1->addWidget(slider);

        row1->addWidget(new QSpinBox);
        row1->addWidget(new QScrollBar);

        auto tab  = new QTabBar();
        tab->addTab("Foo");
        tab->addTab("Bar");
        row1->addWidget(tab);
    }

private:
    QPushButton *button;
    QLineEdit *lineEdit;
    QSlider *slider;
    QHBoxLayout *row1;
};

class Fonts : public QWidget
{
public:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);

        // Points
        int y = 10;
        for (int fontSize = 6; fontSize < 18; fontSize += 2) {
            QFont font;
            font.setPointSize(fontSize);
            QString string = QString(QStringLiteral("This text is in point size %1")).arg(fontSize);
            painter.setFont(font);
            y += (painter.fontMetrics().lineSpacing());
            painter.drawText(10, y, string);
        }

        // Pixels
        y += painter.fontMetrics().lineSpacing();
        for (int fontSize = 6; fontSize < 18; fontSize += 2) {
            QFont font;
            font.setPixelSize(fontSize);
            QString string = QString(QStringLiteral("This text is in pixel size %1")).arg(fontSize);
            painter.setFont(font);
            y += (painter.fontMetrics().lineSpacing());
            painter.drawText(10, y, string);
        }
    }
};


template <typename T>
void apiTestdevicePixelRatioGetter()
{
    if (0) {
        T *t = nullptr;
        t->devicePixelRatio();
    }
}

template <typename T>
void apiTestdevicePixelRatioSetter()
{
    if (0) {
        T *t = nullptr;
        t->setDevicePixelRatio(2.0);
    }
}

void apiTest()
{
    // compile call to devicePixelRatio getter and setter (verify spelling)
    apiTestdevicePixelRatioGetter<QWindow>();
    apiTestdevicePixelRatioGetter<QScreen>();
    apiTestdevicePixelRatioGetter<QGuiApplication>();

    apiTestdevicePixelRatioGetter<QImage>();
    apiTestdevicePixelRatioSetter<QImage>();
    apiTestdevicePixelRatioGetter<QPixmap>();
    apiTestdevicePixelRatioSetter<QPixmap>();
}

// Request and draw an icon at different sizes
class IconDrawing : public QWidget
{
public:
    IconDrawing()
    {
        const QString tempPath = m_temporaryDir.path();
        const QString path32 = tempPath + "/qticon32.png";
        const QString path32_2 = tempPath + "/qticon32-2.png";
        const QString path32_2x = tempPath + "/qticon32@2x.png";

        QFile::copy(":/qticon32.png", path32_2);
        QFile::copy(":/qticon32.png", path32);
        QFile::copy(":/qticon32@2x.png", path32_2x);

        iconHighDPI.reset(new QIcon(path32)); // will auto-load @2x version.
        iconNormalDpi.reset(new QIcon(path32_2)); // does not have a 2x version.
    }

    void paintEvent(QPaintEvent *) override
    {
        int x = 10;
        int y = 10;
        int dx = 50;
        int dy = 50;
        int maxX = 600;
        int minSize = 5;
        int maxSize = 64;
        int sizeIncrement = 5;

        // Disable high-dpi icons
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, false);

        // normal icon
        for (int size = minSize; size < maxSize; size += sizeIncrement) {
            QPainter p(this);
            p.drawPixmap(x, y, iconNormalDpi->pixmap(size, size));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }
        x = 10;
        y+=dy;

        // high-dpi icon
        for (int size = minSize; size < maxSize; size += sizeIncrement) {
            QPainter p(this);
            p.drawPixmap(x, y, iconHighDPI->pixmap(size, size));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }

        x = 10;
        y+=dy;

        // Enable high-dpi icons
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

        // normal icon
        for (int size = minSize; size < maxSize; size += sizeIncrement) {
            QPainter p(this);
            p.drawPixmap(x, y, iconNormalDpi->pixmap(size, size));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }
        x = 10;
        y+=dy;

        // high-dpi icon (draw point)
        for (int size = minSize; size < maxSize; size += sizeIncrement) {
            QPainter p(this);
            p.drawPixmap(x, y, iconHighDPI->pixmap(size, size));
            if (x + dx > maxX)
                y+=dy;
            x = ((x + dx) % maxX);
        }

        x = 10;
        y+=dy;

    }

private:
    QTemporaryDir m_temporaryDir;
    QScopedPointer<QIcon> iconHighDPI;
    QScopedPointer<QIcon> iconNormalDpi;
};

// Icons on buttons
class Buttons : public QWidget
{
public:
    Buttons()
    {
        QIcon icon;
        icon.addFile(":/qticon16@2x.png");

        QPushButton *button =  new QPushButton(this);
        button->setIcon(icon);
        button->setText("16@2x");

        QTabBar *tab = new QTabBar(this);
        tab->addTab(QIcon(":/qticon16.png"), "16@1x");
        tab->addTab(QIcon(":/qticon16@2x.png"), "16@2x");
        tab->addTab(QIcon(":/qticon16.png"), "");
        tab->addTab(QIcon(":/qticon16@2x.png"), "");
        tab->move(10, 100);
        tab->show();

        auto toolBar = new QToolBar(this);
        toolBar->addAction(QIcon(":/qticon16.png"), "16");
        toolBar->addAction(QIcon(":/qticon16@2x.png"), "16@2x");
        toolBar->addAction(QIcon(":/qticon32.png"), "32");
        toolBar->addAction(QIcon(":/qticon32@2x.png"), "32@2x");

        toolBar->move(10, 200);
        toolBar->show();
    }
};

class LinePainter : public QWidget
{
public:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint lastMousePoint;
    QVector<QPoint> linePoints;
};

void LinePainter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(QRect(QPoint(0, 0), size()), QBrush(Qt::gray));

    // Default antialiased line
    p.setRenderHint(QPainter::Antialiasing);
    p.drawLines(linePoints);

    // Cosmetic 1 antialiased line
    QPen pen;
    pen.setCosmetic(true);
    pen.setWidth(1);
    p.setPen(pen);
    p.translate(3, 3);
    p.drawLines(linePoints);

    // Aliased cosmetic 1 line
    p.setRenderHint(QPainter::Antialiasing, false);
    p.translate(3, 3);
    p.drawLines(linePoints);
}

void LinePainter::mousePressEvent(QMouseEvent *event)
{
    lastMousePoint = event->pos();
}

void LinePainter::mouseReleaseEvent(QMouseEvent *)
{
    lastMousePoint = QPoint();
}

void LinePainter::mouseMoveEvent(QMouseEvent *event)
{
    if (lastMousePoint.isNull())
        return;

    QPoint newMousePoint = event->pos();
    if (lastMousePoint == newMousePoint)
        return;
    linePoints.append(lastMousePoint);
    linePoints.append(newMousePoint);
    lastMousePoint = newMousePoint;
    update();
}

class CursorTester : public QWidget
{
public:
    CursorTester() = default;

    inline QRect getRect(int idx) const
    {
        int h = height() / 2;
        return QRect(10, 10 + h * (idx - 1), width() - 20, h - 20);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        QRect r1 = getRect(1);
        QRect r2 = getRect(2);
        p.fillRect(r1, QColor(200, 200, 250));
        p.drawText(r1, "Drag from here to move a window based on QCursor::pos()");
        p.fillRect(r2, QColor(250, 200, 200));
        p.drawText(r2, "Drag from here to move a window based on mouse event position");

        if (moving) {
            p.setPen(Qt::darkGray);
            QFont f = font();
            f.setPointSize(8);
            p.setFont(f);
            p.drawEllipse(mousePos, 30,60);
            QPoint pt = mousePos - QPoint(0, 60);
            QPoint pt2 = pt - QPoint(30,10);
            QPoint offs(30, 0);
            p.drawLine(pt, pt2);
            p.drawLine(pt2 - offs, pt2 + offs);
            p.drawText(pt2 - offs, "mouse pos");

            p.setPen(QColor(50,130,70));
            QPoint cursorPos = mapFromGlobal(QCursor::pos());
            pt = cursorPos - QPoint(0, 30);
            pt2 = pt + QPoint(60, -20);
            p.drawEllipse(cursorPos, 60, 30);
            p.drawLine(pt, pt2);
            p.drawLine(pt2 - offs, pt2 + offs);
            p.drawText(pt2 - offs, "cursor pos");
        }
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (moving)
            return;
        QRect r1 = getRect(1);
        QRect r2 = getRect(2);

        moving = r1.contains(e->pos()) || r2.contains(e->pos());
        if (!moving)
            return;
        useCursorPos = r1.contains(e->pos());

        if (!moveLabel)
            moveLabel = new QLabel(this,Qt::BypassWindowManagerHint|Qt::FramelessWindowHint|Qt::Window );

        if (useCursorPos)
            moveLabel->setText("I'm following QCursor::pos()");
        else
            moveLabel->setText("I'm following QMouseEvent::globalPos()");
        moveLabel->adjustSize();
        mouseMoveEvent(e);
        moveLabel->show();
    }

    void mouseReleaseEvent(QMouseEvent *) override
    {
        if (moveLabel)
            moveLabel->hide();
        update();
        moving = false;
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (!moving)
            return;
        QPoint pos = useCursorPos ? QCursor::pos() : e->globalPos();
        pos -= moveLabel->rect().center();
        moveLabel->move(pos);
        mousePos = e->pos();
        update();
    }

private:
    QLabel *moveLabel = nullptr;
    QPoint mousePos;
    bool useCursorPos = false;
    bool moving = false;
};

class ScreenDisplayer : public QWidget
{
public:
    ScreenDisplayer() = default;

    void timerEvent(QTimerEvent *) override
    {
        update();
    }

    void mousePressEvent(QMouseEvent *) override
    {
        if (!moveLabel)
            moveLabel = new QLabel(this,Qt::BypassWindowManagerHint|Qt::FramelessWindowHint|Qt::Window );
        moveLabel->setText("Hello, Qt this is a label\nwith some text");
        moveLabel->show();
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (!moveLabel)
            return;
        moveLabel->move(e->pos() / scaleFactor);
        QString str;
        QDebug dbg(&str);
        dbg.setAutoInsertSpaces(false);
        dbg << moveLabel->geometry();
        moveLabel->setText(str);
    }

    void mouseReleaseEvent(QMouseEvent *) override
    {
        if (moveLabel)
            moveLabel->hide();
    }

    void showEvent(QShowEvent *) override
    {
        refreshTimer.start(300, this);
    }

    void hideEvent(QHideEvent *) override
    {
        refreshTimer.stop();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        QRectF total;
        const auto screens = QGuiApplication::screens();
        for (const QScreen *screen : screens)
            total |= screen->geometry();
        if (total.isEmpty())
            return;

        scaleFactor = qMin(width()/total.width(), height()/total.height());

        p.fillRect(rect(), Qt::black);
        p.scale(scaleFactor, scaleFactor);
        p.translate(-total.topLeft());
        p.setPen(QPen(Qt::white, 10));
        p.setBrush(Qt::gray);

        for (const QScreen *screen : screens) {
            p.drawRect(screen->geometry());
            QFont f = font();
            f.setPixelSize(screen->geometry().height() / 8);
            p.setFont(f);
            p.drawText(screen->geometry(), Qt::AlignCenter, screen->name());
        }
        p.setBrush(QColor(200,220,255,127));

        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget *widget : topLevels) {
            if (!widget->isHidden())
                p.drawRect(widget->geometry());
        }

        QPolygon cursorShape;
        cursorShape << QPoint(0,0) << QPoint(20, 60)
                    << QPoint(30, 50) << QPoint(60, 80)
                    << QPoint(80, 60) << QPoint(50, 30)
                    << QPoint(60, 20);
        cursorShape.translate(QCursor::pos());
        p.drawPolygon(cursorShape);
    }

private:
    QLabel *moveLabel = nullptr;
    qreal scaleFactor = 1;
    QBasicTimer refreshTimer;
};

class PhysicalSizeTest : public QWidget
{
    Q_OBJECT
public:
    PhysicalSizeTest() = default;

    void paintEvent(QPaintEvent *event) override;

    void resizeEvent(QResizeEvent *) override
    {
        qreal ppi = window()->windowHandle()->screen()->physicalDotsPerInchX();
        QSizeF s = size();
        if (!m_ignoreResize)
            m_physicalSize = s / ppi;
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ScreenChangeInternal) {
            // we will get resize events when the scale factor changes
            m_ignoreResize = true;
            QTimer::singleShot(100, this, &PhysicalSizeTest::handleScreenChange);
        }
        return QWidget::event(event);
    }

public slots:
    void handleScreenChange()
    {
        qreal ppi = window()->windowHandle()->screen()->physicalDotsPerInchX();
        QSizeF newSize = m_physicalSize * ppi;
        resize(newSize.toSize());
        m_ignoreResize = false;
    }

private:
    QSizeF m_physicalSize;
    bool m_ignoreResize = false;
};

void PhysicalSizeTest::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    qreal ppi = window()->windowHandle()->screen()->physicalDotsPerInchX();
    qreal ppmm = ppi / 25.4;
    qreal h = 15 * ppmm;
    QRectF rulerRect(0,0, width(), h);
    rulerRect.moveCenter(rect().center());

    QFont f = font();
    f.setPixelSize(18);
    p.setFont(f);

    // draw a rectangle in (Qt) pixel coordinates, for comparison
    QRect pixelRect(0, 0, 300, 50);
    pixelRect.moveTopLeft(QPoint(5 * ppmm, rulerRect.bottom() + 5 * ppmm));
    p.fillRect(pixelRect, QColor(199,222,255));
    p.drawText(pixelRect, "This rectangle is 300x50 pixels");

    f.setPixelSize(4 * ppmm);
    p.setFont(f);

    QRectF topRect(0, 0, width(), rulerRect.top());
    p.drawText(topRect, Qt::AlignCenter, "The ruler is drawn in physical units.\nThis window tries to keep its physical size\nwhen moved between screens.");

    // draw a ruler in real physical coordinates

    p.fillRect(rulerRect, QColor(255, 222, 111));

    QPen linePen(Qt::black, 0.3 * ppmm);
    p.setPen(linePen);
    f.setBold(true);
    p.setFont(f);

    qreal vCenter = rulerRect.center().y();
    p.drawLine(0, vCenter, width(), vCenter);

    // cm
    for (int i = 0;;) {
        i++;
        qreal x = i * ppmm;
        if (x > width())
            break;
        qreal y = rulerRect.bottom();
        qreal len;
        if (i % 5)
            len = 2 * ppmm;
        else if (i % 10)
            len = 3 * ppmm;
        else
            len = h / 2;

        p.drawLine(QPointF(x, y), QPointF(x, y - len));
        if (i % 10 == 5) {
            QRectF textR(0, 0, 5 * ppmm, h / 2 - 2 * ppmm);
            textR.moveTopLeft(QPointF(x, vCenter));
            int n = i / 10 + 1;
            if (n % 10 == 0)
                p.setPen(Qt::red);
            p.drawText(textR, Qt::AlignCenter, QString::number(n));
            p.setPen(linePen);
        }
    }

    //inches
    for (int i = 0;;) {
        i++;
        qreal x = i * ppi / 16;
        if (x > width())
            break;
        qreal y = rulerRect.top();

        qreal d = h / 10;
        qreal len;
        if (i % 2)
            len = 1 * d;
        else if (i % 4)
            len = 2 * d;
        else if (i % 8)
            len = 3 * d;
        else if (i % 16)
            len = 4 * d;
        else
            len = h / 2;

        p.drawLine(QPointF(x, y), QPointF(x, y + len));
        if (i % 16 == 12) {
            QRectF textR(0, 0, 0.25 * ppi, h / 2 - 2 * d);
            textR.moveBottomLeft(QPointF(x, vCenter));
            p.drawText(textR, Qt::AlignCenter, QString::number(1 + i/16));
        }
    }

}

class GraphicsViewCaching : public QGraphicsView
{
public:
    GraphicsViewCaching()
    {
        auto scene = new QGraphicsScene(0, 0, 400, 400);

        QGraphicsTextItem *item = scene->addText("NoCache");
        item->setCacheMode(QGraphicsItem::NoCache);
        item->setPos(10, 10);

        item = scene->addText("ItemCoordinateCache");
        item->setCacheMode(QGraphicsItem::ItemCoordinateCache);
        item->setPos(10, 30);

        item = scene->addText("DeviceCoordinateCache");
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        item->setPos(10, 50);

        setScene(scene);
    }
};

class MetricsTest : public QTabWidget
{
    Q_OBJECT
public:
    MetricsTest()
    {
        qDebug().noquote().nospace() << "MetricsTest " << QT_VERSION_STR
            << ' ' << QGuiApplication::platformName() << '\n'
<< R"(Relevant environment variables are:
QT_FONT_DPI=N
QT_SCALE_FACTOR=n
QT_ENABLE_HIGHDPI_SCALING=0|1
QT_USE_PHYSICAL_DPI=0|1
QT_SCREEN_SCALE_FACTORS=N;N;N or QT_SCREEN_SCALE_FACTORS=name:N
QT_SCALE_FACTOR_ROUNDING_POLICY=Round|Ceil|Floor|RoundPreferFloor|PassThrough
QT_DPI_ADJUSTMENT_POLICY=AdjustDpi|DontAdjustDpi|AdjustUpOnly)";

        m_textEdit = addTextPage("Parameters");
        m_logEdit = addTextPage("Screen Change Log");

        const auto screens = QGuiApplication::screens();
        for (auto screen : screens)
            connectScreenChangeSignals(screen);
        connect(qApp, &QGuiApplication::screenAdded, this, &MetricsTest::slotScreenAdded);
        connect(qApp, &QGuiApplication::primaryScreenChanged, this, &MetricsTest::slotPrimaryScreenChanged);
        connect(qApp, &QGuiApplication::screenRemoved, this, &MetricsTest::slotScreenRemoved);

        setWindowTitle(formatWindowTitle("Screens"));
        m_logTimer.start();
        logMessage(briefFormatScreens());

        // Resize to roughly match the metrics text.
        const auto metrics = QFontMetrics(m_textEdit->font(), m_textEdit);
        const int width = 10 + metrics.horizontalAdvance(QStringLiteral("X")) * 50;
        const int height = 40 + metrics.height() * (10 + 8 * screens.size());
        resize(width, height);
    }

    void setVisible(bool visible) override
    {
        QWidget::setVisible(visible);
        if (visible && !m_screenChangedConnected) {
            m_screenChangedConnected = true;
            QObject::connect(windowHandle(), &QWindow::screenChanged,
                             this, &MetricsTest::screenChanged);
            updateMetrics();
        }
    }

    void updateMetrics()
    {
        QString text;
        QTextStream str(&text);

        auto currentScreen = windowHandle()->screen();
        const auto screens = QGuiApplication::screens();
        for (int i = 0, size = screens.size(); i < size; ++i) {
            auto screen = screens.at(i);
            auto platformScreen = screen->handle();
            str << "Screen #" << i << " \"" << screen->name() << '"';
            if (screen == currentScreen)
                str << " [current]";
            if (screen == QGuiApplication::primaryScreen())
                str << " [primary]";
            str << "\n  screen geometry: " << screen->geometry()
                << "\n  platform screen geometry: " << platformScreen->geometry()
                << "\n  platform screen logicalDpi: " << platformScreen->logicalDpi().first;

#ifdef HAVE_SCREEN_BASE_DPI
            str << "\n  platform screen logicalBaseDpi: " << platformScreen->logicalBaseDpi().first;
#endif
            str << "\n  platform screen devicePixelRatio: " <<platformScreen->devicePixelRatio()
                << "\n  platform screen physicalDpi: " << screen->physicalDotsPerInch()
                << "\n\n";
        }

        str << "widget devicePixelRatio: " << this->devicePixelRatioF()
            << "\nwidget logicalDpi: " << this->logicalDpiX()
            << "\n\nQT_FONT_DPI: " << qgetenv("QT_FONT_DPI")
            << "\nQT_SCALE_FACTOR: " << qgetenv("QT_SCALE_FACTOR")
            << "\nQT_ENABLE_HIGHDPI_SCALING: " << qgetenv("QT_ENABLE_HIGHDPI_SCALING")
            << "\nQT_SCREEN_SCALE_FACTORS: " << qgetenv("QT_SCREEN_SCALE_FACTORS")
            << "\nQT_USE_PHYSICAL_DPI: " << qgetenv("QT_USE_PHYSICAL_DPI")
            << "\nQT_SCALE_FACTOR_ROUNDING_POLICY: " << qgetenv("QT_SCALE_FACTOR_ROUNDING_POLICY")
            << "\nQT_DPI_ADJUSTMENT_POLICY: " << qgetenv("QT_DPI_ADJUSTMENT_POLICY")
            << '\n';

        m_textEdit->setPlainText(text);
    }

private slots:
    void screenChanged()
    {
        const QString message = QLatin1String("screenChanged ") + windowHandle()->screen()->name();
        qInfo("%s", qPrintable(message));
        logMessage(message);
        updateMetrics();
    }

    void slotScreenAdded(QScreen *);
    void slotScreenRemoved(QScreen *);
    void slotPrimaryScreenChanged(QScreen *);
    void slotScreenGeometryChanged(const QRect &geometry)
    { logScreenChangeSignal(sender(), "geometry", geometry); }
    void slotScreenAvailableGeometryChanged(const QRect &geometry)
    { logScreenChangeSignal(sender(), "availableGeometry", geometry); }
    void slotScreenPhysicalSizeChanged(const QSizeF &size)
    { logScreenChangeSignal(sender(), "physicalSize", size); }
    void slotScreenPhysicalDotsPerInchChanged(qreal dpi)
    { logScreenChangeSignal(sender(), "physicalDotsPerInch", dpi); }
    void slotScreenLogicalDotsPerInchChanged(qreal dpi)
    { logScreenChangeSignal(sender(), "logicalDotsPerInch", dpi); }
    void slotScreenVirtualGeometryChanged(const QRect &rect)
    { logScreenChangeSignal(sender(), "virtualGeometry", rect); }
    void slotScreenPrimaryOrientationChanged(Qt::ScreenOrientation orientation)
    { logScreenChangeSignal(sender(), "primaryOrientation", orientation); }
    void slotScreenOrientationChanged(Qt::ScreenOrientation orientation)
    { logScreenChangeSignal(sender(), "orientation", orientation); }
    void slotScreenRefreshRateChanged(qreal refreshRate)
    { logScreenChangeSignal(sender(), "refreshRate", refreshRate); }

private:
    QPlainTextEdit *addTextPage(const QString &title);
    void logMessage(const QString &);
    void connectScreenChangeSignals(QScreen *s);
    static QString briefFormatScreens();
    template <class T>
    void logScreenChangeSignal(const QObject *o, const char *name, const T &value);

    QPlainTextEdit *m_textEdit;
    QPlainTextEdit *m_logEdit;
    QElapsedTimer m_logTimer;
    bool m_screenChangedConnected = false;
};

void MetricsTest::slotScreenAdded(QScreen *screen)
{
    logMessage(QLatin1String("Added ") + screen->name() + QLatin1Char(' ')
               + briefFormatScreens());
    connectScreenChangeSignals(screen);
}

void MetricsTest::slotScreenRemoved(QScreen *screen)
{
    logMessage(QLatin1String("Removed ") + screen->name() + QLatin1Char(' ')
               + briefFormatScreens());
}

void MetricsTest::slotPrimaryScreenChanged(QScreen *screen)
{
    logMessage(QLatin1String("PrimaryScreenChanged ") + screen->name() + QLatin1Char(' ')
               + briefFormatScreens());
}

QPlainTextEdit *MetricsTest::addTextPage(const QString &title)
{
    auto result = new QPlainTextEdit(this);
    result->setReadOnly(true);
    result->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    addTab(result, title);
    return result;
}

void MetricsTest::logMessage(const QString &m)
{
    const QString timeStamp =
        QStringLiteral("%1ms: %2").arg(m_logTimer.elapsed(), 6, 10, QLatin1Char('0')).arg(m);
    m_logEdit->appendPlainText(timeStamp);
}

void MetricsTest::connectScreenChangeSignals(QScreen *s)
{
    connect(s, &QScreen::geometryChanged, this, &MetricsTest::slotScreenGeometryChanged);
    connect(s, &QScreen::availableGeometryChanged, this, &MetricsTest::slotScreenAvailableGeometryChanged);
    connect(s, &QScreen::physicalSizeChanged, this, &MetricsTest::slotScreenPhysicalSizeChanged);
    connect(s, &QScreen::physicalDotsPerInchChanged, this, &MetricsTest::slotScreenPhysicalDotsPerInchChanged);
    connect(s, &QScreen::logicalDotsPerInchChanged, this, &MetricsTest::slotScreenLogicalDotsPerInchChanged);
    connect(s, &QScreen::virtualGeometryChanged, this, &MetricsTest::slotScreenVirtualGeometryChanged);
    connect(s, &QScreen::primaryOrientationChanged, this, &MetricsTest::slotScreenPrimaryOrientationChanged);
    connect(s, &QScreen::orientationChanged, this, &MetricsTest::slotScreenOrientationChanged);
    connect(s, &QScreen::refreshRateChanged, this, &MetricsTest::slotScreenRefreshRateChanged);
}

QString MetricsTest::briefFormatScreens()
{
    QString message;
    QTextStream str(&message);
    const auto screens = QGuiApplication::screens();
    for (int i = 0, size = screens.size(); i < size; ++i) {
        str << (i ? ", " : "(");
        str << screens.at(i)->name() << " " << screens.at(i)->geometry();
    }
    str << ')';
    return message;
}

template <class T>
void MetricsTest::logScreenChangeSignal(const QObject *o, const char *name, const T &value)
{
    auto screen = qobject_cast<const QScreen *>(o);
    QString message;
    QTextStream(&message) << (screen ? screen->name() : QString()) << ' ' << name << " changed: " << value;
    logMessage(message);
}

int main(int argc, char **argv)
{
#define NOSCALINGOPTION "noscaling"
#define SCALINGOPTION "scaling"

    qInfo("High DPI tester %s", QT_VERSION_STR);

    int preAppOptionCount = 0;
    for (int a = 1; a < argc; ++a) {
        if (qstrcmp(argv[a], "--" NOSCALINGOPTION) == 0) {
            QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
            preAppOptionCount++;
            qInfo("AA_DisableHighDpiScaling");
        } else if (qstrcmp(argv[a], "--" SCALINGOPTION) == 0) {
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
            preAppOptionCount++;
            qInfo("AA_EnableHighDpiScaling");
        }
    }

    QApplication app(argc, argv);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("High DPI tester. Pass one or more of the options to\n"
                                     "test various high-dpi aspects. \n"
                                     "--interactive is a special option and opens a configuration"
                                     " window.");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption controllerOption("interactive", "Show configuration window.");
    parser.addOption(controllerOption);
    parser.addOption(QCommandLineOption(NOSCALINGOPTION, "Set AA_DisableHighDpiScaling"));
    parser.addOption(QCommandLineOption(SCALINGOPTION, "Set AA_EnableHighDpiScaling"));

    DemoContainerList demoList;
    demoList << new DemoContainer<PixmapPainter>("pixmap", "Test pixmap painter");
    demoList << new DemoContainer<TiledPixmapPainter>("tiledpixmap", "Test tiled pixmap painter");
    demoList << new DemoContainer<Labels>("label", "Test Labels");
    demoList << new DemoContainer<MainWindow>("mainwindow", "Test QMainWindow");
    demoList << new DemoContainer<StandardIcons>("standard-icons", "Test standard icons");
    demoList << new DemoContainer<Caching>("caching", "Test caching");
    demoList << new DemoContainer<Style>("styles", "Test style");
    demoList << new DemoContainer<Fonts>("fonts", "Test fonts");
    demoList << new DemoContainer<IconDrawing>("icondrawing", "Test icon drawing");
    demoList << new DemoContainer<Buttons>("buttons", "Test buttons");
    demoList << new DemoContainer<LinePainter>("linepainter", "Test line painting");
    demoList << new DemoContainer<DragWidget>("draganddrop", "Test drag and drop");
    demoList << new DemoContainer<CursorTester>("cursorpos", "Test cursor and window positioning");
    demoList << new DemoContainer<ScreenDisplayer>("screens", "Test screen and window positioning");
    demoList << new DemoContainer<PhysicalSizeTest>("physicalsize", "Test manual highdpi support using physicalDotsPerInch");
    demoList << new DemoContainer<GraphicsViewCaching>("graphicsview", "Test QGraphicsView caching");
    demoList << new DemoContainer<MetricsTest>("metrics", "Show screen metrics");

    for (DemoContainerBase *demo : qAsConst(demoList))
        parser.addOption(demo->option());

    parser.process(app);

    //controller takes ownership of all demos
    DemoController controller(demoList, &parser);

    if (parser.isSet(controllerOption) || (QCoreApplication::arguments().count() - preAppOptionCount) <= 1)
        controller.show();

    if (QApplication::topLevelWidgets().isEmpty())
        parser.showHelp(0);

    return app.exec();
}

#include "main.moc"
