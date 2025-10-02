cmake_minimum_required(VERSION 3.22)
project(longpath)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build our test executable
add_executable(
	longpath
	src/main.cpp
)

# Configuration options for mingw
if(LINUX)
	target_link_libraries(longpath -lshlwapi -lntdll -lwinpthread)
	target_compile_options(longpath PUBLIC -mwindows)
endif()

# Configuration options for Windows
if(WIN32)
	target_link_libraries(longpath shlwapi ntdll)
endif()

set_property(TARGET longpath PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_compile_definitions(longpath PUBLIC UNICODE)
target_compile_definitions(longpath PUBLIC _UNICODE)

# Install the executable to the output directory
install(TARGETS longpath RUNTIME DESTINATION bin)
