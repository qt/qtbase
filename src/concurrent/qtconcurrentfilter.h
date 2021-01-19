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

#ifndef QTCONCURRENT_FILTER_H
#define QTCONCURRENT_FILTER_H

#include <QtConcurrent/qtconcurrent_global.h>

#if !defined(QT_NO_CONCURRENT) || defined(Q_CLANG_QDOC)

#include <QtConcurrent/qtconcurrentfilterkernel.h>
#include <QtConcurrent/qtconcurrentfunctionwrappers.h>

QT_BEGIN_NAMESPACE

namespace QtConcurrent {

//! [QtConcurrent-1]
template <typename Sequence, typename KeepFunctor, typename ReduceFunctor>
ThreadEngineStarter<void> filterInternal(Sequence &sequence, KeepFunctor keep, ReduceFunctor reduce)
{
    typedef FilterKernel<Sequence, KeepFunctor, ReduceFunctor> KernelType;
    return startThreadEngine(new KernelType(sequence, keep, reduce));
}

// filter() on sequences
template <typename Sequence, typename KeepFunctor>
QFuture<void> filter(Sequence &sequence, KeepFunctor keep)
{
    return filterInternal(sequence, QtPrivate::createFunctionWrapper(keep), QtPrivate::PushBackWrapper());
}

// filteredReduced() on sequences
template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor>
QFuture<ResultType> filteredReduced(const Sequence &sequence,
                                    KeepFunctor keep,
                                    ReduceFunctor reduce,
                                    ReduceOptions options = ReduceOptions(UnorderedReduce | SequentialReduce))
{
    return startFilteredReduced<ResultType>(sequence, QtPrivate::createFunctionWrapper(keep), QtPrivate::createFunctionWrapper(reduce), options);
}

#ifndef Q_CLANG_QDOC
template <typename Sequence, typename KeepFunctor, typename ReduceFunctor>
QFuture<typename QtPrivate::ReduceResultType<ReduceFunctor>::ResultType> filteredReduced(const Sequence &sequence,
                                    KeepFunctor keep,
                                    ReduceFunctor reduce,
                                    ReduceOptions options = ReduceOptions(UnorderedReduce | SequentialReduce))
{
    return startFilteredReduced<typename QtPrivate::ReduceResultType<ReduceFunctor>::ResultType>
            (sequence,
             QtPrivate::createFunctionWrapper(keep),
             QtPrivate::createFunctionWrapper(reduce),
             options);
}
#endif

// filteredReduced() on iterators
template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor>
QFuture<ResultType> filteredReduced(Iterator begin,
                                    Iterator end,
                                    KeepFunctor keep,
                                    ReduceFunctor reduce,
                                    ReduceOptions options = ReduceOptions(UnorderedReduce | SequentialReduce))
{
   return startFilteredReduced<ResultType>(begin, end, QtPrivate::createFunctionWrapper(keep), QtPrivate::createFunctionWrapper(reduce), options);
}

#ifndef Q_CLANG_QDOC
template <typename Iterator, typename KeepFunctor, typename ReduceFunctor>
QFuture<typename QtPrivate::ReduceResultType<ReduceFunctor>::ResultType> filteredReduced(Iterator begin,
                                    Iterator end,
                                    KeepFunctor keep,
                                    ReduceFunctor reduce,
                                    ReduceOptions options = ReduceOptions(UnorderedReduce | SequentialReduce))
{
   return startFilteredReduced<typename QtPrivate::ReduceResultType<ReduceFunctor>::ResultType>
           (begin, end,
            QtPrivate::createFunctionWrapper(keep),
            QtPrivate::createFunctionWrapper(reduce),
            options);
}
#endif

// filtered() on sequences
template <typename Sequence, typename KeepFunctor>
QFuture<typename Sequence::value_type> filtered(const Sequence &sequence, KeepFunctor keep)
{
    return startFiltered(sequence, QtPrivate::createFunctionWrapper(keep));
}

// filtered() on iterators
template <typename Iterator, typename KeepFunctor>
QFuture<typename qValueType<Iterator>::value_type> filtered(Iterator begin, Iterator end, KeepFunctor keep)
{
    return startFiltered(begin, end, QtPrivate::createFunctionWrapper(keep));
}

// blocking filter() on sequences
template <typename Sequence, typename KeepFunctor>
void blockingFilter(Sequence &sequence, KeepFunctor keep)
{
    filterInternal(sequence, QtPrivate::createFunctionWrapper(keep), QtPrivate::PushBackWrapper()).startBlocking();
}

// blocking filteredReduced() on sequences
template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor>
ResultType blockingFilteredReduced(const Sequence &sequence,
                                   KeepFunctor keep,
                                   ReduceFunctor reduce,
                                   ReduceOptions options = ReduceOptions(UnorderedReduce | SequentialReduce))
{
    return startFilteredReduced<ResultType>(sequence, QtPrivate::createFunctionWrapper(keep), QtPrivate::createFunctionWrapper(reduce), options)
        .startBlocking();
}

#ifndef Q_CLANG_QDOC
template <typename Sequence, typename KeepFunctor, typename ReduceFunctor>
typename QtPrivate::ReduceResultType<ReduceFunctor>::ResultType blockingFilteredReduced(const Sequence &sequence,
                                   KeepFunctor keep,
                                   ReduceFunctor reduce,
                                   ReduceOptions options = ReduceOptions(UnorderedReduce | SequentialReduce))
{
    return blockingFilteredReduced<typename QtPrivate::ReduceResultType<ReduceFunctor>::ResultType>
        (sequence,
         QtPrivate::createFunctionWrapper(keep),
         QtPrivate::createFunctionWrapper(reduce),
         options);
}
#endif

// blocking filteredReduced() on iterators
template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor>
ResultType blockingFilteredReduced(Iterator begin,
                                   Iterator end,
                                   KeepFunctor keep,
                                   ReduceFunctor reduce,
                                   ReduceOptions options = ReduceOptions(UnorderedReduce | SequentialReduce))
{
    return startFilteredReduced<ResultType>
        (begin, end,
         QtPrivate::createFunctionWrapper(keep),
         QtPrivate::createFunctionWrapper(reduce),
         options)
        .startBlocking();
}

#ifndef Q_CLANG_QDOC
template <typename Iterator, typename KeepFunctor, typename ReduceFunctor>
typename QtPrivate::ReduceResultType<ReduceFunctor>::ResultType blockingFilteredReduced(Iterator begin,
                                   Iterator end,
                                   KeepFunctor keep,
                                   ReduceFunctor reduce,
                                   ReduceOptions options = ReduceOptions(UnorderedReduce | SequentialReduce))
{
    return startFilteredReduced<typename QtPrivate::ReduceResultType<ReduceFunctor>::ResultType>
        (begin, end,
         QtPrivate::createFunctionWrapper(keep),
         QtPrivate::createFunctionWrapper(reduce),
         options)
        .startBlocking();
}
#endif

// blocking filtered() on sequences
template <typename Sequence, typename KeepFunctor>
Sequence blockingFiltered(const Sequence &sequence, KeepFunctor keep)
{
    return startFilteredReduced<Sequence>(sequence, QtPrivate::createFunctionWrapper(keep), QtPrivate::PushBackWrapper(), OrderedReduce).startBlocking();
}

// blocking filtered() on iterators
template <typename OutputSequence, typename Iterator, typename KeepFunctor>
OutputSequence blockingFiltered(Iterator begin, Iterator end, KeepFunctor keep)
{
    return startFilteredReduced<OutputSequence>(begin, end,
        QtPrivate::createFunctionWrapper(keep),
        QtPrivate::PushBackWrapper(),
        OrderedReduce).startBlocking();
}

} // namespace QtConcurrent

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
