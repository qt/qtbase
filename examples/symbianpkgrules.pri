!symbian: error(Only include this file for Symbian platforms)

RSS_RULES ="group_name=\"QtExamples\";"

nokiaVendor = "Nokia, Qt"
vendorinfo = \
    "; Localised Vendor name" \
    "%{$$addLanguageDependentPkgItem(nokiaVendor)}" \
    " " \
    "; Unique Vendor name" \
    ":\"$$nokiaVendor\"" \
    " "

examples_deployment.pkg_prerules += vendorinfo
DEPLOYMENT += examples_deployment

isEmpty(ICON):contains(TEMPLATE, ".*app"):contains(QT, gui):contains(CONFIG, qt):!contains(CONFIG, "no_icon") {
    ICON = $$QT_SOURCE_TREE/src/s60installs/qt.svg
}

