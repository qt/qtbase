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

    QOpenGLContext *m_context;
    QOpenGLFramebufferObject *m_fbo;
    QOpenGLShaderProgram *m_program;

    GLuint m_vertexAttribute;
    GLuint m_texCoordsAttribute;
};
