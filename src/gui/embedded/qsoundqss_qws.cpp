/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsoundqss_qws.h"

#ifndef QT_NO_SOUND
#include <qbytearray.h>
#include <qlist.h>
#include <qsocketnotifier.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qalgorithms.h>
#include <qtimer.h>
#include <qpointer.h>
#include <qendian.h>
#include <private/qcore_unix_p.h> // overrides QT_OPEN

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <qdebug.h>

#include <qvfbhdr.h>

extern int errno;

QT_BEGIN_NAMESPACE

#define QT_QWS_SOUND_16BIT 1 // or 0, or undefined for always 0
#define QT_QWS_SOUND_STEREO 1 // or 0, or undefined for always 0

// Zaurus SL5000D doesn't seem to return any error if setting to 44000 and it fails,
// however 44100 works, 44100 is more common that 44000.
static int sound_speed = 44100;
#ifndef QT_NO_QWS_SOUNDSERVER
extern int qws_display_id;
#endif

static char *zeroMem = 0;

struct QRiffChunk {
    char id[4];
    quint32 size;
    char data[4/*size*/];
};

#if defined(QT_QWS_IPAQ)
static const int sound_fragment_size = 12;
#else
static const int sound_fragment_size = 12;
#endif
static const int sound_buffer_size = 1 << sound_fragment_size;
// nb. there will be an sound startup delay of
//        2^sound_fragment_size / sound_speed seconds.
// (eg. sound_fragment_size==12, sound_speed==44000 means 0.093s delay)

#ifdef QT_QWS_SOUND_STEREO
static int sound_stereo=QT_QWS_SOUND_STEREO;
#else
static const int sound_stereo=0;
#endif
#ifdef QT_QWS_SOUND_16BIT
static bool sound_16bit=QT_QWS_SOUND_16BIT;
#else
static const bool sound_16bit=false;
#endif

#ifndef QT_NO_QWS_SOUNDSERVER
class QWSSoundServerClient : public QObject {
    Q_OBJECT

public:
    QWSSoundServerClient(QWS_SOCK_BASE *s, QObject* parent);
    ~QWSSoundServerClient();

public slots:
    void sendSoundCompleted(int, int);
    void sendDeviceReady(int, int);
    void sendDeviceError(int, int, int);

signals:
    void play(int, int, const QString&);
    void play(int, int, const QString&, int, int);
    void playRaw(int, int, const QString&, int, int, int, int);

    void pause(int, int);
    void stop(int, int);
    void resume(int, int);
    void setVolume(int, int, int, int);
    void setMute(int, int, bool);

    void stopAll(int);

    void playPriorityOnly(bool);

    void setSilent( bool );

private slots:
    void tryReadCommand();

private:
    void sendClientMessage(QString msg);
    int mCurrentID;
    int left, right;
    bool priExist;
    static int lastId;
    static int nextId() { return ++lastId; }
    QPointer<QWS_SOCK_BASE> socket;
};

int QWSSoundServerClient::lastId = 0;

