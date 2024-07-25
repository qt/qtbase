// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsole.h"
#include "qwindowsmimeregistry.h"
#include "qwindowscontext.h"
\
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qpainter.h>
#include <QtGui/qcursor.h>
#include <QtGui/qguiapplication.h>

#include <QtCore/qmimedata.h>
#include <QtCore/qdebug.h>

#include <shlobj.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsOleDataObject
    \brief OLE data container

   The following methods are NOT supported for data transfer using the
   clipboard or drag-drop:
   \list
   \li IDataObject::SetData    -- return E_NOTIMPL
   \li IDataObject::DAdvise    -- return OLE_E_ADVISENOTSUPPORTED
   \li ::DUnadvise
   \li ::EnumDAdvise
   \li IDataObject::GetCanonicalFormatEtc -- return E_NOTIMPL
       (NOTE: must set pformatetcOut->ptd = NULL)
   \endlist

    \internal
*/

QWindowsOleDataObject::QWindowsOleDataObject(QMimeData *mimeData) :
    data(mimeData),
    CF_PERFORMEDDROPEFFECT(RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT))
{
    qCDebug(lcQpaMime) << __FUNCTION__ << mimeData->formats();
}

QWindowsOleDataObject::~QWindowsOleDataObject() = default;

void QWindowsOleDataObject::releaseQt()
{
    data = nullptr;
}

QMimeData *QWindowsOleDataObject::mimeData() const
{
    return data.data();
}

DWORD QWindowsOleDataObject::reportedPerformedEffect() const
{
    return performedEffect;
}

STDMETHODIMP
QWindowsOleDataObject::GetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium) noexcept
{
    HRESULT hr = ResultFromScode(DATA_E_FORMATETC);

    if (data) {
        const QWindowsMimeRegistry &mc = QWindowsContext::instance()->mimeConverter();
        if (auto converter = mc.converterFromMime(*pformatetc, data))
            if (converter->convertFromMime(*pformatetc, data, pmedium))
                hr = ResultFromScode(S_OK);
    }

    if (QWindowsContext::verbose > 1 && lcQpaMime().isDebugEnabled())
        qCDebug(lcQpaMime) <<__FUNCTION__ << *pformatetc << "returns" << Qt::hex << Qt::showbase << quint64(hr);

    return hr;
}

STDMETHODIMP
QWindowsOleDataObject::GetDataHere(LPFORMATETC, LPSTGMEDIUM) noexcept
{
    return ResultFromScode(DATA_E_FORMATETC);
}

STDMETHODIMP
QWindowsOleDataObject::QueryGetData(LPFORMATETC pformatetc) noexcept
{
    HRESULT hr = ResultFromScode(DATA_E_FORMATETC);

    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) << __FUNCTION__;

    if (data) {
        const QWindowsMimeRegistry &mc = QWindowsContext::instance()->mimeConverter();
        hr = mc.converterFromMime(*pformatetc, data) ?
             ResultFromScode(S_OK) : ResultFromScode(S_FALSE);
    }
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) <<  __FUNCTION__ << " returns 0x" << Qt::hex << int(hr);
    return hr;
}

STDMETHODIMP
QWindowsOleDataObject::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC pformatetcOut) noexcept
{
    pformatetcOut->ptd = nullptr;
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP
QWindowsOleDataObject::SetData(LPFORMATETC pFormatetc, STGMEDIUM *pMedium, BOOL fRelease) noexcept
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) << __FUNCTION__;

    HRESULT hr = ResultFromScode(E_NOTIMPL);

    if (pFormatetc->cfFormat == CF_PERFORMEDDROPEFFECT && pMedium->tymed == TYMED_HGLOBAL) {
        auto * val = (DWORD*)GlobalLock(pMedium->hGlobal);
        performedEffect = *val;
        GlobalUnlock(pMedium->hGlobal);
        if (fRelease)
            ReleaseStgMedium(pMedium);
        hr = ResultFromScode(S_OK);
    }
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) <<  __FUNCTION__ << " returns 0x" << Qt::hex << int(hr);
    return hr;
}


STDMETHODIMP
QWindowsOleDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc) noexcept
{
     if (QWindowsContext::verbose > 1)
         qCDebug(lcQpaMime) << __FUNCTION__ << "dwDirection=" << dwDirection;

    if (!data)
        return ResultFromScode(DATA_E_FORMATETC);

    SCODE sc = S_OK;

    QList<FORMATETC> fmtetcs;
    if (dwDirection == DATADIR_GET) {
        QWindowsMimeRegistry &mc = QWindowsContext::instance()->mimeConverter();
        fmtetcs = mc.allFormatsForMime(data);
    } else {
        FORMATETC formatetc;
        formatetc.cfFormat = CLIPFORMAT(CF_PERFORMEDDROPEFFECT);
        formatetc.dwAspect = DVASPECT_CONTENT;
        formatetc.lindex = -1;
        formatetc.ptd = nullptr;
        formatetc.tymed = TYMED_HGLOBAL;
        fmtetcs.append(formatetc);
    }

    auto *enumFmtEtc = new QWindowsOleEnumFmtEtc(fmtetcs);
    *ppenumFormatEtc = enumFmtEtc;
    if (enumFmtEtc->isNull()) {
        delete enumFmtEtc;
        *ppenumFormatEtc = nullptr;
        sc = E_OUTOFMEMORY;
    }

    return ResultFromScode(sc);
}

