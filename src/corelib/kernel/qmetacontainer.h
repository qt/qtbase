/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMETACONTAINER_H
#define QMETACONTAINER_H

#include <QtCore/qcontainerinfo.h>
#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

namespace QtMetaContainerPrivate {

enum IteratorCapability : quint8 {
    ForwardCapability       = 1 << 0,
    BiDirectionalCapability = 1 << 1,
    RandomAccessCapability  = 1 << 2,
};

Q_DECLARE_FLAGS(IteratorCapabilities, IteratorCapability)
Q_DECLARE_OPERATORS_FOR_FLAGS(IteratorCapabilities)

class QMetaSequenceInterface
{
public:
    enum Position : quint8 { AtBegin, AtEnd, Random };

    ushort revision;
    IteratorCapabilities iteratorCapabilities;
    Position addRemovePosition;
    QMetaType valueMetaType;

    using SizeFn = qsizetype(*)(const void *);
    SizeFn sizeFn;
    using ClearFn = void(*)(void *);
    ClearFn clearFn;

    using ElementAtIndexFn = void(*)(const void *, qsizetype, void *);
    ElementAtIndexFn elementAtIndexFn;
    using SetElementAtIndexFn = void(*)(void *, qsizetype, const void *);
    SetElementAtIndexFn setElementAtIndexFn;

    using AddElementFn = void(*)(void *, const void *);
    AddElementFn addElementFn;
    using RemoveElementFn = void(*)(void *);
    RemoveElementFn removeElementFn;

    using CreateIteratorFn = void *(*)(void *, Position);
    CreateIteratorFn createIteratorFn;
    using DestroyIteratorFn = void(*)(const void *);
    DestroyIteratorFn destroyIteratorFn;
    using CompareIteratorFn = bool(*)(const void *, const void *);
    CompareIteratorFn compareIteratorFn;
    using CopyIteratorFn = void(*)(void *, const void *);
    CopyIteratorFn copyIteratorFn;
    using AdvanceIteratorFn = void(*)(void *, qsizetype);
    AdvanceIteratorFn advanceIteratorFn;
    using DiffIteratorFn = qsizetype(*)(const void *, const void *);
    DiffIteratorFn diffIteratorFn;
    using ElementAtIteratorFn = void(*)(const void *, void *);
    ElementAtIteratorFn elementAtIteratorFn;
    using SetElementAtIteratorFn = void(*)(const void *, const void *);
    SetElementAtIteratorFn setElementAtIteratorFn;
    using InsertElementAtIteratorFn = void(*)(void *, const void *, const void *);
    InsertElementAtIteratorFn insertElementAtIteratorFn;
    using EraseElementAtIteratorFn = void(*)(void *, const void *);
    EraseElementAtIteratorFn eraseElementAtIteratorFn;

    using CreateConstIteratorFn = void *(*)(const void *, Position);
    CreateConstIteratorFn createConstIteratorFn;
    DestroyIteratorFn destroyConstIteratorFn;
    CompareIteratorFn compareConstIteratorFn;
    CopyIteratorFn copyConstIteratorFn;
    AdvanceIteratorFn advanceConstIteratorFn;
    DiffIteratorFn diffConstIteratorFn;
    ElementAtIteratorFn elementAtConstIteratorFn;
};

template<typename C>
class QMetaSequenceForContainer
{
    template <typename Iterator>
    static constexpr IteratorCapabilities capabilitiesForIterator()
    {
       using Tag = typename std::iterator_traits<Iterator>::iterator_category;
       IteratorCapabilities caps {};
       if constexpr (std::is_base_of_v<std::forward_iterator_tag, Tag>)
           caps |= ForwardCapability;
       if constexpr (std::is_base_of_v<std::bidirectional_iterator_tag, Tag>)
           caps |= BiDirectionalCapability;
       if constexpr (std::is_base_of_v<std::random_access_iterator_tag, Tag>)
           caps |= RandomAccessCapability;
       return caps;
    }

    static constexpr IteratorCapabilities getIteratorCapabilities()
    {
        if constexpr (QContainerTraits::has_iterator_v<C> && !std::is_const_v<C>)
            return capabilitiesForIterator<QContainerTraits::iterator<C>>();
        else if constexpr (QContainerTraits::has_const_iterator_v<C>)
            return capabilitiesForIterator<QContainerTraits::const_iterator<C>>();
        else
            return {};
    }

    static constexpr QMetaSequenceInterface::Position getAddRemovePosition()
    {
        if constexpr (QContainerTraits::has_push_back_v<C> && QContainerTraits::has_pop_back_v<C>)
            return QMetaSequenceInterface::AtEnd;
        if constexpr (QContainerTraits::has_push_front_v<C> && QContainerTraits::has_pop_front_v<C>)
            return QMetaSequenceInterface::AtBegin;
        return QMetaSequenceInterface::Random;
    }

