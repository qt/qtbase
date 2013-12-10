/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QPAGESIZE_H
#define QPAGESIZE_H

#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

#if defined(B0)
#undef B0 // Terminal hang-up.  We assume that you do not want that.
#endif

class QPageSizePrivate;
class QString;
class QSize;
class QSizeF;

class Q_GUI_EXPORT QPageSize
{
public:

    // ### Qt6 Re-order and remove duplicates
    // NOTE: Must keep in sync with QPagedPrintEngine and QPrinter
    enum PageSizeId {
        // Existing Qt sizes
        A4,
        B5,
        Letter,
        Legal,
        Executive,
        A0,
        A1,
        A2,
        A3,
        A5,
        A6,
        A7,
        A8,
        A9,
        B0,
        B1,
        B10,
        B2,
        B3,
        B4,
        B6,
        B7,
        B8,
        B9,
        C5E,
        Comm10E,
        DLE,
        Folio,
        Ledger,
        Tabloid,
        Custom,

        // New values derived from PPD standard
        A10,
        A3Extra,
        A4Extra,
        A4Plus,
        A4Small,
        A5Extra,
        B5Extra,

        JisB0,
        JisB1,
        JisB2,
        JisB3,
        JisB4,
        JisB5,
        JisB6,
        JisB7,
        JisB8,
        JisB9,
        JisB10,

        // AnsiA = Letter,
        // AnsiB = Ledger,
        AnsiC,
        AnsiD,
        AnsiE,
        LegalExtra,
        LetterExtra,
        LetterPlus,
        LetterSmall,
        TabloidExtra,

        ArchA,
        ArchB,
        ArchC,
        ArchD,
        ArchE,

        Imperial7x9,
        Imperial8x10,
        Imperial9x11,
        Imperial9x12,
        Imperial10x11,
        Imperial10x13,
        Imperial10x14,
        Imperial12x11,
        Imperial15x11,

        ExecutiveStandard,
        Note,
        Quarto,
        Statement,
        SuperA,
        SuperB,
        Postcard,
        DoublePostcard,
        Prc16K,
        Prc32K,
        Prc32KBig,

        FanFoldUS,
        FanFoldGerman,
        FanFoldGermanLegal,

        EnvelopeB4,
        EnvelopeB5,
        EnvelopeB6,
        EnvelopeC0,
        EnvelopeC1,
        EnvelopeC2,
        EnvelopeC3,
        EnvelopeC4,
        // EnvelopeC5 = C5E,
        EnvelopeC6,
        EnvelopeC65,
        EnvelopeC7,
        // EnvelopeDL = DLE,

        Envelope9,
        // Envelope10 = Comm10E,
        Envelope11,
        Envelope12,
        Envelope14,
        EnvelopeMonarch,
        EnvelopePersonal,

        EnvelopeChou3,
        EnvelopeChou4,
        EnvelopeInvite,
        EnvelopeItalian,
        EnvelopeKaku2,
        EnvelopeKaku3,
        EnvelopePrc1,
        EnvelopePrc2,
        EnvelopePrc3,
        EnvelopePrc4,
        EnvelopePrc5,
        EnvelopePrc6,
        EnvelopePrc7,
        EnvelopePrc8,
        EnvelopePrc9,
        EnvelopePrc10,
        EnvelopeYou4,

        // Last item, with commonly used synynoms from QPagedPrintEngine / QPrinter
        LastPageSize = EnvelopeYou4,
        NPageSize = LastPageSize,
        NPaperSize = LastPageSize,

        // Convenience overloads for naming consistency
        AnsiA = Letter,
        AnsiB = Ledger,
        EnvelopeC5 = C5E,
        EnvelopeDL = DLE,
        Envelope10 = Comm10E
    };

    // NOTE: Must keep in sync with QPageLayout::Unit and QPrinter::Unit
    enum Unit {
        Millimeter,
        Point,
        Inch,
        Pica,
        Didot,
        Cicero
    };

    enum SizeMatchPolicy {
        FuzzyMatch,
        FuzzyOrientationMatch,
        ExactMatch
    };

    QPageSize();
    explicit QPageSize(QPageSize::PageSizeId pageSizeId);
    QPageSize(const QSize &pointSize,
              const QString &name = QString(),
              QPageSize::SizeMatchPolicy matchPolicy = QPageSize::FuzzyMatch);
    QPageSize(const QSizeF &size, QPageSize::Unit units,
              const QString &name = QString(),
              QPageSize::SizeMatchPolicy matchPolicy = QPageSize::FuzzyMatch);
    QPageSize(const QPageSize &other);
    ~QPageSize();

    QPageSize &operator=(const QPageSize &other);
#ifdef Q_COMPILER_RVALUE_REFS
    QPageSize &operator=(QPageSize &&other) { swap(other); return *this; }
#endif

    void swap(QPageSize &other) { d.swap(other.d); }

    bool operator==(const QPageSize &other) const;
    bool isEquivalentTo(const QPageSize &other) const;

    bool isValid() const;

    QString key() const;
    QString name() const;

    QPageSize::PageSizeId id() const;

    int windowsId() const;

    QSizeF definitionSize() const;
    QPageSize::Unit definitionUnits() const;

    QSizeF size(QPageSize::Unit units) const;
    QSize sizePoints() const;
    QSize sizePixels(int resolution) const;

    QRectF rect(QPageSize::Unit units) const;
    QRect rectPoints() const;
    QRect rectPixels(int resolution) const;

    static QString key(QPageSize::PageSizeId pageSizeId);
    static QString name(QPageSize::PageSizeId pageSizeId);

    static QPageSize::PageSizeId id(const QSize &pointSize,
                                    QPageSize::SizeMatchPolicy matchPolicy = QPageSize::FuzzyMatch);
    static QPageSize::PageSizeId id(const QSizeF &size, QPageSize::Unit units,
                                    QPageSize::SizeMatchPolicy matchPolicy = QPageSize::FuzzyMatch);

    static QPageSize::PageSizeId id(int windowsId);
    static int windowsId(QPageSize::PageSizeId pageSizeId);

    static QSizeF definitionSize(QPageSize::PageSizeId pageSizeId);
    static QPageSize::Unit definitionUnits(QPageSize::PageSizeId pageSizeId);

    static QSizeF size(QPageSize::PageSizeId pageSizeId, QPageSize::Unit units);
    static QSize sizePoints(QPageSize::PageSizeId pageSizeId);
    static QSize sizePixels(QPageSize::PageSizeId pageSizeId, int resolution);

private:
    friend class QPageSizePrivate;
    friend class QPlatformPrintDevice;
    QPageSize(const QString &key, const QSize &pointSize, const QString &name);
    QPageSize(int windowsId, const QSize &pointSize, const QString &name);
    QPageSize(QPageSizePrivate &dd);
    QSharedDataPointer<QPageSizePrivate> d;
};

Q_DECLARE_SHARED(QPageSize)

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QPageSize &pageSize);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QPageSize)
Q_DECLARE_METATYPE(QPageSize::PageSizeId)
Q_DECLARE_METATYPE(QPageSize::Unit)

#endif // QPAGESIZE_H
