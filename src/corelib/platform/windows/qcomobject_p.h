// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMOBJECT_P_H
#define QCOMOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

#if defined(Q_OS_WIN) || defined(Q_QDOC)

#  include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template <typename... TInterfaces>
struct QComObjectTraits
{
    static constexpr bool isGuidOf(REFIID riid) noexcept
    {
        return ((riid == __uuidof(TInterfaces)) || ...);
    }
};

} // namespace QtPrivate

// NOTE: In order to be able to query the intermediate interface, i.e. the one you do not specify in
// QComObject interface list (TFirstInterface, TInterfaces) but that is a base for any of them
// (except IUnknown) you need to provide an explicit specialization of function
// QComObjectTraits<...>::isGuidOf for that type. For example, if you want to inherit interface
// IMFSampleGrabberSinkCallback which inherits IMFClockStateSink and you want to be able to query
// the latter one you need to provide this explicit specialization:
//
// class SinkCallback : public QComObject<IMFSampleGrabberSinkCallback>
// {
// ...
// };
//
// namespace QtPrivate {
//
// template <>
// struct QComObjectTraits<IMFSampleGrabberSinkCallback>
// {
//     static constexpr bool isGuidOf(REFIID riid) noexcept
//     {
//         return QComObjectTraits<IMFSampleGrabberSinkCallback, IMFClockStateSink>::isGuidOf(riid);
//     }
// };
//
// }

template <typename TFirstInterface, typename... TAdditionalInterfaces>
class QComObject : public TFirstInterface, public TAdditionalInterfaces...
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;

        if (riid == __uuidof(IUnknown)) {
            *ppvObject = static_cast<IUnknown *>(static_cast<TFirstInterface *>(this));
            AddRef();

            return S_OK;
        }

        return tryQueryInterface<TFirstInterface, TAdditionalInterfaces...>(riid, ppvObject);
    }

    // clang-format off
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return ++m_referenceCount;
    }
    // clang-format on

    STDMETHODIMP_(ULONG) Release() override
    {
        const LONG referenceCount = --m_referenceCount;
        if (referenceCount == 0)
            delete this;

        return referenceCount;
    }

protected:
    QComObject() = default;

    // Destructor is not public. Caller should call Release.
    // Derived class should make its destructor private to force this behavior.
    virtual ~QComObject() = default;

private:
    template <typename TInterface, typename... TRest>
    HRESULT tryQueryInterface(REFIID riid, void **ppvObject)
    {
        if (QtPrivate::QComObjectTraits<TInterface>::isGuidOf(riid)) {
            *ppvObject = static_cast<TInterface *>(this);
            AddRef();

            return S_OK;
        }

        if constexpr (sizeof...(TRest) > 0)
            return tryQueryInterface<TRest...>(riid, ppvObject);

        *ppvObject = nullptr;

        return E_NOINTERFACE;
    }

    std::atomic<LONG> m_referenceCount = 1;
};

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QCOMOBJECT_P_H