QWSSoundServerClient::QWSSoundServerClient(QWS_SOCK_BASE *s, QObject* parent) :
    QObject( parent )
{
    socket = s;
    priExist = false;
    mCurrentID = nextId();
    connect(socket,SIGNAL(readyRead()),
        this,SLOT(tryReadCommand()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
}

QWSSoundServerClient::~QWSSoundServerClient()
{
    if (priExist)
	playPriorityOnly(false);
    emit stopAll(mCurrentID);
    if (socket)
        socket->deleteLater();
}

static QString getStringTok(QString &in)
{
    int pos = in.indexOf(QLatin1Char(' '));
    QString ret;
    if (pos > 0) {
	ret = in.left(pos);
	in = in.mid(pos+1);
    } else {
	ret = in;
	in = QString::null;
    }
    return ret;
}

static int getNumTok(QString &in)
{
    return getStringTok(in).toInt();
}

void QWSSoundServerClient::tryReadCommand()
{
    while ( socket->canReadLine() ) {
	QString l = QString::fromAscii(socket->readLine());
	l.truncate(l.length()-1); // chomp
	QString functionName = getStringTok(l);
	int soundid = getNumTok(l);
	if (functionName == QLatin1String("PLAY")) {
	    emit play(mCurrentID, soundid, l);
	} else if (functionName == QLatin1String("PLAYEXTEND")) {
	    int volume = getNumTok(l);
	    int flags = getNumTok(l);
	    emit play(mCurrentID, soundid, l, volume, flags);
	} else if (functionName == QLatin1String("PLAYRAW")) {
	    int chs = getNumTok(l);
	    int freq = getNumTok(l);
	    int bitspersample = getNumTok(l);
	    int flags = getNumTok(l);
	    emit playRaw(mCurrentID, soundid, l, freq, chs, bitspersample, flags);
	} else if (functionName == QLatin1String("PAUSE")) {
	    emit pause(mCurrentID, soundid);
	} else if (functionName == QLatin1String("STOP")) {
	    emit stop(mCurrentID, soundid);
	} else if (functionName == QLatin1String("RESUME")) {
	    emit resume(mCurrentID, soundid);
	} else if (functionName == QLatin1String("SETVOLUME")) {
	    int left = getNumTok(l);
	    int right = getNumTok(l);
	    emit setVolume(mCurrentID, soundid, left, right);
	} else if (functionName == QLatin1String("MUTE")) {
	    emit setMute(mCurrentID, soundid, true);
	} else if (functionName == QLatin1String("UNMUTE")) {
	    emit setMute(mCurrentID, soundid, false);
	} else if (functionName == QLatin1String("PRIORITYONLY")) {
	    bool sPri = soundid != 0;
	    if (sPri != priExist) {
		priExist = sPri;
		emit playPriorityOnly(sPri);
	    }
	} else if(functionName == QLatin1String("SILENT")) {
	    emit setSilent( soundid != 0 );
	}
    }
}

void QWSSoundServerClient::sendClientMessage(QString msg)
{
#ifndef QT_NO_TEXTCODEC
    QByteArray u = msg.toUtf8();
#else
    QByteArray u = msg.toLatin1();
#endif
    socket->write(u.data(), u.length());
    socket->flush();
}

void QWSSoundServerClient::sendSoundCompleted(int gid, int sid)
{
    if (gid == mCurrentID)
        sendClientMessage(QLatin1String("SOUNDCOMPLETED ")
                          + QString::number(sid) + QLatin1Char('\n'));
}

void QWSSoundServerClient::sendDeviceReady(int gid, int sid)
{
    if (gid == mCurrentID)
        sendClientMessage(QLatin1String("DEVICEREADY ")
                          + QString::number(sid) + QLatin1Char('\n'));
}

void QWSSoundServerClient::sendDeviceError(int gid, int sid, int err)
{
    if (gid == mCurrentID)
        sendClientMessage(QLatin1String("DEVICEERROR ")
                          + QString::number(sid) + QLatin1Char(' ')
                          + QString::number(err) + QLatin1Char('\n'));
}
#endif

static const int maxVolume = 100;
static const int runinLength = 2*sound_buffer_size;
class QWSSoundServerProvider {
public:
    QWSSoundServerProvider(int w, int s)
	: mWid(w), mSid(s), mMuted(false)
    {
	leftVolume = maxVolume>>1;
	rightVolume = maxVolume>>1;
	isPriority = false;
        samples_due = 0;
	max1 = max2 = out = 0;// = sound_buffer_size;
	data = data1;
	max = &max1;
	sampleRunin = 0;
	dev = -1;
    }

    virtual ~QWSSoundServerProvider() {
    }

    int groupId() const { return mWid; }
    int soundId() const { return mSid; }

    void startSampleRunin() {
	// inteded to provide even audio return from mute/pause/dead samples.
	//sampleRunin = runinLength; // or more?
    }


    void setVolume(int lv, int rv) {
	leftVolume = qMin(maxVolume, qMax(0, lv));
	rightVolume = qMin(maxVolume, qMax(0, rv));
    }

    void setMute(bool m) { mMuted = m; }
    bool muted() { return mMuted; }

    void setPriority(bool p) {
	if (p != isPriority) {
	    isPriority = p; // currently meaningless.
	}
    }


    static void setPlayPriorityOnly(bool p)
    {
	if (p)
	    priorityExists++;
	else
	    priorityExists--;

	if (priorityExists < 0)
	    qDebug("QSS: got more priority offs than ons");
    }

    // return -1 for file broken, give up.
    // else return sampels ready for playing.
    // argument is max samples server is looking for,
    // in terms of current device status.
    virtual int readySamples(int) = 0;

    int getSample(int off, int bps) {

        //
        //  16-bit audio data is converted to native endian so that it can be scaled
        //  Yes, this is ugly on a BigEndian machine
        //  Perhaps it shouldn't be scaled at all
        //
        return (bps == 1) ? (data[out+off] - 128) * 128 : qToLittleEndian(((short*)data)[(out/2)+off]);
    }

    int add(int* mixl, int* mixr, int count)
    {
        int bytesPerSample = chunkdata.wBitsPerSample >> 3;

        if ( mMuted ) {
            sampleRunin -= qMin(sampleRunin,count);
            while (count && (dev != -1)) {
                if (out >= *max) {
                    // switch buffers
                    out = 0;
                    if (data == data1 && max2 != 0) {
                        data = data2;
                        max = &max2;
                        max1 = 0;
                    } else if (data == data2 && max1 != 0) {
                        data = data1;
                        max = &max1;
                        max2 = 0;
                    } else {
                        qDebug("QSS Read Error: both buffers empty");
                        return 0;
                    }
                }
                samples_due += sound_speed;
                while (count && samples_due >= chunkdata.samplesPerSec) {
                    samples_due -= chunkdata.samplesPerSec;
                    count--;
                }
                out += bytesPerSample * chunkdata.channels;
            }
            return count;
        }

        // This shouldn't be the case
        if ( !mixl || !mixr )
            return 0;

        int lVolNum = leftVolume, lVolDen = maxVolume;
        int rVolNum = rightVolume, rVolDen = maxVolume;
        if (priorityExists > 0 && !isPriority) {
            lVolNum = 0; // later, make this gradually fade in and out.
            lVolDen = 5;
            rVolNum = 0;
            rVolDen = 5;
        }

        while (count && (dev != -1)) {
            if (out >= *max) {
                // switch buffers
                out = 0;
                if (data == data1 && max2 != 0) {
                    data = data2;
                    max = &max2;
                    max1 = 0;
                } else if (data == data2 && max1 != 0) {
                    data = data1;
                    max = &max1;
                    max2 = 0;
                } else {
                    qDebug("QSS Read Error: both buffers empty");
                    return 0;
                }
            }
            samples_due += sound_speed;
            if (count && samples_due >= chunkdata.samplesPerSec) {
                int l = getSample(0,bytesPerSample)*lVolNum/lVolDen;
                int r = (chunkdata.channels == 2) ? getSample(1,bytesPerSample)*rVolNum/rVolDen : l;
                if (!sound_stereo && chunkdata.channels == 2)
                    l += r;
		if (sampleRunin) {
                    while (sampleRunin && count && samples_due >= chunkdata.samplesPerSec) {
                        mixl++;
                        if (sound_stereo)
                            mixr++;
                        samples_due -= chunkdata.samplesPerSec;
		        sampleRunin--;
                        count--;
                    }
                }
                while (count && samples_due >= chunkdata.samplesPerSec) {
                    *mixl++ += l;
                    if (sound_stereo)
                        *mixr++ += r;
                    samples_due -= chunkdata.samplesPerSec;
                    count--;
                }
            }

            // optimize out manipulation of sample if downsampling and we skip it
            out += bytesPerSample * chunkdata.channels;
        }

        return count;
    }

    virtual bool finished() const = 0;

    bool equal(int wid, int sid)
    {
	return (wid == mWid && sid == mSid);
    }

protected:

    char * prepareBuffer( int &size)
    {
	// keep reading as long as there is 50 % or more room in off buffer.
	if (data == data1 && (max2<<1 < sound_buffer_size)) {
	    size=sound_buffer_size - max2;
	    return (char *)data2;
	} else if (data == data2 && (max1<<1 < sound_buffer_size)) {
	    size=sound_buffer_size - max1;
	    return (char *)data1;
	} else {
	    size = 0;
	    return 0;
	}
    }

    void updateBuffer(int read)
    {
	// always reads to off buffer.
	if (read >= 0) {
	    if (data == data2) {
		max1 = read;
	    } else {
		max2 = read;
	    }
	}
    }

    int devSamples()
    {
	int possible = (((max1+max2-out) / ((chunkdata.wBitsPerSample>>3)*chunkdata.channels))
		*sound_speed)/chunkdata.samplesPerSec;

	return possible;
    }


    struct {
        qint16 formatTag;
        qint16 channels;
        qint32 samplesPerSec;
        qint32 avgBytesPerSec;
        qint16 blockAlign;
        qint16 wBitsPerSample;
    } chunkdata;
    int dev;
    int samples_due;
private:
    int mWid;
    int mSid;
    int leftVolume;
    int rightVolume;
    bool isPriority;
    static int priorityExists;
    int *max;
    uchar *data;
    uchar data1[sound_buffer_size+4]; // +4 to handle badly aligned input data
    uchar data2[sound_buffer_size+4]; // +4 to handle badly aligned input data
    int out, max1, max2;
    int sampleRunin;
    bool mMuted;
};

int QWSSoundServerProvider::priorityExists = 0;

class QWSSoundServerBucket : public QWSSoundServerProvider {
public:
    QWSSoundServerBucket(int d, int wid, int sid)
	: QWSSoundServerProvider(wid, sid)
    {
	dev = d;
	wavedata_remaining = -1;
	mFinishedRead = false;
	mInsufficientSamples = false;
    }
    ~QWSSoundServerBucket()
    {
	//dev->close();
	::close(dev);
    }
    bool finished() const
    {
	//return !max;
	return mInsufficientSamples && mFinishedRead ;
    }
    int readySamples(int)
    {
	int size;
	char *dest = prepareBuffer(size);
	// may want to change this to something like
	// if (data == data1 && max2<<1 < sound_buffer_size
	//	||
	//	data == data2 && max1<<1 < sound_buffer_size)
	// so will keep filling off buffer while there is +50% space left
	if (size > 0 && dest != 0) {
	    while ( wavedata_remaining < 0 ) {
		//max = 0;
		wavedata_remaining = -1;
		// Keep reading chunks...
		const int n = sizeof(chunk)-sizeof(chunk.data);
		int nr = ::read(dev, (void*)&chunk,n);
		if ( nr != n ) {
		    // XXX check error? or don't we care?
		    wavedata_remaining = 0;
		    mFinishedRead = true;
		} else if ( qstrncmp(chunk.id,"data",4) == 0 ) {
		    wavedata_remaining = qToLittleEndian( chunk.size );

		    //out = max = sound_buffer_size;

		} else if ( qstrncmp(chunk.id,"RIFF",4) == 0 ) {
		    char d[4];
		    if ( read(dev, d, 4) != 4 ) {
			// XXX check error? or don't we care?
			//qDebug("couldn't read riff");
			mInsufficientSamples = true;
			mFinishedRead = true;
			return 0;
		    } else if ( qstrncmp(d,"WAVE",4) != 0 ) {
			// skip
			if ( chunk.size > 1000000000 || lseek(dev,chunk.size-4, SEEK_CUR) == -1 ) {
			    //qDebug("oversized wav chunk");
			    mFinishedRead = true;
			}
		    }
		} else if ( qstrncmp(chunk.id,"fmt ",4) == 0 ) {
		    if ( ::read(dev,(char*)&chunkdata,sizeof(chunkdata)) != sizeof(chunkdata) ) {
			// XXX check error? or don't we care?
			//qDebug("couldn't ready chunkdata");
			mFinishedRead = true;
		    }

#define WAVE_FORMAT_PCM 1
		    else
            {
                /*
                **  Endian Fix the chuck data
                */
                chunkdata.formatTag         = qToLittleEndian( chunkdata.formatTag );
                chunkdata.channels          = qToLittleEndian( chunkdata.channels );
                chunkdata.samplesPerSec     = qToLittleEndian( chunkdata.samplesPerSec );
                chunkdata.avgBytesPerSec    = qToLittleEndian( chunkdata.avgBytesPerSec );
                chunkdata.blockAlign        = qToLittleEndian( chunkdata.blockAlign );
                chunkdata.wBitsPerSample    = qToLittleEndian( chunkdata.wBitsPerSample );
                if ( chunkdata.formatTag != WAVE_FORMAT_PCM ) {
                    qWarning("WAV file: UNSUPPORTED FORMAT %d",chunkdata.formatTag);
                    mFinishedRead = true;
                }
		    }
		} else {
		    // ignored chunk
		    if ( chunk.size > 1000000000 || lseek(dev, chunk.size, SEEK_CUR) == -1) {
			//qDebug("chunk size too big");
			mFinishedRead = true;
		    }
		}
	    }
	    // this looks wrong.
	    if (wavedata_remaining <= 0) {
		mFinishedRead = true;
	    }

	}
	// may want to change this to something like
	// if (data == data1 && max2<<1 < sound_buffer_size
	//	||
	//	data == data2 && max1<<1 < sound_buffer_size)
	// so will keep filling off buffer while there is +50% space left

	if (wavedata_remaining) {
	    if (size > 0 && dest != 0) {
		int read = ::read(dev, dest, qMin(size, wavedata_remaining));
		// XXX check error? or don't we care?
		wavedata_remaining -= read;
		updateBuffer(read);
		if (read <= 0) // data unexpectidly ended
		    mFinishedRead = true;
	    }
	}
	int possible = devSamples();
	if (possible == 0)
	    mInsufficientSamples = true;
	return possible;
    }

protected:
    QRiffChunk chunk;
    int wavedata_remaining;
    bool mFinishedRead;
    bool mInsufficientSamples;
};

class QWSSoundServerStream : public QWSSoundServerProvider {
public:
    QWSSoundServerStream(int d,int c, int f, int b,
	    int wid, int sid)
	: QWSSoundServerProvider(wid, sid)
    {
	chunkdata.channels = c;
	chunkdata.samplesPerSec = f;
	chunkdata.wBitsPerSample = b;
	dev = d;
	//fcntl( dev, F_SETFL, O_NONBLOCK );
	lasttime = 0;
    }

    ~QWSSoundServerStream()
    {
	if (dev != -1) {
	    ::close(dev);
	    dev = -1;
	}
    }

    bool finished() const
    {
	return (dev == -1);
    }


    int readySamples(int)
    {
	int size;
	char *dest = prepareBuffer(size);
	if (size > 0 && dest != 0 && dev != -1) {

	    int read = ::read(dev, dest, size);
	    if (read < 0) {
		switch(errno) {
		    case EAGAIN:
		    case EINTR:
			// means read may yet succeed on the next attempt
			break;
		    default:
			// unexpected error, fail.
			::close(dev);
			dev = -1;
		}
	    } else if (read == 0) {
		// 0 means writer has closed dev and/or
		// file is at end.
		::close(dev);
		dev = -1;
	    } else {
		updateBuffer(read);
	    }
	}
	int possible = devSamples();
	if (possible == 0)
	    startSampleRunin();
	return possible;
    }

protected:
    time_t lasttime;
};

#ifndef QT_NO_QWS_SOUNDSERVER
QWSSoundServerSocket::QWSSoundServerSocket(QObject *parent) :
    QWSServerSocket(QT_VFB_SOUND_PIPE(qws_display_id), parent)
{
    connect(this, SIGNAL(newConnection()), this, SLOT(newConnection()));
}


#ifdef QT3_SUPPORT
QWSSoundServerSocket::QWSSoundServerSocket(QObject *parent, const char *name) :
    QWSServerSocket(QT_VFB_SOUND_PIPE(qws_display_id), parent)
{
    if (name)
        setObjectName(QString::fromAscii(name));
    connect(this, SIGNAL(newConnection()), this, SLOT(newConnection()));
}
#endif

void QWSSoundServerSocket::newConnection()
{
    while (QWS_SOCK_BASE *sock = nextPendingConnection()) {
        QWSSoundServerClient* client = new QWSSoundServerClient(sock,this);

        connect(client, SIGNAL(play(int,int,QString)),
                this, SIGNAL(playFile(int,int,QString)));
        connect(client, SIGNAL(play(int,int,QString,int,int)),
                this, SIGNAL(playFile(int,int,QString,int,int)));
        connect(client, SIGNAL(playRaw(int,int,QString,int,int,int,int)),
                this, SIGNAL(playRawFile(int,int,QString,int,int,int,int)));

        connect(client, SIGNAL(pause(int,int)),
                this, SIGNAL(pauseFile(int,int)));
        connect(client, SIGNAL(stop(int,int)),
                this, SIGNAL(stopFile(int,int)));
        connect(client, SIGNAL(playPriorityOnly(bool)),
                this, SIGNAL(playPriorityOnly(bool)));
        connect(client, SIGNAL(stopAll(int)),
                this, SIGNAL(stopAll(int)));
        connect(client, SIGNAL(resume(int,int)),
                this, SIGNAL(resumeFile(int,int)));

        connect(client, SIGNAL(setSilent(bool)),
                this, SIGNAL(setSilent(bool)));

        connect(client, SIGNAL(setMute(int,int,bool)),
                this, SIGNAL(setMute(int,int,bool)));
        connect(client, SIGNAL(setVolume(int,int,int,int)),
                this, SIGNAL(setVolume(int,int,int,int)));

        connect(this, SIGNAL(soundFileCompleted(int,int)),
                client, SLOT(sendSoundCompleted(int,int)));
        connect(this, SIGNAL(deviceReady(int,int)),
                client, SLOT(sendDeviceReady(int,int)));
        connect(this, SIGNAL(deviceError(int,int,int)),
                client, SLOT(sendDeviceError(int,int,int)));
    }
}

#endif

class QWSSoundServerPrivate : public QObject {
    Q_OBJECT

public:
    QWSSoundServerPrivate(QObject* parent=0, const char* name=0) :
        QObject(parent)
    {
	timerId = 0;
        if (name)
            setObjectName(QString::fromAscii(name));
#ifndef QT_NO_QWS_SOUNDSERVER
        server = new QWSSoundServerSocket(this);

	connect(server, SIGNAL(playFile(int,int,QString)),
		this, SLOT(playFile(int,int,QString)));
	connect(server, SIGNAL(playFile(int,int,QString,int,int)),
		this, SLOT(playFile(int,int,QString,int,int)));
	connect(server, SIGNAL(playRawFile(int,int,QString,int,int,int,int)),
		this, SLOT(playRawFile(int,int,QString,int,int,int,int)));

	connect(server, SIGNAL(pauseFile(int,int)),
		this, SLOT(pauseFile(int,int)));
	connect(server, SIGNAL(stopFile(int,int)),
		this, SLOT(stopFile(int,int)));
	connect(server, SIGNAL(stopAll(int)),
		this, SLOT(stopAll(int)));
	connect(server, SIGNAL(playPriorityOnly(bool)),
		this, SLOT(playPriorityOnly(bool)));
	connect(server, SIGNAL(resumeFile(int,int)),
		this, SLOT(resumeFile(int,int)));

        connect( server, SIGNAL(setSilent(bool)),
                this, SLOT(setSilent(bool)));

        connect(server, SIGNAL(setMute(int,int,bool)),
                this, SLOT(setMute(int,int,bool)));
	connect(server, SIGNAL(setVolume(int,int,int,int)),
		this, SLOT(setVolume(int,int,int,int)));

	connect(this, SIGNAL(soundFileCompleted(int,int)),
		server, SIGNAL(soundFileCompleted(int,int)));
	connect(this, SIGNAL(deviceReady(int,int)),
		server, SIGNAL(deviceReady(int,int)));
	connect(this, SIGNAL(deviceError(int,int,int)),
		server, SIGNAL(deviceError(int,int,int)));

#endif
        silent = false;
        fd = -1;
        unwritten = 0;
        can_GETOSPACE = true;
    }

    ~QWSSoundServerPrivate()
    {
        qDeleteAll(active);
        qDeleteAll(inactive);
    }

signals:
    void soundFileCompleted(int, int);
    void deviceReady(int, int);
    void deviceError(int, int, int);

public slots:
    void playRawFile(int wid, int sid, const QString &filename, int freq, int channels, int bitspersample, int flags);
    void playFile(int wid, int sid, const QString& filename);
    void playFile(int wid, int sid, const QString& filename, int v, int flags);
    void checkPresetVolumes(int wid, int sid, QWSSoundServerProvider *p);
    void pauseFile(int wid, int sid);
    void resumeFile(int wid, int sid);
    void stopFile(int wid, int sid);
    void stopAll(int wid);
    void setVolume(int wid, int sid, int lv, int rv);
    void setMute(int wid, int sid, bool m);
    void playPriorityOnly(bool p);
    void sendCompletedSignals();
    void feedDevice(int fd);
    void setSilent( bool enabled );

protected:
    void timerEvent(QTimerEvent* event);

private:
    int openFile(int wid, int sid, const QString& filename);
    bool openDevice();
    void closeDevice()
    {
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }

    QList<QWSSoundServerProvider*> active;
    QList<QWSSoundServerProvider*> inactive;
    struct PresetVolume {
	int wid;
	int sid;
	int left;
	int right;
	bool mute;
    };
    QList<PresetVolume> volumes;
    struct CompletedInfo {
	CompletedInfo( ) : groupId( 0 ), soundId( 0 ) { }
	CompletedInfo( int _groupId, int _soundId ) : groupId( _groupId ), soundId( _soundId ) { }
	int groupId;
	int soundId;
    };
    QList<CompletedInfo> completed;

    bool silent;

    int fd;
    int unwritten;
    int timerId;
    char* cursor;
    short data[sound_buffer_size*2];
    bool can_GETOSPACE;
#ifndef QT_NO_QWS_SOUNDSERVER
    QWSSoundServerSocket *server;
#endif
};

void QWSSoundServerPrivate::setSilent( bool enabled )
{
    // Close output device
    closeDevice();
    if( !unwritten && !active.count() ) {
        sendCompletedSignals();
    }
    // Stop processing audio
    killTimer( timerId );
    silent = enabled;
    // If audio remaining, open output device and continue processing
    if( unwritten || active.count() ) {
        openDevice();
    }
}

void QWSSoundServerPrivate::timerEvent(QTimerEvent* event)
{
    // qDebug("QSS timer event");
    if( event->timerId() == timerId ) {
        if (fd >= 0)
            feedDevice(fd);
        if (fd < 0) {
            killTimer(timerId);
            timerId = 0;
        }
    }
}

void QWSSoundServerPrivate::playRawFile(int wid, int sid, const QString &filename,
                                        int freq, int channels, int bitspersample, int flags)
{
#ifdef QT_NO_QWS_SOUNDSERVER
    Q_UNUSED(flags);
#endif
    int f = openFile(wid, sid, filename);
    if ( f ) {
        QWSSoundServerStream *b = new QWSSoundServerStream(f, channels, freq, bitspersample, wid, sid);
        // check preset volumes.
        checkPresetVolumes(wid, sid, b);
#ifndef QT_NO_QWS_SOUNDSERVER
        b->setPriority((flags & QWSSoundClient::Priority) == QWSSoundClient::Priority);
#endif
        active.append(b);
        emit deviceReady(wid, sid);
    }
}

void QWSSoundServerPrivate::playFile(int wid, int sid, const QString& filename)
{
    int f = openFile(wid, sid, filename);
    if ( f ) {
        QWSSoundServerProvider *b = new QWSSoundServerBucket(f, wid, sid);
        checkPresetVolumes(wid, sid, b);
        active.append( b );
        emit deviceReady(wid, sid);
    }
}

void QWSSoundServerPrivate::playFile(int wid, int sid, const QString& filename,
                                     int v, int flags)
{
#ifdef QT_NO_QWS_SOUNDSERVER
    Q_UNUSED(flags);
#endif
    int f = openFile(wid, sid, filename);
    if ( f ) {
        QWSSoundServerProvider *b = new QWSSoundServerBucket(f, wid, sid);
        checkPresetVolumes(wid, sid, b);
        b->setVolume(v, v);
#ifndef QT_NO_QWS_SOUNDSERVER
        b->setPriority((flags & QWSSoundClient::Priority) == QWSSoundClient::Priority);
#endif
        active.append(b);
        emit deviceReady(wid, sid);
    }
}

void QWSSoundServerPrivate::checkPresetVolumes(int wid, int sid, QWSSoundServerProvider *p)
{
    QList<PresetVolume>::Iterator it = volumes.begin();
    while (it != volumes.end()) {
        PresetVolume v = *it;
        if (v.wid == wid && v.sid == sid) {
            p->setVolume(v.left, v.right);
            p->setMute(v.mute);
            it = volumes.erase(it);
            return;
        } else {
            ++it;
        }
    }
}

void QWSSoundServerPrivate::pauseFile(int wid, int sid)
{
    QWSSoundServerProvider *bucket;
    for (int i = 0; i < active.size(); ++i ) {
        bucket = active.at(i);
        if (bucket->equal(wid, sid)) {
            // found bucket....
            active.removeAt(i);
            inactive.append(bucket);
            return;
        }
    }
}

void QWSSoundServerPrivate::resumeFile(int wid, int sid)
{
    QWSSoundServerProvider *bucket;
    for (int i = 0; i < inactive.size(); ++i ) {
        bucket = inactive.at(i);
        if (bucket->equal(wid, sid)) {
            // found bucket....
            inactive.removeAt(i);
            active.append(bucket);
            return;
        }
    }
}

void QWSSoundServerPrivate::stopFile(int wid, int sid)
{
    QWSSoundServerProvider *bucket;
    for (int i = 0; i < active.size(); ++i ) {
        bucket = active.at(i);
        if (bucket->equal(wid, sid)) {
            active.removeAt(i);
            delete bucket;
            return;
        }
    }
    for (int i = 0; i < inactive.size(); ++i ) {
        bucket = inactive.at(i);
        if (bucket->equal(wid, sid)) {
            inactive.removeAt(i);
            delete bucket;
            return;
        }
    }
}

void QWSSoundServerPrivate::stopAll(int wid)
{
    QWSSoundServerProvider *bucket;
    if (!active.isEmpty()) {
        QList<QWSSoundServerProvider*>::Iterator it = active.begin();
        while (it != active.end()) {
            bucket = *it;
            if (bucket->groupId() == wid) {
                it = active.erase(it);
                delete bucket;
            } else {
                ++it;
            }
        }
    }
    if (!inactive.isEmpty()) {
        QList<QWSSoundServerProvider*>::Iterator it = inactive.begin();
        while (it != inactive.end()) {
            bucket = *it;
            if (bucket->groupId() == wid) {
                it = inactive.erase(it);
                delete bucket;
            } else {
                ++it;
            }
        }
    }
}

void QWSSoundServerPrivate::setVolume(int wid, int sid, int lv, int rv)
{
    QWSSoundServerProvider *bucket;
    for( int i = 0; i < active.size(); ++i ) {
        bucket = active.at(i);
        if (bucket->equal(wid, sid)) {
            bucket->setVolume(lv,rv);
            return;
        }
    }
    // If gotten here, then it means wid/sid wasn't set up yet.
    // first find and remove current preset volumes, then add this one.
    QList<PresetVolume>::Iterator it = volumes.begin();
    while (it != volumes.end()) {
        PresetVolume v = *it;
        if (v.wid == wid && v.sid == sid)
            it = volumes.erase(it);
        else
            ++it;
    }
    // and then add this volume
    PresetVolume nv;
    nv.wid = wid;
    nv.sid = sid;
    nv.left = lv;
    nv.right = rv;
    nv.mute = false;
    volumes.append(nv);
}

void QWSSoundServerPrivate::setMute(int wid, int sid, bool m)
{
    QWSSoundServerProvider *bucket;
    for( int i = 0; i < active.size(); ++i ) {
        bucket = active.at(i);
        if (bucket->equal(wid, sid)) {
            bucket->setMute(m);
            return;
        }
    }
    // if gotten here then setting is being applied before item
    // is created.
    QList<PresetVolume>::Iterator it = volumes.begin();
    while (it != volumes.end()) {
        PresetVolume v = *it;
        if (v.wid == wid && v.sid == sid) {
            (*it).mute = m;
            return;
        }
    }
    if (m) {
        PresetVolume nv;
        nv.wid = wid;
        nv.sid = sid;
        nv.left = maxVolume>>1;
        nv.right = maxVolume>>1;
        nv.mute = true;
        volumes.append(nv);
    }
}

void QWSSoundServerPrivate::playPriorityOnly(bool p)
{
    QWSSoundServerProvider::setPlayPriorityOnly(p);
}

void QWSSoundServerPrivate::sendCompletedSignals()
{
    while( !completed.isEmpty() ) {
        emit soundFileCompleted( (*completed.begin()).groupId,
            (*completed.begin()).soundId );
            completed.erase( completed.begin() );
    }
}


int QWSSoundServerPrivate::openFile(int wid, int sid, const QString& filename)
{
    stopFile(wid, sid); // close and re-open.
    int f = QT_OPEN(QFile::encodeName(filename), O_RDONLY|O_NONBLOCK);
    if (f == -1) {
        // XXX check ferror, check reason.
        qDebug("Failed opening \"%s\"",filename.toLatin1().data());
#ifndef QT_NO_QWS_SOUNDSERVER
        emit deviceError(wid, sid, (int)QWSSoundClient::ErrOpeningFile );
#endif
    } else if ( openDevice() ) {
        return f;
    }
#ifndef QT_NO_QWS_SOUNDSERVER
    emit deviceError(wid, sid, (int)QWSSoundClient::ErrOpeningAudioDevice );
#endif
    return 0;
}

bool QWSSoundServerPrivate::openDevice()
{
        if (fd < 0) {
            if( silent ) {
                fd = QT_OPEN( "/dev/null", O_WRONLY );
                // Emulate write to audio device
                int delay = 1000*(sound_buffer_size>>(sound_stereo+sound_16bit))/sound_speed/2;
                timerId = startTimer(delay);

                return true;
            }
            //
            // Don't block open right away.
            //
            bool openOkay = false;
            if ((fd = QT_OPEN("/dev/dsp", O_WRONLY|O_NONBLOCK)) != -1) {
                int flags = fcntl(fd, F_GETFL);
                flags &= ~O_NONBLOCK;
		openOkay = (fcntl(fd, F_SETFL, flags) == 0);
	    }
            if (!openOkay) {
	        qDebug("Failed opening audio device");
		return false;
            }

            // Setup soundcard at 16 bit mono
            int v;
	    //v=0x00010000+sound_fragment_size;
	    // um the media player did this instead.
	    v=0x10000 * 4 + sound_fragment_size;
            if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &v))
                qWarning("Could not set fragments to %08x",v);
#ifdef QT_QWS_SOUND_16BIT
            //
            //  Use native endian
            //  Since we have manipulated the data volume the data
            //  is now in native format, even though its stored
            //  as little endian in the WAV file
            //
            v=AFMT_S16_NE; if (ioctl(fd, SNDCTL_DSP_SETFMT, &v))
                qWarning("Could not set format %d",v);
            if (AFMT_S16_NE != v)
                qDebug("Want format %d got %d", AFMT_S16_LE, v);
#else
            v=AFMT_U8; if (ioctl(fd, SNDCTL_DSP_SETFMT, &v))
                qWarning("Could not set format %d",v);
            if (AFMT_U8 != v)
                qDebug("Want format %d got %d", AFMT_U8, v);
#endif
            v=sound_stereo; if (ioctl(fd, SNDCTL_DSP_STEREO, &v))
                qWarning("Could not set stereo %d",v);
            if (sound_stereo != v)
                qDebug("Want stereo %d got %d", sound_stereo, v);
#ifdef QT_QWS_SOUND_STEREO
            sound_stereo=v;
#endif
            v=sound_speed; if (ioctl(fd, SNDCTL_DSP_SPEED, &sound_speed))
                qWarning("Could not set speed %d",v);
            if (v != sound_speed)
                qDebug("Want speed %d got %d", v, sound_speed);

            int delay = 1000*(sound_buffer_size>>(sound_stereo+sound_16bit))
                                    /sound_speed/2;
	    // qDebug("QSS delay: %d", delay);
            timerId = startTimer(delay);

	    //
	    // Check system volume
	    //
	    int mixerHandle = QT_OPEN( "/dev/mixer", O_RDWR|O_NONBLOCK );
	    if ( mixerHandle >= 0 ) {
		int volume;
		ioctl( mixerHandle, MIXER_READ(0), &volume );
		close( mixerHandle );
		if ( volume < 1<<(sound_stereo+sound_16bit) )
		    qDebug("Want sound at %d got %d",
			    1<<(sound_stereo+sound_16bit), volume);
	    } else
		qDebug( "get volume of audio device failed" );

        }
        return true;
}

