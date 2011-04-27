#include <QWindow>

#include <QtOpenGL/qgl.h>
#include <QtOpenGL/qglshaderprogram.h>

#include <QTime>

class HelloWindow : public QWindow
{
    Q_OBJECT
public:
    HelloWindow();

protected:
    void mousePressEvent(QMouseEvent *);

private slots:
    void render();

private:
    void initialize();
    void updateColor();

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
    uint colorIndex;
};
