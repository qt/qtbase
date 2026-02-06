// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWSOLE_H
#define QWINDOWSOLE_H

#include <QtCore/qt_windows.h>

#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qpointer.h>
#include <QtCore/private/qcomobject_p.h>

#include <objidl.h>

QT_BEGIN_NAMESPACE

class QMimeData;
class QWindow;

class QWindowsOleDataObject : public QComObject<IDataObject>
{
public:
    explicit QWindowsOleDataObject(QMimeData *mimeData);
    ~QWindowsOleDataObject() override;

    void releaseQt();
    QMimeData *mimeData() const;
    DWORD reportedPerformedEffect() const;

    // IDataObject methods
    STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium) noexcept override;
    STDMETHOD(GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium) noexcept override;
    STDMETHOD(QueryGetData)(LPFORMATETC pformatetc) noexcept override;
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut) noexcept override;
    STDMETHOD(SetData)(LPFORMATETC pformatetc, STGMEDIUM FAR *pmedium, BOOL fRelease) noexcept override;
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC FAR *ppenumFormatEtc) noexcept override;
    STDMETHOD(DAdvise)
    (FORMATETC FAR *pFormatetc, DWORD advf, LPADVISESINK pAdvSink,
     DWORD FAR *pdwConnection) noexcept override;
    STDMETHOD(DUnadvise)(DWORD dwConnection) noexcept override;
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA FAR *ppenumAdvise) noexcept override;

private:
    QPointer<QMimeData> data;
    const int CF_PERFORMEDDROPEFFECT;
    DWORD performedEffect = DROPEFFECT_NONE;
};

class QWindowsOleEnumFmtEtc : public QComObject<IEnumFORMATETC>
{
public:
    explicit QWindowsOleEnumFmtEtc(const QList<FORMATETC> &fmtetcs);
    explicit QWindowsOleEnumFmtEtc(const QList<LPFORMATETC> &lpfmtetcs);
    ~QWindowsOleEnumFmtEtc() override;

    bool isNull() const;

    // IEnumFORMATETC methods
    STDMETHOD(Next)(ULONG celt, LPFORMATETC rgelt, ULONG FAR *pceltFetched) noexcept override;
    STDMETHOD(Skip)(ULONG celt) noexcept override;
    STDMETHOD(Reset)(void) noexcept override;
    STDMETHOD(Clone)(LPENUMFORMATETC FAR *newEnum) noexcept override;

private:
    bool copyFormatEtc(LPFORMATETC dest, const FORMATETC *src) const;

    ULONG m_nIndex = 0;
    QList<LPFORMATETC> m_lpfmtetcs;
    bool m_isNull = false;
};

QT_END_NAMESPACE

#endif // QWINDOWSOLE_H
