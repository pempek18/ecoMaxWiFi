cmake_minimum_required(VERSION 3.10)
project(FurnaceReader 
    VERSION 1.0.0
    DESCRIPTION "Furnace Communication Library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Detect operating system
if(WIN32)
    set(IS_WINDOWS TRUE)
    message(STATUS "Building for Windows")
elseif(UNIX AND NOT APPLE)
    set(IS_LINUX TRUE)
    message(STATUS "Building for Linux")
elseif(APPLE)
    set(IS_MACOS TRUE)
    message(STATUS "Building for macOS")
endif()

# Detect MSYS2/MinGW environment
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(IS_MSYS2 TRUE)
    message(STATUS "Using MSYS2/MinGW compiler")
endif()

# Windows-specific settings
if(IS_WINDOWS)
    # Add Windows-specific definitions
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 and later
    
    # For MSVC compiler
    if(MSVC)
        # Enable warnings
        add_compile_options(/W4)
        # Disable specific warnings
        add_compile_options(/wd4996)  # Disable deprecated function warnings
        add_compile_options(/wd4267)  # Disable size_t conversion warnings
    endif()
    
    # For MinGW/GCC on Windows
    if(MINGW OR IS_MSYS2)
        add_compile_options(-Wall -Wextra)
    endif()
    
    # Windows libraries (kernel32 for Windows API)
    set(WINDOWS_LIBS kernel32)
    message(STATUS "Windows libraries: ${WINDOWS_LIBS}")
endif()

# Source files
set(SOURCES
    communicationWithFurner.cpp
)

# Headers
set(HEADERS
    communicationWithFurner.hpp
)

# Windows-specific example
if(IS_WINDOWS)
    add_executable(furnace_reader 
        examples/example_windows.cpp
        ${SOURCES}
        ${HEADERS}
    )
    
    target_link_libraries(furnace_reader PRIVATE ${WINDOWS_LIBS})
    message(STATUS "Added Windows example: furnace_reader")
endif()

# Linux-specific example
if(IS_LINUX)
    add_executable(furnace_reader 
        examples/example_linux.cpp
        ${SOURCES}
        ${HEADERS}
    )
    message(STATUS "Added Linux example: furnace_reader")
endif()

# Print build information
message(STATUS "=========================================")
message(STATUS "Furnace Reader Build Configuration")
message(STATUS "=========================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
if(MSVC)
    message(STATUS "Compiler: MSVC ${MSVC_VERSION}")
elseif(MINGW OR IS_MSYS2)
    message(STATUS "Compiler: MinGW/GCC")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Compiler: GCC")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Compiler: Clang")
endif()
message(STATUS "=========================================")

# Installation (optional)
install(TARGETS furnace_reader
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)
