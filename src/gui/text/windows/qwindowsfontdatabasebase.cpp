// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qwindowsfontdatabasebase_p.h"
#include "qwindowsfontdatabase_p.h"

#include <QtCore/QThreadStorage>
#include <QtCore/QtEndian>

#if QT_CONFIG(directwrite)
#  if QT_CONFIG(directwrite3)
#    include <dwrite_3.h>
#  else
#    include <dwrite_2.h>
#  endif
#  include <d2d1.h>
#  include "qwindowsfontenginedirectwrite_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Helper classes for creating font engines directly from font data
namespace {

#   pragma pack(1)

    // Common structure for all formats of the "name" table
    struct NameTable
    {
        quint16 format;
        quint16 count;
        quint16 stringOffset;
    };

    struct NameRecord
    {
        quint16 platformID;
        quint16 encodingID;
        quint16 languageID;
        quint16 nameID;
        quint16 length;
        quint16 offset;
    };

    struct OffsetSubTable
    {
        quint32 scalerType;
        quint16 numTables;
        quint16 searchRange;
        quint16 entrySelector;
        quint16 rangeShift;
    };

    struct TableDirectory : public QWindowsFontDatabaseBase::FontTable
    {
        quint32 identifier;
        quint32 checkSum;
        quint32 offset;
        quint32 length;
    };

    struct OS2Table
    {
        quint16 version;
        qint16  avgCharWidth;
        quint16 weightClass;
        quint16 widthClass;
        quint16 type;
        qint16  subscriptXSize;
        qint16  subscriptYSize;
        qint16  subscriptXOffset;
        qint16  subscriptYOffset;
        qint16  superscriptXSize;
        qint16  superscriptYSize;
        qint16  superscriptXOffset;
        qint16  superscriptYOffset;
        qint16  strikeOutSize;
        qint16  strikeOutPosition;
        qint16  familyClass;
        quint8  panose[10];
        quint32 unicodeRanges[4];
        quint8  vendorID[4];
        quint16 selection;
        quint16 firstCharIndex;
        quint16 lastCharIndex;
        qint16  typoAscender;
        qint16  typoDescender;
        qint16  typoLineGap;
        quint16 winAscent;
        quint16 winDescent;
        quint32 codepageRanges[2];
        qint16  height;
        qint16  capHeight;
        quint16 defaultChar;
        quint16 breakChar;
        quint16 maxContext;
    };

#   pragma pack()

} // Anonymous namespace

QWindowsFontDatabaseBase::FontTable *QWindowsFontDatabaseBase::EmbeddedFont::tableDirectoryEntry(const QByteArray &tagName)
{
    Q_ASSERT(tagName.size() == 4);
    quint32 tagId = *(reinterpret_cast<const quint32 *>(tagName.constData()));
    const size_t fontDataSize = m_fontData.size();
    if (Q_UNLIKELY(fontDataSize < sizeof(OffsetSubTable)))
        return nullptr;

    OffsetSubTable *offsetSubTable = reinterpret_cast<OffsetSubTable *>(m_fontData.data());
    TableDirectory *tableDirectory = reinterpret_cast<TableDirectory *>(offsetSubTable + 1);

    const size_t tableCount = qFromBigEndian<quint16>(offsetSubTable->numTables);
    if (Q_UNLIKELY(fontDataSize < sizeof(OffsetSubTable) + sizeof(TableDirectory) * tableCount))
        return nullptr;

    TableDirectory *tableDirectoryEnd = tableDirectory + tableCount;
    for (TableDirectory *entry = tableDirectory; entry < tableDirectoryEnd; ++entry) {
        if (entry->identifier == tagId)
            return entry;
    }

    return nullptr;
}

