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

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QSharedPointer>
#include <QToolBar>

#include <QBitmap>
#include <QCursor>
#include <QDrag>
#include <QPainter>
#include <QPixmap>

#include <QDebug>
#include <QMimeData>
#include <QStringList>
#include <QTextStream>

#if QT_VERSION > 0x050000
#  include <QScreen>
#  include <QWindow>
#  include <private/qhighdpiscaling_p.h>
#  include <qpa/qplatformwindow.h>
#else
#   define Q_NULLPTR 0
#   define Q_DECL_OVERRIDE
#endif

#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

#include <algorithm>
#include <iterator>

#if QT_VERSION < 0x050000
QDebug operator<<(QDebug d, const QPixmap &p)
{
    d.nospace() << "QPixmap(" << p.size() << ')';
    return d;
}
#endif // Qt 4

// High DPI cursor test for testing cursor sizes in multi-screen setups.
// It creates one widget per screen with a grid of standard cursors,
// pixmap / bitmap cursors and pixmap / bitmap cursors with device pixel ratio 2.
// On the left, there is a ruler with 10 DIP marks.
// The code is meant to compile with Qt 4 also.

static QString screenInfo(const QWidget *w)
{
    QString result;
    QTextStream str(&result);
#if QT_VERSION > 0x050000
    QScreen *screen = Q_NULLPTR;
    if (const QWindow *window = w->windowHandle())
        screen = window->screen();
    if (screen) {
        str << '"' << screen->name() << "\" " << screen->size().width() << 'x'
            << screen->size().height() << ", DPR=" << screen->devicePixelRatio()
            << ", " << screen->logicalDotsPerInchX() << "DPI ";
        if (QHighDpiScaling::isActive())
            str << ", factor=" << QHighDpiScaling::factor(screen);
        else
            str << ", no scaling";
    } else {
        str << "<null>";
    }
#else
    QDesktopWidget *desktop = QApplication::desktop();
    int screenNumber = desktop->screenNumber(w);
    str << "Screen #" <<screenNumber << ' ' << desktop->screenGeometry(screenNumber).width()
        << 'x' << desktop->screenGeometry(screenNumber).height() << " PD: " << w->logicalDpiX() << "DPI";
#endif
#ifdef Q_OS_WIN
    str << ", SM_C_CURSOR: " << GetSystemMetrics(SM_CXCURSOR) << 'x' << GetSystemMetrics(SM_CYCURSOR);
#endif
    return result;
}

// Helpers for painting pixmaps and creating cursors
static QPixmap paintPixmap(int size, QColor c)
{
    QPixmap result(size, size);
    result.fill(c);
    QPainter p(&result);
    p.drawRect(QRect(QPoint(0, 0), result.size() - QSize(1, 1)));
    p.drawLine(0, 0, size, size);
    p.drawLine(0, size, size, 0);
    return result;
}

static QCursor pixmapCursor(int size)
{
    QCursor result(paintPixmap(size, Qt::red), size / 2, size / 2);
    return result;
}

static QPair<QBitmap, QBitmap> paintBitmaps(int size)
{
    QBitmap bitmap(size, size);
    bitmap.fill(Qt::color1);
    QBitmap mask(size, size);
    mask.fill(Qt::color1);
    {
        QPainter mp(&mask);
        mp.fillRect(QRect(0, 0, size / 2, size / 2), Qt::color0);
    }
    return QPair<QBitmap, QBitmap>(bitmap, mask);
}

static QCursor bitmapCursor(int size)
{
    QPair<QBitmap, QBitmap> bitmaps = paintBitmaps(size);
    return QCursor(bitmaps.first, bitmaps.second, size / 2, size / 2);
}

#if QT_VERSION > 0x050000
static QCursor pixmapCursorDevicePixelRatio(int size, int dpr)
{
    QPixmap pixmap = paintPixmap(dpr * size, Qt::yellow);
    pixmap.setDevicePixelRatio(dpr);
    return QCursor(pixmap, size / 2, size / 2);
}

static QCursor bitmapCursorDevicePixelRatio(int size, int dpr)
{
    QPair<QBitmap, QBitmap> bitmaps = paintBitmaps(dpr * size);
    bitmaps.first.setDevicePixelRatio(dpr);
    bitmaps.second.setDevicePixelRatio(dpr);
    return QCursor(bitmaps.first, bitmaps.second, size / 2, size / 2);
}
#endif // Qt 5

// A label from which a pixmap can be dragged for testing drag with pixmaps/DPR.
class DraggableLabel : public QLabel {
public:
    explicit DraggableLabel(const QPixmap &p, const QString &text, QWidget *parent = Q_NULLPTR)
        : QLabel(text, parent), m_pixmap(p)
    {
        setToolTip(QLatin1String("Click to drag away the pixmap. Press Shift to set a circular mask."));
    }

protected:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    const QPixmap m_pixmap;
};

void DraggableLabel::mousePressEvent(QMouseEvent *)
{
    QMimeData *mimeData = new QMimeData;
    mimeData->setImageData(QVariant::fromValue(m_pixmap));
    QDrag *drag = new QDrag(this);
    QPixmap pixmap = m_pixmap;
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        QBitmap mask(pixmap.width(), pixmap.height());
        mask.clear();
        QPainter painter(&mask);
        painter.setBrush(Qt::color1);
        const int hx = pixmap.width() / 2;
        const int hy = pixmap.width() / 2;
        painter.drawEllipse(QPoint(hx, hy), hx, hy);
        pixmap.setMask(mask);
    }
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);
    QPoint sizeP = QPoint(m_pixmap.width(), m_pixmap.height());
