// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ICONPREVIEWAREA_H
#define ICONPREVIEWAREA_H

#include <QIcon>
#include <QWidget>
#include <QStringList>
#include <QList>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

//! [0]
class IconPreviewArea : public QWidget
{
    Q_OBJECT

public:
    explicit IconPreviewArea(QWidget *parent = nullptr);

    void setIcon(const QIcon &icon);
    void setSize(const QSize &size);

    static QList<QIcon::Mode> iconModes();
    static QList<QIcon::State> iconStates();
    static QStringList iconModeNames();
    static QStringList iconStateNames();

private:
    QLabel *createHeaderLabel(const QString &text);
    QLabel *createPixmapLabel();
    void updatePixmapLabels();

    enum { NumModes = 4, NumStates = 2 };

    QIcon icon;
    QSize size;
    QLabel *stateLabels[NumStates];
    QLabel *modeLabels[NumModes];
    QLabel *pixmapLabels[NumModes][NumStates];
};
//! [0]

#endif