QString QWindowsFontDatabaseBase::EmbeddedFont::familyName(QWindowsFontDatabaseBase::FontTable *directoryEntry)
{
    QString name;

    TableDirectory *nameTableDirectoryEntry = static_cast<TableDirectory *>(directoryEntry);
    if (nameTableDirectoryEntry == nullptr)
        nameTableDirectoryEntry = static_cast<TableDirectory *>(tableDirectoryEntry("name"));

    if (nameTableDirectoryEntry != nullptr) {
        quint32 offset = qFromBigEndian<quint32>(nameTableDirectoryEntry->offset);
        if (Q_UNLIKELY(quint32(m_fontData.size()) < offset + sizeof(NameTable)))
            return QString();

        NameTable *nameTable = reinterpret_cast<NameTable *>(m_fontData.data() + offset);
        NameRecord *nameRecord = reinterpret_cast<NameRecord *>(nameTable + 1);

        quint16 nameTableCount = qFromBigEndian<quint16>(nameTable->count);
        if (Q_UNLIKELY(quint32(m_fontData.size()) < offset + sizeof(NameRecord) * nameTableCount))
            return QString();

        for (int i = 0; i < nameTableCount; ++i, ++nameRecord) {
            if (qFromBigEndian<quint16>(nameRecord->nameID) == 1
             && qFromBigEndian<quint16>(nameRecord->platformID) == 3 // Windows
             && qFromBigEndian<quint16>(nameRecord->languageID) == 0x0409) { // US English
                quint16 stringOffset = qFromBigEndian<quint16>(nameTable->stringOffset);
                quint16 nameOffset = qFromBigEndian<quint16>(nameRecord->offset);
                quint16 nameLength = qFromBigEndian<quint16>(nameRecord->length);

                if (Q_UNLIKELY(quint32(m_fontData.size()) < offset + stringOffset + nameOffset + nameLength))
                    return QString();

                const void *ptr = reinterpret_cast<const quint8 *>(nameTable)
                                                    + stringOffset
                                                    + nameOffset;

                const quint16 *s = reinterpret_cast<const quint16 *>(ptr);
                const quint16 *e = s + nameLength / sizeof(quint16);
                while (s != e)
                    name += QChar( qFromBigEndian<quint16>(*s++));
                break;
            }
        }
    }

    return name;
}

void QWindowsFontDatabaseBase::EmbeddedFont::updateFromOS2Table(QFontEngine *fontEngine)
{
    TableDirectory *os2TableEntry = static_cast<TableDirectory *>(tableDirectoryEntry("OS/2"));
    if (os2TableEntry != nullptr) {
        const OS2Table *os2Table =
                reinterpret_cast<const OS2Table *>(m_fontData.constData()
                                                   + qFromBigEndian<quint32>(os2TableEntry->offset));

        bool italic = qFromBigEndian<quint16>(os2Table->selection)  & (1 << 0);
        bool oblique = qFromBigEndian<quint16>(os2Table->selection) & (1 << 9);

        if (italic)
            fontEngine->fontDef.style = QFont::StyleItalic;
        else if (oblique)
            fontEngine->fontDef.style = QFont::StyleOblique;
        else
            fontEngine->fontDef.style = QFont::StyleNormal;

        fontEngine->fontDef.weight = qFromBigEndian<quint16>(os2Table->weightClass);
    }
}

QString QWindowsFontDatabaseBase::EmbeddedFont::changeFamilyName(const QString &newFamilyName)
{
    TableDirectory *nameTableDirectoryEntry = static_cast<TableDirectory *>(tableDirectoryEntry("name"));
    if (nameTableDirectoryEntry == nullptr)
        return QString();

    QString oldFamilyName = familyName(nameTableDirectoryEntry);

    // Reserve size for name table header, five required name records and string
    const int requiredRecordCount = 5;
    quint16 nameIds[requiredRecordCount] = { 1, 2, 3, 4, 6 };

    int sizeOfHeader = sizeof(NameTable) + sizeof(NameRecord) * requiredRecordCount;
    int newFamilyNameSize = newFamilyName.size() * int(sizeof(quint16));

    const QString regularString = QString::fromLatin1("Regular");
    int regularStringSize = regularString.size() * int(sizeof(quint16));

    // Align table size of table to 32 bits (pad with 0)
    int fullSize = ((sizeOfHeader + newFamilyNameSize + regularStringSize) & ~3) + 4;

    QByteArray newNameTable(fullSize, char(0));

    {
        NameTable *nameTable = reinterpret_cast<NameTable *>(newNameTable.data());
        nameTable->count = qbswap<quint16>(requiredRecordCount);
        nameTable->stringOffset = qbswap<quint16>(sizeOfHeader);

        NameRecord *nameRecord = reinterpret_cast<NameRecord *>(nameTable + 1);
        for (int i = 0; i < requiredRecordCount; ++i, nameRecord++) {
            nameRecord->nameID = qbswap<quint16>(nameIds[i]);
            nameRecord->encodingID = qbswap<quint16>(1);
            nameRecord->languageID = qbswap<quint16>(0x0409);
            nameRecord->platformID = qbswap<quint16>(3);
            nameRecord->length = qbswap<quint16>(newFamilyNameSize);

            // Special case for sub-family
            if (nameIds[i] == 4) {
                nameRecord->offset = qbswap<quint16>(newFamilyNameSize);
                nameRecord->length = qbswap<quint16>(regularStringSize);
            }
        }

        // nameRecord now points to string data
        quint16 *stringStorage = reinterpret_cast<quint16 *>(nameRecord);
        for (QChar ch : newFamilyName)
            *stringStorage++ = qbswap<quint16>(quint16(ch.unicode()));

        for (QChar ch : regularString)
            *stringStorage++ = qbswap<quint16>(quint16(ch.unicode()));
    }

    quint32 *p = reinterpret_cast<quint32 *>(newNameTable.data());
    quint32 *tableEnd = reinterpret_cast<quint32 *>(newNameTable.data() + fullSize);

    quint32 checkSum = 0;
    while (p < tableEnd)
        checkSum +=  qFromBigEndian<quint32>(*(p++));

    nameTableDirectoryEntry->checkSum = qbswap<quint32>(checkSum);
    nameTableDirectoryEntry->offset = qbswap<quint32>(m_fontData.size());
    nameTableDirectoryEntry->length = qbswap<quint32>(fullSize);

    m_fontData.append(newNameTable);

    return oldFamilyName;
}