STDMETHODIMP
QWindowsOleDataObject::DAdvise(FORMATETC FAR*, DWORD,
                       LPADVISESINK, DWORD FAR*) noexcept
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}


STDMETHODIMP
QWindowsOleDataObject::DUnadvise(DWORD) noexcept
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP
QWindowsOleDataObject::EnumDAdvise(LPENUMSTATDATA FAR*) noexcept
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

/*!
    \class QWindowsOleEnumFmtEtc
    \brief Enumerates the FORMATETC structures supported by QWindowsOleDataObject.
    \internal
*/

QWindowsOleEnumFmtEtc::QWindowsOleEnumFmtEtc(const QList<FORMATETC> &fmtetcs)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) << __FUNCTION__ << fmtetcs;
    m_lpfmtetcs.reserve(fmtetcs.count());
    for (int idx = 0; idx < fmtetcs.count(); ++idx) {
        auto destetc = new FORMATETC();
        if (copyFormatEtc(destetc, &(fmtetcs.at(idx)))) {
            m_lpfmtetcs.append(destetc);
        } else {
            m_isNull = true;
            delete destetc;
            break;
        }
    }
}

QWindowsOleEnumFmtEtc::QWindowsOleEnumFmtEtc(const QList<LPFORMATETC> &lpfmtetcs)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) << __FUNCTION__;
    m_lpfmtetcs.reserve(lpfmtetcs.count());
    for (int idx = 0; idx < lpfmtetcs.count(); ++idx) {
        LPFORMATETC srcetc = lpfmtetcs.at(idx);
        auto destetc = new FORMATETC();
        if (copyFormatEtc(destetc, srcetc)) {
            m_lpfmtetcs.append(destetc);
        } else {
            m_isNull = true;
            delete destetc;
            break;
        }
    }
}

QWindowsOleEnumFmtEtc::~QWindowsOleEnumFmtEtc()
{
    LPMALLOC pmalloc;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) == NOERROR) {
        for (int idx = 0; idx < m_lpfmtetcs.count(); ++idx) {
            LPFORMATETC tmpetc = m_lpfmtetcs.at(idx);
            if (tmpetc->ptd)
                pmalloc->Free(tmpetc->ptd);
            delete tmpetc;
        }

        pmalloc->Release();
    }
    m_lpfmtetcs.clear();
}

bool QWindowsOleEnumFmtEtc::isNull() const
{
    return m_isNull;
}

// IEnumFORMATETC methods
STDMETHODIMP
QWindowsOleEnumFmtEtc::Next(ULONG celt, LPFORMATETC rgelt, ULONG FAR* pceltFetched) noexcept
{
    ULONG i=0;
    ULONG nOffset;

    if (rgelt == nullptr)
        return ResultFromScode(E_INVALIDARG);

    while (i < celt) {
        nOffset = m_nIndex + i;

        if (nOffset < ULONG(m_lpfmtetcs.count())) {
            copyFormatEtc(rgelt + i, m_lpfmtetcs.at(int(nOffset)));
            i++;
        } else {
            break;
        }
    }

    m_nIndex += i;

    if (pceltFetched != nullptr)
        *pceltFetched = i;

    if (i != celt)
        return ResultFromScode(S_FALSE);

    return NOERROR;
}

STDMETHODIMP
QWindowsOleEnumFmtEtc::Skip(ULONG celt) noexcept
{
    ULONG i=0;
    ULONG nOffset;

    while (i < celt) {
        nOffset = m_nIndex + i;

        if (nOffset < ULONG(m_lpfmtetcs.count())) {
            i++;
        } else {
            break;
        }
    }

    m_nIndex += i;

    if (i != celt)
        return ResultFromScode(S_FALSE);

    return NOERROR;
}

STDMETHODIMP
QWindowsOleEnumFmtEtc::Reset() noexcept
{
    m_nIndex = 0;
    return NOERROR;
}

STDMETHODIMP
QWindowsOleEnumFmtEtc::Clone(LPENUMFORMATETC FAR* newEnum) noexcept
{
    if (newEnum == nullptr)
        return ResultFromScode(E_INVALIDARG);

    auto *result = new QWindowsOleEnumFmtEtc(m_lpfmtetcs);
    result->m_nIndex = m_nIndex;

    if (result->isNull()) {
        delete result;
        return ResultFromScode(E_OUTOFMEMORY);
    }

    *newEnum = result;
    return NOERROR;
}

bool QWindowsOleEnumFmtEtc::copyFormatEtc(LPFORMATETC dest, const FORMATETC *src) const
{
    if (dest == nullptr || src == nullptr)
        return false;

    *dest = *src;

    if (src->ptd) {
        LPMALLOC pmalloc;

        if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR)
            return false;

        pmalloc->Alloc(src->ptd->tdSize);
        memcpy(dest->ptd, src->ptd, size_t(src->ptd->tdSize));

        pmalloc->Release();
    }

    return true;
}

QT_END_NAMESPACE
