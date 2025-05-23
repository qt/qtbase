// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

/*!
    \page qtconcurrentfilter.html
    \title Concurrent Filter and Filter-Reduce
    \brief Selecting values from a sequence and combining them, all in parallel.
    \ingroup thread

    The QtConcurrent::filter(), QtConcurrent::filtered() and
    QtConcurrent::filteredReduced() functions filter items in a sequence such
    as a QList in parallel. QtConcurrent::filter() modifies a
    sequence in-place, QtConcurrent::filtered() returns a new sequence
    containing the filtered content, and QtConcurrent::filteredReduced()
    returns a single result.

    These functions are part of the \l {Qt Concurrent} framework.

    Each of the above functions have a blocking variant that returns the final
    result instead of a QFuture. You use them in the same way as the
    asynchronous variants.

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 6

    Note that the result types above are not QFuture objects, but real result
    types (in this case, QStringList and QSet<QString>).

    \section1 Concurrent Filter

    QtConcurrent::filtered() takes an input sequence and a filter function.
    This filter function is then called for each item in the sequence, and a
    new sequence containing the filtered values is returned.

    The filter function must be of the form:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 0

    T must match the type stored in the sequence. The function returns \c true if
    the item should be kept, false if it should be discarded.

    This example shows how to keep strings that are all lower-case from a
    QStringList:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 1

    The results of the filter are made available through QFuture. See the
    QFuture and QFutureWatcher documentation for more information on how to
    use QFuture in your applications.

    If you want to modify a sequence in-place, use QtConcurrent::filter():

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 2

    Since the sequence is modified in place, QtConcurrent::filter() does not
    return any results via QFuture. However, you can still use QFuture and
    QFutureWatcher to monitor the status of the filter.

    \section2 Concurrent Filtered and Continuations

    The result of QtConcurrent::filtered() call is a QFuture that contains
    multiple results. When attaching a \c {.then()} continuation to such
    QFuture, make sure to use a continuation that takes QFuture as a parameter,
    otherwise only the first result will be processed:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 18

    In this example \c {badFuture} will only print a single result, while
    \c {goodFuture} will print all results.

    \section1 Concurrent Filter-Reduce

    QtConcurrent::filteredReduced() is similar to QtConcurrent::filtered(),
    but instead of returning a sequence with the filtered results, the results
    are combined into a single value using a reduce function.

    The reduce function must be of the form:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 3

    T is the type of the final result, U is the type of items being filtered.
    Note that the return value and return type of the reduce function are not
    used.

    Call QtConcurrent::filteredReduced() like this:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 4

    The reduce function will be called once for each result kept by the filter
    function, and should merge the \e{intermediate} into the \e{result}
    variable. QtConcurrent::filteredReduced() guarantees that only one thread
    will call reduce at a time, so using a mutex to lock the result variable
    is not necessary. The QtConcurrent::ReduceOptions enum provides a way to
    control the order in which the reduction is done.

    \section1 Additional API Features

    \section2 Using Iterators instead of Sequence

    Each of the above functions has a variant that takes an iterator range
    instead of a sequence. You use them in the same way as the sequence
    variants:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 5


    \section2 Using Member Functions

    QtConcurrent::filter(), QtConcurrent::filtered(), and
    QtConcurrent::filteredReduced() accept pointers to member functions.
    The member function class type must match the type stored in the sequence:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 7

    Note the use of qOverload. It is needed to resolve the ambiguity for the
    methods, that have multiple overloads.

    Also note that when using QtConcurrent::filteredReduced(), you can mix the use of
    normal and member functions freely:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 8

    \section2 Using Function Objects

    QtConcurrent::filter(), QtConcurrent::filtered(), and
    QtConcurrent::filteredReduced() accept function objects
    for the filter function. These function objects can be used to
    add state to a function call:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 13

    Function objects are also supported for the reduce function:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 14

    \section2 Using Lambda Expressions

    QtConcurrent::filter(), QtConcurrent::filtered(), and
    QtConcurrent::filteredReduced() accept lambda expressions for the filter and
    reduce function:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 15

    When using QtConcurrent::filteredReduced() or
    QtConcurrent::blockingFilteredReduced(), you can mix the use of normal
    functions, member functions and lambda expressions freely.

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 16

    You can also pass a lambda as a reduce object:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 17

    \section2 Wrapping Functions that Take Multiple Arguments

    If you want to use a filter function takes more than one argument, you can
    use a lambda function or \c std::bind() to transform it onto a function that
    takes one argument.

    As an example, we use QString::contains():

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 9

    QString::contains() takes 2 arguments (including the "this" pointer) and
    can't be used with QtConcurrent::filtered() directly, because
    QtConcurrent::filtered() expects a function that takes one argument. To
    use QString::contains() with QtConcurrent::filtered() we have to provide a
    value for the \e regexp argument:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 12
*/

