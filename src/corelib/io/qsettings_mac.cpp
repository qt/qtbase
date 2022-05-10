// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsettings.h"

#include "qsettings_p.h"
#include "qdatetime.h"
#include "qdir.h"
#include "qvarlengtharray.h"
#include "private/qcore_mac_p.h"
#ifndef QT_NO_QOBJECT
#include "qcoreapplication.h"
#endif // QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const CFStringRef hostNames[2] = { kCFPreferencesCurrentHost, kCFPreferencesAnyHost };
static const int numHostNames = 2;

/*
    On the Mac, it is more natural to use '.' as the key separator
    than '/'. Therefore, it makes sense to replace '/' with '.' in
    keys. Then we replace '.' with middle dots (which we can't show
    here) and middle dots with '/'. A key like "4.0/BrowserCommand"
    becomes "4<middot>0.BrowserCommand".
*/

enum RotateShift { Macify = 1, Qtify = 2 };

static QString rotateSlashesDotsAndMiddots(const QString &key, int shift)
{
    static const int NumKnights = 3;
    static const char knightsOfTheRoundTable[NumKnights] = { '/', '.', '\xb7' };
    QString result = key;

    for (int i = 0; i < result.size(); ++i) {
        for (int j = 0; j < NumKnights; ++j) {
            if (result.at(i) == QLatin1Char(knightsOfTheRoundTable[j])) {
                result[i] = QLatin1Char(knightsOfTheRoundTable[(j + shift) % NumKnights]).unicode();
                break;
            }
        }
    }
    return result;
}

static QCFType<CFStringRef> macKey(const QString &key)
{
    return rotateSlashesDotsAndMiddots(key, Macify).toCFString();
}

static QString qtKey(CFStringRef cfkey)
{
    return rotateSlashesDotsAndMiddots(QString::fromCFString(cfkey), Qtify);
}

static QCFType<CFPropertyListRef> macValue(const QVariant &value);

static CFArrayRef macList(const QList<QVariant> &list)
{
    int n = list.size();
    QVarLengthArray<QCFType<CFPropertyListRef>> cfvalues(n);
    for (int i = 0; i < n; ++i)
        cfvalues[i] = macValue(list.at(i));
    return CFArrayCreate(kCFAllocatorDefault, reinterpret_cast<const void **>(cfvalues.data()),
                         CFIndex(n), &kCFTypeArrayCallBacks);
}

static QCFType<CFPropertyListRef> macValue(const QVariant &value)
{
    CFPropertyListRef result = 0;

    switch (value.metaType().id()) {
    case QMetaType::QByteArray:
        {
            QByteArray ba = value.toByteArray();
            result = CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(ba.data()),
                                  CFIndex(ba.size()));
        }
        break;
    // should be same as below (look for LIST)
    case QMetaType::QVariantList:
    case QMetaType::QStringList:
    case QMetaType::QPolygon:
        result = macList(value.toList());
        break;
    case QMetaType::QVariantMap:
        {
            const QVariantMap &map = value.toMap();
            const int mapSize = map.size();

            QVarLengthArray<QCFType<CFPropertyListRef>> cfkeys;
            cfkeys.reserve(mapSize);
            std::transform(map.keyBegin(), map.keyEnd(),
                           std::back_inserter(cfkeys),
                           [](const auto &key) { return key.toCFString(); });

            QVarLengthArray<QCFType<CFPropertyListRef>> cfvalues;
            cfvalues.reserve(mapSize);
            std::transform(map.begin(), map.end(),
                           std::back_inserter(cfvalues),
                           [](const auto &value) { return macValue(value); });

            result = CFDictionaryCreate(kCFAllocatorDefault,
                                        reinterpret_cast<const void **>(cfkeys.data()),
                                        reinterpret_cast<const void **>(cfvalues.data()),
                                        CFIndex(mapSize),
                                        &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);
        }
        break;
    case QMetaType::QDateTime:
        {
            QDateTime dateTime = value.toDateTime();
            // CFDate, unlike QDateTime, doesn't store timezone information
            if (dateTime.timeSpec() == Qt::LocalTime)
                result = dateTime.toCFDate();
            else
                goto string_case;
        }
        break;
    case QMetaType::Bool:
        result = value.toBool() ? kCFBooleanTrue : kCFBooleanFalse;
        break;
    case QMetaType::Int:
    case QMetaType::UInt:
        {
            int n = value.toInt();
            result = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &n);
        }
        break;
    case QMetaType::Double:
        {
            double n = value.toDouble();
            result = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &n);
        }
        break;
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        {
            qint64 n = value.toLongLong();
            result = CFNumberCreate(0, kCFNumberLongLongType, &n);
        }
        break;
    case QMetaType::QString:
    string_case:
    default:
        QString string = QSettingsPrivate::variantToString(value);
        if (string.contains(QChar::Null))
            result = std::move(string).toUtf8().toCFData();
        else
            result = string.toCFString();
    }
    return result;
}