void  QWSSoundServerPrivate::feedDevice(int fd)
{
    if ( !unwritten && active.size() == 0 ) {
        closeDevice();
        sendCompletedSignals();
        return;
    } else {
        sendCompletedSignals();
    }

    QWSSoundServerProvider* bucket;

    // find out how much audio is possible
    int available = sound_buffer_size;
    QList<QWSSoundServerProvider*> running;
    for (int i = 0; i < active.size(); ++i) {
        bucket = active.at(i);
        int ready = bucket->readySamples(available);
        if (ready > 0) {
            available = qMin(available, ready);
            running.append(bucket);
        }
    }

    audio_buf_info info;
    if (can_GETOSPACE && ioctl(fd,SNDCTL_DSP_GETOSPACE,&info)) {
        can_GETOSPACE = false;
        fcntl(fd, F_SETFL, O_NONBLOCK);
    }
    if (!can_GETOSPACE)
        info.fragments = 4; // #### configurable?
    if (info.fragments > 0) {
        if (!unwritten) {
            int left[sound_buffer_size];
            memset(left,0,available*sizeof(int));
            int right[sound_buffer_size];
            if ( sound_stereo )
                memset(right,0,available*sizeof(int));

            if (running.size() > 0) {
            // should do volume mod here in regards to each bucket to avoid flattened/bad peaks.
                for (int i = 0; i < running.size(); ++i ) {
                    bucket = running.at(i);
                    int unused = bucket->add(left,right,available);
                    if (unused > 0) {
                        // this error is quite serious, as
                        // it will really screw up mixing.
                        qDebug("provider lied about samples ready");
                    }
                }
                if ( sound_16bit ) {
                    short *d = (short*)data;
                    for (int i=0; i<available; i++) {
                        *d++ = (short)qMax(qMin(left[i],32767),-32768);
                        if ( sound_stereo )
                            *d++ = (short)qMax(qMin(right[i],32767),-32768);
                    }
                } else {
                    signed char *d = (signed char *)data;
                    for (int i=0; i<available; i++) {
                        *d++ = (signed char)qMax(qMin(left[i]/256,127),-128)+128;
                        if ( sound_stereo )
                            *d++ = (signed char)qMax(qMin(right[i]/256,127),-128)+128;
                    }
                }
                unwritten = available*(sound_16bit+1)*(sound_stereo+1);
                cursor = (char*)data;
            }
        }
        // sound open, but nothing written.  Should clear the buffer.

        int w;
        if (unwritten) {
            w = ::write(fd,cursor,unwritten);

            if (w < 0) {
                if (can_GETOSPACE)
                    return;
                w = 0;
            }

            cursor += w;
            unwritten -= w;
        } else {
            // write some zeros to clear the buffer?
            if (!zeroMem)
                zeroMem = (char *)calloc(sound_buffer_size, sizeof(char));
            w = ::write(fd, zeroMem, sound_buffer_size);
            if (w < 0)
                w = 0;
        }
    }

    QList<QWSSoundServerProvider*>::Iterator it = active.begin();
    while (it != active.end()) {
        bucket = *it;
        if (bucket->finished()) {
            completed.append(CompletedInfo(bucket->groupId(), bucket->soundId()));
            it = active.erase(it);
            delete bucket;
        } else {
            ++it;
        }
    }
}


