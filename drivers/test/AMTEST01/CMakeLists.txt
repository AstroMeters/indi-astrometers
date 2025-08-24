# AMTEST01 Driver
set(AMTEST01_VERSION_MAJOR 1)
set(AMTEST01_VERSION_MINOR 0)

# Source files
set(AMTEST01_SOURCES
    amtest01.cpp
)

# Add executable
add_executable(indi_amtest01 ${AMTEST01_SOURCES})

# Set include directories
target_include_directories(indi_amtest01 PRIVATE
    /usr/include/libindi
)

# Link libraries directly
target_link_libraries(indi_amtest01 
    indidriver
    indiclient
    XISF
    pthread
)

# Generate XML file from template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_amtest01.xml.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/indi_amtest01.xml
    @ONLY
)

# Install
install(TARGETS indi_amtest01 RUNTIME DESTINATION bin)

# Install XML file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_amtest01.xml DESTINATION /usr/share/indi)
