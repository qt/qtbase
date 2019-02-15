# Generated from deviceintegration.pro.

if(QT_FEATURE_egl_x11)
#     TODO #     TODO add_subdirectory(eglfs_x11)
endif()

if(QT_FEATURE_eglfs_gbm OR QT_FEATURE_eglfs_egldevice)
    add_subdirectory(eglfs_kms_support)
endif()

if(QT_FEATURE_eglfs_gbm)
    add_subdirectory(eglfs_kms)
endif()

if(QT_FEATURE_eglfs_egldevice)
#     TODO add_subdirectory(eglfs_kms_egldevice)
endif()

if(QT_FEATURE_eglfs_vsp2)
#     TODO add_subdirectory(eglfs_kms_vsp2)
endif()

if(QT_FEATURE_eglfs_brcm)
#     TODO add_subdirectory(eglfs_brcm)
endif()

if(QT_FEATURE_eglfs_mali)
#     TODO add_subdirectory(eglfs_mali)
endif()

if(QT_FEATURE_eglfs_viv)
#     TODO add_subdirectory(eglfs_viv)
endif()

if(QT_FEATURE_eglfs_rcar)
#     TODO add_subdirectory(eglfs_rcar)
endif()

if(QT_FEATURE_eglfs_viv_wl)
#     TODO add_subdirectory(eglfs_viv_wl)
endif()

if(QT_FEATURE_eglfs_openwfd)
#     TODO add_subdirectory(eglfs_openwfd)
endif()

if(QT_FEATURE_opengl)
#     TODO add_subdirectory(eglfs_emu)
endif()