#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)

namespace {
    class DirectWriteFontFileStream: public IDWriteFontFileStream
    {
        Q_DISABLE_COPY(DirectWriteFontFileStream)
    public:
        DirectWriteFontFileStream(const QByteArray &fontData)
            : m_fontData(fontData)
            , m_referenceCount(0)
        {
        }
        virtual ~DirectWriteFontFileStream()
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        HRESULT STDMETHODCALLTYPE ReadFileFragment(const void **fragmentStart, UINT64 fileOffset,
                                                   UINT64 fragmentSize, OUT void **fragmentContext) override;
        void STDMETHODCALLTYPE ReleaseFileFragment(void *fragmentContext) override;
        HRESULT STDMETHODCALLTYPE GetFileSize(OUT UINT64 *fileSize) override;
        HRESULT STDMETHODCALLTYPE GetLastWriteTime(OUT UINT64 *lastWriteTime) override;

    private:
        QByteArray m_fontData;
        ULONG m_referenceCount;
    };

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::QueryInterface(REFIID iid, void **object)
    {
        if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontFileStream)) {
            *object = this;
            AddRef();
            return S_OK;
        } else {
            *object = NULL;
            return E_NOINTERFACE;
        }
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileStream::AddRef()
    {
        return InterlockedIncrement(&m_referenceCount);
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileStream::Release()
    {
        ULONG newCount = InterlockedDecrement(&m_referenceCount);
        if (newCount == 0)
            delete this;
        return newCount;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::ReadFileFragment(
        const void **fragmentStart,
        UINT64 fileOffset,
        UINT64 fragmentSize,
        OUT void **fragmentContext)
    {
        *fragmentContext = NULL;
        if (fileOffset + fragmentSize <= quint64(m_fontData.size())) {
            *fragmentStart = m_fontData.data() + fileOffset;
            return S_OK;
        } else {
            *fragmentStart = NULL;
            return E_FAIL;
        }
    }

    void STDMETHODCALLTYPE DirectWriteFontFileStream::ReleaseFileFragment(void *)
    {
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::GetFileSize(UINT64 *fileSize)
    {
        *fileSize = m_fontData.size();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::GetLastWriteTime(UINT64 *lastWriteTime)
    {
        *lastWriteTime = 0;
        return E_NOTIMPL;
    }

    class DirectWriteFontFileLoader: public IDWriteLocalFontFileLoader
    {
    public:
        DirectWriteFontFileLoader() : m_referenceCount(0) {}
        virtual ~DirectWriteFontFileLoader()
        {
        }

        inline void addKey(const QByteArray &fontData, const QString &filename)
        {
            if (!m_fontDatas.contains(fontData.data()))
                m_fontDatas.insert(fontData.data(), qMakePair(fontData, filename));
        }

        HRESULT STDMETHODCALLTYPE GetFilePathLengthFromKey(void const* fontFileReferenceKey,
                                                           UINT32 fontFileReferenceKeySize,
                                                           UINT32* filePathLength) override
        {
            Q_UNUSED(fontFileReferenceKeySize);
            const void *key = *reinterpret_cast<void * const *>(fontFileReferenceKey);
            auto it = m_fontDatas.constFind(key);
            if (it == m_fontDatas.constEnd())
                return E_FAIL;

            *filePathLength = it.value().second.size();
            return 0;
        }

        HRESULT STDMETHODCALLTYPE GetFilePathFromKey(void const* fontFileReferenceKey,
                                                     UINT32 fontFileReferenceKeySize,
                                                     WCHAR* filePath,
                                                     UINT32 filePathSize) override
        {
            Q_UNUSED(fontFileReferenceKeySize);
            const void *key = *reinterpret_cast<void * const *>(fontFileReferenceKey);
            const auto it = m_fontDatas.constFind(key);
            if (it == m_fontDatas.constEnd())
                return E_FAIL;

            const QString &path = it.value().second;
            if (filePathSize < path.size() + 1)
                return E_FAIL;

            const qsizetype length = path.toWCharArray(filePath);
            filePath[length] = '\0';

            return 0;
        }

        HRESULT STDMETHODCALLTYPE GetLastWriteTimeFromKey(void const* fontFileReferenceKey,
                                                          UINT32 fontFileReferenceKeySize,
                                                          FILETIME* lastWriteTime) override
        {
            Q_UNUSED(fontFileReferenceKey);
            Q_UNUSED(fontFileReferenceKeySize);
            Q_UNUSED(lastWriteTime);
            // We never call this, so just fail
            return E_FAIL;
        }


        inline void removeKey(const void *key)
        {
            m_fontDatas.remove(key);
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        HRESULT STDMETHODCALLTYPE CreateStreamFromKey(void const *fontFileReferenceKey,
                                                      UINT32 fontFileReferenceKeySize,
                                                      OUT IDWriteFontFileStream **fontFileStream) override;

        void clear()
        {
            m_fontDatas.clear();
        }

    private:
        ULONG m_referenceCount;
        QHash<const void *, QPair<QByteArray, QString> > m_fontDatas;
    };

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileLoader::QueryInterface(const IID &iid,
                                                                        void **object)
    {
        if (iid == IID_IUnknown
            || iid == __uuidof(IDWriteFontFileLoader)
            || iid == __uuidof(IDWriteLocalFontFileLoader)) {
            *object = this;
            AddRef();
            return S_OK;
        } else {
            *object = NULL;
            return E_NOINTERFACE;
        }
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileLoader::AddRef()
    {
        return InterlockedIncrement(&m_referenceCount);
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileLoader::Release()
    {
        ULONG newCount = InterlockedDecrement(&m_referenceCount);
        if (newCount == 0)
            delete this;
        return newCount;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileLoader::CreateStreamFromKey(
        void const *fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        IDWriteFontFileStream **fontFileStream)
    {
        Q_UNUSED(fontFileReferenceKeySize);

        if (fontFileReferenceKeySize != sizeof(const void *)) {
            qWarning("%s: Wrong key size", __FUNCTION__);
            return E_FAIL;
        }

        const void *key = *reinterpret_cast<void * const *>(fontFileReferenceKey);
        *fontFileStream = NULL;
        auto it = m_fontDatas.constFind(key);
        if (it == m_fontDatas.constEnd())
            return E_FAIL;

        QByteArray fontData = it.value().first;
        DirectWriteFontFileStream *stream = new DirectWriteFontFileStream(fontData);
        stream->AddRef();
        *fontFileStream = stream;

        return S_OK;
    }

} // Anonymous namespace

class QCustomFontFileLoader
{
public:
    QCustomFontFileLoader(IDWriteFactory *factory)
    {
        m_directWriteFactory = factory;

        if (m_directWriteFactory) {
            m_directWriteFactory->AddRef();

            m_directWriteFontFileLoader = new DirectWriteFontFileLoader();
            m_directWriteFactory->RegisterFontFileLoader(m_directWriteFontFileLoader);
        }
    }

    ~QCustomFontFileLoader()
    {
        clear();

        if (m_directWriteFactory != nullptr && m_directWriteFontFileLoader != nullptr)
            m_directWriteFactory->UnregisterFontFileLoader(m_directWriteFontFileLoader);

        if (m_directWriteFactory != nullptr)
            m_directWriteFactory->Release();
    }

    void addKey(const QByteArray &fontData, const QString &filename)
    {
        if (m_directWriteFontFileLoader != nullptr)
            m_directWriteFontFileLoader->addKey(fontData, filename);
    }

    void removeKey(const void *key)
    {
        if (m_directWriteFontFileLoader != nullptr)
            m_directWriteFontFileLoader->removeKey(key);
    }

    IDWriteFontFileLoader *loader() const
    {
        return m_directWriteFontFileLoader;
    }

    void clear()
    {
        if (m_directWriteFontFileLoader != nullptr)
            m_directWriteFontFileLoader->clear();
    }

private:
    IDWriteFactory *m_directWriteFactory                    = nullptr;
    DirectWriteFontFileLoader *m_directWriteFontFileLoader  = nullptr;
};


#endif // directwrite && direct2d


QWindowsFontEngineData::~QWindowsFontEngineData()
{
    if (hdc)
        DeleteDC(hdc);

#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)
    if (directWriteGdiInterop)
        directWriteGdiInterop->Release();
    if (directWriteFactory)
        directWriteFactory->Release();
#endif
}

QWindowsFontDatabaseBase::QWindowsFontDatabaseBase()
{
}

QWindowsFontDatabaseBase::~QWindowsFontDatabaseBase()
{
}

typedef QSharedPointer<QWindowsFontEngineData> QWindowsFontEngineDataPtr;
typedef QThreadStorage<QWindowsFontEngineDataPtr> FontEngineThreadLocalData;
Q_GLOBAL_STATIC(FontEngineThreadLocalData, fontEngineThreadLocalData)

QSharedPointer<QWindowsFontEngineData> QWindowsFontDatabaseBase::data()
{
    FontEngineThreadLocalData *data = fontEngineThreadLocalData();
    if (!data->hasLocalData())
        data->setLocalData(QSharedPointer<QWindowsFontEngineData>::create());

    if (!init(data->localData()))
        qCWarning(lcQpaFonts) << "Cannot initialize common font database data";

    return data->localData();
}

bool QWindowsFontDatabaseBase::init(QSharedPointer<QWindowsFontEngineData> d)
{
#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)
    if (!d->directWriteFactory) {
        createDirectWriteFactory(&d->directWriteFactory);
        if (!d->directWriteFactory)
            return false;
    }
    if (!d->directWriteGdiInterop) {
        const HRESULT  hr = d->directWriteFactory->GetGdiInterop(&d->directWriteGdiInterop);
        if (FAILED(hr)) {
            qErrnoWarning("%s: GetGdiInterop failed", __FUNCTION__);
            return false;
        }
    }
#else
    Q_UNUSED(d);
#endif // directwrite && direct2d
    return true;
}

#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)
void QWindowsFontDatabaseBase::createDirectWriteFactory(IDWriteFactory **factory)
{
    *factory = nullptr;
    IUnknown *result = nullptr;

#  if QT_CONFIG(directwrite3)
    qCDebug(lcQpaFonts) << "Trying to create IDWriteFactory6";
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory6), &result);

    if (result == nullptr) {
        qCDebug(lcQpaFonts) << "Trying to create IDWriteFactory5";
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), &result);
    }

    if (result == nullptr) {
        qCDebug(lcQpaFonts) << "Trying to create IDWriteFactory3";
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), &result);
    }
#  endif

    if (result == nullptr) {
        qCDebug(lcQpaFonts) << "Trying to create IDWriteFactory2";
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2), &result);
    }

    if (result == nullptr) {
        qCDebug(lcQpaFonts) << "Trying to create plain IDWriteFactory";
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &result))) {
            qErrnoWarning("DWriteCreateFactory failed");
            return;
        }
    }

    *factory = static_cast<IDWriteFactory *>(result);
}
#endif // directwrite && direct2d

int QWindowsFontDatabaseBase::defaultVerticalDPI()
{
    return 96;
}

LOGFONT QWindowsFontDatabaseBase::fontDefToLOGFONT(const QFontDef &request, const QString &faceName)
{
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));

    lf.lfHeight = -qRound(request.pixelSize);
    lf.lfWidth                = 0;
    lf.lfEscapement        = 0;
    lf.lfOrientation        = 0;
    if (request.weight == QFont::Normal)
        lf.lfWeight = FW_DONTCARE;
    else
        lf.lfWeight = request.weight;
    lf.lfItalic         = request.style != QFont::StyleNormal;
    lf.lfCharSet        = DEFAULT_CHARSET;

    int strat = OUT_DEFAULT_PRECIS;
    if (request.styleStrategy & QFont::PreferBitmap) {
        strat = OUT_RASTER_PRECIS;
    } else if (request.styleStrategy & QFont::PreferDevice) {
        strat = OUT_DEVICE_PRECIS;
    } else if (request.styleStrategy & QFont::PreferOutline) {
        strat = OUT_OUTLINE_PRECIS;
    } else if (request.styleStrategy & QFont::ForceOutline) {
        strat = OUT_TT_ONLY_PRECIS;
    }

    lf.lfOutPrecision   = strat;

    int qual = DEFAULT_QUALITY;

    if (request.styleStrategy & QFont::PreferMatch)
        qual = DRAFT_QUALITY;
    else if (request.styleStrategy & QFont::PreferQuality)
        qual = PROOF_QUALITY;

    if (request.styleStrategy & QFont::PreferAntialias) {
        qual = (request.styleStrategy & QFont::NoSubpixelAntialias) == 0
            ? CLEARTYPE_QUALITY : ANTIALIASED_QUALITY;
    } else if (request.styleStrategy & QFont::NoAntialias) {
        qual = NONANTIALIASED_QUALITY;
    } else if ((request.styleStrategy & QFont::NoSubpixelAntialias) && data()->clearTypeEnabled) {
        qual = ANTIALIASED_QUALITY;
    }

    lf.lfQuality        = qual;

    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;

    int hint = FF_DONTCARE;
    switch (request.styleHint) {
        case QFont::Helvetica:
            hint = FF_SWISS;
            break;
        case QFont::Times:
            hint = FF_ROMAN;
            break;
        case QFont::Courier:
            hint = FF_MODERN;
            break;
        case QFont::OldEnglish:
            hint = FF_DECORATIVE;
            break;
        case QFont::System:
            hint = FF_MODERN;
            break;
        default:
            break;
    }

    lf.lfPitchAndFamily = DEFAULT_PITCH | hint;

    QString fam = faceName;
    if (fam.isEmpty())
        fam = request.families.first();
    if (Q_UNLIKELY(fam.size() >= LF_FACESIZE)) {
        qCritical("%s: Family name '%s' is too long.", __FUNCTION__, qPrintable(fam));
        fam.truncate(LF_FACESIZE - 1);
    }

    memcpy(lf.lfFaceName, fam.utf16(), fam.size() * sizeof(wchar_t));

    return lf;
}

