cmake_minimum_required(VERSION 3.22.1)
project(ModMenu LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================== ImGui ====================
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_android.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ==================== Loader Library ====================
add_library(Loader SHARED
    ${CMAKE_SOURCE_DIR}/loader/Loader.cpp
)
target_link_libraries(Loader log dl)

# ==================== Main Library ====================
add_library(ModMenu SHARED
    ${CMAKE_SOURCE_DIR}/main/Main.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(ModMenu PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_definitions(ModMenu PRIVATE
    IMGUI_IMPL_OPENGL_ES3
    __ANDROID__
    ANDROID
)

target_compile_options(ModMenu PRIVATE
    -O2
    -fno-exceptions
    -fno-rtti
    -ffunction-sections
    -fdata-sections
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

target_link_libraries(ModMenu
    log
    android
    EGL
    GLESv3
    GLESv2
    z
    dl
)
