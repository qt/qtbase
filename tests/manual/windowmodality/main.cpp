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

#include "ui_dialog.h"
#include "ui_widget.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QPageSetupDialog>
#include <QPrintDialog>

enum DialogType
{
    CustomDialogType,
    ColorDialogType,
    FileDialogType,
    FontDialogType,
    PageSetupDialogType,
    PrintDialogType
};

class CustomDialog : public QDialog, public Ui::Dialog
{
    Q_OBJECT
public:
    CustomDialog(QWidget *parent = 0)
        : QDialog(parent)
    {
        setupUi(this);

        // hide the "Create new dialogs as siblings of this dialog" button when
        // we don't have a parent of our own (they would be parentless anyway)
        if (!parent) {
            createSiblingDialogCheckBox->setChecked(false);
            createSiblingDialogCheckBox->setVisible(false);
        }
    }

private slots:
    void on_modelessCustomDialogButton_clicked()
    { newDialog(CustomDialogType, Qt::NonModal); }
    void on_modelessColorDialogButton_clicked()
    { newDialog(ColorDialogType, Qt::NonModal); }
    void on_modelessFontDialogButton_clicked()
    { newDialog(FontDialogType, Qt::NonModal); }

    void on_windowModalCustomDialogButton_clicked()
    { newDialog(CustomDialogType, Qt::WindowModal); }
    void on_windowModalColorDialogButton_clicked()
    { newDialog(ColorDialogType, Qt::WindowModal); }
    void on_windowModalFileDialogButton_clicked()
    { newDialog(FileDialogType, Qt::WindowModal); }
    void on_windowModalFontDialogButton_clicked()
    { newDialog(FontDialogType, Qt::WindowModal); }
    void on_windowModalPageSetupDialogButton_clicked()
    { newDialog(PageSetupDialogType, Qt::WindowModal); }
    void on_windowModalPrintDialogButton_clicked()
    { newDialog(PrintDialogType, Qt::WindowModal); }

    void on_applicationModalCustomDialogButton_clicked()
    { newDialog(CustomDialogType, Qt::ApplicationModal); }
    void on_applicationModalColorDialogButton_clicked()
    { newDialog(ColorDialogType, Qt::ApplicationModal); }
    void on_applicationModalFileDialogButton_clicked()
    { newDialog(FileDialogType, Qt::ApplicationModal); }
    void on_applicationModalFontDialogButton_clicked()
    { newDialog(FontDialogType, Qt::ApplicationModal); }
    void on_applicationModalPageSetupDialogButton_clicked()
    { newDialog(PageSetupDialogType, Qt::ApplicationModal); }
    void on_applicationModalPrintDialogButton_clicked()
    { newDialog(PrintDialogType, Qt::ApplicationModal); }

private:
    void newDialog(DialogType dialogType, Qt::WindowModality windowModality)
    {
        QWidget *parent = 0;
        if (useThisAsParentCheckBox->isChecked())
            parent = this;
        else if (createSiblingDialogCheckBox->isChecked())
            parent = parentWidget();

        QDialog *dialog;
        switch (dialogType) {
        case CustomDialogType:
            dialog = new CustomDialog(parent);
            break;
        case ColorDialogType:
            if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked()) {
                QColorDialog::getColor(Qt::white, parent);
                return;
            }
            dialog = new QColorDialog(parent);
            break;
        case FileDialogType:
            if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked()) {
                QFileDialog::getOpenFileName(parent);
                return;
            }
            dialog = new QFileDialog(parent);
            break;
        case FontDialogType:
            if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked()) {
                bool unused = false;
                QFontDialog::getFont(&unused, parent);
                return;
            }
            dialog = new QFontDialog(parent);
            break;
        case PageSetupDialogType:
            dialog = new QPageSetupDialog(parent);
            break;
        case PrintDialogType:
            dialog = new QPrintDialog(parent);
            break;
        }

        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowModality(windowModality);

        if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked())
            dialog->exec();
        else if (windowModality == Qt::WindowModal)
            dialog->open();
        else
            dialog->show();
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

    void on_modelessCustomDialogButton_clicked()
    { newDialog(CustomDialogType, Qt::NonModal); }
    void on_modelessColorDialogButton_clicked()
    { newDialog(ColorDialogType, Qt::NonModal); }
    void on_modelessFontDialogButton_clicked()
    { newDialog(FontDialogType, Qt::NonModal); }

    void on_windowModalCustomDialogButton_clicked()
    { newDialog(CustomDialogType, Qt::WindowModal); }
    void on_windowModalColorDialogButton_clicked()
    { newDialog(ColorDialogType, Qt::WindowModal); }
    void on_windowModalFileDialogButton_clicked()
    { newDialog(FileDialogType, Qt::WindowModal); }
    void on_windowModalFontDialogButton_clicked()
    { newDialog(FontDialogType, Qt::WindowModal); }
    void on_windowModalPageSetupDialogButton_clicked()
    { newDialog(PageSetupDialogType, Qt::WindowModal); }
    void on_windowModalPrintDialogButton_clicked()
    { newDialog(PrintDialogType, Qt::WindowModal); }

    void on_applicationModalCustomDialogButton_clicked()
    { newDialog(CustomDialogType, Qt::ApplicationModal); }
    void on_applicationModalColorDialogButton_clicked()
    { newDialog(ColorDialogType, Qt::ApplicationModal); }
    void on_applicationModalFileDialogButton_clicked()
    { newDialog(FileDialogType, Qt::ApplicationModal); }
    void on_applicationModalFontDialogButton_clicked()
    { newDialog(FontDialogType, Qt::ApplicationModal); }
    void on_applicationModalPageSetupDialogButton_clicked()
    { newDialog(PageSetupDialogType, Qt::ApplicationModal); }
    void on_applicationModalPrintDialogButton_clicked()
    { newDialog(PrintDialogType, Qt::ApplicationModal); }

private:
    void newDialog(DialogType dialogType, Qt::WindowModality windowModality)
    {
        QWidget *parent = 0;
        if (useThisAsParentCheckBox->isChecked())
            parent = this;

        QDialog *dialog;
        switch (dialogType) {
        case CustomDialogType:
            dialog = new CustomDialog(parent);
            break;
        case ColorDialogType:
            if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked()) {
                QColorDialog::getColor(Qt::white, parent);
                return;
            }
            dialog = new QColorDialog(parent);
            break;
        case FileDialogType:
            if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked()) {
                QFileDialog::getOpenFileName(parent);
                return;
            }
            dialog = new QFileDialog(parent);
            break;
        case FontDialogType:
            if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked()) {
                bool unused = false;
                QFontDialog::getFont(&unused, parent);
                return;
            }
            dialog = new QFontDialog(parent);
            break;
        case PageSetupDialogType:
            dialog = new QPageSetupDialog(parent);
            break;
        case PrintDialogType:
            dialog = new QPrintDialog(parent);
            break;
        }

        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowModality(windowModality);

        if (windowModality == Qt::ApplicationModal && applicationModalUseExecCheckBox->isChecked())
            dialog->exec();
        else if (windowModality == Qt::WindowModal)
            dialog->open();
        else
            dialog->show();
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
