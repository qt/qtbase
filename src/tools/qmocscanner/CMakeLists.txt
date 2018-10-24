set(_mocscanner "${PROJECT_BINARY_DIR}/${INSTALL_BINDIR}/qmocscanner${CMAKE_EXECUTABLE_SUFFIX}")
try_compile(mocscanner_built "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" COPY_FILE "${_mocscanner}")

add_executable(qmocscanner IMPORTED GLOBAL)
set_target_properties(qmocscanner PROPERTIES IMPORTED_LOCATION "${_mocscanner}")

install(PROGRAMS "${_mocscanner}" DESTINATION "${INSTALL_BINDIR}")
add_executable("Qt::qmocscanner" ALIAS "qmocscanner")

# FIXME: This should get exported to ToolsTargets!
