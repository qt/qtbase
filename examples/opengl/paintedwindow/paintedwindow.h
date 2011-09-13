#include <QWindow>

#include <QtGui/qopengl.h>
#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopenglframebufferobject.h>

#include <QColor>
#include <QTime>

class QOpenGLContext;

class PaintedWindow : public QWindow
{
    Q_OBJECT
public:
    PaintedWindow();

private slots:
    void paint();

private:
    void resizeEvent(QResizeEvent *);
    void exposeEvent(QExposeEvent *);

    QOpenGLContext *m_context;
};
