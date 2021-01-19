/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtConcurrent module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qtconcurrentiteratekernel.h"

#include <qdeadlinetimer.h>
#include "private/qfunctions_p.h"


#if !defined(QT_NO_CONCURRENT) || defined(Q_CLANG_QDOC)

QT_BEGIN_NAMESPACE

enum {
    TargetRatio = 100,
    MedianSize = 7
};

static qint64 getticks()
{
    return QDeadlineTimer::current(Qt::PreciseTimer).deadlineNSecs();
}

static double elapsed(qint64 after, qint64 before)
{
    return double(after - before);
}

namespace QtConcurrent {

/*!
  \class QtConcurrent::Median
  \inmodule QtConcurrent
  \internal
 */

/*!
  \class QtConcurrent::MedianDouble
  \inmodule QtConcurrent
  \internal
 */

/*!
  \class QtConcurrent::BlockSizeManager
  \inmodule QtConcurrent
  \internal
 */

/*!
  \class QtConcurrent::BlockSizeManagerV2
  \inmodule QtConcurrent
  \internal
 */

/*!
  \class QtConcurrent::ResultReporter
  \inmodule QtConcurrent
  \internal
 */

/*! \fn bool QtConcurrent::selectIteration(std::bidirectional_iterator_tag)
  \internal
 */

/*! \fn bool QtConcurrent::selectIteration(std::forward_iterator_tag)
  \internal
 */

/*! \fn bool QtConcurrent::selectIteration(std::random_access_iterator_tag)
  \internal
 */

/*!
  \class QtConcurrent::IterateKernel
  \inmodule QtConcurrent
  \internal
 */

/*! \internal

*/
BlockSizeManager::BlockSizeManager(int iterationCount)
: maxBlockSize(iterationCount / (QThreadPool::globalInstance()->maxThreadCount() * 2)),
  beforeUser(0), afterUser(0),
  controlPartElapsed(MedianSize), userPartElapsed(MedianSize),
  m_blockSize(1)
{ }

// Records the time before user code.
void BlockSizeManager::timeBeforeUser()
{
    if (blockSizeMaxed())
        return;

    beforeUser = getticks();
    controlPartElapsed.addValue(elapsed(beforeUser, afterUser));
}

 // Records the time after user code and adjust the block size if we are spending
 // to much time in the for control code compared with the user code.
void BlockSizeManager::timeAfterUser()
{
    if (blockSizeMaxed())
        return;

    afterUser = getticks();
    userPartElapsed.addValue(elapsed(afterUser, beforeUser));

    if (controlPartElapsed.isMedianValid() == false)
        return;

    if (controlPartElapsed.median() * TargetRatio < userPartElapsed.median())
        return;

    m_blockSize = qMin(m_blockSize * 2,  maxBlockSize);

#ifdef QTCONCURRENT_FOR_DEBUG
    qDebug() << QThread::currentThread() << "adjusting block size" << controlPartElapsed.median() << userPartElapsed.median() << m_blockSize;
#endif

    // Reset the medians after adjusting the block size so we get
    // new measurements with the new block size.
    controlPartElapsed.reset();
    userPartElapsed.reset();
}

int BlockSizeManager::blockSize()
{
    return m_blockSize;
}

/*! \internal

*/
BlockSizeManagerV2::BlockSizeManagerV2(int iterationCount)
    : maxBlockSize(iterationCount / (QThreadPool::globalInstance()->maxThreadCount() * 2)),
      beforeUser(0), afterUser(0),
      m_blockSize(1)
{ }

// Records the time before user code.
void BlockSizeManagerV2::timeBeforeUser()
{
    if (blockSizeMaxed())
        return;

    beforeUser = getticks();
    controlPartElapsed.addValue(elapsed(beforeUser, afterUser));
}

 // Records the time after user code and adjust the block size if we are spending
 // to much time in the for control code compared with the user code.
void BlockSizeManagerV2::timeAfterUser()
{
    if (blockSizeMaxed())
        return;

    afterUser = getticks();
    userPartElapsed.addValue(elapsed(afterUser, beforeUser));

    if (controlPartElapsed.isMedianValid() == false)
        return;

    if (controlPartElapsed.median() * TargetRatio < userPartElapsed.median())
        return;

    m_blockSize = qMin(m_blockSize * 2,  maxBlockSize);

#ifdef QTCONCURRENT_FOR_DEBUG
    qDebug() << QThread::currentThread() << "adjusting block size" << controlPartElapsed.median() << userPartElapsed.median() << m_blockSize;
#endif

    // Reset the medians after adjusting the block size so we get
    // new measurements with the new block size.
    controlPartElapsed.reset();
    userPartElapsed.reset();
}

int BlockSizeManagerV2::blockSize()
{
    return m_blockSize;
}

} // namespace QtConcurrent

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT
