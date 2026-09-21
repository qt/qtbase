# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The script allows signing of Android packages using either jarsigner or
# apksigner program.
#
# Usage:
# cmake -DPROGRAM=/path/to/signer [-DZIPALIGN_PATH=/path/to/zipalign]
#    -DUNSIGNED_PACKAGE=path/to/input -DSIGNED_PACKAGE=path/to/output
#    -P QtAndroidSignPackage.cmake

if(NOT DEFINED PROGRAM)
    message(FATAL_ERROR "PROGRAM variable is not defined. Please"
        " specify the path to the apksigner or jarsigner program.")
endif()

if(NOT DEFINED UNSIGNED_PACKAGE)
    message(FATAL_ERROR "UNSIGNED_PACKAGE variable is not defined. Please"
        " specify the path to the input file.")
endif()

if(NOT DEFINED SIGNED_PACKAGE)
    message(FATAL_ERROR "SIGNED_PACKAGE variable is not defined. Please"
        " specify the path to the output file.")
endif()

get_filename_component(program_name "${PROGRAM}" NAME_WE)
get_filename_component(type "${UNSIGNED_PACKAGE}" LAST_EXT)
if(type STREQUAL ".aab")
    if(NOT program_name STREQUAL "jarsigner")
        message(WARNING "Unexpected signer program for .aab files: ${program_name}. "
            "Expected 'jarsigner'.")
    endif()

    set(digestalg "SHA-256")
    set(sigalg "SHA256withRSA")

    # Name the variable so the password stays out of the process list.
    if(DEFINED ENV{QT_ANDROID_KEYSTORE_KEY_PASS})
        set(keypass_arg -keypass:env QT_ANDROID_KEYSTORE_KEY_PASS)
    else()
        set(keypass_arg "")
    endif()

    if(DEFINED ENV{QT_ANDROID_KEYSTORE_STORE_PASS})
        set(storepass_arg -storepass:env QT_ANDROID_KEYSTORE_STORE_PASS)
    else()
        set(storepass_arg "")
    endif()

    execute_process(
        COMMAND ${PROGRAM}
            -sigalg ${sigalg} -digestalg ${digestalg}
            -keystore "$ENV{QT_ANDROID_KEYSTORE_PATH}"
            ${storepass_arg}
            ${keypass_arg}
            -signedjar "${SIGNED_PACKAGE}"
            "${UNSIGNED_PACKAGE}"
            "$ENV{QT_ANDROID_KEYSTORE_ALIAS}"
        RESULT_VARIABLE sign_result
        OUTPUT_VARIABLE sign_output
        ERROR_VARIABLE sign_error
    )

    if(NOT sign_result EQUAL 0)
        message(FATAL_ERROR "Unable to sign ${UNSIGNED_PACKAGE}: ${sign_output} ${sign_error}")
    endif()
elseif(type STREQUAL ".apk")
    if(NOT DEFINED ZIPALIGN_PATH)
        message(FATAL_ERROR "ZIPALIGN_PATH variable is not defined. Please"
            " specify the path to the zipalign program.")
    endif()

    if(NOT program_name STREQUAL "apksigner")
        message(WARNING "Unexpected signer program for .apk files: ${program_name}. "
            "Expected 'apksigner'.")
    endif()

    execute_process(
        COMMAND "${ZIPALIGN_PATH}" -c 4 "${UNSIGNED_PACKAGE}")

    # An undefined variable still means an empty password, as before.
    if(DEFINED ENV{QT_ANDROID_KEYSTORE_STORE_PASS})
        set(ks_pass_arg env:QT_ANDROID_KEYSTORE_STORE_PASS)
    else()
        set(ks_pass_arg "pass:")
    endif()

    if(DEFINED ENV{QT_ANDROID_KEYSTORE_KEY_PASS})
        set(key_pass_arg env:QT_ANDROID_KEYSTORE_KEY_PASS)
    else()
        set(key_pass_arg "pass:")
    endif()

    execute_process(
        COMMAND "${PROGRAM}" sign
            --ks "$ENV{QT_ANDROID_KEYSTORE_PATH}"
            --ks-pass "${ks_pass_arg}"
            --key-pass "${key_pass_arg}"
            --ks-key-alias "$ENV{QT_ANDROID_KEYSTORE_ALIAS}"
            --out "${SIGNED_PACKAGE}" "${UNSIGNED_PACKAGE}"
        RESULT_VARIABLE sign_result
        OUTPUT_VARIABLE sign_output
        ERROR_VARIABLE sign_error
    )

    if(NOT sign_result EQUAL 0)
        message(FATAL_ERROR "Unable to sign ${UNSIGNED_PACKAGE}: ${sign_output} ${sign_error}")
    endif()
else()
    message(FATAL_ERROR "Unsupported file type: ${type}. Only .apk and .aab files are supported.")
endif()