    static constexpr QMetaType getValueMetaType()
    {
        return QMetaType::fromType<typename C::value_type>();
    }

    static constexpr QMetaSequenceInterface::SizeFn getSizeFn()
    {
        if constexpr (QContainerTraits::has_size_v<C>) {
            return [](const void *c) -> qsizetype { return static_cast<const C *>(c)->size(); };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::ClearFn getClearFn()
    {
        if constexpr (QContainerTraits::has_clear_v<C>) {
            return [](void *c) { return static_cast<C *>(c)->clear(); };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::ElementAtIndexFn getElementAtIndexFn()
    {
        if constexpr (QContainerTraits::has_at_v<C>) {
            return [](const void *c, qsizetype i, void *r) {
                *static_cast<QContainerTraits::value_type<C> *>(r)
                        = static_cast<const C *>(c)->at(i);
            };
        } else if constexpr (QContainerTraits::can_get_at_index_v<C>) {
            return [](const void *c, qsizetype i, void *r) {
                *static_cast<QContainerTraits::value_type<C> *>(r)
                        = (*static_cast<const C *>(c))[i];
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::SetElementAtIndexFn getSetElementAtIndexFn()
    {
        if constexpr (QContainerTraits::can_set_at_index_v<C>) {
            return [](void *c, qsizetype i, const void *e) {
                (*static_cast<C *>(c))[i]
                        = *static_cast<const QContainerTraits::value_type<C> *>(e);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::AddElementFn getAddElementFn()
    {
        if constexpr (QContainerTraits::has_push_back_v<C>) {
            return [](void *c, const void *v) {
                static_cast<C *>(c)->push_back(
                            *static_cast<const QContainerTraits::value_type<C> *>(v));
            };
        } else if constexpr (QContainerTraits::has_push_front_v<C>) {
            return [](void *c, const void *v) {
                static_cast<C *>(c)->push_front(
                            *static_cast<const QContainerTraits::value_type<C> *>(v));
            };
        } else if constexpr (QContainerTraits::has_insert_v<C>) {
            return [](void *c, const void *v) {
                static_cast<C *>(c)->insert(
                            *static_cast<const QContainerTraits::value_type<C> *>(v));
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::RemoveElementFn getRemoveElementFn()
    {
        if constexpr (QContainerTraits::has_pop_back_v<C>) {
            return [](void *c) { static_cast<C *>(c)->pop_back(); };
        } else if constexpr (QContainerTraits::has_pop_front_v<C>) {
            return [](void *c) { static_cast<C *>(c)->pop_front(); };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::CreateIteratorFn getCreateIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C> && !std::is_const_v<C>) {
            return [](void *c, QMetaSequenceInterface::Position p) -> void* {
                using Iterator = QContainerTraits::iterator<C>;
                switch (p) {
                case QMetaSequenceInterface::AtBegin:
                case QMetaSequenceInterface::Random:
                    return new Iterator(static_cast<C *>(c)->begin());
                    break;
                case QMetaSequenceInterface::AtEnd:
                    return new Iterator(static_cast<C *>(c)->end());
                    break;
                }
                return nullptr;
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::DestroyIteratorFn getDestroyIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C> && !std::is_const_v<C>) {
            return [](const void *i) {
                using Iterator = QContainerTraits::iterator<C>;
                delete static_cast<const Iterator *>(i);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::CompareIteratorFn getCompareIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C> && !std::is_const_v<C>) {
            return [](const void *i, const void *j) {
                using Iterator = QContainerTraits::iterator<C>;
                return *static_cast<const Iterator *>(i) == *static_cast<const Iterator *>(j);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::CopyIteratorFn getCopyIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C> && !std::is_const_v<C>) {
            return [](void *i, const void *j) {
                using Iterator = QContainerTraits::iterator<C>;
                *static_cast<Iterator *>(i) = *static_cast<const Iterator *>(j);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::AdvanceIteratorFn getAdvanceIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C> && !std::is_const_v<C>) {
            return [](void *i, qsizetype step) {
                std::advance(*static_cast<QContainerTraits::iterator<C> *>(i), step);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::DiffIteratorFn getDiffIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C> && !std::is_const_v<C>) {
            return [](const void *i, const void *j) -> qsizetype {
                return std::distance(*static_cast<const QContainerTraits::iterator<C> *>(j),
                                     *static_cast<const QContainerTraits::iterator<C> *>(i));
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::ElementAtIteratorFn getElementAtIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C>
                && QContainerTraits::can_get_at_iterator_v<C> && !std::is_const_v<C>) {
            return [](const void *i, void *r) {
                *static_cast<QContainerTraits::value_type<C> *>(r) =
                        *(*static_cast<const QContainerTraits::iterator<C> *>(i));
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::SetElementAtIteratorFn getSetElementAtIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C>
                && QContainerTraits::can_set_at_iterator_v<C> && !std::is_const_v<C>) {
            return [](const void *i, const void *e) {
                *(*static_cast<const QContainerTraits::iterator<C> *>(i))
                        = *static_cast<const QContainerTraits::value_type<C> *>(e);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::InsertElementAtIteratorFn getInsertElementAtIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C>
                && QContainerTraits::can_insert_at_iterator_v<C> && !std::is_const_v<C>) {
            return [](void *c, const void *i, const void *e) {
                static_cast<C *>(c)->insert(
                            *static_cast<const QContainerTraits::iterator<C> *>(i),
                            *static_cast<const QContainerTraits::value_type<C> *>(e));
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::EraseElementAtIteratorFn getEraseElementAtIteratorFn()
    {
        if constexpr (QContainerTraits::has_iterator_v<C>
                && QContainerTraits::can_erase_at_iterator_v<C> && !std::is_const_v<C>) {
            return [](void *c, const void *i) {
                static_cast<C *>(c)->erase(*static_cast<const QContainerTraits::iterator<C> *>(i));
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::CreateConstIteratorFn getCreateConstIteratorFn()
    {
        if constexpr (QContainerTraits::has_const_iterator_v<C>) {
            return [](const void *c, QMetaSequenceInterface::Position p) -> void* {
                using Iterator = QContainerTraits::const_iterator<C>;
                switch (p) {
                case QMetaSequenceInterface::AtBegin:
                case QMetaSequenceInterface::Random:
                    return new Iterator(static_cast<const C *>(c)->begin());
                    break;
                case QMetaSequenceInterface::AtEnd:
                    return new Iterator(static_cast<const C *>(c)->end());
                    break;
                }
                return nullptr;
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::DestroyIteratorFn getDestroyConstIteratorFn()
    {
        if constexpr (QContainerTraits::has_const_iterator_v<C>) {
            return [](const void *i) {
                using Iterator = QContainerTraits::const_iterator<C>;
                delete static_cast<const Iterator *>(i);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::CompareIteratorFn getCompareConstIteratorFn()
    {
        if constexpr (QContainerTraits::has_const_iterator_v<C>) {
            return [](const void *i, const void *j) {
                using Iterator = QContainerTraits::const_iterator<C>;
                return *static_cast<const Iterator *>(i) == *static_cast<const Iterator *>(j);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::CopyIteratorFn getCopyConstIteratorFn()
    {
        if constexpr (QContainerTraits::has_const_iterator_v<C>) {
            return [](void *i, const void *j) {
                using Iterator = QContainerTraits::const_iterator<C>;
                *static_cast<Iterator *>(i) = *static_cast<const Iterator *>(j);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::AdvanceIteratorFn getAdvanceConstIteratorFn()
    {
        if constexpr (QContainerTraits::has_const_iterator_v<C>) {
            return [](void *i, qsizetype step) {
                std::advance(*static_cast<QContainerTraits::const_iterator<C> *>(i), step);
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::DiffIteratorFn getDiffConstIteratorFn()
    {
        if constexpr (QContainerTraits::has_const_iterator_v<C>) {
            return [](const void *i, const void *j) -> qsizetype {
                return std::distance(*static_cast<const QContainerTraits::const_iterator<C> *>(j),
                                     *static_cast<const QContainerTraits::const_iterator<C> *>(i));
            };
        } else {
            return nullptr;
        }
    }

    static constexpr QMetaSequenceInterface::ElementAtIteratorFn getElementAtConstIteratorFn()
    {
        if constexpr (QContainerTraits::has_const_iterator_v<C>
                && QContainerTraits::can_get_at_iterator_v<C>) {
            return [](const void *i, void *r) {
                *static_cast<QContainerTraits::value_type<C> *>(r) =
                        *(*static_cast<const QContainerTraits::const_iterator<C> *>(i));
            };
        } else {
            return nullptr;
        }
    }

public:
    static QMetaSequenceInterface metaSequence;
};

template<typename C>
QMetaSequenceInterface QMetaSequenceForContainer<C>::metaSequence = {
    /*.revision=*/                  0,
    /*.iteratorCapabilities=*/      getIteratorCapabilities(),
    /*.addRemovePosition=*/         getAddRemovePosition(),
    /*.valueMetaType=*/             getValueMetaType(),
    /*.sizeFn=*/                    getSizeFn(),
    /*.clearFn=*/                   getClearFn(),
    /*.elementAtIndexFn=*/          getElementAtIndexFn(),
    /*.setElementAtIndexFn=*/       getSetElementAtIndexFn(),
    /*.addElementFn=*/              getAddElementFn(),
    /*.removeLastElementFn=*/       getRemoveElementFn(),
    /*.createIteratorFn=*/          getCreateIteratorFn(),
    /*.destroyIteratorFn=*/         getDestroyIteratorFn(),
    /*.equalIteratorFn=*/           getCompareIteratorFn(),
    /*.copyIteratorFn=*/            getCopyIteratorFn(),
    /*.advanceIteratorFn=*/         getAdvanceIteratorFn(),
    /*.diffIteratorFn=*/            getDiffIteratorFn(),
    /*.elementAtIteratorFn=*/       getElementAtIteratorFn(),
    /*.setElementAtIteratorFn=*/    getSetElementAtIteratorFn(),
    /*.insertElementAtIteratorFn=*/ getInsertElementAtIteratorFn(),
    /*.eraseElementAtIteratorFn=*/  getEraseElementAtIteratorFn(),
    /*.createConstIteratorFn=*/     getCreateConstIteratorFn(),
    /*.destroyConstIteratorFn=*/    getDestroyConstIteratorFn(),
    /*.equalConstIteratorFn=*/      getCompareConstIteratorFn(),
    /*.copyConstIteratorFn=*/       getCopyConstIteratorFn(),
    /*.advanceConstIteratorFn=*/    getAdvanceConstIteratorFn(),
    /*.diffConstIteratorFn=*/       getDiffConstIteratorFn(),
    /*.elementAtConstIteratorFn=*/  getElementAtConstIteratorFn(),
};

template<typename C>
constexpr QMetaSequenceInterface *qMetaSequenceInterfaceForContainer()
{
    return &QMetaSequenceForContainer<C>::metaSequence;
}

} // namespace QtMetaContainerPrivate

class Q_CORE_EXPORT QMetaSequence
{
public:
    QMetaSequence() = default;
    explicit QMetaSequence(QtMetaContainerPrivate::QMetaSequenceInterface *d) : d_ptr(d) {}

    template<typename T>
    static constexpr QMetaSequence fromContainer()
    {
        return QMetaSequence(QtMetaContainerPrivate::qMetaSequenceInterfaceForContainer<T>());
    }

    bool hasForwardIterator() const;
    bool hasBidirectionalIterator() const;
    bool hasRandomAccessIterator() const;

    QMetaType valueMetaType() const;

    bool isOrdered() const;
    bool addsAndRemovesElementsAtBegin() const;
    bool addsAndRemovesElementsAtEnd() const;

    bool hasSize() const;
    qsizetype size(const void *container) const;

    bool canClear() const;
    void clear(void *container) const;

    bool canGetElementAtIndex() const;
    void elementAtIndex(const void *container, qsizetype index, void *result) const;

    bool canSetElementAtIndex() const;
    void setElementAtIndex(void *container, qsizetype index, const void *element) const;

    bool canAddElement() const;
    void addElement(void *container, const void *element) const;

    bool canRemoveElement() const;
    void removeElement(void *container) const;

    bool hasIterator() const;
    void *begin(void *container) const;
    void *end(void *container) const;
    void destroyIterator(const void *iterator) const;
    bool compareIterator(const void *i, const void *j) const;
    void copyIterator(void *target, const void *source) const;
    void advanceIterator(void *iterator, qsizetype step) const;
    qsizetype diffIterator(const void *i, const void *j) const;

    bool canGetElementAtIterator() const;
    void elementAtIterator(const void *iterator, void *result) const;

    bool canSetElementAtIterator() const;
    void setElementAtIterator(const void *iterator, const void *element) const;

    bool canInsertElementAtIterator() const;
    void insertElementAtIterator(void *container, const void *iterator, const void *element) const;

    bool canEraseElementAtIterator() const;
    void eraseElementAtIterator(void *container, const void *iterator) const;

    bool hasConstIterator() const;
    void *constBegin(const void *container) const;
    void *constEnd(const void *container) const;
    void destroyConstIterator(const void *iterator) const;
    bool compareConstIterator(const void *i, const void *j) const;
    void copyConstIterator(void *target, const void *source) const;
    void advanceConstIterator(void *iterator, qsizetype step) const;
    qsizetype diffConstIterator(const void *i, const void *j) const;

    bool canGetElementAtConstIterator() const;
    void elementAtConstIterator(const void *iterator, void *result) const;

    friend bool operator==(const QMetaSequence &a, const QMetaSequence &b)
    {
        return a.d_ptr == b.d_ptr;
    }
    friend bool operator!=(const QMetaSequence &a, const QMetaSequence &b)
    {
        return a.d_ptr != b.d_ptr;
    }

private:
    QtMetaContainerPrivate::QMetaSequenceInterface *d_ptr = nullptr;
};

QT_END_NAMESPACE

#endif // QMETACONTAINER_H