static QVariant qtValue(CFPropertyListRef cfvalue)
{
    if (!cfvalue)
        return QVariant();

    CFTypeID typeId = CFGetTypeID(cfvalue);

    /*
        Sorted grossly from most to least frequent type.
    */
    if (typeId == CFStringGetTypeID()) {
        return QSettingsPrivate::stringToVariant(QString::fromCFString(static_cast<CFStringRef>(cfvalue)));
    } else if (typeId == CFNumberGetTypeID()) {
        CFNumberRef cfnumber = static_cast<CFNumberRef>(cfvalue);
        if (CFNumberIsFloatType(cfnumber)) {
            double d;
            CFNumberGetValue(cfnumber, kCFNumberDoubleType, &d);
            return d;
        } else {
            int i;
            qint64 ll;

            if (CFNumberGetType(cfnumber) == kCFNumberIntType) {
                CFNumberGetValue(cfnumber, kCFNumberIntType, &i);
                return i;
            }
            CFNumberGetValue(cfnumber, kCFNumberLongLongType, &ll);
            return ll;
        }
    } else if (typeId == CFArrayGetTypeID()) {
        CFArrayRef cfarray = static_cast<CFArrayRef>(cfvalue);
        QList<QVariant> list;
        CFIndex size = CFArrayGetCount(cfarray);
        bool metNonString = false;
        for (CFIndex i = 0; i < size; ++i) {
            QVariant value = qtValue(CFArrayGetValueAtIndex(cfarray, i));
            if (value.typeId() != QMetaType::QString)
                metNonString = true;
            list << value;
        }
        if (metNonString)
            return list;
        else
            return QVariant(list).toStringList();
    } else if (typeId == CFBooleanGetTypeID()) {
        return (bool)CFBooleanGetValue(static_cast<CFBooleanRef>(cfvalue));
    } else if (typeId == CFDataGetTypeID()) {
        QByteArray byteArray = QByteArray::fromRawCFData(static_cast<CFDataRef>(cfvalue));

        // Fast-path for QByteArray, so that we don't have to go
        // though the expensive and lossy conversion via UTF-8.
        if (!byteArray.startsWith('@')) {
            byteArray.detach();
            return byteArray;
        }

        const QString str = QString::fromUtf8(byteArray.constData(), byteArray.size());
        QVariant variant = QSettingsPrivate::stringToVariant(str);
        if (variant == QVariant(str)) {
            // We did not find an encoded variant in the string,
            // so return the raw byte array instead.
            byteArray.detach();
            return byteArray;
        }

        return variant;
    } else if (typeId == CFDictionaryGetTypeID()) {
        CFDictionaryRef cfdict = static_cast<CFDictionaryRef>(cfvalue);
        CFTypeID arrayTypeId = CFArrayGetTypeID();
        int size = (int)CFDictionaryGetCount(cfdict);
        QVarLengthArray<CFPropertyListRef> keys(size);
        QVarLengthArray<CFPropertyListRef> values(size);
        CFDictionaryGetKeysAndValues(cfdict, keys.data(), values.data());

        QVariantMap map;
        for (int i = 0; i < size; ++i) {
            QString key = QString::fromCFString(static_cast<CFStringRef>(keys[i]));

            if (CFGetTypeID(values[i]) == arrayTypeId) {
                CFArrayRef cfarray = static_cast<CFArrayRef>(values[i]);
                CFIndex arraySize = CFArrayGetCount(cfarray);
                QVariantList list;
                list.reserve(arraySize);
                for (CFIndex j = 0; j < arraySize; ++j)
                    list.append(qtValue(CFArrayGetValueAtIndex(cfarray, j)));
                map.insert(key, list);
            } else {
                map.insert(key, qtValue(values[i]));
            }
        }
        return map;
    } else if (typeId == CFDateGetTypeID()) {
        return QDateTime::fromCFDate(static_cast<CFDateRef>(cfvalue));
    }
    return QVariant();
}

