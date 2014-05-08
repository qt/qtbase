/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include <QtTest/QtTest>
#include <QtOpenGL/qgl.h>
#include <QtOpenGL/qglbuffer.h>

class tst_QGLBuffer : public QObject
{
    Q_OBJECT
public:
    tst_QGLBuffer() {}
    ~tst_QGLBuffer() {}

private slots:
    void vertexBuffer_data();
    void vertexBuffer();
    void indexBuffer_data();
    void indexBuffer();
    void bufferSharing();

private:
    void testBuffer(QGLBuffer::Type type);
};

void tst_QGLBuffer::vertexBuffer_data()
{
    QTest::addColumn<int>("usagePattern");

    QTest::newRow("StreamDraw") << int(QGLBuffer::StreamDraw);
    QTest::newRow("StaticDraw") << int(QGLBuffer::StaticDraw);
    QTest::newRow("DynamicDraw") << int(QGLBuffer::DynamicDraw);
}

void tst_QGLBuffer::vertexBuffer()
{
    testBuffer(QGLBuffer::VertexBuffer);
}

void tst_QGLBuffer::indexBuffer_data()
{
    vertexBuffer_data();
}

void tst_QGLBuffer::indexBuffer()
{
    testBuffer(QGLBuffer::IndexBuffer);
}

void tst_QGLBuffer::testBuffer(QGLBuffer::Type type)
{
    QFETCH(int, usagePattern);

    QGLWidget w;
    w.makeCurrent();

    // Create the local object, but not the buffer in the server.
    QGLBuffer buffer(type);
    QVERIFY(buffer.usagePattern() == QGLBuffer::StaticDraw);
    buffer.setUsagePattern(QGLBuffer::UsagePattern(usagePattern));

    // Check the initial state.
    QVERIFY(buffer.type() == type);
    QVERIFY(!buffer.isCreated());
    QVERIFY(buffer.bufferId() == 0);
    QVERIFY(buffer.usagePattern() == QGLBuffer::UsagePattern(usagePattern));
    QCOMPARE(buffer.size(), -1);

    // Should not be able to bind it yet because it isn't created.
    QVERIFY(!buffer.bind());

    // Create the buffer - if this fails, then assume that the
    // GL implementation does not support buffers at all.
    if (!buffer.create())
        QSKIP("Buffers are not supported on this platform");

    // Should now have a buffer id.
    QVERIFY(buffer.bufferId() != 0);

    // Bind the buffer and upload some data.
    QVERIFY(buffer.bind());
    static GLfloat const data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    buffer.allocate(data, sizeof(data));

    // Check the buffer size.
    QCOMPARE(buffer.size(), int(sizeof(data)));

    // Map the buffer and read back its contents.
    bool haveMap = false;
    GLfloat *mapped = reinterpret_cast<GLfloat *>
        (buffer.map(QGLBuffer::ReadOnly));
    if (mapped) {
        for (int index = 0; index < 9; ++index)
            QCOMPARE(mapped[index], data[index]);
        buffer.unmap();
        haveMap = true;
    } else {
        qWarning("QGLBuffer::map() is not supported on this platform");
    }

    // Read back the buffer contents using read().
    bool haveRead = false;
    GLfloat readdata[9];
    if (buffer.read(0, readdata, sizeof(readdata))) {
        for (int index = 0; index < 9; ++index)
            QCOMPARE(readdata[index], data[index]);
        haveRead = true;
    } else {
        qWarning("QGLBuffer::read() is not supported on this platform");
    }

    // Write some different data to a specific location and check it.
    static GLfloat const diffdata[] = {11, 12, 13};
    buffer.write(sizeof(GLfloat) * 3, diffdata, sizeof(diffdata));
    if (haveMap) {
        mapped = reinterpret_cast<GLfloat *>(buffer.map(QGLBuffer::ReadOnly));
        QVERIFY(mapped != 0);
        for (int index = 0; index < 9; ++index) {
            if (index >= 3 && index <= 5)
                QCOMPARE(mapped[index], diffdata[index - 3]);
            else
                QCOMPARE(mapped[index], data[index]);
        }
        buffer.unmap();
    }
    if (haveRead) {
        QVERIFY(buffer.read(0, readdata, sizeof(readdata)));
        for (int index = 0; index < 9; ++index) {
            if (index >= 3 && index <= 5)
                QCOMPARE(readdata[index], diffdata[index - 3]);
            else
                QCOMPARE(readdata[index], data[index]);
        }
    }

    // Write to the buffer using the return value from map.
    if (haveMap) {
        mapped = reinterpret_cast<GLfloat *>(buffer.map(QGLBuffer::WriteOnly));
        QVERIFY(mapped != 0);
        mapped[6] = 14;
        buffer.unmap();

        mapped = reinterpret_cast<GLfloat *>(buffer.map(QGLBuffer::ReadOnly));
        QVERIFY(mapped != 0);
        static GLfloat const diff2data[] = {11, 12, 13, 14};
        for (int index = 0; index < 9; ++index) {
            if (index >= 3 && index <= 6)
                QCOMPARE(mapped[index], diff2data[index - 3]);
            else
                QCOMPARE(mapped[index], data[index]);
        }
        buffer.unmap();
    }

    // Resize the buffer.
    buffer.allocate(sizeof(GLfloat) * 20);
    QCOMPARE(buffer.size(), int(sizeof(GLfloat) * 20));
    buffer.allocate(0, sizeof(GLfloat) * 32);
    QCOMPARE(buffer.size(), int(sizeof(GLfloat) * 32));

    // Release the buffer.
    buffer.release();
}

