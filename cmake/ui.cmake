# Editor UI dependencies: pugl (native window embedded in the host), Dear ImGui
# (drawn with OpenGL 3) and ImGuiColorTextEdit (the code editor widget).

set(PUGL_DIR ${CMAKE_SOURCE_DIR}/third_party/pugl)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
set(TEXTEDIT_DIR ${CMAKE_SOURCE_DIR}/third_party/ImGuiColorTextEdit)

find_package(OpenGL REQUIRED)

# TODO(macos): add pugl's mac*.m sources and their system frameworks.
if(WIN32)
    add_library(ui_pugl STATIC ${PUGL_DIR}/src/common.c ${PUGL_DIR}/src/internal.c ${PUGL_DIR}/src/win.c ${PUGL_DIR}/src/win_gl.c)
    target_compile_definitions(ui_pugl PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
    target_link_libraries(ui_pugl PUBLIC dwmapi gdi32 shell32 shlwapi user32)
else()
    find_package(X11 REQUIRED COMPONENTS Xcursor Xrandr Xext)
    set(PUGL_PATCHED_DIR ${CMAKE_BINARY_DIR}/pugl_patched)
    patch_sources(${PUGL_DIR} ${PUGL_PATCHED_DIR} ${CMAKE_SOURCE_DIR}/patches/pugl src/x11.c)
    add_library(ui_pugl STATIC
        ${PUGL_DIR}/src/common.c
        ${PUGL_DIR}/src/internal.c
        ${PUGL_PATCHED_DIR}/src/x11.c
        ${PUGL_DIR}/src/x11_gl.c
    )
    target_compile_definitions(ui_pugl PRIVATE USE_XCURSOR=1 USE_XRANDR=1 USE_XSYNC=1 _POSIX_C_SOURCE=200809L)
    target_link_libraries(ui_pugl PUBLIC X11::X11 X11::Xcursor X11::Xrandr X11::Xext)
endif()
target_include_directories(ui_pugl PUBLIC ${PUGL_DIR}/include PRIVATE ${PUGL_DIR}/src)
target_compile_definitions(ui_pugl PUBLIC PUGL_STATIC PRIVATE PUGL_INTERNAL)
target_link_libraries(ui_pugl PUBLIC OpenGL::GL)

add_library(ui_imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(ui_imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)

set(TEXTEDIT_PATCHED_DIR ${CMAKE_BINARY_DIR}/texteditor_patched)
patch_sources(${TEXTEDIT_DIR} ${TEXTEDIT_PATCHED_DIR} ${CMAKE_SOURCE_DIR}/patches/ImGuiColorTextEdit TextEditor.cpp)
add_library(ui_texteditor STATIC ${TEXTEDIT_PATCHED_DIR}/TextEditor.cpp)
target_include_directories(ui_texteditor PUBLIC ${TEXTEDIT_DIR})
target_link_libraries(ui_texteditor PUBLIC ui_imgui)
