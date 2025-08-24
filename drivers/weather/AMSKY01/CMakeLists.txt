# AMSKY01 Weather Station Driver
set(AMSKY01_VERSION_MAJOR 1)
set(AMSKY01_VERSION_MINOR 2)

# Source files
set(AMSKY01_SOURCES
    amsky01.cpp
)

# Add executable
add_executable(indi_amsky01 ${AMSKY01_SOURCES})

# Set include directories
target_include_directories(indi_amsky01 PRIVATE
    /usr/include/libindi
)

# Link libraries
target_link_libraries(indi_amsky01 
    indidriver
    indiclient
    pthread
)

# Generate XML file from template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_amsky01.xml.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/indi_amsky01.xml
    @ONLY
)

# Install
install(TARGETS indi_amsky01 RUNTIME DESTINATION /usr/bin)

# Install XML file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_amsky01.xml DESTINATION /usr/share/indi)
