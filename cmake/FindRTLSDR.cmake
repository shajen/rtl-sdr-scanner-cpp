# - Find RTLSDR
# Find the native RTLSDR includes and library
# This module defines
#  RTLSDR_INCLUDE_DIR, where to find rtlsdr.h, etc.
#  RTLSDR_LIBRARIES, the libraries needed to use RTLSDR.
#  RTLSDR_FOUND, If false, do not try to use RTLSDR.
# also defined, but not for general use are
#  RTLSDR_LIBRARY, where to find the RTLSDR library.

#MESSAGE("RTLSDR_DIR set to ${RTLSDR_DIR}" )

FIND_PATH(RTLSDR_INCLUDE_DIR rtl-sdr.h
  ${RTLSDR_DIR}/include
  /usr/pkgs64/include
  /usr/include
  /usr/local/include
)

FIND_LIBRARY(RTLSDR_LIBRARY
  NAMES rtlsdr
  PATHS ${RTLSDR_DIR}/libs
  "${RTLSDR_DIR}\\win32\\lib"
  /usr/pkgs64/lib
  /usr/lib64
  /usr/lib
  /usr/local/lib
  NO_DEFAULT_PATH
)

IF (RTLSDR_LIBRARY AND RTLSDR_INCLUDE_DIR)
  SET(RTLSDR_LIBRARIES ${RTLSDR_LIBRARY})
  SET(RTLSDR_FOUND "YES")
ELSE (RTLSDR_LIBRARY AND RTLSDR_INCLUDE_DIR)
  SET(RTLSDR_FOUND "NO")
ENDIF (RTLSDR_LIBRARY AND RTLSDR_INCLUDE_DIR)

IF (RTLSDR_FOUND)
  IF (NOT RTLSDR_FIND_QUIETLY)
    MESSAGE(STATUS "Found RTLSDR: ${RTLSDR_LIBRARIES}")
  ENDIF (NOT RTLSDR_FIND_QUIETLY)
ELSE (RTLSDR_FOUND)
  IF (RTLSDR_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find RTLSDR library")
  ENDIF (RTLSDR_FIND_REQUIRED)
ENDIF (RTLSDR_FOUND)

# Deprecated declarations.
GET_FILENAME_COMPONENT (NATIVE_RTLSDR_LIB_PATH ${RTLSDR_LIBRARY} PATH)

MARK_AS_ADVANCED(
  RTLSDR_LIBRARY
  RTLSDR_INCLUDE_DIR
)
