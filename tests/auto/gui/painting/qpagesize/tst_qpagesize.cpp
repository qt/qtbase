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

#include <QtTest/QtTest>
#include <QtGlobal>
#include <QtAlgorithms>
#include <QtGui/QPageSize>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif // Q_OS_WIN

class tst_QPageSize : public QObject
{
    Q_OBJECT

private slots:
    void basics();
    void fuzzy();
    void custom();
    void statics();
};

void tst_QPageSize::basics()
{
    // Invalid
    QPageSize invalid = QPageSize(QPageSize::Custom);
    QCOMPARE(invalid.isValid(), false);
    invalid = QPageSize(QSize());
    QCOMPARE(invalid.isValid(), false);
    invalid = QPageSize(QSizeF(), QPageSize::Millimeter);
    QCOMPARE(invalid.isValid(), false);

    // Simple QPageSize::PaperSizeId
    QPageSize a4 = QPageSize(QPageSize::A4);
    QCOMPARE(a4.isValid(), true);
    QCOMPARE(a4.key(), QString("A4"));
    QCOMPARE(a4.name(), QString("A4"));
    QCOMPARE(a4.id(), QPageSize::A4);
#ifdef Q_OS_WIN
    QCOMPARE(a4.windowsId(), DMPAPER_A4);
#else
    QCOMPARE(a4.windowsId(), 9); // DMPAPER_A4
#endif
    QCOMPARE(a4.definitionSize(), QSizeF(210, 297));
    QCOMPARE(a4.definitionUnits(), QPageSize::Millimeter);
    QCOMPARE(a4.size(QPageSize::Millimeter), QSizeF(210, 297));
    QCOMPARE(a4.size(QPageSize::Inch), QSizeF(8.27, 11.69));
    QCOMPARE(a4.size(QPageSize::Pica), QSizeF(49.58, 70.17));
    QCOMPARE(a4.sizePoints(), QSize(595, 842));
    QCOMPARE(a4.sizePixels(72), QSize(595, 842));
    QCOMPARE(a4.rect(QPageSize::Millimeter), QRectF(0, 0, 210, 297));
    QCOMPARE(a4.rect(QPageSize::Inch), QRectF(0, 0, 8.27, 11.69));
    QCOMPARE(a4.rect(QPageSize::Pica), QRectF(0, 0, 49.58, 70.17));
    QCOMPARE(a4.rectPoints(), QRect(0, 0, 595, 842));
    QCOMPARE(a4.rectPixels(72), QRect(0, 0, 595, 842));

    // Simple QPageSize::PaperSizeId later in list
    QPageSize folio = QPageSize(QPageSize::Folio);
    QCOMPARE(folio.isValid(), true);
    QCOMPARE(folio.key(), QString("Folio"));
    QCOMPARE(folio.name(), QString("Folio (8.27 x 13 in)"));
    QCOMPARE(folio.id(), QPageSize::Folio);
    QCOMPARE(folio.definitionSize(), QSizeF(210, 330));
    QCOMPARE(folio.definitionUnits(), QPageSize::Millimeter);
    QCOMPARE(folio.size(QPageSize::Millimeter), QSizeF(210, 330));
    QCOMPARE(folio.sizePoints(), QSize(595, 935));
    QCOMPARE(folio.sizePixels(72), QSize(595, 935));
    QCOMPARE(folio.size(QPageSize::Inch), QSizeF(8.27, 13));

    // Simple QPageSize::PaperSizeId last in list
    QPageSize you4 = QPageSize(QPageSize::EnvelopeYou4);
    QCOMPARE(you4.isValid(), true);
    QCOMPARE(you4.key(), QString("EnvYou4"));
    QCOMPARE(you4.name(), QString("Envelope You 4"));
    QCOMPARE(you4.id(), QPageSize::EnvelopeYou4);
#ifdef Q_OS_WIN
    QCOMPARE(you4.windowsId(), DMPAPER_JENV_YOU4);
#else
    QCOMPARE(you4.windowsId(), 91);
#endif
    QCOMPARE(you4.size(QPageSize::Millimeter), QSizeF(105,  235));
    QCOMPARE(you4.size(QPageSize::Inch), QSizeF(4.13, 9.25));
    QCOMPARE(you4.sizePoints(), QSize(298,  666));
    QCOMPARE(you4.sizePixels(72), QSize(298,  666));

    // Simple QSize in Points
    QPageSize a4b = QPageSize(QSize(595, 842));
    QCOMPARE(a4b.isValid(), true);
    QCOMPARE(a4b.id(), QPageSize::A4);
    QCOMPARE(a4b.sizePoints(), QSize(595, 842));

    // Simple QSize in Points later in list, custom name
    QPageSize folio2 = QPageSize(QSize(595, 935), QStringLiteral("Folio2"));
    QCOMPARE(folio2.isValid(), true);
    QCOMPARE(folio2.name(), QString("Folio2"));
    QCOMPARE(folio2.id(), QPageSize::Folio);
    QCOMPARE(folio2.sizePoints(), QSize(595, 935));

    // Comparisons
    QCOMPARE((a4 == folio), false);
    QCOMPARE((a4 != folio), true);
    QCOMPARE((a4.isEquivalentTo(folio)), false);
    QCOMPARE((a4 == a4b), true);
    QCOMPARE((a4 != a4b), false);
    QCOMPARE((a4.isEquivalentTo(a4b)), true);
    QCOMPARE((folio == folio2), false);  // Name different
    QCOMPARE((folio != folio2), true);  // Name different
    QCOMPARE((folio.isEquivalentTo(folio2)), true);

    // Simple QSize in Millimeters
    QPageSize folio3 = QPageSize(QSizeF(210, 330), QPageSize::Millimeter);
    QCOMPARE(folio3.isValid(), true);
    QCOMPARE(folio3.id(), QPageSize::Folio);
    QCOMPARE(folio3.sizePoints(), QSize(595, 935));
}

