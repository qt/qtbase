// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinpermissionplugin_p_p.h"

#include <deque>

#include <CoreLocation/CoreLocation.h>

@interface QDarwinLocationPermissionHandler () <CLLocationManagerDelegate>
@property (nonatomic, retain) CLLocationManager *manager;
@end

Q_STATIC_LOGGING_CATEGORY(lcLocationPermission, "qt.permissions.location");

namespace {

void warmUpLocationServices()
{
    // After creating a CLLocationManager the authorizationStatus
    // will initially be kCLAuthorizationStatusNotDetermined. The
    // status will then update to an actual status if the app was
    // previously authorized/denied once the location services
    // do some initial book-keeping in the background. By kicking
    // off a CLLocationManager early on here, we ensure that by
    // the time the user calls checkPermission the authorization
    // status has been resolved.
    qCDebug(lcLocationPermission) << "Warming up location services";
    [[CLLocationManager new] release];
}

Q_CONSTRUCTOR_FUNCTION(warmUpLocationServices);

struct PermissionRequest
{
    QPermission permission;
    PermissionCallback callback;
};

} // namespace

@implementation QDarwinLocationPermissionHandler  {
    std::deque<PermissionRequest> m_requests;
}

- (instancetype)init
{
    if ((self = [super init])) {
        // The delegate callbacks will come in on the thread that
        // the CLLocationManager is created on, and we want those
        // to come in on the main thread, so we defer creation
        // of the manger until requestPermission, where we know
        // we are on the main thread.
        self.manager = nil;
    }

    return self;
}

- (Qt::PermissionStatus)checkPermission:(QPermission)permission
{
    const auto locationPermission = *permission.value<QLocationPermission>();

    auto status = [self authorizationStatus:locationPermission];
    if (status != Qt::PermissionStatus::Granted)
        return status;

    return [self accuracyAuthorization:locationPermission];
}

- (Qt::PermissionStatus)authorizationStatus:(QLocationPermission)permission
{
    NSString *bundleIdentifier = NSBundle.mainBundle.bundleIdentifier;
    if (!bundleIdentifier || !bundleIdentifier.length) {
        qCWarning(lcLocationPermission) << "Missing bundle identifier"
            << "in Info.plist. Can not use location permissions.";
        return Qt::PermissionStatus::Denied;
    }

#if defined(Q_OS_VISIONOS)
    if (permission.availability() == QLocationPermission::Always)
        return Qt::PermissionStatus::Denied;
#endif

    auto status = [self authorizationStatus];
    switch (status) {
    case kCLAuthorizationStatusRestricted:
    case kCLAuthorizationStatusDenied:
        return Qt::PermissionStatus::Denied;
    case kCLAuthorizationStatusNotDetermined:
        return Qt::PermissionStatus::Undetermined;
#if !defined(Q_OS_VISIONOS)
    case kCLAuthorizationStatusAuthorizedAlways:
        return Qt::PermissionStatus::Granted;
#endif
#if defined(Q_OS_IOS) || defined(Q_OS_VISIONOS)
    case kCLAuthorizationStatusAuthorizedWhenInUse:
        if (permission.availability() == QLocationPermission::Always)
            return Qt::PermissionStatus::Denied;
        return Qt::PermissionStatus::Granted;
#endif
    }

    qCWarning(lcPermissions) << "Unknown permission status" << status << "detected in" << self;
    return Qt::PermissionStatus::Denied;
}

- (CLAuthorizationStatus)authorizationStatus
{
    if (self.manager)
        return self.manager.authorizationStatus;

    return QT_IGNORE_DEPRECATIONS(CLLocationManager.authorizationStatus);
}

- (Qt::PermissionStatus)accuracyAuthorization:(QLocationPermission)permission
{
    auto status = self.manager.accuracyAuthorization;

    switch (status) {
    case CLAccuracyAuthorizationFullAccuracy:
        return Qt::PermissionStatus::Granted;
    case CLAccuracyAuthorizationReducedAccuracy:
        if (permission.accuracy() == QLocationPermission::Approximate)
            return Qt::PermissionStatus::Granted;
        else
            return Qt::PermissionStatus::Denied;
    }

    qCWarning(lcPermissions) << "Unknown accuracy status" << status << "detected in" << self;
    return Qt::PermissionStatus::Denied;
}

