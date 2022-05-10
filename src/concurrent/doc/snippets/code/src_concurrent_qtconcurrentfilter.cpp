// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
bool function(const T &t);
//! [0]


//! [1]
bool allLowerCase(const QString &string)
{
    return string.lowered() == string;
}

QStringList strings = ...;
QFuture<QString> lowerCaseStrings = QtConcurrent::filtered(strings, allLowerCase);
//! [1]


//! [2]
QStringList strings = ...;
QFuture<void> future = QtConcurrent::filter(strings, allLowerCase);
//! [2]


//! [3]
V function(T &result, const U &intermediate)
//! [3]


//! [4]
void addToDictionary(QSet<QString> &dictionary, const QString &string)
{
    dictionary.insert(string);
}

QStringList strings = ...;
QFuture<QSet<QString>> dictionary = QtConcurrent::filteredReduced(strings, allLowerCase, addToDictionary);
//! [4]


//! [5]
QStringList strings = ...;
QFuture<QString> lowerCaseStrings = QtConcurrent::filtered(strings.constBegin(), strings.constEnd(), allLowerCase);

// filter in-place only works on non-const iterators
QFuture<void> future = QtConcurrent::filter(strings.begin(), strings.end(), allLowerCase);

QFuture<QSet<QString>> dictionary = QtConcurrent::filteredReduced(strings.constBegin(), strings.constEnd(), allLowerCase, addToDictionary);
//! [5]


//! [6]
QStringList strings = ...;

// each call blocks until the entire operation is finished
QStringList lowerCaseStrings = QtConcurrent::blockingFiltered(strings, allLowerCase);


QtConcurrent::blockingFilter(strings, allLowerCase);

QSet<QString> dictionary = QtConcurrent::blockingFilteredReduced(strings, allLowerCase, addToDictionary);
//! [6]


//! [7]
// keep only images with an alpha channel
QList<QImage> images = ...;
QFuture<void> alphaImages = QtConcurrent::filter(images, &QImage::hasAlphaChannel);

// retrieve gray scale images
QList<QImage> images = ...;
QFuture<QImage> grayscaleImages = QtConcurrent::filtered(images, &QImage::isGrayscale);

// create a set of all printable characters
QList<QChar> characters = ...;
QFuture<QSet<QChar>> set = QtConcurrent::filteredReduced(characters, qOverload<>(&QChar::isPrint),
                                                         qOverload<const QChar&>(&QSet<QChar>::insert));
//! [7]


//! [8]
// can mix normal functions and member functions with QtConcurrent::filteredReduced()

// create a dictionary of all lower cased strings
extern bool allLowerCase(const QString &string);
QStringList strings = ...;
QFuture<QSet<QString>> lowerCase = QtConcurrent::filteredReduced(strings, allLowerCase,
                                                                 qOverload<const QString&>(&QSet<QString>::insert));

// create a collage of all gray scale images
extern void addToCollage(QImage &collage, const QImage &grayscaleImage);
QList<QImage> images = ...;
QFuture<QImage> collage = QtConcurrent::filteredReduced(images, &QImage::isGrayscale, addToCollage);
//! [8]


//! [9]
bool QString::contains(const QRegularExpression &regexp) const;
//! [9]


//! [12]
QStringList strings = ...;
QFuture<QString> future = QtConcurrent::filtered(list, [](const QString &str) {
    return str.contains(QRegularExpression("^\\S+$")); // matches strings without whitespace
});
//! [12]

//! [13]
struct StartsWith
{
    StartsWith(const QString &string)
    : m_string(string) { }

    bool operator()(const QString &testString)
    {
        return testString.startsWith(m_string);
    }

    QString m_string;
};

QList<QString> strings = ...;
QFuture<QString> fooString = QtConcurrent::filtered(strings, StartsWith(QLatin1String("Foo")));
//! [13]

//! [14]
struct StringTransform
{
    void operator()(QString &result, const QString &value);
};

QFuture<QString> fooString =
        QtConcurrent::filteredReduced(strings, StartsWith(QLatin1String("Foo")), StringTransform());
//! [14]

//! [15]
// keep only even integers
QList<int> list { 1, 2, 3, 4 };
QtConcurrent::blockingFilter(list, [](int n) { return (n & 1) == 0; });

// retrieve only even integers
QList<int> list2 { 1, 2, 3, 4 };
QFuture<int> future = QtConcurrent::filtered(list2, [](int x) {
    return (x & 1) == 0;
});
QList<int> results = future.results();

// add up all even integers
QList<int> list3 { 1, 2, 3, 4 };
QFuture<int> sum = QtConcurrent::filteredReduced(list3,
    [](int x) {
        return (x & 1) == 0;
    },
    [](int &sum, int x) {
        sum += x;
    }
);
//! [15]

//! [16]
void intSumReduce(int &sum, int x)
{
    sum += x;
}

QList<int> list { 1, 2, 3, 4 };
QFuture<int> sum = QtConcurrent::filteredReduced(list,
    [] (int x) {
        return (x & 1) == 0;
    },
    intSumReduce
);
//! [16]

//! [17]
bool keepEvenIntegers(int x)
{
    return (x & 1) == 0;
}

QList<int> list { 1, 2, 3, 4 };
QFuture<int> sum = QtConcurrent::filteredReduced(list,
    keepEvenIntegers,
    [](int &sum, int x) {
        sum += x;
    }
);
//! [17]