QWSSoundServer::QWSSoundServer(QObject* parent) :
    QObject(parent)
{
    d = new QWSSoundServerPrivate(this);

    connect( d, SIGNAL(soundFileCompleted(int,int)),
        this, SLOT(translateSoundCompleted(int,int)) );
}

void QWSSoundServer::playFile( int sid, const QString& filename )
{
    //wid == 0, as it is the server initiating rather than a client
    // if wid was passable, would accidently collide with server
    // sockect's wids.
    d->playFile(0, sid, filename);
}

void QWSSoundServer::pauseFile( int sid )
{
    d->pauseFile(0, sid);
}

void QWSSoundServer::stopFile( int sid )
{
    d->stopFile(0, sid);
}

void QWSSoundServer::resumeFile( int sid )
{
    d->resumeFile(0, sid);
}

QWSSoundServer::~QWSSoundServer()
{
    d->stopAll(0);
}

void QWSSoundServer::translateSoundCompleted( int, int sid )
{
    emit soundCompleted( sid );
}

#ifndef QT_NO_QWS_SOUNDSERVER
QWSSoundClient::QWSSoundClient(QObject* parent) :
    QWSSocket(parent)
{
    connectToLocalFile(QT_VFB_SOUND_PIPE(qws_display_id));
    QObject::connect(this,SIGNAL(readyRead()),
	this,SLOT(tryReadCommand()));
    if( state() == QWS_SOCK_BASE::ConnectedState ) QTimer::singleShot(1, this, SIGNAL(connected()));
    else QTimer::singleShot(1, this, SLOT(emitConnectionRefused()));
}

