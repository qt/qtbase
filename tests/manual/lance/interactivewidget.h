// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef INTERACTIVEWIDGET_H
#define INTERACTIVEWIDGET_H

#include "widgets.h"
#include "paintcommands.h"

#include <QMainWindow>

#include <private/qmath_p.h>

QT_FORWARD_DECLARE_CLASS(QToolBox)

class InteractiveWidget : public QMainWindow
{
    Q_OBJECT
public:
    InteractiveWidget();

public slots:
    void run();
    void load();
    void load(const QString &fname);
    void save();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

protected slots:
    void cmdSelected(QListWidgetItem *item);
    void enumSelected(QListWidgetItem *item);

private:
    QToolBox *m_commandsToolBox;
    QToolBox *m_enumsToolBox;
    OnScreenWidget<QWidget> *m_onScreenWidget;
    QTextEdit *ui_textEdit;
    QString m_filename;
};

#endif