void tst_QGLBuffer::bufferSharing()
{
#if defined(Q_OS_WIN)
    // Needs investigation on Windows: https://bugreports.qt-project.org/browse/QTBUG-29692
    QSKIP("Unreproducible timeout on Windows (MSVC/MinGW) CI bots");
#endif

#if defined(Q_OS_QNX)
    QSKIP("Crashes on QNX when destroying the second QGLWidget (see QTBUG-38275)");
#endif

    QGLWidget *w1 = new QGLWidget();
    w1->makeCurrent();

    QGLWidget *w2 = new QGLWidget(0, w1);
    if (!w2->isSharing()) {
        delete w2;
        delete w1;
        QSKIP("Context sharing is not supported on this platform");
    }

    // Bind the buffer in the first context and write some data to it.
    QGLBuffer buffer(QGLBuffer::VertexBuffer);
    if (!buffer.create())
        QSKIP("Buffers are not supported on this platform");
    QVERIFY(buffer.isCreated());
    QVERIFY(buffer.bind());
    static GLfloat const data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    buffer.allocate(data, sizeof(data));
    QCOMPARE(buffer.size(), int(sizeof(data)));
    buffer.release();

    // Bind the buffer in the second context and read back the data.
    w2->makeCurrent();
    QVERIFY(buffer.bind());
    QCOMPARE(buffer.size(), int(sizeof(data)));
    GLfloat readdata[9];
    if (buffer.read(0, readdata, sizeof(readdata))) {
        for (int index = 0; index < 9; ++index)
            QCOMPARE(readdata[index], data[index]);
    }
    buffer.release();

    // Delete the first context.
    delete w1;

    // Make the second context current again because deleting the first
    // one will call doneCurrent() even though it wasn't current!
    w2->makeCurrent();

    // The buffer should still be valid in the second context.
    QVERIFY(buffer.bufferId() != 0);
    QVERIFY(buffer.isCreated());
    QVERIFY(buffer.bind());
    QCOMPARE(buffer.size(), int(sizeof(data)));
    buffer.release();

    // Delete the second context.
    delete w2;

    // The buffer should now be invalid.
    QVERIFY(buffer.bufferId() == 0);
    QVERIFY(!buffer.isCreated());
}

QTEST_MAIN(tst_QGLBuffer)

#include "tst_qglbuffer.moc"
