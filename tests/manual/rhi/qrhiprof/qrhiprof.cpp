/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Simple QRhiProfiler receiver app. Start it and then in a QRhi-based
// application connect with a QTcpSocket to 127.0.0.1:30667 and set that as the
// QRhiProfiler's device.

#include <QTcpServer>
#include <QTcpSocket>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QLabel>
#include <QTime>
#include <QtGui/private/qrhiprofiler_p.h>

const int MIN_KNOWN_OP = 1;
const int MAX_KNOWN_OP = 18;

class Parser : public QObject
{
    Q_OBJECT

public:
    void feed(const QByteArray &line);

    struct Event {
        QRhiProfiler::StreamOp op;
        qint64 timestamp;
        quint64 resource;
        QByteArray resourceName;

        struct Param {
            enum ValueType {
                Int64,
                Float
            };
            QByteArray key;
            ValueType valueType;
            union {
                qint64 intValue;
                float floatValue;
            };
        };

        QVector<Param> params;

        const Param *param(const char *key) const {
            auto it = std::find_if(params.cbegin(), params.cend(), [key](const Param &p) {
                return !strcmp(p.key.constData(), key);
            });
            return it == params.cend() ? nullptr : &*it;
        }
    };

signals:
    void eventReceived(const Event &e);
};

void Parser::feed(const QByteArray &line)
{
    const QList<QByteArray> elems = line.split(',');
    if (elems.count() < 4) {
        qWarning("Malformed line '%s'", line.constData());
        return;
    }
    bool ok = false;
    const int op = elems[0].toInt(&ok);
    if (!ok) {
        qWarning("Invalid op %s", elems[0].constData());
        return;
    }
    if (op < MIN_KNOWN_OP || op > MAX_KNOWN_OP) {
        qWarning("Unknown op %d", op);
        return;
    }

    Event e;
    e.op = QRhiProfiler::StreamOp(op);
    e.timestamp = elems[1].toLongLong();
    e.resource = elems[2].toULongLong();
    e.resourceName = elems[3];

    const int elemCount = elems.count();
    for (int i = 4; i < elemCount; i += 2) {
        if (i + 1 < elemCount && !elems[i].isEmpty() && !elems[i + 1].isEmpty()) {
            QByteArray key = elems[i];
            if (key.startsWith('F')) {
                key = key.mid(1);
                bool ok = false;
                const float value = elems[i + 1].toFloat(&ok);
                if (!ok) {
                    qWarning("Failed to parse float %s in line '%s'", elems[i + 1].constData(), line.constData());
                    continue;
                }
                Event::Param param;
                param.key = key;
                param.valueType = Event::Param::Float;
                param.floatValue = value;
                e.params.append(param);
            } else {
                const qint64 value = elems[i + 1].toLongLong();
                Event::Param param;
                param.key = key;
                param.valueType = Event::Param::Int64;
                param.intValue = value;
                e.params.append(param);
            }
        }
    }

    emit eventReceived(e);
}

class Tracker : public QObject
{
    Q_OBJECT

public slots:
    void handleEvent(const Parser::Event &e);

signals:
    void buffersTouched();
    void texturesTouched();
    void swapchainsTouched();
    void frameTimeTouched();
    void gpuFrameTimeTouched();
    void gpuMemAllocStatsTouched();

public:
    Tracker() {
        reset();
    }

    static const int MAX_STAGING_SLOTS = 3;

    struct Buffer {
        Buffer()
        {
            memset(stagingExtraSize, 0, sizeof(stagingExtraSize));
        }
        quint64 lastTimestamp;
        QByteArray resourceName;
        qint64 effectiveSize = 0;
        int backingGpuBufCount = 1;
        qint64 stagingExtraSize[MAX_STAGING_SLOTS];
    };
    QHash<qint64, Buffer> m_buffers;
    qint64 m_totalBufferApproxByteSize;
    qint64 m_peakBufferApproxByteSize;
    qint64 m_totalStagingBufferApproxByteSize;
    qint64 m_peakStagingBufferApproxByteSize;

    struct Texture {
        Texture()
        {
            memset(stagingExtraSize, 0, sizeof(stagingExtraSize));
        }
        quint64 lastTimestamp;
        QByteArray resourceName;
        qint64 approxByteSize = 0;
        bool ownsNativeResource = true;
        qint64 stagingExtraSize[MAX_STAGING_SLOTS];
    };
    QHash<qint64, Texture> m_textures;
    qint64 m_totalTextureApproxByteSize;
    qint64 m_peakTextureApproxByteSize;
    qint64 m_totalTextureStagingBufferApproxByteSize;
    qint64 m_peakTextureStagingBufferApproxByteSize;

