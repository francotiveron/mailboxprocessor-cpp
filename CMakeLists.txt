project(cpp-mailbox-processor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mailbox_processor INTERFACE)
target_include_directories(mailbox_processor INTERFACE include ../threading-cpp/include)
target_link_libraries(mailbox_processor INTERFACE pthread)

add_subdirectory(examples)
