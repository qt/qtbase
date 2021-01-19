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

#ifndef QTCONCURRENT_MAPKERNEL_H
#define QTCONCURRENT_MAPKERNEL_H

#include <QtConcurrent/qtconcurrent_global.h>

#if !defined(QT_NO_CONCURRENT) || defined (Q_CLANG_QDOC)

#include <QtConcurrent/qtconcurrentiteratekernel.h>
#include <QtConcurrent/qtconcurrentreducekernel.h>

QT_BEGIN_NAMESPACE


namespace QtConcurrent {

// map kernel, works with both parallel-for and parallel-while
template <typename Iterator, typename MapFunctor>
class MapKernel : public IterateKernel<Iterator, void>
{
    MapFunctor map;
public:
    typedef void ReturnType;
    MapKernel(Iterator begin, Iterator end, MapFunctor _map)
        : IterateKernel<Iterator, void>(begin, end), map(_map)
    { }

    bool runIteration(Iterator it, int, void *) override
    {
        map(*it);
        return false;
    }

    bool runIterations(Iterator sequenceBeginIterator, int beginIndex, int endIndex, void *) override
    {
        Iterator it = sequenceBeginIterator;
        std::advance(it, beginIndex);
        for (int i = beginIndex; i < endIndex; ++i) {
            runIteration(it, i, nullptr);
            std::advance(it, 1);
        }

        return false;
    }
};

template <typename ReducedResultType,
          typename Iterator,
          typename MapFunctor,
          typename ReduceFunctor,
          typename Reducer = ReduceKernel<ReduceFunctor,
                                          ReducedResultType,
                                          typename MapFunctor::result_type> >
class MappedReducedKernel : public IterateKernel<Iterator, ReducedResultType>
{
    ReducedResultType reducedResult;
    MapFunctor map;
    ReduceFunctor reduce;
    Reducer reducer;
public:
    typedef ReducedResultType ReturnType;
    MappedReducedKernel(Iterator begin, Iterator end, MapFunctor _map, ReduceFunctor _reduce, ReduceOptions reduceOptions)
        : IterateKernel<Iterator, ReducedResultType>(begin, end), reducedResult(), map(_map), reduce(_reduce), reducer(reduceOptions)
    { }

    MappedReducedKernel(ReducedResultType initialValue,
                     MapFunctor _map,
                     ReduceFunctor _reduce)
        : reducedResult(initialValue), map(_map), reduce(_reduce)
    { }

    bool runIteration(Iterator it, int index, ReducedResultType *) override
    {
        IntermediateResults<typename MapFunctor::result_type> results;
        results.begin = index;
        results.end = index + 1;

        results.vector.append(map(*it));
        reducer.runReduce(reduce, reducedResult, results);
        return false;
    }

    bool runIterations(Iterator sequenceBeginIterator, int beginIndex, int endIndex, ReducedResultType *) override
    {
        IntermediateResults<typename MapFunctor::result_type> results;
        results.begin = beginIndex;
        results.end = endIndex;
        results.vector.reserve(endIndex - beginIndex);

        Iterator it = sequenceBeginIterator;
        std::advance(it, beginIndex);
        for (int i = beginIndex; i < endIndex; ++i) {
            results.vector.append(map(*(it)));
            std::advance(it, 1);
        }

        reducer.runReduce(reduce, reducedResult, results);
        return false;
    }

    void finish() override
    {
        reducer.finish(reduce, reducedResult);
    }

    bool shouldThrottleThread() override
    {
        return IterateKernel<Iterator, ReducedResultType>::shouldThrottleThread() || reducer.shouldThrottle();
    }

    bool shouldStartThread() override
    {
        return IterateKernel<Iterator, ReducedResultType>::shouldStartThread() && reducer.shouldStartThread();
    }

    typedef ReducedResultType ResultType;
    ReducedResultType *result() override
    {
        return &reducedResult;
    }
};

template <typename Iterator, typename MapFunctor>
class MappedEachKernel : public IterateKernel<Iterator, typename MapFunctor::result_type>
{
    MapFunctor map;
    typedef typename MapFunctor::result_type T;
public:
    typedef T ReturnType;
    typedef T ResultType;

