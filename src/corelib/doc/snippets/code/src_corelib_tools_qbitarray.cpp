// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QBitArray ba(200);
//! [0]


//! [1]
QBitArray ba;
ba.resize(3);
ba[0] = true;
ba[1] = false;
ba[2] = true;
//! [1]


//! [2]
QBitArray ba(3);
ba.setBit(0, true);
ba.setBit(1, false);
ba.setBit(2, true);
//! [2]


//! [3]
QBitArray x(5);
x.setBit(3, true);
// x: [ 0, 0, 0, 1, 0 ]

QBitArray y(5);
y.setBit(4, true);
// y: [ 0, 0, 0, 0, 1 ]

x |= y;
// x: [ 0, 0, 0, 1, 1 ]
//! [3]


//! [4]
QBitArray().isNull();           // returns true
QBitArray().isEmpty();          // returns true

QBitArray(0).isNull();          // returns false
QBitArray(0).isEmpty();         // returns true

QBitArray(3).isNull();          // returns false
QBitArray(3).isEmpty();         // returns false
//! [4]


//! [5]
QBitArray().isNull();           // returns true
QBitArray(0).isNull();          // returns false
QBitArray(3).isNull();          // returns false
//! [5]


//! [6]
QBitArray ba(8);
ba.fill(true);
// ba: [ 1, 1, 1, 1, 1, 1, 1, 1 ]

ba.fill(false, 2);
// ba: [ 0, 0 ]
//! [6]


//! [7]
QBitArray a(3);
a[0] = false;
a[1] = true;
a[2] = a[0] ^ a[1];
//! [7]


//! [8]
QBitArray a(3);
QBitArray b(2);
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
a &= b;                         // a: [ 1, 0, 0 ]
//! [8]


//! [9]
QBitArray a(3);
QBitArray b(2);
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
a |= b;                         // a: [ 1, 1, 1 ]
//! [9]


//! [10]
QBitArray a(3);
QBitArray b(2);
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
a ^= b;                         // a: [ 0, 1, 1 ]
//! [10]


//! [11]
QBitArray a(3);
QBitArray b;
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b = ~a;                         // b: [ 0, 1, 0 ]
//! [11]


//! [12]
QBitArray a(3);
QBitArray b(2);
QBitArray c;
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
c = a & b;                      // c: [ 1, 0, 0 ]
//! [12]


//! [13]
QBitArray a(3);
QBitArray b(2);
QBitArray c;
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
c = a | b;                      // c: [ 1, 1, 1 ]
//! [13]


//! [14]
QBitArray a(3);
QBitArray b(2);
QBitArray c;
a[0] = 1; a[1] = 0; a[2] = 1;   // a: [ 1, 0, 1 ]
b[0] = 1; b[1] = 1;             // b: [ 1, 1 ]
c = a ^ b;                      // c: [ 0, 1, 1 ]
//! [14]

//! [15]
QBitArray ba(4);
ba.fill(true, 1, 2);            // ba: [ 0, 1, 0, 0 ]
ba.fill(true, 1, 3);            // ba: [ 0, 1, 1, 0 ]
ba.fill(true, 1, 4);            // ba: [ 0, 1, 1, 1 ]
//! [15]