void tst_QPageSize::fuzzy()
{
    // Using FuzzyMatch by default

    // Simple QSize within 3 Points
    QPageSize a4a = QPageSize(QSize(592, 845));
    QCOMPARE(a4a.isValid(), true);
    QCOMPARE(a4a.id(), QPageSize::A4);
    QCOMPARE(a4a.sizePoints(), QSize(595, 842));

    // Simple QSizeF within 1mm
    QPageSize a4b = QPageSize(QSizeF(211, 298), QPageSize::Millimeter);
    QCOMPARE(a4b.isValid(), true);
    QCOMPARE(a4b.id(), QPageSize::A4);
    QCOMPARE(a4b.size(QPageSize::Millimeter), QSizeF(210, 297));
    QCOMPARE(a4b.sizePoints(), QSize(595, 842));

    // Using FuzzyOrientationMatch

    // Exact A4 in landscape mode
    QPageSize a4l = QPageSize(QSize(842, 595));
    QCOMPARE(a4l.isValid(), true);
    QCOMPARE(a4l.id(), QPageSize::Custom);
    QCOMPARE(a4l.sizePoints(), QSize(842, 595));

    a4l = QPageSize(QSize(842, 595), QString(), QPageSize::FuzzyOrientationMatch);
    QCOMPARE(a4l.isValid(), true);
    QCOMPARE(a4l.id(), QPageSize::A4);
    QCOMPARE(a4l.sizePoints(), QSize(595, 842));

    // Using ExactMatch

    // Simple QSize within 3 Points
    QPageSize a4d = QPageSize(QSize(592, 845), QString(), QPageSize::ExactMatch);
    QCOMPARE(a4d.isValid(), true);
    QCOMPARE(a4d.id(), QPageSize::Custom);
    QCOMPARE(a4d.sizePoints(), QSize(592, 845));

    // Simple QSizeF within 1mm
    QPageSize a4e = QPageSize(QSizeF(211, 298), QPageSize::Millimeter, QString(), QPageSize::ExactMatch);
    QCOMPARE(a4e.isValid(), true);
    QCOMPARE(a4e.id(), QPageSize::Custom);
    QCOMPARE(a4e.size(QPageSize::Millimeter), QSizeF(211, 298));
    QCOMPARE(a4e.sizePoints(), QSize(598, 845));
}

