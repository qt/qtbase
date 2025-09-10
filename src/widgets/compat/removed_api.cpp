// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_WIDGETS_BUILD_REMOVED_API

#include "qtwidgetsglobal.h"

QT_USE_NAMESPACE

#if QT_WIDGETS_REMOVED_SINCE(6, 3)

#if QT_CONFIG(menu)
#include "qmenu.h"

QAction *QMenu::addAction(const QString &text)
{
    return QWidget::addAction(text);
}

QAction *QMenu::addAction(const QIcon &icon, const QString &text)
{
    return QWidget::addAction(icon, text);
}
#endif

#if !QT_CONFIG(shortcut)
// the overloads taking QKeySequence as a trailing argument are deprecated, not removed,
// so remained in qmenu.cpp
QAction *QMenu::addAction(const QString &text, const QObject *receiver, const char* member)
{
    return QWidget::addAction(text, receiver, member);
}

QAction *QMenu::addAction(const QIcon &icon, const QString &text,
                          const QObject *receiver, const char* member)
{
    return QWidget::addAction(icon, text, receiver, member);
}
#endif

#if QT_CONFIG(toolbar)
#include "qtoolbar.h"

QAction *QToolBar::addAction(const QString &text)
{
    return QWidget::addAction(text);
}

QAction *QToolBar::addAction(const QIcon &icon, const QString &text)
{
    return QWidget::addAction(icon, text);
}

QAction *QToolBar::addAction(const QString &text,
                             const QObject *receiver, const char* member)
{
    return QWidget::addAction(text, receiver, member);
}

QAction *QToolBar::addAction(const QIcon &icon, const QString &text,
                             const QObject *receiver, const char* member)
{
    return QWidget::addAction(icon, text, receiver, member);
}
#endif

#if QT_CONFIG(menubar)
#include "qmenubar.h"

QAction *QMenuBar::addAction(const QString &text)
{
    return QWidget::addAction(text);
}

QAction *QMenuBar::addAction(const QString &text, const QObject *receiver, const char* member)
{
    return QWidget::addAction(text, receiver, member);
}
#endif

#endif // QT_WIDGETS_REMOVED_SINCE(6, 3)

#if QT_WIDGETS_REMOVED_SINCE(6, 7)


#if QT_CONFIG(filedialog)
#include "qfiledialog.h"

void QFileDialog::getOpenFileContent(const QString &nameFilter, const std::function<void(const QString &, const QByteArray &)> &fileOpenCompleted)
{
    QFileDialog::getOpenFileContent(nameFilter, fileOpenCompleted, nullptr);
}

void QFileDialog::saveFileContent(const QByteArray &fileContent, const QString &fileNameHint)
{
    saveFileContent(fileContent, fileNameHint, nullptr);
}
#endif

// #include <qotherheader.h>
// // implement removed functions from qotherheader.h

#endif // QT_WIDGETS_REMOVED_SINCE(6, 7)
