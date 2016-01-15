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
    bool eventFilter(QObject *o, QEvent *e);

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