QFont QWindowsFontDatabaseBase::LOGFONT_to_QFont(const LOGFONT& logFont, int verticalDPI_In)
{
    if (verticalDPI_In <= 0)
        verticalDPI_In = defaultVerticalDPI();
    QFont qFont(QString::fromWCharArray(logFont.lfFaceName));
    qFont.setItalic(logFont.lfItalic);
    if (logFont.lfWeight != FW_DONTCARE)
        qFont.setWeight(QFont::Weight(logFont.lfWeight));
    const qreal logFontHeight = qAbs(logFont.lfHeight);
    qFont.setPointSizeF(logFontHeight * 72.0 / qreal(verticalDPI_In));
    qFont.setUnderline(logFont.lfUnderline);
    qFont.setOverline(false);
    qFont.setStrikeOut(logFont.lfStrikeOut);
    return qFont;
}

// ### fixme Qt 6 (QTBUG-58610): See comment at QWindowsFontDatabase::systemDefaultFont()
HFONT QWindowsFontDatabaseBase::systemFont()
{
    static const auto stock_sysfont =
        reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    return stock_sysfont;
}

QFont QWindowsFontDatabaseBase::systemDefaultFont()
{
    // Qt 6: Obtain default GUI font (typically "Segoe UI, 9pt", see QTBUG-58610)
    NONCLIENTMETRICS ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0, defaultVerticalDPI());
    const QFont systemFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfMessageFont);
    qCDebug(lcQpaFonts) << __FUNCTION__ << systemFont;
    return systemFont;
}

