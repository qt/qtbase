/* * This file is part of libgq *
 *
 * Copyright (C) 2009 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef GCONFITEM_H
#define GCONFITEM_H

#include <QVariant>
#include <QStringList>
#include <QObject>

/*!

  \brief GConfItem is a simple C++ wrapper for GConf.

  Creating a GConfItem instance gives you access to a single GConf
  key.  You can get and set its value, and connect to its
  valueChanged() signal to be notified about changes.

  The value of a GConf key is returned to you as a QVariant, and you
  pass in a QVariant when setting the value.  GConfItem converts
  between a QVariant and GConf values as needed, and according to the
  following rules:

  - A QVariant of type QVariant::Invalid denotes an unset GConf key.

  - QVariant::Int, QVariant::Double, QVariant::Bool are converted to
    and from the obvious equivalents.

  - QVariant::String is converted to/from a GConf string and always
    uses the UTF-8 encoding.  No other encoding is supported.

  - QVariant::StringList is converted to a list of UTF-8 strings.

  - QVariant::List (which denotes a QList<QVariant>) is converted
    to/from a GConf list.  All elements of such a list must have the
    same type, and that type must be one of QVariant::Int,
    QVariant::Double, QVariant::Bool, or QVariant::String.  (A list of
    strings is returned as a QVariant::StringList, however, when you
    get it back.)

  - Any other QVariant or GConf value is essentially ignored.

  \warning GConfItem is as thread-safe as GConf.

*/

class GConfItem : public QObject
{
    Q_OBJECT

 public:
    /*! Initializes a GConfItem to access the GConf key denoted by
        \a key.  Key names should follow the normal GConf conventions
        like "/myapp/settings/first".

        \param key    The name of the key.
        \param parent Parent object
    */
    explicit GConfItem(const QString &key, QObject *parent = 0);

    /*! Finalizes a GConfItem.
     */
    virtual ~GConfItem();

    /*! Returns the key of this item, as given to the constructor.
     */
    QString key() const;

    /*! Returns the current value of this item, as a QVariant.
     */
    QVariant value() const;

    /*! Returns the current value of this item, as a QVariant.  If
     *  there is no value for this item, return \a def instead.
     */
    QVariant value(const QVariant &def) const;

    /*! Set the value of this item to \a val.  If \a val can not be
        represented in GConf or GConf refuses to accept it for other
        reasons, the current value is not changed and nothing happens.

        When the new value is different from the old value, the
        changedValue() signal is emitted on this GConfItem as part
        of calling set(), but other GConfItem:s for the same key do
        only receive a notification once the main loop runs.

        \param val  The new value.
    */
    void set(const QVariant &val);

    /*! Unset this item.  This is equivalent to

        \code
        item.set(QVariant(QVariant::Invalid));
        \endcode
     */
    void unset();

    /*! Return a list of the directories below this item.  The
        returned strings are absolute key names like
        "/myapp/settings".

        A directory is a key that has children.  The same key might
        also have a value, but that is confusing and best avoided.
    */
    QList<QString> listDirs() const;

    /*! Return a list of entries below this item.  The returned
        strings are absolute key names like "/myapp/settings/first".

        A entry is a key that has a value.  The same key might also
        have children, but that is confusing and is best avoided.
    */
    QList<QString> listEntries() const;

 signals:
    /*! Emitted when the value of this item has changed.
     */
    void valueChanged();

 private:
    friend struct GConfItemPrivate;
    struct GConfItemPrivate *priv;

    void update_value(bool emit_signal);
};

#endif // GCONFITEM_H
