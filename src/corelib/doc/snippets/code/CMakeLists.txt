add_library(corelib_snippets_code OBJECT
    doc_src_containers.cpp
    doc_src_groups.cpp
    doc_src_properties.cpp
    doc_src_qalgorithms.cpp
    doc_src_qcache.cpp
    doc_src_qiterator.cpp
    doc_src_qnamespace.cpp
    doc_src_qplugin.cpp
    doc_src_qset.cpp
    doc_src_qvarlengtharray.cpp
    doc_src_resources.cpp
    src_corelib_animation_qparallelanimationgroup.cpp
    src_corelib_animation_qsequentialanimationgroup.cpp
    src_corelib_animation_qvariantanimation.cpp
    src_corelib_concurrent_qthreadpool.cpp
    src_corelib_global_qglobal.cpp
    src_corelib_global_qlogging.cpp
    src_corelib_global_qnumeric.cpp
    src_corelib_global_qoperatingsystemversion.cpp
    src_corelib_global_qrandom.cpp
    src_corelib_io_qdatastream.cpp
    src_corelib_io_qdebug.cpp
    src_corelib_io_qdir.cpp
    src_corelib_io_qdiriterator.cpp
    src_corelib_io_qdirlisting.cpp
    src_corelib_io_qfile.cpp
    src_corelib_io_qfileinfo.cpp
    src_corelib_io_qfileselector.cpp
    src_corelib_io_qsettings.cpp
    src_corelib_io_qstorageinfo.cpp
    src_corelib_io_qtemporarydir.cpp
    src_corelib_io_qtemporaryfile.cpp
    src_corelib_io_qtextstream.cpp
    src_corelib_io_qurl.cpp
    src_corelib_kernel_qabstractitemmodel.cpp
    src_corelib_kernel_qabstractnativeeventfilter_win.cpp
    src_corelib_kernel_qabstractnativeeventfilter.cpp
    src_corelib_kernel_qcoreapplication.cpp
    src_corelib_kernel_qdeadlinetimer.cpp
    src_corelib_kernel_qmath.cpp
    src_corelib_kernel_qmetaobject.cpp
    src_corelib_kernel_qmetatype.cpp
    src_corelib_kernel_qproperty.cpp
    src_corelib_kernel_qvariant.cpp
    src_corelib_plugin_qlibrary.cpp
    src_corelib_plugin_quuid.cpp
    src_corelib_serialization_qcborstream.cpp
    src_corelib_serialization_qcborvalue.cpp
    src_corelib_serialization_qdatastream.cpp
    src_corelib_serialization_qjsonobject.cpp
    src_corelib_text_qanystringview.cpp
    src_corelib_text_qbytearray.cpp
    src_corelib_text_qbytearraymatcher.cpp
    src_corelib_text_qbytearrayview.cpp
    src_corelib_text_qcollator.cpp
    src_corelib_text_qlocale.cpp
    src_corelib_text_qregularexpression.cpp
    src_corelib_text_qstaticlatin1stringmatcher.cpp
    src_corelib_text_qstring.cpp
    src_corelib_text_qstringconverter.cpp
    src_corelib_text_qstringview.cpp
    src_corelib_text_qutf8stringview.cpp
    src_corelib_thread_qmutex.cpp
    src_corelib_thread_qreadwritelock.cpp
    src_corelib_thread_qsemaphore.cpp
    src_corelib_thread_qthread.cpp
    src_corelib_thread_qwaitcondition_unix.cpp
    src_corelib_time_qdatetime.cpp
    src_corelib_tools_qbitarray.cpp
    src_corelib_tools_qcommandlineoption.cpp
    src_corelib_tools_qcommandlineparser_main.cpp
    src_corelib_tools_qcommandlineparser.cpp
    src_corelib_tools_qcontiguouscache.cpp
    src_corelib_tools_qeasingcurve.cpp
    src_corelib_tools_qhash.cpp
    src_corelib_tools_qlist.cpp
    src_corelib_tools_qmap.cpp
    src_corelib_tools_qmultimap.cpp
    src_corelib_tools_qpoint.cpp
    src_corelib_tools_qqueue.cpp
    src_corelib_tools_qrect.cpp
    src_corelib_tools_qscopedpointer.cpp
    src_corelib_tools_qscopeguard.cpp
    src_corelib_tools_qshareddata.cpp
    src_corelib_tools_qsharedpointer.cpp
    src_corelib_tools_qsize.cpp
    src_corelib_xml_qxmlstream.cpp
    src_gui_itemviews_qitemselectionmodel.cpp
)

set_target_properties(corelib_snippets_code PROPERTIES
    UNITY_BUILD OFF
)

target_link_libraries(corelib_snippets_code PRIVATE
    Qt::Core
)

if(APPLE)
    target_sources(corelib_snippets_code PRIVATE
        src_corelib_kernel_qabstractnativeeventfilter.mm
    )
endif()


if(QT_FEATURE_mimetype)
    target_sources(corelib_snippets_code PRIVATE
        src_corelib_kernel_qmimedata.cpp
        src_corelib_mimetype_qmimedatabase.cpp
    )
endif()

if(QT_FEATURE_process)
    target_sources(corelib_snippets_code PRIVATE
        src_corelib_io_qiodevice.cpp
        src_corelib_io_qprocess.cpp
    )
endif()

if(NOT EMSCRIPTEN)
    target_sources(corelib_snippets_code PRIVATE
        src_corelib_kernel_qsystemsemaphore.cpp
    )
endif()

if(QT_FEATURE_widgets)
    target_link_libraries(corelib_snippets_code PRIVATE
        Qt::Widgets
    )
    target_sources(corelib_snippets_code PRIVATE
        doc_src_objecttrees.cpp
        src_corelib_animation_qpropertyanimation.cpp
        src_corelib_global_qglobal_widgets.cpp
        src_corelib_kernel_qobject.cpp
        src_corelib_kernel_qtimer.cpp
        src_corelib_tools_qtimeline.cpp
    )
endif()

if(QT_FEATURE_gui)
    target_link_libraries(corelib_snippets_code PRIVATE
        Qt::Gui
    )
    if(QT_FEATURE_identityproxymodel)
        target_sources(corelib_snippets_code PRIVATE
            src_gui_itemviews_qidentityproxymodel.cpp
        )
    endif()
endif()

if(QT_FEATURE_concurrent)
    target_link_libraries(corelib_snippets_code PRIVATE
        Qt::Concurrent
    )
    target_sources(corelib_snippets_code PRIVATE
        src_corelib_thread_qexception.cpp
        src_corelib_thread_qfuturesynchronizer.cpp
        src_corelib_thread_qfuturewatcher.cpp
    )
    if(QT_FEATURE_network)
        target_sources(corelib_snippets_code PRIVATE
            src_corelib_thread_qfuture.cpp
        )
    endif()
endif()

if(QT_FEATURE_network)
    target_link_libraries(corelib_snippets_code PRIVATE
        Qt::Network
    )
endif()

set_target_properties(corelib_snippets_code PROPERTIES COMPILE_OPTIONS "-w")