/*!
  \class QtConcurrent::qValueType
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::qValueType<const T*>
  \inmodule QtConcurrent
  \internal
*/


/*!
  \class QtConcurrent::qValueType<T*>
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::FilterKernel
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::FilteredReducedKernel
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::FilteredEachKernel
  \inmodule QtConcurrent
  \internal
*/

/*!
  \fn [QtConcurrent-1] template <typename Sequence, typename KeepFunctor, typename ReduceFunctor> ThreadEngineStarter<void> QtConcurrent::filterInternal(Sequence &sequence, KeepFunctor &&keep, ReduceFunctor &&reduce)
  \internal
*/

/*!
    \fn template <typename Sequence, typename KeepFunctor> QFuture<void> QtConcurrent::filter(QThreadPool *pool, Sequence &sequence, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item in \a sequence.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true, the item is kept in \a sequence;
    otherwise, the item is removed from \a sequence.

    Note that this method doesn't have an overload working with iterators, because
    it invalidates the iterators of the sequence it operates on.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename Sequence, typename KeepFunctor> QFuture<void> QtConcurrent::filter(Sequence &sequence, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item in \a sequence. If
    \a filterFunction returns \c true, the item is kept in \a sequence;
    otherwise, the item is removed from \a sequence.

    Note that this method doesn't have an overload working with iterators, because
    it invalidates the iterators of the sequence it operates on.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename Sequence, typename KeepFunctor> QFuture<Sequence::value_type> QtConcurrent::filtered(QThreadPool *pool, Sequence &&sequence, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item in \a sequence and returns a
    new Sequence of kept items. All calls to \a filterFunction are invoked from the threads
    taken from the QThreadPool \a pool. If \a filterFunction returns \c true, a copy of
    the item is put in the new Sequence. Otherwise, the item will \e not
    appear in the new Sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename Sequence, typename KeepFunctor> QFuture<Sequence::value_type> QtConcurrent::filtered(Sequence &&sequence, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item in \a sequence and returns a
    new Sequence of kept items. If \a filterFunction returns \c true, a copy of
    the item is put in the new Sequence. Otherwise, the item will \e not
    appear in the new Sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename Iterator, typename KeepFunctor> QFuture<typename QtConcurrent::qValueType<Iterator>::value_type> QtConcurrent::filtered(QThreadPool *pool, Iterator begin, Iterator end, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item from \a begin to \a end and
    returns a new Sequence of kept items. All calls to \a filterFunction are invoked from the threads
    taken from the QThreadPool \a pool. If \a filterFunction returns \c true, a
    copy of the item is put in the new Sequence. Otherwise, the item will
    \e not appear in the new Sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename Iterator, typename KeepFunctor> QFuture<typename QtConcurrent::qValueType<Iterator>::value_type> QtConcurrent::filtered(Iterator begin, Iterator end, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item from \a begin to \a end and
    returns a new Sequence of kept items. If \a filterFunction returns \c true, a
    copy of the item is put in the new Sequence. Otherwise, the item will
    \e not appear in the new Sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor> QFuture<ResultType> QtConcurrent::filteredReduced(QThreadPool *pool, Sequence &&sequence, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item in \a sequence.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which \a reduceFunction
    is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
    the original sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor> QFuture<ResultType> QtConcurrent::filteredReduced(Sequence &&sequence, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item in \a sequence. If
    \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which \a reduceFunction
    is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
    the original sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor, typename InitialValueType> QFuture<ResultType> QtConcurrent::filteredReduced(QThreadPool *pool, Sequence &&sequence, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, InitialValueType &&initialValue, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item in \a sequence.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.
    The result value is initialized to \a initialValue when the function is
    called, and the first call to \a reduceFunction will operate on
    this value.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which \a reduceFunction
    is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
    the original sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor, typename InitialValueType> QFuture<ResultType> QtConcurrent::filteredReduced(Sequence &&sequence, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, InitialValueType &&initialValue, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item in \a sequence. If
    \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.
    The result value is initialized to \a initialValue when the function is
    called, and the first call to \a reduceFunction will operate on
    this value.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which \a reduceFunction
    is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
    the original sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor> QFuture<ResultType> QtConcurrent::filteredReduced(QThreadPool *pool, Iterator begin, Iterator end, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item from \a begin to \a end.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which
    \a reduceFunction is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
    of the original sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor> QFuture<ResultType> QtConcurrent::filteredReduced(Iterator begin, Iterator end, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item from \a begin to \a end. If
    \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which
    \a reduceFunction is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
    of the original sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor, typename InitialValueType> QFuture<ResultType> QtConcurrent::filteredReduced(QThreadPool *pool, Iterator begin, Iterator end, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, InitialValueType &&initialValue, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item from \a begin to \a end.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.
    The result value is initialized to \a initialValue when the function is
    called, and the first call to \a reduceFunction will operate on
    this value.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which
    \a reduceFunction is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
    of the original sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor, typename InitialValueType> QFuture<ResultType> QtConcurrent::filteredReduced(Iterator begin, Iterator end, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, InitialValueType &&initialValue, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item from \a begin to \a end. If
    \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.
    The result value is initialized to \a initialValue when the function is
    called, and the first call to \a reduceFunction will operate on
    this value.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which
    \a reduceFunction is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
    of the original sequence.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename Sequence, typename KeepFunctor> void QtConcurrent::blockingFilter(QThreadPool *pool, Sequence &sequence, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item in \a sequence.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true, the item is kept in \a sequence;
    otherwise, the item is removed from \a sequence.

    Note that this method doesn't have an overload working with iterators, because
    it invalidates the iterators of the sequence it operates on.

    \note This function will block until all items in the sequence have been processed.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename Sequence, typename KeepFunctor> void QtConcurrent::blockingFilter(Sequence &sequence, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item in \a sequence. If
    \a filterFunction returns \c true, the item is kept in \a sequence;
    otherwise, the item is removed from \a sequence.

    Note that this method doesn't have an overload working with iterators, because
    it invalidates the iterators of the sequence it operates on.

    \note This function will block until all items in the sequence have been processed.

    \sa {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename Sequence, typename KeepFunctor> Sequence QtConcurrent::blockingFiltered(QThreadPool *pool, Sequence &&sequence, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item in \a sequence and returns a
    new Sequence of kept items. All calls to \a filterFunction are invoked from the threads
    taken from the QThreadPool \a pool. If \a filterFunction returns \c true, a copy of
    the item is put in the new Sequence. Otherwise, the item will \e not
    appear in the new Sequence.

    \note This function will block until all items in the sequence have been processed.

    \sa filtered(), {Concurrent Filter and Filter-Reduce}
*/

/*!
  \fn template <typename Sequence, typename KeepFunctor> Sequence QtConcurrent::blockingFiltered(Sequence &&sequence, KeepFunctor &&filterFunction)

  Calls \a filterFunction once for each item in \a sequence and returns a
  new Sequence of kept items. If \a filterFunction returns \c true, a copy of
  the item is put in the new Sequence. Otherwise, the item will \e not
  appear in the new Sequence.

  \note This function will block until all items in the sequence have been processed.

  \sa filtered(), {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename OutputSequence, typename Iterator, typename KeepFunctor> OutputSequence QtConcurrent::blockingFiltered(QThreadPool *pool, Iterator begin, Iterator end, KeepFunctor &&filterFunction)

    Calls \a filterFunction once for each item from \a begin to \a end and
    returns a new Sequence of kept items. All calls to \a filterFunction are invoked from the threads
    taken from the QThreadPool \a pool. If \a filterFunction returns \c true, a
    copy of the item is put in the new Sequence. Otherwise, the item will
    \e not appear in the new Sequence.

    \note This function will block until the iterator reaches the end of the
    sequence being processed.

    \sa filtered(), {Concurrent Filter and Filter-Reduce}
*/

/*!
  \fn template <typename OutputSequence, typename Iterator, typename KeepFunctor> OutputSequence QtConcurrent::blockingFiltered(Iterator begin, Iterator end, KeepFunctor &&filterFunction)

  Calls \a filterFunction once for each item from \a begin to \a end and
  returns a new Sequence of kept items. If \a filterFunction returns \c true, a
  copy of the item is put in the new Sequence. Otherwise, the item will
  \e not appear in the new Sequence.

  \note This function will block until the iterator reaches the end of the
  sequence being processed.

  \sa filtered(), {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor> ResultType QtConcurrent::blockingFilteredReduced(QThreadPool *pool, Sequence &&sequence, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item in \a sequence.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which \a reduceFunction
    is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
    the original sequence.

    \note This function will block until all items in the sequence have been processed.

    \sa filteredReduced(), {Concurrent Filter and Filter-Reduce}
*/

/*!
  \fn template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor> ResultType QtConcurrent::blockingFilteredReduced(Sequence &&sequence, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

  Calls \a filterFunction once for each item in \a sequence. If
  \a filterFunction returns \c true for an item, that item is then passed to
  \a reduceFunction. In other words, the return value is the result of
  \a reduceFunction for each item where \a filterFunction returns \c true.

  Note that while \a filterFunction is called concurrently, only one thread
  at a time will call \a reduceFunction. The order in which \a reduceFunction
  is called is undefined if \a reduceOptions is
  QtConcurrent::UnorderedReduce. If \a reduceOptions is
  QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
  the original sequence.

  \note This function will block until all items in the sequence have been processed.

  \sa filteredReduced(), {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor, typename InitialValueType> ResultType QtConcurrent::blockingFilteredReduced(QThreadPool *pool, Sequence &&sequence, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, InitialValueType &&initialValue, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item in \a sequence.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.
    The result value is initialized to \a initialValue when the function is
    called, and the first call to \a reduceFunction will operate on
    this value.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which \a reduceFunction
    is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
    the original sequence.

    \note This function will block until all items in the sequence have been processed.

    \sa filteredReduced(), {Concurrent Filter and Filter-Reduce}
*/

/*!
  \fn template <typename ResultType, typename Sequence, typename KeepFunctor, typename ReduceFunctor, typename InitialValueType> ResultType QtConcurrent::blockingFilteredReduced(Sequence &&sequence, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, InitialValueType &&initialValue, QtConcurrent::ReduceOptions reduceOptions)

  Calls \a filterFunction once for each item in \a sequence. If
  \a filterFunction returns \c true for an item, that item is then passed to
  \a reduceFunction. In other words, the return value is the result of
  \a reduceFunction for each item where \a filterFunction returns \c true.
  The result value is initialized to \a initialValue when the function is
  called, and the first call to \a reduceFunction will operate on
  this value.

  Note that while \a filterFunction is called concurrently, only one thread
  at a time will call \a reduceFunction. The order in which \a reduceFunction
  is called is undefined if \a reduceOptions is
  QtConcurrent::UnorderedReduce. If \a reduceOptions is
  QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
  the original sequence.

  \note This function will block until all items in the sequence have been processed.

  \sa filteredReduced(), {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor> ResultType QtConcurrent::blockingFilteredReduced(QThreadPool *pool, Iterator begin, Iterator end, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item from \a begin to \a end.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which
    \a reduceFunction is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
    of the original sequence.

    \note This function will block until the iterator reaches the end of the
    sequence being processed.

    \sa filteredReduced(), {Concurrent Filter and Filter-Reduce}
*/

/*!
  \fn template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor> ResultType QtConcurrent::blockingFilteredReduced(Iterator begin, Iterator end, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

  Calls \a filterFunction once for each item from \a begin to \a end. If
  \a filterFunction returns \c true for an item, that item is then passed to
  \a reduceFunction. In other words, the return value is the result of
  \a reduceFunction for each item where \a filterFunction returns \c true.

  Note that while \a filterFunction is called concurrently, only one thread
  at a time will call \a reduceFunction. The order in which
  \a reduceFunction is called is undefined if \a reduceOptions is
  QtConcurrent::UnorderedReduce. If \a reduceOptions is
  QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
  of the original sequence.

  \note This function will block until the iterator reaches the end of the
  sequence being processed.

  \sa filteredReduced(), {Concurrent Filter and Filter-Reduce}
*/

/*!
    \fn template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor, typename InitialValueType> ResultType QtConcurrent::blockingFilteredReduced(QThreadPool *pool, Iterator begin, Iterator end, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, InitialValueType &&initialValue, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item from \a begin to \a end.
    All calls to \a filterFunction are invoked from the threads taken from the QThreadPool \a pool.
    If \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.
    The result value is initialized to \a initialValue when the function is
    called, and the first call to \a reduceFunction will operate on
    this value.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which
    \a reduceFunction is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
    of the original sequence.

    \note This function will block until the iterator reaches the end of the
    sequence being processed.

    \sa filteredReduced(), {Concurrent Filter and Filter-Reduce}
*/

/*!
  \fn template <typename ResultType, typename Iterator, typename KeepFunctor, typename ReduceFunctor, typename InitialValueType> ResultType QtConcurrent::blockingFilteredReduced(Iterator begin, Iterator end, KeepFunctor &&filterFunction, ReduceFunctor &&reduceFunction, InitialValueType &&initialValue, QtConcurrent::ReduceOptions reduceOptions)

  Calls \a filterFunction once for each item from \a begin to \a end. If
  \a filterFunction returns \c true for an item, that item is then passed to
  \a reduceFunction. In other words, the return value is the result of
  \a reduceFunction for each item where \a filterFunction returns \c true.
  The result value is initialized to \a initialValue when the function is
  called, and the first call to \a reduceFunction will operate on
  this value.

  Note that while \a filterFunction is called concurrently, only one thread
  at a time will call \a reduceFunction. The order in which
  \a reduceFunction is called is undefined if \a reduceOptions is
  QtConcurrent::UnorderedReduce. If \a reduceOptions is
  QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
  of the original sequence.

  \note This function will block until the iterator reaches the end of the
  sequence being processed.

  \sa filteredReduced(), {Concurrent Filter and Filter-Reduce}
*/

/*!
  \fn [QtConcurrent-2] ThreadEngineStarter<typename qValueType<Iterator>::value_type> QtConcurrent::startFiltered(QThreadPool *pool, Iterator begin, Iterator end, KeepFunctor &&functor)
  \internal
*/

/*!
  \fn [QtConcurrent-3] ThreadEngineStarter<typename Sequence::value_type> QtConcurrent::startFiltered(QThreadPool *pool, Sequence &&sequence, KeepFunctor &&functor)
  \internal
*/

/*!
  \fn [QtConcurrent-4] ThreadEngineStarter<ResultType> QtConcurrent::startFilteredReduced(QThreadPool *pool, Sequence &&sequence, MapFunctor &&mapFunctor, ReduceFunctor &&reduceFunctor, ReduceOptions options)
  \internal
*/

/*!
  \fn [QtConcurrent-5] ThreadEngineStarter<ResultType> QtConcurrent::startFilteredReduced(QThreadPool *pool, Iterator begin, Iterator end, MapFunctor &&mapFunctor, ReduceFunctor &&reduceFunctor, ReduceOptions options)
  \internal
*/

/*!
  \fn [QtConcurrent-6] ThreadEngineStarter<ResultType> QtConcurrent::startFilteredReduced(QThreadPool *pool, Sequence &&sequence, MapFunctor &&mapFunctor, ReduceFunctor &&reduceFunctor, ResultType &&initialValue, ReduceOptions options)
  \internal
*/

/*!
  \fn [QtConcurrent-7] ThreadEngineStarter<ResultType> QtConcurrent::startFilteredReduced(QThreadPool *pool, Iterator begin, Iterator end, MapFunctor &&mapFunctor, ReduceFunctor &&reduceFunctor, ResultType &&initialValue, ReduceOptions options)
  \internal
*/
