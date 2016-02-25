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

#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QtWidgets/QOpenGLWidget>
#include <QtGui/QVector3D>

class OpenGLWidgetPrivate;
class OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    OpenGLWidget(int interval = 30, const QVector3D &rotAxis = QVector3D(0, 1, 0), QWidget *parent = 0);
    ~OpenGLWidget();

    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void setClearColor(const float *c);

private:
    QScopedPointer<OpenGLWidgetPrivate> d;
};

#endif // OPENGLWIDGET_H