static QString comify(const QString &organization)
{
    for (int i = organization.size() - 1; i >= 0; --i) {
        QChar ch = organization.at(i);
        if (ch == u'.' || ch == QChar(0x3002) || ch == QChar(0xff0e)
                || ch == QChar(0xff61)) {
            QString suffix = organization.mid(i + 1).toLower();
            if (suffix.size() == 2 || suffix == "com"_L1 || suffix == "org"_L1
                    || suffix == "net"_L1 || suffix == "edu"_L1 || suffix == "gov"_L1
                    || suffix == "mil"_L1 || suffix == "biz"_L1 || suffix == "info"_L1
                    || suffix == "name"_L1 || suffix == "pro"_L1 || suffix == "aero"_L1
                    || suffix == "coop"_L1 || suffix == "museum"_L1) {
                QString result = organization;
                result.replace(u'/', u' ');
                return result;
            }
            break;
        }
        int uc = ch.unicode();
        if ((uc < 'a' || uc > 'z') && (uc < 'A' || uc > 'Z'))
            break;
    }

    QString domain;
    for (int i = 0; i < organization.size(); ++i) {
        QChar ch = organization.at(i);
        int uc = ch.unicode();
        if ((uc >= 'a' && uc <= 'z') || (uc >= '0' && uc <= '9')) {
            domain += ch;
        } else if (uc >= 'A' && uc <= 'Z') {
            domain += ch.toLower();
        } else {
           domain += u' ';
        }
    }
    domain = domain.simplified();
    domain.replace(u' ', u'-');
    if (!domain.isEmpty())
        domain.append(".com"_L1);
    return domain;
}

class QMacSettingsPrivate : public QSettingsPrivate
{
public:
    QMacSettingsPrivate(QSettings::Scope scope, const QString &organization,
                        const QString &application);
    ~QMacSettingsPrivate();

    void remove(const QString &key) override;
    void set(const QString &key, const QVariant &value) override;
    std::optional<QVariant> get(const QString &key) const override;
    QStringList children(const QString &prefix, ChildSpec spec) const override;
    void clear() override;
    void sync() override;
    void flush() override;
    bool isWritable() const override;
    QString fileName() const override;

private:
    struct SearchDomain
    {
        CFStringRef userName;
        CFStringRef applicationOrSuiteId;
    };

    QCFString applicationId;
    QCFString suiteId;
    QCFString hostName;
    SearchDomain domains[6];
    int numDomains;
};

QMacSettingsPrivate::QMacSettingsPrivate(QSettings::Scope scope, const QString &organization,
                                         const QString &application)
    : QSettingsPrivate(QSettings::NativeFormat, scope, organization, application)
{
    QString javaPackageName;
    int curPos = 0;
    int nextDot;

    // attempt to use the organization parameter
    QString domainName = comify(organization);
    // if not found, attempt to use the bundle identifier.
    if (domainName.isEmpty()) {
        CFBundleRef main_bundle = CFBundleGetMainBundle();
        if (main_bundle != NULL) {
            CFStringRef main_bundle_identifier = CFBundleGetIdentifier(main_bundle);
            if (main_bundle_identifier != NULL) {
                QString bundle_identifier(qtKey(main_bundle_identifier));
                // CFBundleGetIdentifier returns identifier separated by slashes rather than periods.
                QStringList bundle_identifier_components = bundle_identifier.split(u'/');
                // pre-reverse them so that when they get reversed again below, they are in the com.company.product format.
                QStringList bundle_identifier_components_reversed;
                for (int i=0; i<bundle_identifier_components.size(); ++i) {
                    const QString &bundle_identifier_component = bundle_identifier_components.at(i);
                    bundle_identifier_components_reversed.push_front(bundle_identifier_component);
                }
                domainName = bundle_identifier_components_reversed.join(u'.');
            }
        }
    }
    // if no bundle identifier yet. use a hard coded string.
    if (domainName.isEmpty())
        domainName = "unknown-organization.trolltech.com"_L1;

    while ((nextDot = domainName.indexOf(u'.', curPos)) != -1) {
        javaPackageName.prepend(QStringView{domainName}.mid(curPos, nextDot - curPos));
        javaPackageName.prepend(u'.');
        curPos = nextDot + 1;
    }
    javaPackageName.prepend(QStringView{domainName}.mid(curPos));
    javaPackageName = std::move(javaPackageName).toLower();
    if (curPos == 0)
        javaPackageName.prepend("com."_L1);
    suiteId = javaPackageName;

    if (!application.isEmpty()) {
        javaPackageName += u'.' + application;
        applicationId = javaPackageName;
    }

    numDomains = 0;
    for (int i = (scope == QSettings::SystemScope) ? 1 : 0; i < 2; ++i) {
        for (int j = (application.isEmpty()) ? 1 : 0; j < 3; ++j) {
            SearchDomain &domain = domains[numDomains++];
            domain.userName = (i == 0) ? kCFPreferencesCurrentUser : kCFPreferencesAnyUser;
            if (j == 0)
                domain.applicationOrSuiteId = applicationId;
            else if (j == 1)
                domain.applicationOrSuiteId = suiteId;
            else
                domain.applicationOrSuiteId = kCFPreferencesAnyApplication;
        }
    }

    hostName = (scope == QSettings::SystemScope) ? kCFPreferencesCurrentHost : kCFPreferencesAnyHost;
    sync();
}

