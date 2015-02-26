/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qringbuffer_p.h>
#include <QByteArray>

#include <qtest.h>

class tst_qringbuffer : public QObject
{
    Q_OBJECT
private slots:
    void reserveAndRead();
    void free();
};

void tst_qringbuffer::reserveAndRead()
{
    QRingBuffer ringBuffer;
    QBENCHMARK {
        for (qint64 i = 1; i < 256; ++i)
            ringBuffer.reserve(i);

        for (qint64 i = 1; i < 256; ++i)
            ringBuffer.read(0, i);
    }
}

void tst_qringbuffer::free()
{
    QRingBuffer ringBuffer;
    QBENCHMARK {
        ringBuffer.reserve(4096);
        ringBuffer.reserve(2048);
        ringBuffer.append(QByteArray("01234", 5));

        ringBuffer.free(1);
        ringBuffer.free(4096);
        ringBuffer.free(48);
        ringBuffer.free(2000);
    }
}

QTEST_MAIN(tst_qringbuffer)

#include "main.moc"
