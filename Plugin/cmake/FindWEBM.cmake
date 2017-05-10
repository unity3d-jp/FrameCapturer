set(LIBRARY_PATHS
    /usr/lib
    /usr/local/lib
    ${WEBM_DIR}/lib
)

find_path(WEBM_INCLUDE_DIR
    libwebm/mkvparser.hpp
    PATHS ${WEBM_DIR}/include/libwebm
)
set(WEBM_INCLUDE_DIR ${WEBM_INCLUDE_DIR}/libwebm)

find_library(WEBM_LIBRARY
    NAMES webm
    PATHS ${LIBRARY_PATHS}
)

mark_as_advanced(WEBM_INCLUDE_DIR)
mark_as_advanced(WEBM_LIBRARY)

find_package_handle_standard_args("WEBM"
    DEFAULT_MSG
    WEBM_LIBRARY
    WEBM_INCLUDE_DIR
)
