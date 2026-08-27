install(TARGETS rastertoqpdl pstoqpdl
    RUNTIME DESTINATION "${SPLIX_FILTER_DIR}")

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/samsung"
    FILES_MATCHING
        PATTERN "clp*.ppd"
        PATTERN "clx*.ppd"
        PATTERN "m[0-9]*.ppd"
        PATTERN "ml*.ppd"
        PATTERN "scx*.ppd"
        PATTERN "sf*.ppd")

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/xerox"
    FILES_MATCHING
        PATTERN "ph*.ppd"
        PATTERN "wc*.ppd")

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/dell"
    FILES_MATCHING
        PATTERN "1100*.ppd"
        PATTERN "1110*.ppd")

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/lexmark"
    FILES_MATCHING PATTERN "x215*.ppd")

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/toshiba"
    FILES_MATCHING PATTERN "es*.ppd")
