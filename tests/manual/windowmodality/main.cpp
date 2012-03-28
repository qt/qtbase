/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "ui_dialog.h"
#include "ui_widget.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>

class Dialog : public QDialog, public Ui::Dialog
{
    Q_OBJECT
public:
    Dialog(QWidget *parent = 0)
        : QDialog(parent)
    {
        setupUi(this);
        connect(this, SIGNAL(finished(int)), SLOT(dialogFinished(int)));
        connect(this, SIGNAL(accepted()), SLOT(dialogAccepted()));
        connect(this, SIGNAL(rejected()), SLOT(dialogRejected()));
    }

private slots:
    void on_modelessButton_clicked()
    { newDialog(Qt::NonModal, this); }
    void on_modelessNoParentButton_clicked()
    { newDialog(Qt::NonModal, 0); }
    void on_windowModalButton_clicked()
    { newDialog(Qt::WindowModal, this); }
    void on_windowModalNoParentButton_clicked()
    { newDialog(Qt::WindowModal, 0); }
    void on_windowModalChildButton_clicked()
    { newChildWidget(Qt::WindowModal); }
    void on_siblingWindowModalButton_clicked()
    { newDialog(Qt::WindowModal, parentWidget()); }
    void on_applicationModalButton_clicked()
    { newDialog(Qt::ApplicationModal, this); }
    void on_applicationModalNoParentButton_clicked()
    { newDialog(Qt::ApplicationModal, 0); }
    void on_applicationModalChildButton_clicked()
    { newChildWidget(Qt::ApplicationModal); }
    void on_siblingApplicationModalButton_clicked()
    { newDialog(Qt::ApplicationModal, parentWidget()); }

    void dialogFinished(int result)
    { qDebug() << "Dialog finished, result" << result; }
    void dialogAccepted()
    { qDebug() << "Dialog accepted"; }
    void dialogRejected()
    { qDebug() << "Dialog rejected"; }

private:
    void newDialog(Qt::WindowModality windowModality, QWidget *parent)
    {
        Dialog *dialog = new Dialog(parent);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowModality(windowModality);
        if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked())
            dialog->exec();
        else
            dialog->show();
    }
    void newChildWidget(Qt::WindowModality windowModality)
    {
        QWidget *w = new QWidget(this);
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setWindowModality(windowModality);
        w->setGeometry(0, 0, 0, 0);
        w->show();
        QTimer::singleShot(5000, w, SLOT(close()));
    }
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::WindowBlocked)
            setPalette(Qt::darkGray);
        else if (event->type() == QEvent::WindowUnblocked)
            setPalette(QPalette());
        return QWidget::event(event);
    }
};

class Widget : public QWidget, public Ui::Widget
{
    Q_OBJECT
public:
    Widget(QWidget *parent = 0)
        : QWidget(parent)
    {
        setupUi(this);
    }

private slots:
    void on_windowButton_clicked()
    { (new Widget)->show(); }
    void on_groupLeaderButton_clicked()
    {
        Widget *w = new Widget;
        w->setAttribute(Qt::WA_GroupLeader);
        w->show();
    }
    void on_modelessButton_clicked()
    { newDialog(Qt::NonModal); }
    void on_modelessNoParentButton_clicked()
    { newDialog(Qt::NonModal, false); }
    void on_windowModalButton_clicked()
    { newDialog(Qt::WindowModal); }
    void on_windowModalNoParentButton_clicked()
    { newDialog(Qt::WindowModal, false); }
    void on_windowModalChildButton_clicked()
    { newChildWidget(Qt::WindowModal); }
    void on_applicationModalButton_clicked()
    { newDialog(Qt::ApplicationModal); }
    void on_applicationModalNoParentButton_clicked()
    { newDialog(Qt::ApplicationModal, false); }
    void on_applicationModalChildButton_clicked()
    { newChildWidget(Qt::ApplicationModal); }

private:
    void newDialog(Qt::WindowModality windowModality, bool withParent = true)
    {
        Dialog *dialog = new Dialog(withParent ? this : 0);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowModality(windowModality);
        if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked())
            dialog->exec();
        else
            dialog->show();
    }
    void newChildWidget(Qt::WindowModality windowModality)
    {
        QWidget *w = new QWidget(this);
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setWindowModality(windowModality);
        w->setGeometry(0, 0, 0, 0);
        w->show();
        QTimer::singleShot(5000, w, SLOT(close()));
    }
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::WindowBlocked)
            setPalette(Qt::darkGray);
        else if (event->type() == QEvent::WindowUnblocked)
            setPalette(QPalette());
        return QWidget::event(event);
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Widget widget;
    widget.show();
    return app.exec();
}

#include "main.moc"
