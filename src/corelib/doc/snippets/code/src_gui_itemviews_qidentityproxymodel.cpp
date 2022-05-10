// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class DateFormatProxyModel : public QIdentityProxyModel
{
  // ...

  void setDateFormatString(const QString &formatString)
  {
    m_formatString = formatString;
  }

  QVariant data(const QModelIndex &index, int role) const override
  {
    if (role != Qt::DisplayRole)
      return QIdentityProxyModel::data(index, role);

    const QDateTime dateTime = sourceModel()->data(SourceClass::DateRole).toDateTime();

    return dateTime.toString(m_formatString);
  }

private:
  QString m_formatString;
};
//! [0]
