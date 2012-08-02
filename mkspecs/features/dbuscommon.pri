load(moc)
qtPrepareTool(QMAKE_QDBUSXML2CPP, qdbusxml2cpp)

defineReplace(qdbusOutputBasename) {
    return($$lower($$section($$list($$basename(1)),.,-2,-2)))
}

dbus_TYPE = $$upper($$dbus_type)
group = dbus_$${dbus_type}
GROUP = DBUS_$${dbus_TYPE}
input_list = $${GROUP}_LIST

for(entry, $$list($$unique($${GROUP}S))) {

    !contains(entry, .*\\w\\.xml$) {
        warning("Invalid D-BUS $${dbus_type}: '$$entry', please use 'com.mydomain.myinterface.xml' instead.")
        next()
    }

    $$input_list += $$entry
}

# funny indent to avoid re-indentation in next commit
        hdr_flags = $$eval(QDBUSXML2CPP_$${dbus_TYPE}_HEADER_FLAGS)
        src_flags = $$eval(QDBUSXML2CPP_$${dbus_TYPE}_SOURCE_FLAGS)

    dthc = $${group}_header.commands
    $$dthc = $$QMAKE_QDBUSXML2CPP $$hdr_flags $$qdbusxml2cpp_option ${QMAKE_FILE_OUT}: ${QMAKE_FILE_IN}
    dtho = $${group}_header.output
    $$dtho = ${QMAKE_FUNC_FILE_IN_qdbusOutputBasename}_$${dbus_type}.h
    dthn = $${group}_header.name
    $$dthn = DBUSXML2CPP $${dbus_TYPE} HEADER ${QMAKE_FILE_IN}
    dthvo = $${group}_header.variable_out
    $$dthvo = $${GROUP}_HEADERS
    dthi = $${group}_header.input
    $$dthi = $$input_list

    dtsc = $${group}_source.commands
    $$dtsc = $$QMAKE_QDBUSXML2CPP -i ${QMAKE_FILE_OUT_BASE}.h $$src_flags $$qdbusxml2cpp_option :${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
    dtso = $${group}_source.output
    $$dtso = ${QMAKE_FUNC_FILE_IN_qdbusOutputBasename}_$${dbus_type}.cpp
    dtsn = $${group}_source.name
    $$dtsn = DBUSXML2CPP $${dbus_TYPE} SOURCE ${QMAKE_FILE_IN}
    dtsvo = $${group}_source.variable_out
    $$dtsvo = SOURCES
    dtsi = $${group}_source.input
    $$dtsi = $$input_list

    dtmc = $${group}_moc.commands
    $$dtmc = $$moc_header.commands
    dtmo = $${group}_moc.output
    $$dtmo = $$moc_header.output
    dtmi = $${group}_moc.input
    $$dtmi = $${GROUP}_HEADERS
    dtmvo = $${group}_moc.variable_out
    $$dtmvo = GENERATED_SOURCES
    dtmn = $${group}_moc.name
    $$dtmn = $$moc_header.name

    QMAKE_EXTRA_COMPILERS += $${group}_header $${group}_source $${group}_moc
