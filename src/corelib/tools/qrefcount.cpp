// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

/*!
  \class QtPrivate::RefCount
  \inmodule QtCore
  \internal

  QRefCount implements atomic ref counting for Qt's shared classes. It behaves very similar
  to QAtomicInt, but ignores negative ref counts.

  This can be used to allow to implement e.g. const read-only QStringData objects. QString::shared_null and
  the qs(...) macro make use of this feature.
*/