- (QStringList)usageDescriptionsFor:(QPermission)permission
{
#if defined(Q_OS_MACOS)
    return { "NSLocationUsageDescription" };
#else // iOS 11 and above
    QStringList usageDescriptions = { "NSLocationWhenInUseUsageDescription" };
    const auto locationPermission = *permission.value<QLocationPermission>();
    if (locationPermission.availability() == QLocationPermission::Always)
        usageDescriptions << "NSLocationAlwaysAndWhenInUseUsageDescription";
    return usageDescriptions;
#endif
}

- (void)requestPermission:(QPermission)permission withCallback:(PermissionCallback)callback
{
    const bool requestAlreadyInFlight = !m_requests.empty();

    m_requests.push_back({ permission, callback });

    if (requestAlreadyInFlight) {
        qCDebug(lcLocationPermission).nospace() << "Already processing "
            << m_requests.front().permission << ". Deferring request";
    } else {
        [self requestQueuedPermission];
    }
}

- (void)requestQueuedPermission
{
    Q_ASSERT(!m_requests.empty());
    const auto permission = m_requests.front().permission;

    qCDebug(lcLocationPermission) << "Requesting" << permission;

    if (!self.manager) {
        self.manager = [[CLLocationManager new] autorelease];
        self.manager.delegate = self;
    }

    const auto locationPermission = *permission.value<QLocationPermission>();
    switch (locationPermission.availability()) {
    case QLocationPermission::WhenInUse:
        // The documentation specifies that requestWhenInUseAuthorization can
        // only be called when the current authorization status is undetermined.
        switch ([self authorizationStatus]) {
        case kCLAuthorizationStatusNotDetermined:
            [self.manager requestWhenInUseAuthorization];
            break;
        default:
            [self deliverResult];
        }
        break;
    case QLocationPermission::Always:
#if defined(Q_OS_VISIONOS)
        [self deliverResult]; // Not supported
#else
        // The documentation specifies that requestAlwaysAuthorization can only
        // be called when the current authorization status is either undetermined,
        // or authorized when in use.
        switch ([self authorizationStatus]) {
        case kCLAuthorizationStatusNotDetermined:
            [self.manager requestAlwaysAuthorization];
            break;
#ifdef Q_OS_IOS
        case kCLAuthorizationStatusAuthorizedWhenInUse:
            // Unfortunately when asking for AlwaysAuthorization when in
            // the WhenInUse state, to "upgrade" the permission, the iOS
            // location system will not give us a callback if the user
            // denies the request, leaving us hanging without a way to
            // respond to the permission request.
            qCWarning(lcLocationPermission) << "QLocationPermission::WhenInUse"
                << "can not be upgraded to QLocationPermission::Always on iOS."
                << "Please request QLocationPermission::Always directly.";
            Q_FALLTHROUGH();
#endif
        default:
            [self deliverResult];
        }
#endif
        break;
    }
}

- (void)locationManager:(CLLocationManager *)manager didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
    qCDebug(lcLocationPermission) << "Processing authorization"
        << "update with status" << status;

    if (m_requests.empty()) {
        qCDebug(lcLocationPermission) << "No requests in flight. Ignoring.";
        return;
    }

    if (status == kCLAuthorizationStatusNotDetermined) {
        // Initializing a CLLocationManager will result in an initial
        // callback to the delegate even before we've requested any
        // location permissions. Normally we would ignore this callback
        // due to the request queue check above, but if this callback
        // comes in after the application has requested a permission
        // we don't want to report the undetermined status, but rather
        // wait for the actual result to come in.
        qCDebug(lcLocationPermission) << "Ignoring delegate callback"
            << "with status kCLAuthorizationStatusNotDetermined";
        return;
    }

    [self deliverResult];
}

- (void)deliverResult
{
    auto request = m_requests.front();
    m_requests.pop_front();

    auto status = [self checkPermission:request.permission];
    qCDebug(lcLocationPermission) << "Result for"
        << request.permission << "was" << status;

    request.callback(status);

    if (!m_requests.empty()) {
        qCDebug(lcLocationPermission) << "Still have"
            << m_requests.size() << "deferred request(s)";
        [self requestQueuedPermission];
    }
}

@end

#include "moc_qdarwinpermissionplugin_p_p.cpp"
