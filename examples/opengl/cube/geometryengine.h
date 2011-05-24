#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLShaderProgram>

class GeometryEngine : protected QGLFunctions
{
public:
    GeometryEngine();
    virtual ~GeometryEngine();

    void init();

    void drawCubeGeometry(QGLShaderProgram *program);

private:
    void initCubeGeometry();

    GLuint *vboIds;

};

#endif // GEOMETRYENGINE_H