    struct SwapChain {
        quint64 lastTimestamp;
        QByteArray resourceName;
        qint64 approxByteSize = 0;
    };
    QHash<qint64, SwapChain> m_swapchains;
    qint64 m_totalSwapChainApproxByteSize;
    qint64 m_peakSwapChainApproxByteSize;

    struct FrameTime {
        qint64 framesSinceResize = 0;
        int minDelta = 0;
        int maxDelta = 0;
        float avgDelta = 0;
    };
    FrameTime m_lastFrameTime;

    struct GpuFrameTime {
        float minTime = 0;
        float maxTime = 0;
        float avgTime = 0;
    };
    GpuFrameTime m_lastGpuFrameTime;

    struct GpuMemAllocStats {
        qint64 realAllocCount;
        qint64 subAllocCount;
        qint64 totalSize;
        qint64 unusedSize;
    };
    GpuMemAllocStats m_lastGpuMemAllocStats;

    void reset() {
        m_buffers.clear();
        m_textures.clear();
        m_totalBufferApproxByteSize = 0;
        m_peakBufferApproxByteSize = 0;
        m_totalStagingBufferApproxByteSize = 0;
        m_peakStagingBufferApproxByteSize = 0;
        m_totalTextureApproxByteSize = 0;
        m_peakTextureApproxByteSize = 0;
        m_totalTextureStagingBufferApproxByteSize = 0;
        m_peakTextureStagingBufferApproxByteSize = 0;
        m_totalSwapChainApproxByteSize = 0;
        m_peakSwapChainApproxByteSize = 0;
        m_lastFrameTime = FrameTime();
        m_lastGpuFrameTime = GpuFrameTime();
        m_lastGpuMemAllocStats = GpuMemAllocStats();
        emit buffersTouched();
        emit texturesTouched();
        emit swapchainsTouched();
        emit frameTimeTouched();
        emit gpuFrameTimeTouched();
        emit gpuMemAllocStatsTouched();
    }
};

Q_DECLARE_TYPEINFO(Tracker::Buffer, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracker::Texture, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracker::FrameTime, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracker::GpuFrameTime, Q_MOVABLE_TYPE);

