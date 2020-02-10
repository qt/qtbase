# special case skip regeneration
#
# Copy/Install doc configuration files to the build/install directory
#
qt_path_join(doc_install_dir ${QT_INSTALL_DIR} ${INSTALL_DOCDIR})
qt_copy_or_install(DIRECTORY global DESTINATION ${doc_install_dir})
qt_copy_or_install(DIRECTORY config DESTINATION ${doc_install_dir})

