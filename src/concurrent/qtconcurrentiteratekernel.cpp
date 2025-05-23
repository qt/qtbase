// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qtconcurrentiteratekernel.h"

#include <qdeadlinetimer.h>
#include "private/qfunctions_p.h"


#if !defined(QT_NO_CONCURRENT) || defined(Q_QDOC)

QT_BEGIN_NAMESPACE

enum {
    TargetRatio = 100
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
  \class QtConcurrent::BlockSizeManager
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
BlockSizeManager::BlockSizeManager(QThreadPool *pool, int iterationCount)
    : maxBlockSize(iterationCount / (std::max(pool->maxThreadCount(), 1) * 2)),
      beforeUser(0), afterUser(0),
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

    if (controlPartElapsed.median() * int(TargetRatio) < userPartElapsed.median())
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

} // namespace QtConcurrent

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT
