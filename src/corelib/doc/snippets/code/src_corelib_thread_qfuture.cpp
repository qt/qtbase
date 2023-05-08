// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QFuture<QString> future = ...;

QFuture<QString>::const_iterator i;
for (i = future.constBegin(); i != future.constEnd(); ++i)
    cout << qPrintable(*i) << endl;
//! [0]


//! [1]
QFuture<QString> future;
...
QFutureIterator<QString> i(future);
while (i.hasNext())
    QString s = i.next();
//! [1]


//! [2]
QFutureIterator<QString> i(future);
i.toBack();
while (i.hasPrevious())
    QString s = i.previous();
//! [2]

//! [3]
using NetworkReply = std::variant<QByteArray, QNetworkReply::NetworkError>;

enum class IOError { FailedToRead, FailedToWrite };
using IOResult = std::variant<QString, IOError>;
//! [3]

//! [4]
QFuture<IOResult> future = QtConcurrent::run([url] {
        ...
        return NetworkReply(QNetworkReply::TimeoutError);
}).then([](NetworkReply reply) {
    if (auto error = std::get_if<QNetworkReply::NetworkError>(&reply))
        return IOResult(IOError::FailedToRead);

    auto data = std::get_if<QByteArray>(&reply);
    // try to write *data and return IOError::FailedToWrite on failure
    ...
});

auto result = future.result();
if (auto filePath = std::get_if<QString>(&result)) {
    // do something with *filePath
else
    // process the error
//! [4]

//! [5]
QFuture<int> future = ...;
    future.then([](QFuture<int> f) {
        try {
            ...
            auto result = f.result();
            ...
        } catch (QException &e) {
            // handle the exception
        }
    }).then(...);
//! [5]

//! [6]
QFuture<int> future = ...;
auto continuation = future.then([](int res1){ ... }).then([](int res2){ ... })...
...
// future throws an exception
try {
    auto result = continuation.result();
} catch (QException &e) {
    // handle the exception
}
//! [6]

//! [7]
QFuture<int> future = ...;
auto resultFuture = future.then([](int res) {
    ...
    throw Error();
    ...
}).onFailed([](const Error &e) {
    // Handle exceptions of type Error
    ...
    return -1;
}).onFailed([] {
    // Handle all other types of errors
    ...
    return -1;
});

auto result = resultFuture.result(); // result is -1
//! [7]

//! [8]
QFuture<int> future = ...;
future.then([](int res) {
    ...
    throw std::runtime_error("message");
    ...
}).onFailed([](const std::exception &e) {
    // This handler will be invoked
}).onFailed([](const std::runtime_error &e) {
    // This handler won't be invoked, because of the handler above.
});
//! [8]

//! [9]
QFuture<int> future = ...;
auto resultFuture = future.then([](int res) {
    ...
    throw Error("message");
    ...
}).onFailed([](const std::exception &e) {
    // Won't be invoked
}).onFailed([](const QException &e) {
    // Won't be invoked
});

try {
    auto result = resultFuture.result();
} catch(...) {
    // Handle the exception
}
//! [9]

//! [10]
class Object : public QObject
{
    Q_OBJECT
    ...
signals:
    void noArgSignal();
    void singleArgSignal(int value);
    void multipleArgs(int value1, double value2, const QString &value3);
};
//! [10]

//! [11]
Object object;
QFuture<void> voidFuture = QtFuture::connect(&object, &Object::noArgSignal);
QFuture<int> intFuture = QtFuture::connect(&object, &Object::singleArgSignal);

using Args = std::tuple<int, double, QString>;
QFuture<Args> tupleFuture = QtFuture::connect(&object, &Object::multipleArgs)
//! [11]

//! [12]
QtFuture::connect(&object, &Object::singleArgSignal).then([](int value) {
    // do something with the value
});
//! [12]

//! [13]
QtFuture::connect(&object, &Object::singleArgSignal).then(QtFuture::Launch::Async, [](int value) {
    // this will run in a new thread
});
//! [13]

//! [14]
QtFuture::connect(&object, &Object::singleArgSignal).then([](int value) {
    ...
    throw std::exception();
    ...
}).onFailed([](const std::exception &e) {
    // handle the exception
}).onFailed([] {
    // handle other exceptions
});
//! [14]

//! [15]
QFuture<int> testFuture = ...;
auto resultFuture = testFuture.then([](int res) {
    // Block 1
}).onCanceled([] {
    // Block 2
}).onFailed([] {
    // Block 3
}).then([] {
    // Block 4
}).onFailed([] {
    // Block 5
}).onCanceled([] {
    // Block 6
});
//! [15]

//! [16]
QFuture<int> testFuture = ...;
auto resultFuture = testFuture.then([](int res) {
    // Block 1
}).onFailed([] {
    // Block 3
}).then([] {
    // Block 4
}).onFailed([] {
    // Block 5
}).onCanceled([] {
    // Block 6
});
//! [16]

//! [17]
// somewhere in the main thread
auto future = QtConcurrent::run([] {
    // This will run in a separate thread
    ...
}).then(this, [] {
   // Update UI elements
});
//! [17]

//! [18]
auto future = QtConcurrent::run([] {
    ...
}).then(this, [] {
   // Update UI elements
}).then([] {
    // This will also run in the main thread
});
//! [18]

//! [19]
// somewhere in the main thread
auto future = QtConcurrent::run([] {
    // This will run in a separate thread
    ...
    throw std::exception();
}).onFailed(this, [] {
   // Update UI elements
});
//! [19]

//! [20]
QObject *context = ...;
auto future = cachedResultsReady ? QtFuture::makeReadyValueFuture(result)
                                 : QtConcurrent::run([] { /* compute result */});
auto continuation = future.then(context, [] (Result result) {
    // Runs in the context's thread
}).then([] {
    // May or may not run in the context's thread
});
//! [20]

//! [21]
QFuture<int> testFuture = ...;
auto resultFuture = testFuture.then([](int res) {
    // Block 1
    ...
    return 1;
}).then([](int res) {
    // Block 2
    ...
    return 2;
}).onCanceled([] {
    // Block 3
    ...
    return -1;
});
//! [21]

//! [22]
QList<QFuture<int>> inputFutures {...};

// whenAll has type QFuture<QList<QFuture<int>>>
auto whenAll = QtFuture::whenAll(inputFutures.begin(), inputFutures.end());

// whenAllVector has type QFuture<std::vector<QFuture<int>>>
auto whenAllVector =
        QtFuture::whenAll<std::vector<QFuture<int>>>(inputFutures.begin(), inputFutures.end());
//! [22]

//! [23]
QList<QFuture<int>> inputFutures {...};

QtFuture::whenAll(inputFutures.begin(), inputFutures.end())
        .then([](const QList<QFuture<int>> &results) {
            for (auto future : results) {
                if (future.isCanceled())
                    // handle the cancellation (possibly due to an exception)
                else
                    // do something with the result
            }
        });
//! [23]

//! [24]

QFuture<int> intFuture = ...;
QFuture<QString> stringFuture = ...;
QFuture<void> voidFuture = ...;

using FuturesVariant = std::variant<QFuture<int>, QFuture<QString>, QFuture<void>>;

// whenAll has type QFuture<QList<FuturesVariant>>
auto whenAll = QtFuture::whenAll(intFuture, stringFuture, voidFuture);

// whenAllVector has type QFuture<std::vector<FuturesVariant>>
auto whenAllVector =
        QtFuture::whenAll<std::vector<FuturesVariant>>(intFuture, stringFuture, voidFuture);

//! [24]

//! [25]
QFuture<int> intFuture = ...;
QFuture<QString> stringFuture = ...;
QFuture<void> voidFuture = ...;

using FuturesVariant = std::variant<QFuture<int>, QFuture<QString>, QFuture<void>>;

QtFuture::whenAll(intFuture, stringFuture, voidFuture)
        .then([](const QList<FuturesVariant> &results) {
            ...
            for (auto result : results)
            {
                // assuming handleResult() is overloaded based on the QFuture type
                std::visit([](auto &&future) { handleResult(future); }, result);
            }
            ...
        });
//! [25]

//! [26]
QList<QFuture<int>> inputFutures = ...;

QtFuture::whenAny(inputFutures.begin(), inputFutures.end())
        .then([](const QtFuture::WhenAnyResult<int> &result) {
            qsizetype index = result.index;
            QFuture<int> future = result.future;
            // ...
        });
//! [26]

//! [27]
QFuture<int> intFuture = ...;
QFuture<QString> stringFuture = ...;
QFuture<void> voidFuture = ...;

using FuturesVariant = std::variant<QFuture<int>, QFuture<QString>, QFuture<void>>;

QtFuture::whenAny(intFuture, stringFuture, voidFuture).then([](const FuturesVariant &result) {
    ...
    // assuming handleResult() is overloaded based on the QFuture type
    std::visit([](auto &&future) { handleResult(future); }, result);
    ...
});
//! [27]

//! [28]

QFuture<QFuture<int>> outerFuture = ...;
QFuture<int> unwrappedFuture = outerFuture.unwrap();

//! [28]

//! [29]

auto downloadImages = [] (const QUrl &url) {
    QList<QImage> images;
    ...
    return images;
};

auto processImages = [](const QList<QImage> &images) {
   return QtConcurrent::mappedReduced(images, scale, reduceImages);
}

auto show = [](const QImage &image) { ... };

auto future = QtConcurrent::run(downloadImages, url)
               .then(processImages)
               .unwrap()
               .then(show);
//! [29]

//! [30]

QFuture<QFuture<QFuture<int>>>> outerFuture;
QFuture<int> unwrappedFuture = outerFuture.unwrap();

//! [30]

//! [31]
QPromise<int> p;

QFuture<int> f1 = p.future();
f1.then([](int) { qDebug("first"); });

QFuture<int> f2 = p.future();
f2.then([](int) { qDebug("second"); });

p.start();
p.addResult(42);
p.finish();
//! [31]

//! [32]
const std::vector<int> values{1, 2, 3};
auto f = QtFuture::makeReadyRangeFuture(values);
//! [32]

//! [33]
auto f = QtFuture::makeReadyRangeFuture({1, 2, 3});
//! [33]

//! [34]
const int count = f.resultCount(); // count == 3
const auto results = f.results(); // results == { 1, 2, 3 }
//! [34]

//! [35]
auto f = QtFuture::makeReadyValueFuture(std::make_unique<int>(42));
...
const int result = *f.takeResult(); // result == 42
//! [35]

//! [36]
auto f = QtFuture::makeReadyVoidFuture();
...
const bool started = f.isStarted(); // started == true
const bool running = f.isRunning(); // running == false
const bool finished = f.isFinished(); // finished == true
//! [36]

//! [37]
QObject *context = ...;
auto future = ...;
auto continuation = future.then(context, [context](Result result) {
                               // ...
                           }).onCanceled([context = QPointer(context)] {
                               if (!context)
                                   return;  // context was destroyed already
                               // handle cancellation
                           });

//! [37]
