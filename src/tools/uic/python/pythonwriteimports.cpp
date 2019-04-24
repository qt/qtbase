/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "pythonwriteimports.h"

#include <customwidgetsinfo.h>
#include <uic.h>

#include <ui4.h>

#include <QtCore/qtextstream.h>

QT_BEGIN_NAMESPACE

static const char *standardImports =
R"I(from PySide2.QtCore import (QCoreApplication, QMetaObject, QObject, QPoint,
    QRect, QSize, QUrl, Qt)
from PySide2.QtGui import (QBrush, QColor, QConicalGradient, QFont,
    QFontDatabase, QIcon, QLinearGradient, QPalette, QPainter, QPixmap,
    QRadialGradient)
from PySide2.QtWidgets import *
)I";

namespace Python {

WriteImports::WriteImports(Uic *uic) : m_uic(uic)
{
}

void WriteImports::acceptUI(DomUI *node)
{
    auto &output = m_uic->output();
    output << standardImports << '\n';
    if (auto customWidgets = node->elementCustomWidgets()) {
        TreeWalker::acceptCustomWidgets(customWidgets);
        output << '\n';
    }
}

QString WriteImports::qtModuleOf(const DomCustomWidget *node) const
{
    if (m_uic->customWidgetsInfo()->extends(node->elementClass(), QLatin1String("QAxWidget")))
        return QStringLiteral("QtAxContainer");
    if (const auto headerElement = node->elementHeader()) {
        const auto &header = headerElement->text();
        if (header.startsWith(QLatin1String("Qt"))) {
            const int slash = header.indexOf(QLatin1Char('/'));
            if (slash != -1)
                return header.left(slash);
        }
    }
    return QString();
}

void WriteImports::acceptCustomWidget(DomCustomWidget *node)
{
    const auto &className = node->elementClass();
    if (className.contains(QLatin1String("::")))
        return; // Exclude namespaced names (just to make tests pass).
    const QString &qtModule = qtModuleOf(node);
    auto &output = m_uic->output();
    if (!qtModule.isEmpty())
        output << "from PySide2." << qtModule << ' ';
    output << "import " << className << '\n';
}

} // namespace Python

QT_END_NAMESPACE