void Tracker::handleEvent(const Parser::Event &e)
{
    switch (e.op) {
    case QRhiProfiler::NewBuffer:
    {
        Buffer b;
        b.lastTimestamp = e.timestamp;
        b.resourceName = e.resourceName;
        // type,0,usage,1,logical_size,84,effective_size,84,backing_gpu_buf_count,1,backing_cpu_buf_count,0
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("effective_size"))
                b.effectiveSize = p.intValue;
            else if (p.key == QByteArrayLiteral("backing_gpu_buf_count"))
                b.backingGpuBufCount = p.intValue;
        }
        m_totalBufferApproxByteSize += b.effectiveSize * b.backingGpuBufCount;
        m_peakBufferApproxByteSize = qMax(m_peakBufferApproxByteSize, m_totalBufferApproxByteSize);
        m_buffers.insert(e.resource, b);
        emit buffersTouched();
    }
        break;
    case QRhiProfiler::ReleaseBuffer:
    {
        auto it = m_buffers.find(e.resource);
        if (it != m_buffers.end()) {
            m_totalBufferApproxByteSize -= it->effectiveSize * it->backingGpuBufCount;
            m_buffers.erase(it);
            emit buffersTouched();
        }
    }
        break;

    case QRhiProfiler::NewBufferStagingArea:
    {
        qint64 slot = -1;
        qint64 size = 0;
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("slot"))
                slot = p.intValue;
            else if (p.key == QByteArrayLiteral("size"))
                size = p.intValue;
        }
        if (slot >= 0 && slot < MAX_STAGING_SLOTS) {
            auto it = m_buffers.find(e.resource);
            if (it != m_buffers.end()) {
                it->stagingExtraSize[slot] = size;
                m_totalStagingBufferApproxByteSize += size;
                m_peakStagingBufferApproxByteSize = qMax(m_peakStagingBufferApproxByteSize, m_totalStagingBufferApproxByteSize);
                emit buffersTouched();
            }
        }
    }
        break;
    case QRhiProfiler::ReleaseBufferStagingArea:
    {
        qint64 slot = -1;
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("slot"))
                slot = p.intValue;
        }
        if (slot >= 0 && slot < MAX_STAGING_SLOTS) {
            auto it = m_buffers.find(e.resource);
            if (it != m_buffers.end()) {
                m_totalStagingBufferApproxByteSize -= it->stagingExtraSize[slot];
                it->stagingExtraSize[slot] = 0;
                emit buffersTouched();
            }
        }
    }
        break;

    case QRhiProfiler::NewTexture:
    {
        Texture t;
        t.lastTimestamp = e.timestamp;
        t.resourceName = e.resourceName;
        // width,256,height,256,format,1,owns_native_resource,1,mip_count,9,layer_count,1,effective_sample_count,1,approx_byte_size,349524
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("approx_byte_size"))
                t.approxByteSize = p.intValue;
            else if (p.key == QByteArrayLiteral("owns_native_resource"))
                t.ownsNativeResource = p.intValue;
        }
        if (t.ownsNativeResource) {
            m_totalTextureApproxByteSize += t.approxByteSize;
            m_peakTextureApproxByteSize = qMax(m_peakTextureApproxByteSize, m_totalTextureApproxByteSize);
        }
        m_textures.insert(e.resource, t);
        emit texturesTouched();
    }
        break;
    case QRhiProfiler::ReleaseTexture:
    {
        auto it = m_textures.find(e.resource);
        if (it != m_textures.end()) {
            if (it->ownsNativeResource)
                m_totalTextureApproxByteSize -= it->approxByteSize;
            m_textures.erase(it);
            emit texturesTouched();
        }
    }
        break;

    case QRhiProfiler::NewTextureStagingArea:
    {
        qint64 slot = -1;
        qint64 size = 0;
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("slot"))
                slot = p.intValue;
            else if (p.key == QByteArrayLiteral("size"))
                size = p.intValue;
        }
        if (slot >= 0 && slot < MAX_STAGING_SLOTS) {
            auto it = m_textures.find(e.resource);
            if (it != m_textures.end()) {
                it->stagingExtraSize[slot] = size;
                m_totalTextureStagingBufferApproxByteSize += size;
                m_peakTextureStagingBufferApproxByteSize = qMax(m_peakTextureStagingBufferApproxByteSize, m_totalTextureStagingBufferApproxByteSize);
                emit texturesTouched();
            }
        }
    }
        break;
    case QRhiProfiler::ReleaseTextureStagingArea:
    {
        qint64 slot = -1;
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("slot"))
                slot = p.intValue;
        }
        if (slot >= 0 && slot < MAX_STAGING_SLOTS) {
            auto it = m_textures.find(e.resource);
            if (it != m_textures.end()) {
                m_totalTextureStagingBufferApproxByteSize -= it->stagingExtraSize[slot];
                it->stagingExtraSize[slot] = 0;
                emit texturesTouched();
            }
        }
    }
        break;

    case QRhiProfiler::ResizeSwapChain:
    {
        auto it = m_swapchains.find(e.resource);
        if (it != m_swapchains.end())
            m_totalSwapChainApproxByteSize -= it->approxByteSize;

        SwapChain s;
        s.lastTimestamp = e.timestamp;
        s.resourceName = e.resourceName;
        // width,1280,height,720,buffer_count,2,msaa_buffer_count,0,effective_sample_count,1,approx_total_byte_size,7372800
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("approx_total_byte_size"))
                s.approxByteSize = p.intValue;
        }
        m_totalSwapChainApproxByteSize += s.approxByteSize;
        m_peakSwapChainApproxByteSize = qMax(m_peakSwapChainApproxByteSize, m_totalSwapChainApproxByteSize);
        m_swapchains.insert(e.resource, s);
        emit swapchainsTouched();
    }
        break;
    case QRhiProfiler::ReleaseSwapChain:
    {
        auto it = m_swapchains.find(e.resource);
        if (it != m_swapchains.end()) {
            m_totalSwapChainApproxByteSize -= it->approxByteSize;
            m_swapchains.erase(it);
            emit swapchainsTouched();
        }
    }
        break;

    case QRhiProfiler::GpuFrameTime:
    {
        // Fmin_ms_gpu_frame_time,0.15488,Fmax_ms_gpu_frame_time,0.494592,Favg_ms_gpu_frame_time,0.33462
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("min_ms_gpu_frame_time"))
                m_lastGpuFrameTime.minTime = p.floatValue;
            else if (p.key == QByteArrayLiteral("max_ms_gpu_frame_time"))
                m_lastGpuFrameTime.maxTime = p.floatValue;
            else if (p.key == QByteArrayLiteral("avg_ms_gpu_frame_time"))
                m_lastGpuFrameTime.avgTime = p.floatValue;
        }
        emit gpuFrameTimeTouched();
    }
        break;
    case QRhiProfiler::FrameToFrameTime:
    {
        // frames_since_resize,121,min_ms_frame_delta,9,max_ms_frame_delta,33,Favg_ms_frame_delta,16.1167
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("frames_since_resize"))
                m_lastFrameTime.framesSinceResize = p.intValue;
            else if (p.key == QByteArrayLiteral("min_ms_frame_delta"))
                m_lastFrameTime.minDelta = p.intValue;
            else if (p.key == QByteArrayLiteral("max_ms_frame_delta"))
                m_lastFrameTime.maxDelta = p.intValue;
            else if (p.key == QByteArrayLiteral("avg_ms_frame_delta"))
                m_lastFrameTime.avgDelta = p.floatValue;
        }
        emit frameTimeTouched();
    }
        break;

    case QRhiProfiler::GpuMemAllocStats:
    {
        // real_alloc_count,2,sub_alloc_count,154,total_size,10752,unused_size,50320896
        for (const Parser::Event::Param &p : e.params) {
            if (p.key == QByteArrayLiteral("real_alloc_count"))
                m_lastGpuMemAllocStats.realAllocCount = p.intValue;
            else if (p.key == QByteArrayLiteral("sub_alloc_count"))
                m_lastGpuMemAllocStats.subAllocCount = p.intValue;
            else if (p.key == QByteArrayLiteral("total_size"))
                m_lastGpuMemAllocStats.totalSize = p.intValue;
            else if (p.key == QByteArrayLiteral("unused_size"))
                m_lastGpuMemAllocStats.unusedSize = p.intValue;
        }
        emit gpuMemAllocStatsTouched();
    }
        break;

    default:
        break;
    }
}

