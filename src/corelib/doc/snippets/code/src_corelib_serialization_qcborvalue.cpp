// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    QCborValue(uuid) == QCborValue(QCborKnownTags::Uuid, uuid.toRfc4122());
//! [0]

//! [1]
    QCborValue value(QCborSimpleType(12));
//! [1]

//! [2]
    value.isSimpleType(QCborSimpleType(12));
//! [2]

//! [3]
    QCborValue(QUrl("https://example.com")) == QCborValue(QCborKnownTags::Url, "https://example.com");
//! [3]

//! [4]
    value.toMap().value(key);
//! [4]

//! [5]
    value.toMap().value(key);
//! [5]

//! [6]
    if (reader.isTag() && reader.toTag() == QCborKnownTags::Signature)
        reader.next();

    QCborValue contents = QCborValue::fromCbor(reader);
//! [6]
