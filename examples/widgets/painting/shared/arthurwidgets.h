// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ARTHURWIDGETS_H
#define ARTHURWIDGETS_H

#include "arthurstyle.h"
#include <QBitmap>
#include <QPushButton>
#include <QGroupBox>

QT_FORWARD_DECLARE_CLASS(QOpenGLWindow)
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

    bool preferImage() const { return m_preferImage; }
#if QT_CONFIG(opengl)
    QOpenGLWindow *glWindow() const { return m_glWindow; }
#endif

public slots:
    void setPreferImage(bool pi) { m_preferImage = pi; }
    void setDescriptionEnabled(bool enabled);
    void showSource();

#if QT_CONFIG(opengl)
    void enableOpenGL(bool use_opengl);
    bool usesOpenGL() { return m_use_opengl; }
#endif

signals:
    void descriptionEnabledChanged(bool);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;

#if QT_CONFIG(opengl)
    virtual void createGlWindow();
    QOpenGLWindow *m_glWindow = nullptr;
    QWidget *m_glWidget = nullptr;
    bool m_use_opengl = false;
#endif
    QPixmap m_tile;

    bool m_showDoc = false;
    bool m_preferImage = false;
    QTextDocument *m_document = nullptr;;

    QString m_sourceFileName;
};

#endif