void QWindowsFontDatabaseBase::invalidate()
{
#if QT_CONFIG(directwrite)
    m_fontFileLoader.reset(nullptr);
#endif
}

#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)
IDWriteFontFace *QWindowsFontDatabaseBase::createDirectWriteFace(const QByteArray &fontData)
{
    QList<IDWriteFontFace *> faces = createDirectWriteFaces(fontData, QString{}, false);
    Q_ASSERT(faces.size() <= 1);

    return faces.isEmpty() ? nullptr : faces.first();
}

QList<IDWriteFontFace *> QWindowsFontDatabaseBase::createDirectWriteFaces(const QByteArray &fontData,
                                                                          const QString &filename,
                                                                          bool queryVariations) const
{
    QList<IDWriteFontFace *> ret;
    QSharedPointer<QWindowsFontEngineData> fontEngineData = data();
    if (fontEngineData->directWriteFactory == nullptr) {
        qCWarning(lcQpaFonts) << "DirectWrite factory not created in QWindowsFontDatabaseBase::createDirectWriteFace()";
        return ret;
    }

    if (m_fontFileLoader == nullptr)
        m_fontFileLoader.reset(new QCustomFontFileLoader(fontEngineData->directWriteFactory));

    m_fontFileLoader->addKey(fontData, filename);

    IDWriteFontFile *fontFile = nullptr;
    const void *key = fontData.data();

    HRESULT hres = fontEngineData->directWriteFactory->CreateCustomFontFileReference(&key,
                                                                                     sizeof(void *),
                                                                                     m_fontFileLoader->loader(),
                                                                                     &fontFile);
    if (FAILED(hres)) {
        qErrnoWarning(hres, "%s: CreateCustomFontFileReference failed", __FUNCTION__);
        return ret;
    }

    BOOL isSupportedFontType;
    DWRITE_FONT_FILE_TYPE fontFileType;
    DWRITE_FONT_FACE_TYPE fontFaceType;
    UINT32 numberOfFaces;
    fontFile->Analyze(&isSupportedFontType, &fontFileType, &fontFaceType, &numberOfFaces);
    if (!isSupportedFontType) {
        fontFile->Release();
        return ret;
    }

#if QT_CONFIG(directwrite3)
    IDWriteFactory5 *factory5 = nullptr;
    if (queryVariations && SUCCEEDED(fontEngineData->directWriteFactory->QueryInterface(__uuidof(IDWriteFactory5),
                                                                                        reinterpret_cast<void **>(&factory5)))) {

        IDWriteFontSetBuilder1 *builder;
        if (SUCCEEDED(factory5->CreateFontSetBuilder(&builder))) {
            if (SUCCEEDED(builder->AddFontFile(fontFile))) {
                IDWriteFontSet *fontSet;
                if (SUCCEEDED(builder->CreateFontSet(&fontSet))) {
                    int count = fontSet->GetFontCount();
                    qCDebug(lcQpaFonts) << "Found" << count << "variations in font file";
                    for (int i = 0; i < count; ++i) {
                        IDWriteFontFaceReference *ref;
                        if (SUCCEEDED(fontSet->GetFontFaceReference(i, &ref))) {
                            IDWriteFontFace3 *face;
                            if (SUCCEEDED(ref->CreateFontFace(&face))) {
                                ret.append(face);
                            }
                            ref->Release();
                        }
                    }
                    fontSet->Release();
                }
            }

            builder->Release();
        }

        factory5->Release();
    }
#else
    Q_UNUSED(queryVariations);
#endif

    // ### Currently no support for .ttc, but we could easily return a list here.
    if (ret.isEmpty()) {
        IDWriteFontFace *directWriteFontFace = nullptr;
        hres = fontEngineData->directWriteFactory->CreateFontFace(fontFaceType,
                                                                  1,
                                                                  &fontFile,
                                                                  0,
                                                                  DWRITE_FONT_SIMULATIONS_NONE,
                                                                  &directWriteFontFace);
        if (FAILED(hres)) {
            qErrnoWarning(hres, "%s: CreateFontFace failed", __FUNCTION__);
            fontFile->Release();
            return ret;
        } else {
            ret.append(directWriteFontFace);
        }
    }

    fontFile->Release();

    return ret;
}
#endif // directwrite && direct2d