void tst_QPageSize::custom()
{
    // Simple non-standard Points QSize
    QPageSize custom1 = QPageSize(QSize(500, 600));
    QCOMPARE(custom1.isValid(), true);
    QCOMPARE(custom1.key(), QString("Custom.500x600"));
    QCOMPARE(custom1.name(), QString("Custom (500pt x 600pt)"));
    QCOMPARE(custom1.id(), QPageSize::Custom);
    QCOMPARE(custom1.definitionSize(), QSizeF(500, 600));
    QCOMPARE(custom1.definitionUnits(), QPageSize::Point);
    QCOMPARE(custom1.size(QPageSize::Millimeter), QSizeF(176.39, 211.67));
    QCOMPARE(custom1.size(QPageSize::Pica), QSizeF(41.67, 50));
    QCOMPARE(custom1.sizePoints(), QSize(500, 600));
    QCOMPARE(custom1.sizePixels(72), QSize(500, 600));

    // Simple non-standard MM QSizeF
    QPageSize custom2 = QPageSize(QSizeF(500.3, 600.57), QPageSize::Millimeter);
    QCOMPARE(custom2.isValid(), true);
    QCOMPARE(custom2.key(), QString("Custom.500.3x600.57mm"));
    QCOMPARE(custom2.name(), QString("Custom (500.3mm x 600.57mm)"));
    QCOMPARE(custom2.id(), QPageSize::Custom);
    QCOMPARE(custom2.definitionSize(), QSizeF(500.3, 600.57));
    QCOMPARE(custom2.definitionUnits(), QPageSize::Millimeter);
    QCOMPARE(custom2.size(QPageSize::Millimeter), QSizeF(500.3, 600.57));
    QCOMPARE(custom2.size(QPageSize::Pica), QSizeF(118.18, 141.87));
    QCOMPARE(custom2.sizePoints(), QSize(1418, 1702));
    QCOMPARE(custom2.sizePixels(72), QSize(1418, 1702));
}

void tst_QPageSize::statics()
{
    QCOMPARE(QPageSize::key(QPageSize::EnvelopeYou4), QString("EnvYou4"));
    QCOMPARE(QPageSize::name(QPageSize::EnvelopeYou4), QString("Envelope You 4"));

#ifdef Q_OS_WIN
    QCOMPARE(QPageSize::windowsId(QPageSize::EnvelopeYou4), DMPAPER_JENV_YOU4);
    QCOMPARE(QPageSize::id(DMPAPER_JENV_YOU4), QPageSize::EnvelopeYou4);
    QCOMPARE(QPageSize::id(DMPAPER_A4_ROTATED), QPageSize::A4);
#else
    QCOMPARE(QPageSize::windowsId(QPageSize::EnvelopeYou4), 91);
    QCOMPARE(QPageSize::id(91), QPageSize::EnvelopeYou4);
    QCOMPARE(QPageSize::id(77), QPageSize::A4);
#endif

    QCOMPARE(QPageSize::id(QSize(298,  666)), QPageSize::EnvelopeYou4);
    QCOMPARE(QPageSize::id(QSizeF(105,  235), QPageSize::Millimeter), QPageSize::EnvelopeYou4);

    QCOMPARE(QPageSize::definitionSize(QPageSize::Letter), QSizeF(8.5, 11));
    QCOMPARE(QPageSize::definitionUnits(QPageSize::Letter), QPageSize::Inch);
    QCOMPARE(QPageSize::definitionSize(QPageSize::EnvelopeYou4), QSizeF(105,  235));
    QCOMPARE(QPageSize::definitionUnits(QPageSize::EnvelopeYou4), QPageSize::Millimeter);

    QCOMPARE(QPageSize::size(QPageSize::EnvelopeYou4, QPageSize::Millimeter), QSizeF(105,  235));
    QCOMPARE(QPageSize::sizePoints(QPageSize::EnvelopeYou4), QSize(298,  666));
    QCOMPARE(QPageSize::sizePixels(QPageSize::EnvelopeYou4, 72), QSize(298,  666));
}

QTEST_APPLESS_MAIN(tst_QPageSize)

#include "tst_qpagesize.moc"
