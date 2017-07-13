set(CMAKE_PREFIX_PATH
    /usr/lib
    /usr/local/lib
    ${WEBM_DIR}/lib
)

find_path(WEBM_INCLUDE_DIR
    mkvparser.hpp
    PATHS ${WEBM_DIR}/include
)

find_library(WEBM_LIBRARY
    NAMES webm
)

mark_as_advanced(WEBM_INCLUDE_DIR)
mark_as_advanced(WEBM_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("WEBM"
    DEFAULT_MSG
    WEBM_LIBRARY
    WEBM_INCLUDE_DIR
)
