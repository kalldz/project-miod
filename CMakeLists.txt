cmake_minimum_required(VERSION 3.16)
project(miod VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/argument_parser.cpp
    src/user_agent_manager.cpp
    src/waf_detector.cpp
    src/http_client.cpp
    src/rate_limiter.cpp
    src/statistics_collector.cpp
    src/worker_pool.cpp
    src/reporter.cpp
)

# Header files
set(HEADERS
    include/argument_parser.h
    include/user_agent_manager.h
    include/waf_detector.h
    include/http_client.h
    include/rate_limiter.h
    include/statistics_collector.h
    include/worker_pool.h
    include/reporter.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    CURL::libcurl
    Threads::Threads
)

# Windows specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0601)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
