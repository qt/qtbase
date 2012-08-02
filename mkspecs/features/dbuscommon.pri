load(moc)
qtPrepareTool(QMAKE_QDBUSXML2CPP, qdbusxml2cpp)

defineReplace(qdbusOutputBasename) {
    return($$lower($$section($$list($$basename(1)),.,-2,-2)))
}

dbus_TYPE = $$upper($$dbus_type)

groups =
for(entry, DBUS_$${dbus_TYPE}S) {

    files = $$eval($${entry}.files)
    isEmpty(files) {
        files = $$entry
        group = dbus_$${dbus_type}
    } else {
        group = $${entry}_dbus_$${dbus_type}
    }
    groups *= $$group

    input_list = $$upper($$group)_LIST
    for(subent, $$list($$unique(files))) {

        !contains(subent, .*\\w\\.xml$) {
            warning("Invalid D-BUS $${dbus_type}: '$$subent', please use 'com.mydomain.myinterface.xml' instead.")
            next()
        }

        $$input_list += $$subent
    }
}

for(group, groups) {
    GROUP = $$upper($$group)
    input_list = $${GROUP}_LIST

    # qmake does not keep empty elements in lists, so we reverse-engineer the short name
    grp = $$replace(group, _?dbus_$${dbus_type}\$, )
    isEmpty(grp) {
        hdr_flags = $$eval(QDBUSXML2CPP_$${dbus_TYPE}_HEADER_FLAGS)
        src_flags = $$eval(QDBUSXML2CPP_$${dbus_TYPE}_SOURCE_FLAGS)
    } else {
        hdr_flags = $$eval($${grp}.header_flags)
        src_flags = $$eval($${grp}.source_flags)
    }

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
}