QWSSoundClient::~QWSSoundClient( )
{
    flush();
}

void QWSSoundClient::reconnect()
{
    connectToLocalFile(QT_VFB_SOUND_PIPE(qws_display_id));
    if( state() == QWS_SOCK_BASE::ConnectedState ) emit connected();
    else emit error( QTcpSocket::ConnectionRefusedError );
}

void QWSSoundClient::sendServerMessage(QString msg)
{
#ifndef QT_NO_TEXTCODEC
    QByteArray u = msg.toUtf8();
#else
    QByteArray u = msg.toLatin1();
#endif
    write(u.data(), u.length());
    flush();
}

void QWSSoundClient::play( int id, const QString& filename )
{
    QFileInfo fi(filename);
    sendServerMessage(QLatin1String("PLAY ")
                      + QString::number(id) + QLatin1Char(' ')
                      + fi.absoluteFilePath() + QLatin1Char('\n'));
}

void QWSSoundClient::play( int id, const QString& filename, int volume, int flags)
{
    QFileInfo fi(filename);
    sendServerMessage(QLatin1String("PLAYEXTEND ")
        + QString::number(id) + QLatin1Char(' ')
        + QString::number(volume) + QLatin1Char(' ')
        + QString::number(flags) + QLatin1Char(' ')
        + fi.absoluteFilePath() + QLatin1Char('\n'));
}

