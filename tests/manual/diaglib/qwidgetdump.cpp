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

#include "qwidgetdump.h"

#include <QWidget>
#if QT_VERSION > 0x050000
#  include <QtGui/QScreen>
#  include <QtGui/QWindow>
#endif
#include <QApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

namespace QtDiag {

static const char *qtWidgetClasses[] = {
    "QAbstractItemView", "QAbstractScrollArea", "QAbstractSlider", "QAbstractSpinBox",
    "QCalendarWidget", "QCheckBox", "QColorDialog", "QColumnView", "QComboBox",
    "QCommandLinkButton", "QDateEdit", "QDateTimeEdit", "QDesktopWidget", "QDial",
    "QDialog", "QDialogButtonBox", "QDockWidget", "QDoubleSpinBox", "QErrorMessage",
    "QFileDialog", "QFontComboBox", "QFontDialog", "QFrame", "QGraphicsView",
    "QGroupBox", "QHeaderView", "QInputDialog", "QLCDNumber", "QLabel", "QLineEdit",
    "QListView", "QListWidget", "QMainWindow", "QMdiArea", "QMdiSubWindow", "QMenu",
    "QMenuBar", "QMessageBox", "QOpenGLWidget", "QPlainTextEdit", "QProgressBar",
    "QProgressDialog", "QPushButton", "QRadioButton", "QRubberBand", "QScrollArea",
    "QScrollBar", "QSlider", "QSpinBox", "QSplashScreen", "QSplitter",
    "QStackedWidget", "QStatusBar", "QTabBar", "QTabWidget", "QTableView",
    "QTableWidget", "QTextBrowser", "QTextEdit", "QTimeEdit", "QToolBar",
    "QToolBox", "QToolButton", "QTreeView", "QTreeWidget", "QWidget",
    "QWizard", "QWizardPage"
};

static bool isQtWidget(const char *className)
{
    for (size_t i = 0, count = sizeof(qtWidgetClasses) / sizeof(qtWidgetClasses[0]); i < count; ++i) {
        if (!qstrcmp(className, qtWidgetClasses[i]))
            return true;
    }
    return false;
}

static void formatWidgetClass(QTextStream &str, const QWidget *w)
{
    const QMetaObject *mo = w->metaObject();
    str << mo->className();
    while (!isQtWidget(mo->className())) {
        mo = mo->superClass();
        str << ':' << mo->className();
    }
    const QString on = w->objectName();
    if (!on.isEmpty())
        str << "/\"" << on << '"';
}

static void dumpWidgetRecursion(QTextStream &str, const QWidget *w,
                                FormatWindowOptions options, int depth = 0)
{
    indentStream(str, 2 * depth);
    formatWidgetClass(str, w);
    str << ' ' << (w->isVisible() ? "[visible] " : "[hidden] ");
    if (const WId nativeWinId = w->internalWinId())
        str << "[native: " << hex << showbase << nativeWinId << dec << noshowbase << "] ";
    if (w->isWindow())
        str << "[top] ";
    str << (w->testAttribute(Qt::WA_Mapped) ? "[mapped] " : "[not mapped] ");
    if (w->testAttribute(Qt::WA_DontCreateNativeAncestors))
        str << "[NoNativeAncestors] ";
    if (const int states = w->windowState())
        str << "windowState=" << hex << showbase << states << dec << noshowbase << ' ';
    formatRect(str, w->geometry());
    if (w->isWindow()) {
        str << ' ' << w->logicalDpiX() << "DPI";
#if QT_VERSION > 0x050600
        const qreal dpr = w->devicePixelRatioF();
        if (!qFuzzyCompare(dpr, qreal(1)))
            str << " dpr=" << dpr;
#endif // Qt 5.6
        const QRect normalGeometry = w->normalGeometry();
        if (normalGeometry.isValid() && !normalGeometry.isEmpty() && normalGeometry != w->geometry()) {
            str << " normal=";
            formatRect(str, w->normalGeometry());
        }
    }
    if (!(options & DontPrintWindowFlags)) {
        str << ' ';
        formatWindowFlags(str, w->windowFlags());
    }
    if (options & PrintSizeConstraints) {
        str << ' ';
        const QSize minimumSize = w->minimumSize();
        if (minimumSize.width() > 0 || minimumSize.height() > 0)
            str << "minimumSize=" << minimumSize.width() << 'x' << minimumSize.height() << ' ';
        const QSize sizeHint = w->sizeHint();
        const QSize minimumSizeHint = w->minimumSizeHint();
        if (minimumSizeHint.isValid() && !(sizeHint.isValid() && minimumSizeHint == sizeHint))
            str << "minimumSizeHint=" << minimumSizeHint.width() << 'x' << minimumSizeHint.height() << ' ';
        if (sizeHint.isValid())
            str << "sizeHint=" << sizeHint.width() << 'x' << sizeHint.height() << ' ';
        const QSize maximumSize = w->maximumSize();
        if (maximumSize.width() < QWIDGETSIZE_MAX || maximumSize.height() < QWIDGETSIZE_MAX)
            str << "maximumSize=" << maximumSize.width() << 'x' << maximumSize.height() << ' ';
    }
    str << '\n';
#if QT_VERSION > 0x050000
    if (const QWindow *win = w->windowHandle()) {
        indentStream(str, 2 * (1 + depth));
        formatWindow(str, win, options);
        str << '\n';
    }
#endif // Qt 5
    foreach (const QObject *co, w->children()) {
        if (co->isWidgetType())
            dumpWidgetRecursion(str, static_cast<const QWidget *>(co), options, depth + 1);
    }
}

void dumpAllWidgets(FormatWindowOptions options, const QWidget *root)
{
    QString d;
    QTextStream str(&d);
    str << "### QWidgets:\n";
    QWidgetList topLevels;
    if (root)
        topLevels.append(const_cast<QWidget *>(root));
    else
        topLevels = QApplication::topLevelWidgets();
    foreach (QWidget *tw, topLevels)
        dumpWidgetRecursion(str, tw, options);
#if QT_VERSION >= 0x050400
    {
        foreach (const QString &line, d.split(QLatin1Char('\n')))
            qDebug().noquote() << line;
    }
#else
    qDebug("%s", qPrintable(d));
#endif
}

} // namespace QtDiag