#if QT_VERSION > 0x050000
    sizeP /= int(m_pixmap.devicePixelRatio());
#endif // Qt 5
    drag->setHotSpot(sizeP / 2);
    qDebug() << "Dragging:" << m_pixmap;
    drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
}

// Vertical ruler widget with 10 px marks
class VerticalRuler : public QWidget {
public:
    VerticalRuler(QWidget *parent = Q_NULLPTR);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
};

VerticalRuler::VerticalRuler(QWidget *parent) : QWidget(parent)
{
    const int screenWidth = screen()->geometry().width();
    setFixedWidth(screenWidth / 48); // 1920 pixel monitor ->40
}

void VerticalRuler::paintEvent(QPaintEvent *)
{
    const QSize sizeS(size());
    const QPoint sizeP(sizeS.width(), sizeS.height());
    const QPoint center = sizeP / 2;
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(0, 0), sizeS), Qt::white);
    painter.drawLine(center.x(), 0, center.x(), sizeP.y());
    for (int y = 0; y < sizeP.y(); y += 10)
        painter.drawLine(center.x() - 5, y, center.x() + 5, y);
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = Q_NULLPTR);
    void updateScreenInfo() { m_screenInfoLabel->setText(screenInfo(this)); }

public slots:
    void screenChanged() { updateScreenInfo(); }

private:
    QLabel *m_screenInfoLabel;
};

static QLabel *createCursorLabel(const QCursor &cursor, const QString &additionalText = QString())
{
    QString labelText;
    QDebug(&labelText).nospace() << cursor.shape();
#if QT_VERSION > 0x050000
    labelText.remove(0, labelText.indexOf('(') + 1);
    labelText.chop(1);
#endif // Qt 5
    if (!additionalText.isEmpty())
        labelText += ' ' + additionalText;
    const QPixmap cursorPixmap = cursor.pixmap();
    QLabel *result = Q_NULLPTR;
    if (cursorPixmap.size().isEmpty()) {
        result = new QLabel(labelText);
        result->setFrameShape(QFrame::Box);
    } else {
        result = new DraggableLabel(cursor.pixmap(), labelText);
        result->setFrameShape(QFrame::StyledPanel);
    }
    result->setCursor(cursor);
    return result;
}

static void addToGrid(QWidget *w, QGridLayout *gridLayout, int columnCount, int &row, int &col)
{
    gridLayout->addWidget(w, row, col);
    if (col >= columnCount) {
        col = 0;
        row++;
    } else {
        col++;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_screenInfoLabel(new QLabel)
{
    QString title = "Cursors ";
#if QT_VERSION > 0x050000
    title += '(' + QGuiApplication::platformName() + ") ";
#endif
    title += QT_VERSION_STR;
    setWindowTitle(title);

    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *quitAction = fileMenu->addAction("Quit");
    quitAction->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    QToolBar *fileToolBar = addToolBar("File");
    fileToolBar->addAction(quitAction);

    QWidget *cw = new QWidget;
    QHBoxLayout *hLayout = new QHBoxLayout(cw);
    hLayout->addWidget(new VerticalRuler(cw));
    QGridLayout *gridLayout = new QGridLayout;
    hLayout->addLayout(gridLayout);

    const int columnCount = 5;
    const int size = 32;

    int row = 0;
    int col = 0;
    for (int i = 0; i < Qt::BitmapCursor; ++i)
        addToGrid(createCursorLabel(QCursor(static_cast<Qt::CursorShape>(i))), gridLayout, columnCount, row, col);

    addToGrid(createCursorLabel(QCursor(pixmapCursor(size)),
                                QLatin1String("Plain PX ") + QString::number(size)),
                                gridLayout, columnCount, row, col);

    addToGrid(createCursorLabel(bitmapCursor(size),
                                QLatin1String("Plain BM ") + QString::number(size)),
                                gridLayout, columnCount, row, col);

#if QT_VERSION > 0x050000
    addToGrid(createCursorLabel(QCursor(pixmapCursorDevicePixelRatio(size, 2)),
                                "PX with DPR 2 " + QString::number(size)),
                                gridLayout, columnCount, row, col);

    addToGrid(createCursorLabel(QCursor(bitmapCursorDevicePixelRatio(size, 2)),
                                "BM with DPR 2 " + QString::number(size)),
                                gridLayout, columnCount, row, col);
#endif // Qt 5

    gridLayout->addWidget(m_screenInfoLabel, row + 1, 0, 1, columnCount);

    setCentralWidget(cw);
}

typedef QSharedPointer<MainWindow> MainWindowPtr;
typedef QList<MainWindowPtr> MainWindowPtrList;

int main(int argc, char *argv[])
{
    QStringList arguments;
    std::copy(argv + 1, argv + argc, std::back_inserter(arguments));

#if QT_VERSION > 0x050000
    if (arguments.contains("-s"))
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    else if (arguments.contains("-n"))
        QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
#endif // Qt 5

    QApplication app(argc, argv);

    MainWindowPtrList windows;
    const int lastScreen = arguments.contains("-p")
        ? 0  // Primary screen only
        : QGuiApplication::screens().size() - 1; // All screens
    for (int s = lastScreen; s >= 0; --s) {
        MainWindowPtr window(new MainWindow());
        const QPoint pos = QGuiApplication::screens().at(s)->geometry().center() - QPoint(200, 100);
        window->move(pos);
        windows.append(window);
        window->show();
        window->updateScreenInfo();
#if QT_VERSION > 0x050000
        QObject::connect(window->windowHandle(), &QWindow::screenChanged,
                         window.data(), &MainWindow::updateScreenInfo);
#endif
    }
    return app.exec();
}
#include "main.moc"
