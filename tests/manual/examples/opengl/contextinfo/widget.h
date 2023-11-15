// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QSurfaceFormat)
QT_FORWARD_DECLARE_CLASS(QSurface)

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);

private slots:
    void start();
    void renderWindowReady();
    void renderWindowError(const QString &msg);

private:
    void addVersions(QLayout *layout);
    void addProfiles(QLayout *layout);
    void addOptions(QLayout *layout);
    void addRenderableTypes(QLayout *layout);
    void addRenderWindow();
    void printFormat(const QSurfaceFormat &format);

    QComboBox *m_version;
    QLayout *m_profiles;
    QLayout *m_options;
    QLayout *m_renderables;
    QTextEdit *m_output;
    QTextEdit *m_extensions;
    QVBoxLayout *m_renderWindowLayout;
    QWidget *m_renderWindowContainer;
    QSurface *m_surface;
};

#endif // WIDGET_H