void QWSSoundClient::pause( int id )
{
    sendServerMessage(QLatin1String("PAUSE ")
        + QString::number(id) + QLatin1Char('\n'));
}

void QWSSoundClient::stop( int id )
{
    sendServerMessage(QLatin1String("STOP ")
        + QString::number(id) + QLatin1Char('\n'));
}

void QWSSoundClient::resume( int id )
{
    sendServerMessage(QLatin1String("RESUME ")
        + QString::number(id) + QLatin1Char('\n'));
}

void QWSSoundClient::playRaw( int id, const QString& filename,
	int freq, int chs, int bitspersample, int flags)
{
    QFileInfo fi(filename);
    sendServerMessage(QLatin1String("PLAYRAW ")
        + QString::number(id) + QLatin1Char(' ')
        + QString::number(chs) + QLatin1Char(' ')
        + QString::number(freq) + QLatin1Char(' ')
        + QString::number(bitspersample) + QLatin1Char(' ')
        + QString::number(flags) + QLatin1Char(' ')
        + fi.absoluteFilePath() + QLatin1Char('\n'));
}

void QWSSoundClient::setMute( int id, bool m )
{
    sendServerMessage(QLatin1String(m ? "MUTE " : "UNMUTE ")
        + QString::number(id) + QLatin1Char('\n'));
}

