/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cppextractimages.h"
#include "cppwriteicondata.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"
#include "uic.h"

#include <qdatastream.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>

QT_BEGIN_NAMESPACE

namespace CPP {

ExtractImages::ExtractImages(const Option &opt)
    : m_output(0), m_option(opt)
{
}

void ExtractImages::acceptUI(DomUI *node)
{
    if (!m_option.extractImages)
        return;

    if (node->elementImages() == 0)
        return;

    QString className = node->elementClass() + m_option.postfix;

    QFile f;
    if (m_option.qrcOutputFile.size()) {
        f.setFileName(m_option.qrcOutputFile);
        if (!f.open(QIODevice::WriteOnly | QFile::Text)) {
            fprintf(stderr, "%s: Error: Could not create resource file\n", qPrintable(m_option.messagePrefix()));
            return;
        }

        QFileInfo fi(m_option.qrcOutputFile);
        QDir dir = fi.absoluteDir();
        if (!dir.exists(QLatin1String("images")) && !dir.mkdir(QLatin1String("images"))) {
            fprintf(stderr, "%s: Error: Could not create image dir\n", qPrintable(m_option.messagePrefix()));
            return;
        }
        dir.cd(QLatin1String("images"));
        m_imagesDir = dir;

        m_output = new QTextStream(&f);
        m_output->setCodec(QTextCodec::codecForName("UTF-8"));

        QTextStream &out = *m_output;

        out << "<RCC>\n";
        out << "    <qresource prefix=\"/" << className << "\" >\n";
        TreeWalker::acceptUI(node);
        out << "    </qresource>\n";
        out << "</RCC>\n";

        f.close();
        delete m_output;
        m_output = 0;
    }
}

void ExtractImages::acceptImages(DomImages *images)
{
    TreeWalker::acceptImages(images);
}

void ExtractImages::acceptImage(DomImage *image)
{
    QString format = image->elementData()->attributeFormat();
    QString extension = format.left(format.indexOf(QLatin1Char('.'))).toLower();
    QString fname = m_imagesDir.absoluteFilePath(image->attributeName() + QLatin1Char('.') + extension);

    *m_output << "        <file>images/" << image->attributeName() << QLatin1Char('.') + extension << "</file>\n";

    QFile f;
    f.setFileName(fname);
    const bool isXPM_GZ = format == QLatin1String("XPM.GZ");
    QIODevice::OpenMode openMode = QIODevice::WriteOnly;
    if (isXPM_GZ)
        openMode |= QIODevice::Text;
    if (!f.open(openMode)) {
        fprintf(stderr, "%s: Error: Could not create image file %s: %s",
                qPrintable(m_option.messagePrefix()),
                qPrintable(fname), qPrintable(f.errorString()));
        return;
    }

    if (isXPM_GZ) {
        QTextStream *imageOut = new QTextStream(&f);
        imageOut->setCodec(QTextCodec::codecForName("UTF-8"));

        CPP::WriteIconData::writeImage(*imageOut, QString(), m_option.limitXPM_LineLength, image);
        delete imageOut;
    } else {
        CPP::WriteIconData::writeImage(f, image);
    }

    f.close();
}

} // namespace CPP

QT_END_NAMESPACE