QMacSettingsPrivate::~QMacSettingsPrivate()
{
}

void QMacSettingsPrivate::remove(const QString &key)
{
    QStringList keys = children(key + u'/', AllKeys);

    // If i == -1, then delete "key" itself.
    for (int i = -1; i < keys.size(); ++i) {
        QString subKey = key;
        if (i >= 0) {
            subKey += u'/';
            subKey += keys.at(i);
        }
        CFPreferencesSetValue(macKey(subKey), 0, domains[0].applicationOrSuiteId,
                              domains[0].userName, hostName);
    }
}

void QMacSettingsPrivate::set(const QString &key, const QVariant &value)
{
    CFPreferencesSetValue(macKey(key), macValue(value), domains[0].applicationOrSuiteId,
                          domains[0].userName, hostName);
}

std::optional<QVariant> QMacSettingsPrivate::get(const QString &key) const
{
    QCFString k = macKey(key);
    for (int i = 0; i < numDomains; ++i) {
        for (int j = 0; j < numHostNames; ++j) {
            QCFType<CFPropertyListRef> ret =
                    CFPreferencesCopyValue(k, domains[i].applicationOrSuiteId, domains[i].userName,
                                           hostNames[j]);
            if (ret)
                return qtValue(ret);
        }

        if (!fallbacks)
            break;
    }
    return std::nullopt;
}

QStringList QMacSettingsPrivate::children(const QString &prefix, ChildSpec spec) const
{
    QStringList result;
    int startPos = prefix.size();

    for (int i = 0; i < numDomains; ++i) {
        for (int j = 0; j < numHostNames; ++j) {
            QCFType<CFArrayRef> cfarray = CFPreferencesCopyKeyList(domains[i].applicationOrSuiteId,
                                                                   domains[i].userName,
                                                                   hostNames[j]);
            if (cfarray) {
                CFIndex size = CFArrayGetCount(cfarray);
                for (CFIndex k = 0; k < size; ++k) {
                    QString currentKey =
                            qtKey(static_cast<CFStringRef>(CFArrayGetValueAtIndex(cfarray, k)));
                    if (currentKey.startsWith(prefix))
                        processChild(QStringView{currentKey}.mid(startPos), spec, result);
                }
            }
        }

        if (!fallbacks)
            break;
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()),
                 result.end());
    return result;
}

void QMacSettingsPrivate::clear()
{
    QCFType<CFArrayRef> cfarray = CFPreferencesCopyKeyList(domains[0].applicationOrSuiteId,
                                                           domains[0].userName, hostName);
    CFPreferencesSetMultiple(0, cfarray, domains[0].applicationOrSuiteId, domains[0].userName,
                             hostName);
}

void QMacSettingsPrivate::sync()
{
    for (int i = 0; i < numDomains; ++i) {
        for (int j = 0; j < numHostNames; ++j) {
            Boolean ok = CFPreferencesSynchronize(domains[i].applicationOrSuiteId,
                                                  domains[i].userName, hostNames[j]);
            // only report failures for the primary file (the one we write to)
            if (!ok && i == 0 && hostNames[j] == hostName && status == QSettings::NoError) {
                setStatus(QSettings::AccessError);
            }
        }
    }
}

void QMacSettingsPrivate::flush()
{
    sync();
}

