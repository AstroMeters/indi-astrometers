# AMSKY01 Weather Station Driver
set(AMSKY01_VERSION_MAJOR 1)
set(AMSKY01_VERSION_MINOR 2)

# Find required packages
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Serial driver source files
set(AMSKY01_SOURCES
    amsky01.cpp
)

# API driver source files
set(AMSKY01_API_SOURCES
    amsky01_api.cpp
)

# Add executable for serial driver
add_executable(indi_amsky01 ${AMSKY01_SOURCES})

# Add executable for API driver
add_executable(indi_amsky01_api ${AMSKY01_API_SOURCES})

# Set include directories for serial driver
target_include_directories(indi_amsky01 PRIVATE
    /usr/include/libindi
)

# Set include directories for API driver
target_include_directories(indi_amsky01_api PRIVATE
    /usr/include/libindi
    ${JSONCPP_INCLUDE_DIRS}
)

# Link libraries for serial driver
target_link_libraries(indi_amsky01 
    indidriver
    indiclient
    pthread
)

# Link libraries for API driver
target_link_libraries(indi_amsky01_api 
    indidriver
    indiclient
    pthread
    CURL::libcurl
    ${JSONCPP_LIBRARIES}
)

# Generate XML file for serial driver
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_amsky01.xml.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/indi_amsky01.xml
    @ONLY
)

# Generate XML file for API driver
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_amsky01_api.xml.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/indi_amsky01_api.xml
    @ONLY
)

# Install serial driver
install(TARGETS indi_amsky01 RUNTIME DESTINATION /usr/bin)

# Install API driver
install(TARGETS indi_amsky01_api RUNTIME DESTINATION /usr/bin)

# Install XML files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_amsky01.xml DESTINATION /usr/share/indi)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_amsky01_api.xml DESTINATION /usr/share/indi)
