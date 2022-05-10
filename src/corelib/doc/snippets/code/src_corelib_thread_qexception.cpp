// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]

class MyException : public QException
{
public:
    void raise() const override { throw *this; }
    MyException *clone() const override { return new MyException(*this); }
};

//! [0]


//! [1]

try  {
    QtConcurrent::blockingMap(list, throwFunction); // throwFunction throws MyException
} catch (MyException &e) {
    // handle exception
}

//! [1]


//! [2]

void MyException::raise() const { throw *this; }

//! [2]


//! [3]

MyException *MyException::clone() const { return new MyException(*this); }

//! [3]

//! [4]

try {
    auto f = QtConcurrent::run([] { throw MyException {}; });
    // ...
} catch (const QUnhandledException &e) {
    try {
        if (e.exception())
            std::rethrow_exception(e.exception());
    } catch (const MyException &ex) {
        // Process 'ex'
    }
}

//! [4]