QFontEngine *QWindowsFontDatabaseBase::fontEngine(const QFontDef &fontDef, void *handle)
{
    // This function was apparently not used before, and probably isn't now either,
    // call the base implementation which just prints that it's not supported.
    return QPlatformFontDatabase::fontEngine(fontDef, handle);
}

QFontEngine *QWindowsFontDatabaseBase::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    QFontEngine *fontEngine = nullptr;

#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)
    QSharedPointer<QWindowsFontEngineData> fontEngineData = data();
    if (fontEngineData->directWriteFactory == nullptr)
        return nullptr;

    IDWriteFontFace * directWriteFontFace = createDirectWriteFace(fontData);
    if (directWriteFontFace == nullptr)
        return nullptr;

    fontEngine = new QWindowsFontEngineDirectWrite(directWriteFontFace,
                                                   pixelSize,
                                                   fontEngineData);

    // Get font family from font data
    EmbeddedFont font(fontData);
    font.updateFromOS2Table(fontEngine);
    fontEngine->fontDef.families = QStringList(font.familyName());
    fontEngine->fontDef.hintingPreference = hintingPreference;

    directWriteFontFace->Release();
#else // directwrite && direct2d
    Q_UNUSED(fontData);
    Q_UNUSED(pixelSize);
    Q_UNUSED(hintingPreference);
