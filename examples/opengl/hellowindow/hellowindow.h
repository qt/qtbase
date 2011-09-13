#include <QWindow>

#include <QtGui/qopengl.h>
#include <QtGui/qopenglshaderprogram.h>

#include <QColor>
#include <QTime>

class QOpenGLContext;

class Renderer : public QObject
{
    Q_OBJECT
public:
    Renderer(const QSurfaceFormat &format, Renderer *share = 0);

    QSurfaceFormat format() const { return m_format; }

public slots:
    void render(QSurface *surface, const QColor &color, const QSize &viewSize);

private:
    void initialize();

    qreal m_fAngle;
    bool m_showBubbles;
    void paintQtLogo();
    void createGeometry();
    void createBubbles(int number);
    void quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4);
    void extrude(qreal x1, qreal y1, qreal x2, qreal y2);
    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    int vertexAttr;
    int normalAttr;
    int matrixUniform;
    int colorUniform;

    bool m_initialized;
    QSurfaceFormat m_format;
    QOpenGLContext *m_context;
    QOpenGLShaderProgram *m_program;
};

class HelloWindow : public QWindow
{
    Q_OBJECT
public:
    HelloWindow(Renderer *renderer);

    void updateColor();

signals:
    void needRender(QSurface *surface, const QColor &color, const QSize &viewSize);

private slots:
    void render();

private:
    void mousePressEvent(QMouseEvent *);

    int m_colorIndex;
    QColor m_color;
    Renderer *m_renderer;
};