void QWSSoundClient::setVolume( int id, int leftVol, int rightVol )
{
    sendServerMessage(QLatin1String("SETVOLUME ")
        + QString::number(id) + QLatin1Char(' ')
        + QString::number(leftVol) + QLatin1Char(' ')
        + QString::number(rightVol) + QLatin1Char('\n'));
}

void QWSSoundClient::playPriorityOnly( bool pri )
{
    sendServerMessage(QLatin1String("PRIORITYONLY ")
        + QString::number(pri ? 1 : 0) + QLatin1Char('\n'));
}

void QWSSoundClient::setSilent( bool enable )
{
    sendServerMessage(QLatin1String("SILENT ")
            + QString::number( enable ? 1 : 0 ) + QLatin1Char('\n'));
}

void QWSSoundClient::tryReadCommand()
{
    while ( canReadLine() ) {
	QString l = QString::fromAscii(readLine());
	l.truncate(l.length()-1); // chomp
	QStringList token = l.split(QLatin1Char(' '));
	if (token[0] == QLatin1String("SOUNDCOMPLETED")) {
	    emit soundCompleted(token[1].toInt());
	} else if (token[0] == QLatin1String("DEVICEREADY")) {
            emit deviceReady(token[1].toInt());
	} else if (token[0] == QLatin1String("DEVICEERROR")) {
            emit deviceError(token[1].toInt(),(DeviceErrors)token[2].toInt());
	}
    }
}

void QWSSoundClient::emitConnectionRefused()
{
    emit error( QTcpSocket::ConnectionRefusedError );
}
#endif

QT_END_NAMESPACE

#include "qsoundqss_qws.moc"

#endif        // QT_NO_SOUND