class Server : public QTcpServer
{
    Q_OBJECT

protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void clientConnected();
    void clientDisconnected();
    void receiveStarted();
    void lineReceived(const QByteArray &line);

private:
    bool m_valid = false;
    QTcpSocket m_socket;
    QByteArray m_buf;
};

void Server::incomingConnection(qintptr socketDescriptor)
{
    if (m_valid)
        return;

    m_socket.setSocketDescriptor(socketDescriptor);
    m_valid = true;
    emit clientConnected();
    connect(&m_socket, &QAbstractSocket::readyRead, this, [this] {
        bool receiveStartedSent = false;
        m_buf += m_socket.readAll();
        while (m_buf.contains('\n')) {
            const int lfpos = m_buf.indexOf('\n');
            const QByteArray line = m_buf.left(lfpos).trimmed();
            m_buf = m_buf.mid(lfpos + 1);
            if (!receiveStartedSent) {
                receiveStartedSent = true;
                emit receiveStarted();
            }
            emit lineReceived(line);
        }
    });
    connect(&m_socket, &QAbstractSocket::disconnected, this, [this] {
        if (m_valid) {
            m_valid = false;
            emit clientDisconnected();
        }
    });
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Tracker tracker;
    Parser parser;
    QObject::connect(&parser, &Parser::eventReceived, &tracker, &Tracker::handleEvent);

    Server server;
    if (!server.listen(QHostAddress::Any, 30667))
        qFatal("Failed to start server: %s", qPrintable(server.errorString()));

    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *infoLabel = new QLabel(QLatin1String("<i>Launch a Qt Quick application with QSG_RHI_PROFILE=1 and QSG_RHI_PROFILE_HOST set to the IP address.<br>"
                                                 "(resource memory usage reporting works best with the Vulkan backend)</i>"));
    layout->addWidget(infoLabel);

    QGroupBox *groupBox = new QGroupBox(QLatin1String("RHI statistics"));
    QVBoxLayout *groupLayout = new QVBoxLayout;

    QLabel *buffersLabel = new QLabel;
    QObject::connect(&tracker, &Tracker::buffersTouched, buffersLabel, [buffersLabel, &tracker] {
        const QString msg = QString::asprintf("%d buffers with ca. %lld bytes of current memory (sub)allocations (peak %lld) + %lld bytes of known staging buffers (peak %lld)",
                                              tracker.m_buffers.count(),
                                              tracker.m_totalBufferApproxByteSize, tracker.m_peakBufferApproxByteSize,
                                              tracker.m_totalStagingBufferApproxByteSize, tracker.m_peakStagingBufferApproxByteSize);
        buffersLabel->setText(msg);
    });
    groupLayout->addWidget(buffersLabel);

    QLabel *texturesLabel = new QLabel;
    QObject::connect(&tracker, &Tracker::texturesTouched, texturesLabel, [texturesLabel, &tracker] {
        const QString msg = QString::asprintf("%d textures with ca. %lld bytes of current memory (sub)allocations (peak %lld) + %lld bytes of known staging buffers (peak %lld)",
                                              tracker.m_textures.count(),
                                              tracker.m_totalTextureApproxByteSize, tracker.m_peakTextureApproxByteSize,
                                              tracker.m_totalTextureStagingBufferApproxByteSize, tracker.m_peakTextureStagingBufferApproxByteSize);
        texturesLabel->setText(msg);
    });
    groupLayout->addWidget(texturesLabel);

    QLabel *swapchainsLabel = new QLabel;
    QObject::connect(&tracker, &Tracker::swapchainsTouched, swapchainsLabel, [swapchainsLabel, &tracker] {
        const QString msg = QString::asprintf("Estimated total swapchain color buffer size is %lld bytes (peak %lld)",
                                              tracker.m_totalSwapChainApproxByteSize, tracker.m_peakSwapChainApproxByteSize);
        swapchainsLabel->setText(msg);
    });
    groupLayout->addWidget(swapchainsLabel);

    QLabel *frameTimeLabel = new QLabel;
    QObject::connect(&tracker, &Tracker::frameTimeTouched, frameTimeLabel, [frameTimeLabel, &tracker] {
        const QString msg = QString::asprintf("Frames since resize %lld Frame delta min %d ms max %d ms avg %f ms",
                                              tracker.m_lastFrameTime.framesSinceResize,
                                              tracker.m_lastFrameTime.minDelta,
                                              tracker.m_lastFrameTime.maxDelta,
                                              tracker.m_lastFrameTime.avgDelta);
        frameTimeLabel->setText(msg);
    });
    groupLayout->addWidget(frameTimeLabel);

    QLabel *gpuFrameTimeLabel = new QLabel;
    QObject::connect(&tracker, &Tracker::gpuFrameTimeTouched, gpuFrameTimeLabel, [gpuFrameTimeLabel, &tracker] {
        const QString msg = QString::asprintf("GPU frame time min %f ms max %f ms avg %f ms",
                                              tracker.m_lastGpuFrameTime.minTime,
                                              tracker.m_lastGpuFrameTime.maxTime,
                                              tracker.m_lastGpuFrameTime.avgTime);
        gpuFrameTimeLabel->setText(msg);
    });
    groupLayout->addWidget(gpuFrameTimeLabel);

    QLabel *gpuMemAllocStatsLabel = new QLabel;
    QObject::connect(&tracker, &Tracker::gpuMemAllocStatsTouched, gpuMemAllocStatsLabel, [gpuMemAllocStatsLabel, &tracker] {
        const QString msg = QString::asprintf("GPU memory allocator status: %lld real allocations %lld sub-allocations %lld total bytes %lld unused bytes",
                                              tracker.m_lastGpuMemAllocStats.realAllocCount,
                                              tracker.m_lastGpuMemAllocStats.subAllocCount,
                                              tracker.m_lastGpuMemAllocStats.totalSize,
                                              tracker.m_lastGpuMemAllocStats.unusedSize);
        gpuMemAllocStatsLabel->setText(msg);
    });
    groupLayout->addWidget(gpuMemAllocStatsLabel);

    groupBox->setLayout(groupLayout);
    layout->addWidget(groupBox);

    QTextEdit *rawLog = new QTextEdit;
    rawLog->setReadOnly(true);
    layout->addWidget(rawLog);

    QObject::connect(&server, &Server::clientConnected, rawLog, [rawLog] {
        rawLog->append(QLatin1String("\nCONNECTED\n"));
    });
    QObject::connect(&server, &Server::clientDisconnected, rawLog, [rawLog, &tracker] {
        rawLog->append(QLatin1String("\nDISCONNECTED\n"));
        tracker.reset();
    });
    QObject::connect(&server, &Server::receiveStarted, rawLog, [rawLog] {
        rawLog->setFontItalic(true);
        rawLog->append(QLatin1String("[") + QTime::currentTime().toString() + QLatin1String("]"));
        rawLog->setFontItalic(false);
    });

    QObject::connect(&server, &Server::lineReceived, rawLog, [rawLog, &parser](const QByteArray &line) {
        rawLog->append(QString::fromUtf8(line));
        parser.feed(line);
    });

    QWidget w;
    w.resize(800, 600);
    w.setLayout(layout);
    w.show();

    return app.exec();
}

#include "qrhiprof.moc"
