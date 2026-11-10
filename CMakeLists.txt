cmake_minimum_required(VERSION 3.20)
project(FolderSizeClientFix LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FolderSize WIN32
    client/src/main.cpp
    client/src/pipe_client.cpp
    client/src/explorer_tracker.cpp
    client/src/popup_window.cpp
    client/src/tray_icon.cpp
    client/src/explorer_path.cpp
    client/resources/app.rc
    client/resources/app.manifest
)

target_include_directories(FolderSize PRIVATE client/src)

target_compile_definitions(FolderSize PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
)

target_link_libraries(FolderSize PRIVATE
    comctl32
    ole32
    oleaut32
    shell32
    shlwapi
    version
    uuid
)

if(MSVC)
    target_compile_options(FolderSize PRIVATE /W4 /permissive-)
    set_target_properties(FolderSize PROPERTIES
        OUTPUT_NAME "FolderSize"
    )
endif()
