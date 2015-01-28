/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ARTHURWIDGETS_H
#define ARTHURWIDGETS_H

#include "arthurstyle.h"
#include <QBitmap>
#include <QPushButton>
#include <QGroupBox>

#if defined(QT_OPENGL_SUPPORT)
#include <QGLWidget>
#include <QEvent>
class GLWidget : public QGLWidget
{
public:
    GLWidget(QWidget *parent)
        : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
    {
        setAttribute(Qt::WA_AcceptTouchEvents);
    }
    void disableAutoBufferSwap() { setAutoBufferSwap(false); }
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE { parentWidget()->update(); }
protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE
    {
        switch (event->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
            event->ignore();
            return false;
            break;
        default:
            break;
        }
        return QGLWidget::event(event);
    }
};
#endif

QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

class ArthurFrame : public QWidget
{
    Q_OBJECT
public:
    ArthurFrame(QWidget *parent);
    virtual void paint(QPainter *) {}


    void paintDescription(QPainter *p);

    void loadDescription(const QString &filename);
    void setDescription(const QString &htmlDesc);

    void loadSourceFile(const QString &fileName);

    bool preferImage() const { return m_prefer_image; }

#if defined(QT_OPENGL_SUPPORT)
    QGLWidget *glWidget() const { return glw; }
#endif

public slots:
    void setPreferImage(bool pi) { m_prefer_image = pi; }
    void setDescriptionEnabled(bool enabled);
    void showSource();

#if defined(QT_OPENGL_SUPPORT)
    void enableOpenGL(bool use_opengl);
    bool usesOpenGL() { return m_use_opengl; }
#endif

signals:
    void descriptionEnabledChanged(bool);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;

#if defined(QT_OPENGL_SUPPORT)
    GLWidget *glw;
    bool m_use_opengl;
#endif
    QPixmap m_tile;

    bool m_show_doc;
    bool m_prefer_image;
    QTextDocument *m_document;

    QString m_sourceFileName;

};

#endif
