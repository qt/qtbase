#=============================================================================
# Copyright 2005-2011 Kitware, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

######################################
#
#       Macros for building Qt files
#
######################################

MACRO (QT5_EXTRACT_OPTIONS _qt5_files _qt5_options)
  SET(${_qt5_files})
  SET(${_qt5_options})
  SET(_QT5_DOING_OPTIONS FALSE)
  FOREACH(_currentArg ${ARGN})
    IF ("${_currentArg}" STREQUAL "OPTIONS")
      SET(_QT5_DOING_OPTIONS TRUE)
    ELSE ("${_currentArg}" STREQUAL "OPTIONS")
      IF(_QT5_DOING_OPTIONS)
        LIST(APPEND ${_qt5_options} "${_currentArg}")
      ELSE(_QT5_DOING_OPTIONS)
        LIST(APPEND ${_qt5_files} "${_currentArg}")
      ENDIF(_QT5_DOING_OPTIONS)
    ENDIF ("${_currentArg}" STREQUAL "OPTIONS")
  ENDFOREACH(_currentArg)
ENDMACRO (QT5_EXTRACT_OPTIONS)


# macro used to create the names of output files preserving relative dirs
MACRO (QT5_MAKE_OUTPUT_FILE infile prefix ext outfile )
  STRING(LENGTH ${CMAKE_CURRENT_BINARY_DIR} _binlength)
  STRING(LENGTH ${infile} _infileLength)
  SET(_checkinfile ${CMAKE_CURRENT_SOURCE_DIR})
  IF(_infileLength GREATER _binlength)
    STRING(SUBSTRING "${infile}" 0 ${_binlength} _checkinfile)
    IF(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
      FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_BINARY_DIR} ${infile})
    ELSE(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
      FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
    ENDIF(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  ELSE(_infileLength GREATER _binlength)
    FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
  ENDIF(_infileLength GREATER _binlength)
  IF(WIN32 AND rel MATCHES "^[a-zA-Z]:") # absolute path
    STRING(REGEX REPLACE "^([a-zA-Z]):(.*)$" "\\1_\\2" rel "${rel}")
  ENDIF(WIN32 AND rel MATCHES "^[a-zA-Z]:")
  SET(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${rel}")
  STRING(REPLACE ".." "__" _outfile ${_outfile})
  GET_FILENAME_COMPONENT(outpath ${_outfile} PATH)
  GET_FILENAME_COMPONENT(_outfile ${_outfile} NAME_WE)
  FILE(MAKE_DIRECTORY ${outpath})
  SET(${outfile} ${outpath}/${prefix}${_outfile}.${ext})
ENDMACRO (QT5_MAKE_OUTPUT_FILE )


MACRO (QT5_GET_MOC_FLAGS _moc_flags)
  SET(${_moc_flags})
  GET_DIRECTORY_PROPERTY(_inc_DIRS INCLUDE_DIRECTORIES)

  FOREACH(_current ${_inc_DIRS})
    IF("${_current}" MATCHES "\\.framework/?$")
      STRING(REGEX REPLACE "/[^/]+\\.framework" "" framework_path "${_current}")
      SET(${_moc_flags} ${${_moc_flags}} "-F${framework_path}")
    ELSE("${_current}" MATCHES "\\.framework/?$")
      SET(${_moc_flags} ${${_moc_flags}} "-I${_current}")
    ENDIF("${_current}" MATCHES "\\.framework/?$")
  ENDFOREACH(_current ${_inc_DIRS})

  GET_DIRECTORY_PROPERTY(_defines COMPILE_DEFINITIONS)
  FOREACH(_current ${_defines})
    SET(${_moc_flags} ${${_moc_flags}} "-D${_current}")
  ENDFOREACH(_current ${_defines})

  IF(Q_WS_WIN)
    SET(${_moc_flags} ${${_moc_flags}} -DWIN32)
  ENDIF(Q_WS_WIN)

ENDMACRO(QT5_GET_MOC_FLAGS)


# helper macro to set up a moc rule
MACRO (QT5_CREATE_MOC_COMMAND infile outfile moc_flags moc_options)
  # For Windows, create a parameters file to work around command line length limit
  IF (WIN32)
    # Pass the parameters in a file.  Set the working directory to
    # be that containing the parameters file and reference it by
    # just the file name.  This is necessary because the moc tool on
    # MinGW builds does not seem to handle spaces in the path to the
    # file given with the @ syntax.
    GET_FILENAME_COMPONENT(_moc_outfile_name "${outfile}" NAME)
    GET_FILENAME_COMPONENT(_moc_outfile_dir "${outfile}" PATH)
    IF(_moc_outfile_dir)
      SET(_moc_working_dir WORKING_DIRECTORY ${_moc_outfile_dir})
    ENDIF(_moc_outfile_dir)
    SET (_moc_parameters_file ${outfile}_parameters)
    SET (_moc_parameters ${moc_flags} ${moc_options} -o "${outfile}" "${infile}")
    STRING (REPLACE ";" "\n" _moc_parameters "${_moc_parameters}")
    FILE (WRITE ${_moc_parameters_file} "${_moc_parameters}")
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                       COMMAND ${QT_MOC_EXECUTABLE} @${_moc_outfile_name}_parameters
                       DEPENDS ${infile}
                       ${_moc_working_dir}
                       VERBATIM)
  ELSE (WIN32)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                       COMMAND ${QT_MOC_EXECUTABLE}
                       ARGS ${moc_flags} ${moc_options} -o ${outfile} ${infile}
                       DEPENDS ${infile})
  ENDIF (WIN32)
ENDMACRO (QT5_CREATE_MOC_COMMAND)


MACRO (QT5_GENERATE_MOC infile outfile )
# get include dirs and flags
   QT5_GET_MOC_FLAGS(moc_flags)
   GET_FILENAME_COMPONENT(abs_infile ${infile} ABSOLUTE)
   SET(_outfile "${outfile}")
   IF(NOT IS_ABSOLUTE "${outfile}")
     SET(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfile}")
   ENDIF(NOT IS_ABSOLUTE "${outfile}")
   QT5_CREATE_MOC_COMMAND(${abs_infile} ${_outfile} "${moc_flags}" "")
   SET_SOURCE_FILES_PROPERTIES(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
ENDMACRO (QT5_GENERATE_MOC)


# QT5_WRAP_CPP(outfiles inputfile ... )

MACRO (QT5_WRAP_CPP outfiles )
  # get include dirs
  QT5_GET_MOC_FLAGS(moc_flags)
  QT5_EXTRACT_OPTIONS(moc_files moc_options ${ARGN})

  FOREACH (it ${moc_files})
    GET_FILENAME_COMPONENT(it ${it} ABSOLUTE)
    QT5_MAKE_OUTPUT_FILE(${it} moc_ cxx outfile)
    QT5_CREATE_MOC_COMMAND(${it} ${outfile} "${moc_flags}" "${moc_options}")
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH(it)

ENDMACRO (QT5_WRAP_CPP)


# QT5_ADD_RESOURCES(outfiles inputfile ... )

MACRO (QT5_ADD_RESOURCES outfiles )
  QT5_EXTRACT_OPTIONS(rcc_files rcc_options ${ARGN})

  FOREACH (it ${rcc_files})
    GET_FILENAME_COMPONENT(outfilename ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    GET_FILENAME_COMPONENT(rc_path ${infile} PATH)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cxx)
    #  parse file for dependencies
    #  all files are absolute paths or relative to the location of the qrc file
    FILE(READ "${infile}" _RC_FILE_CONTENTS)
    STRING(REGEX MATCHALL "<file[^<]+" _RC_FILES "${_RC_FILE_CONTENTS}")
    SET(_RC_DEPENDS)
    FOREACH(_RC_FILE ${_RC_FILES})
      STRING(REGEX REPLACE "^<file[^>]*>" "" _RC_FILE "${_RC_FILE}")
      IF(NOT IS_ABSOLUTE "${_RC_FILE}")
        SET(_RC_FILE "${rc_path}/${_RC_FILE}")
      ENDIF(NOT IS_ABSOLUTE "${_RC_FILE}")
      SET(_RC_DEPENDS ${_RC_DEPENDS} "${_RC_FILE}")
    ENDFOREACH(_RC_FILE)
    # Since this cmake macro is doing the dependency scanning for these files,
    # let's make a configured file and add it as a dependency so cmake is run
    # again when dependencies need to be recomputed.
    QT5_MAKE_OUTPUT_FILE("${infile}" "" "qrc.depends" out_depends)
    CONFIGURE_FILE("${infile}" "${out_depends}" COPY_ONLY)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${QT_RCC_EXECUTABLE}
      ARGS ${rcc_options} -name ${outfilename} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile}
      DEPENDS ${_RC_DEPENDS} "${out_depends}")
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH (it)

ENDMACRO (QT5_ADD_RESOURCES)