    MappedEachKernel(Iterator begin, Iterator end, MapFunctor _map)
        : IterateKernel<Iterator, T>(begin, end), map(_map) { }

    bool runIteration(Iterator it, int,  T *result) override
    {
        *result = map(*it);
        return true;
    }

    bool runIterations(Iterator sequenceBeginIterator, int beginIndex, int endIndex, T *results) override
    {

        Iterator it = sequenceBeginIterator;
        std::advance(it, beginIndex);
        for (int i = beginIndex; i < endIndex; ++i) {
            runIteration(it, i, results + (i - beginIndex));
            std::advance(it, 1);
        }

        return true;
    }
};

//! [qtconcurrentmapkernel-1]
template <typename Iterator, typename Functor>
inline ThreadEngineStarter<void> startMap(Iterator begin, Iterator end, Functor functor)
{
    return startThreadEngine(new MapKernel<Iterator, Functor>(begin, end, functor));
}

//! [qtconcurrentmapkernel-2]
template <typename T, typename Iterator, typename Functor>
inline ThreadEngineStarter<T> startMapped(Iterator begin, Iterator end, Functor functor)
{
    return startThreadEngine(new MappedEachKernel<Iterator, Functor>(begin, end, functor));
}

/*
    The SequnceHolder class is used to hold a reference to the
    sequence we are working on.
*/
template <typename Sequence, typename Base, typename Functor>
struct SequenceHolder1 : public Base
{
    SequenceHolder1(const Sequence &_sequence, Functor functor)
        : Base(_sequence.begin(), _sequence.end(), functor), sequence(_sequence)
    { }

    Sequence sequence;

    void finish() override
    {
        Base::finish();
        // Clear the sequence to make sure all temporaries are destroyed
        // before finished is signaled.
        sequence = Sequence();
    }
};

//! [qtconcurrentmapkernel-3]
template <typename T, typename Sequence, typename Functor>
inline ThreadEngineStarter<T> startMapped(const Sequence &sequence, Functor functor)
{
    typedef SequenceHolder1<Sequence,
                            MappedEachKernel<typename Sequence::const_iterator , Functor>, Functor>
                            SequenceHolderType;

    return startThreadEngine(new SequenceHolderType(sequence, functor));
}

//! [qtconcurrentmapkernel-4]
template <typename IntermediateType, typename ResultType, typename Sequence, typename MapFunctor, typename ReduceFunctor>
inline ThreadEngineStarter<ResultType> startMappedReduced(const Sequence & sequence,
                                                           MapFunctor mapFunctor, ReduceFunctor reduceFunctor,
                                                           ReduceOptions options)
{
    typedef typename Sequence::const_iterator Iterator;
    typedef ReduceKernel<ReduceFunctor, ResultType, IntermediateType> Reducer;
    typedef MappedReducedKernel<ResultType, Iterator, MapFunctor, ReduceFunctor, Reducer> MappedReduceType;
    typedef SequenceHolder2<Sequence, MappedReduceType, MapFunctor, ReduceFunctor> SequenceHolderType;
    return startThreadEngine(new SequenceHolderType(sequence, mapFunctor, reduceFunctor, options));
}

//! [qtconcurrentmapkernel-5]
template <typename IntermediateType, typename ResultType, typename Iterator, typename MapFunctor, typename ReduceFunctor>
inline ThreadEngineStarter<ResultType> startMappedReduced(Iterator begin, Iterator end,
                                                           MapFunctor mapFunctor, ReduceFunctor reduceFunctor,
                                                           ReduceOptions options)
{
    typedef ReduceKernel<ReduceFunctor, ResultType, IntermediateType> Reducer;
    typedef MappedReducedKernel<ResultType, Iterator, MapFunctor, ReduceFunctor, Reducer> MappedReduceType;
    return startThreadEngine(new MappedReduceType(begin, end, mapFunctor, reduceFunctor, options));
}

} // namespace QtConcurrent


QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