bool QMacSettingsPrivate::isWritable() const
{
    QMacSettingsPrivate *that = const_cast<QMacSettingsPrivate *>(this);
    QString impossibleKey("qt_internal/"_L1);

    QSettings::Status oldStatus = that->status;
    that->status = QSettings::NoError;

    that->set(impossibleKey, QVariant());
    that->sync();
    bool writable = (status == QSettings::NoError) && that->get(impossibleKey).has_value();
    that->remove(impossibleKey);
    that->sync();

    that->status = oldStatus;
    return writable;
}

QString QMacSettingsPrivate::fileName() const
{
    QString result;
    if (scope == QSettings::UserScope)
        result = QDir::homePath();
    result += "/Library/Preferences/"_L1;
    result += QString::fromCFString(domains[0].applicationOrSuiteId);
    result += ".plist"_L1;
    return result;
}

QSettingsPrivate *QSettingsPrivate::create(QSettings::Format format,
                                           QSettings::Scope scope,
                                           const QString &organization,
                                           const QString &application)
{
#ifndef QT_BOOTSTRAPPED
    if (organization == "Qt"_L1)
    {
        QString organizationDomain = QCoreApplication::organizationDomain();
        QString applicationName = QCoreApplication::applicationName();

        QSettingsPrivate *newSettings;
        if (format == QSettings::NativeFormat) {
            newSettings = new QMacSettingsPrivate(scope, organizationDomain, applicationName);
        } else {
            newSettings = new QConfFileSettingsPrivate(format, scope, organizationDomain, applicationName);
        }

        newSettings->beginGroupOrArray(QSettingsGroup(normalizedKey(organization)));
        if (!application.isEmpty())
            newSettings->beginGroupOrArray(QSettingsGroup(normalizedKey(application)));

        return newSettings;
    }
#endif
    if (format == QSettings::NativeFormat) {
        return new QMacSettingsPrivate(scope, organization, application);
    } else {
        return new QConfFileSettingsPrivate(format, scope, organization, application);
    }
}

bool QConfFileSettingsPrivate::readPlistFile(const QByteArray &data, ParsedSettingsMap *map) const
{
    QCFType<CFDataRef> cfData = data.toRawCFData();
    QCFType<CFPropertyListRef> propertyList =
            CFPropertyListCreateWithData(kCFAllocatorDefault, cfData, kCFPropertyListImmutable, nullptr, nullptr);

    if (!propertyList)
        return true;
    if (CFGetTypeID(propertyList) != CFDictionaryGetTypeID())
        return false;

    CFDictionaryRef cfdict =
            static_cast<CFDictionaryRef>(static_cast<CFPropertyListRef>(propertyList));
    int size = (int)CFDictionaryGetCount(cfdict);
    QVarLengthArray<CFPropertyListRef> keys(size);
    QVarLengthArray<CFPropertyListRef> values(size);
    CFDictionaryGetKeysAndValues(cfdict, keys.data(), values.data());

    for (int i = 0; i < size; ++i) {
        QString key = qtKey(static_cast<CFStringRef>(keys[i]));
        map->insert(QSettingsKey(key, Qt::CaseSensitive), qtValue(values[i]));
    }
    return true;
}

bool QConfFileSettingsPrivate::writePlistFile(QIODevice &file, const ParsedSettingsMap &map) const
{
    QVarLengthArray<QCFType<CFStringRef> > cfkeys(map.size());
    QVarLengthArray<QCFType<CFPropertyListRef> > cfvalues(map.size());
    int i = 0;
    ParsedSettingsMap::const_iterator j;
    for (j = map.constBegin(); j != map.constEnd(); ++j) {
        cfkeys[i] = macKey(j.key());
        cfvalues[i] = macValue(j.value());
        ++i;
    }

    QCFType<CFDictionaryRef> propertyList =
            CFDictionaryCreate(kCFAllocatorDefault,
                               reinterpret_cast<const void **>(cfkeys.data()),
                               reinterpret_cast<const void **>(cfvalues.data()),
                               CFIndex(map.size()),
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks);

    QCFType<CFDataRef> xmlData = CFPropertyListCreateData(
                 kCFAllocatorDefault, propertyList, kCFPropertyListXMLFormat_v1_0, 0, 0);

    return file.write(QByteArray::fromRawCFData(xmlData)) == CFDataGetLength(xmlData);
}

QT_END_NAMESPACE
