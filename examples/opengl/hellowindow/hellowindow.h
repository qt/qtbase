#include <QWindow>

#include <QtOpenGL/qgl.h>
#include <QtOpenGL/qglshaderprogram.h>

#include <QTime>

class QGuiGLContext;

class Renderer : public QObject
{
public:
    Renderer();

    QSurfaceFormat format() const;

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
    QGLShaderProgram program;
    int vertexAttr;
    int normalAttr;
    int matrixUniform;
    int colorUniform;

    bool m_initialized;
    QSurfaceFormat m_format;
    QGuiGLContext *m_context;
};

class HelloWindow : public QWindow
{
    Q_OBJECT
public:
    HelloWindow(Renderer *renderer);

private slots:
    void render();

protected:
    void mousePressEvent(QMouseEvent *);

private:
    void updateColor();

    int m_colorIndex;
    QColor m_color;
    Renderer *m_renderer;
};
