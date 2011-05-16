#include "window.h"

#include <private/qguiapplication_p.h>
#include <private/qwindowsurface_p.h>

#include <QPainter>

static int colorIndexId = 0;

QColor colorTable[] =
{
    QColor("#f09f8f"),
    QColor("#a2bff2"),
    QColor("#c0ef8f")
};

Window::Window(QWindow *parent)
    : QWindow(parent)
    , m_backgroundColorIndex(colorIndexId++)
{
    setSurfaceType(RasterSurface);
    setWindowTitle(QLatin1String("Window"));

    if (parent)
        setGeometry(QRect(160, 120, 320, 240));
    else {
        setGeometry(QRect(10, 10, 640, 480));

        setSizeIncrement(QSize(10, 10));
        setBaseSize(QSize(640, 480));
        setMinimumSize(QSize(240, 160));
        setMaximumSize(QSize(800, 600));
    }

    create();
    QGuiApplicationPrivate::platformIntegration()->createWindowSurface(this, winId());

    m_image = QImage(geometry().size(), QImage::Format_RGB32);
    m_image.fill(colorTable[m_backgroundColorIndex % (sizeof(colorTable) / sizeof(colorTable[0]))].rgba());

    m_lastPos = QPoint(-1, -1);

    render();
}

void Window::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
}

void Window::mouseMoveEvent(QMouseEvent *event)
{
    if (m_lastPos != QPoint(-1, -1)) {
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(m_lastPos, event->pos());
        m_lastPos = event->pos();
    }

    render();
}

void Window::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_lastPos != QPoint(-1, -1)) {
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(m_lastPos, event->pos());
        m_lastPos = QPoint(-1, -1);
    }

    render();
}

void Window::resizeEvent(QResizeEvent *)
{
    QImage old = m_image;

    int width = qMax(geometry().width(), old.width());
    int height = qMax(geometry().height(), old.height());

    if (width > old.width() || height > old.height()) {
        m_image = QImage(width, height, QImage::Format_RGB32);
        m_image.fill(colorTable[(m_backgroundColorIndex) % (sizeof(colorTable) / sizeof(colorTable[0]))].rgba());

        QPainter p(&m_image);
        p.drawImage(0, 0, old);
    }

    render();
}

void Window::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Backspace:
        m_text.chop(1);
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        m_text.append('\n');
        break;
    default:
        m_text.append(event->text());
        break;
    }
    render();
}

void Window::render()
{
    QRect rect(QPoint(), geometry().size());
    surface()->resize(rect.size());

    surface()->beginPaint(rect);

    QPaintDevice *device = surface()->paintDevice();

    QPainter p(device);
    p.drawImage(0, 0, m_image);

    QFont font;
    font.setPixelSize(32);

    p.setFont(font);
    p.drawText(rect, 0, m_text);

    surface()->endPaint(rect);
    surface()->flush(this, rect, QPoint());
}


