/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PRINTDIALOGPANEL_H
#define PRINTDIALOGPANEL_H

#ifndef QT_NO_PRINTER

#include "ui_printdialogpanel.h"

#if QT_VERSION >= 0x050300
#include <QPageLayout>
#endif
#include <QPrinter>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPrinter;
class QComboBox;
class QGroupBox;
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE

class PageSizeControl;
class OptionsControl;

#if QT_VERSION < 0x050300
// Copied from class QPageLayout introduced in Qt 5.3
namespace QPageLayout
{
    enum Unit {
        Millimeter,
        Point,
        Inch,
        Pica,
        Didot,
        Cicero
    };

    enum Orientation {
        Portrait,
        Landscape
    };

    enum Mode {
        StandardMode,  // Paint Rect includes margins
        FullPageMode   // Paint Rect excludes margins
    };
}
#endif

class PrintDialogPanel  : public QWidget
{
    Q_OBJECT
public:
    explicit PrintDialogPanel(QWidget *parent = 0);
    ~PrintDialogPanel();

private slots:
    void createPrinter();
    void deletePrinter();
    void showPrintDialog();
    void showPreviewDialog();
    void showPageSetupDialog();
    void directPrint();
    void unitsChanged();
    void pageSizeChanged();
    void pageDimensionsChanged();
    void orientationChanged();
    void marginsChanged();
    void layoutModeChanged();
    void printerChanged();

private:
    QSizeF customPageSize() const;
    void applySettings(QPrinter *printer) const;
    void retrieveSettings(const QPrinter *printer);
    void updatePageLayoutWidgets();
    void enablePanels();

    bool m_blockSignals;
    Ui::PrintDialogPanel m_panel;

#if QT_VERSION >= 0x050300
    QPageLayout m_pageLayout;
#else
    QPrinter m_printerLayout;
    QPrinter::Unit m_units;
#endif
    QScopedPointer<QPrinter> m_printer;
};

#endif // !QT_NO_PRINTER
#endif // PRINTDIALOGPANEL_H