#endif

    return fontEngine;
}

QStringList QWindowsFontDatabaseBase::familiesForScript(QFontDatabasePrivate::ExtendedScript script)
{
    if (script == QFontDatabasePrivate::Script_Emoji)
        return QStringList{} << QStringLiteral("Segoe UI Emoji");
    else
        return QStringList{};
}

QString QWindowsFontDatabaseBase::familyForStyleHint(QFont::StyleHint styleHint)
{
    switch (styleHint) {
    case QFont::Times:
        return QStringLiteral("Times New Roman");
    case QFont::Courier:
        return QStringLiteral("Courier New");
    case QFont::Monospace:
        return QStringLiteral("Courier New");
    case QFont::Cursive:
        return QStringLiteral("Comic Sans MS");
    case QFont::Fantasy:
        return QStringLiteral("Impact");
    case QFont::Decorative:
        return QStringLiteral("Old English");
    case QFont::Helvetica:
        return QStringLiteral("Arial");
    case QFont::System:
    default:
        break;
    }
    return QStringLiteral("Tahoma");
}

// Creation functions

static const char *other_tryFonts[] = {
    "Arial",
    "MS UI Gothic",
    "Gulim",
    "SimSun",
    "PMingLiU",
    "Arial Unicode MS",
    0
};

static const char *jp_tryFonts [] = {
    "Yu Gothic UI",
    "MS UI Gothic",
    "Arial",
    "Gulim",
    "SimSun",
    "PMingLiU",
    "Arial Unicode MS",
    0
};

static const char *ch_CN_tryFonts [] = {
    "SimSun",
    "Arial",
    "PMingLiU",
    "Gulim",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char *ch_TW_tryFonts [] = {
    "PMingLiU",
    "Arial",
    "SimSun",
    "Gulim",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char *kr_tryFonts[] = {
    "Gulim",
    "Arial",
    "PMingLiU",
    "SimSun",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char **tryFonts = nullptr;

QStringList QWindowsFontDatabaseBase::extraTryFontsForFamily(const QString &family)
{
    QStringList result;
    if (!QFontDatabase::writingSystems(family).contains(QFontDatabase::Symbol)) {
        if (!tryFonts) {
            LANGID lid = GetUserDefaultLangID();
            switch (lid&0xff) {
            case LANG_CHINESE: // Chinese
                if ( lid == 0x0804 || lid == 0x1004) // China mainland and Singapore
                    tryFonts = ch_CN_tryFonts;
                else
                    tryFonts = ch_TW_tryFonts; // Taiwan, Hong Kong and Macau
                break;
            case LANG_JAPANESE:
                tryFonts = jp_tryFonts;
                break;
            case LANG_KOREAN:
                tryFonts = kr_tryFonts;
                break;
            default:
                tryFonts = other_tryFonts;
                break;
            }
        }
        const QStringList families = QFontDatabase::families();
        const char **tf = tryFonts;
        while (tf && *tf) {
            // QTBUG-31689, family might be an English alias for a localized font name.
            const QString family = QString::fromLatin1(*tf);
            if (families.contains(family) || QFontDatabase::hasFamily(family))
                result << family;
            ++tf;
        }
    }
    result.append(QStringLiteral("Segoe UI Emoji"));
    result.append(QStringLiteral("Segoe UI Symbol"));
    return result;
}

QFontDef QWindowsFontDatabaseBase::sanitizeRequest(QFontDef request) const
{
    QFontDef req = request;
    const QString fam = request.families.front();
    if (fam.isEmpty())
        req.families[0] = QStringLiteral("MS Sans Serif");

    if (fam == "MS Sans Serif"_L1) {
        int height = -qRound(request.pixelSize);
        // MS Sans Serif has bearing problems in italic, and does not scale
        if (request.style == QFont::StyleItalic || (height > 18 && height != 24))
            req.families[0] = QStringLiteral("Arial");
    }

    if (!(request.styleStrategy & QFont::StyleStrategy::PreferBitmap) && fam == u"Courier")
        req.families[0] = QStringLiteral("Courier New");
    return req;
}

QT_END_NAMESPACE
