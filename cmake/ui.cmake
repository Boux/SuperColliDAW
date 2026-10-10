# Editor UI dependencies: pugl (native window embedded in the host), Dear ImGui
# (drawn with OpenGL 3) and ImGuiColorTextEdit (the code editor widget).

set(PUGL_DIR ${CMAKE_SOURCE_DIR}/third_party/pugl)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
set(TEXTEDIT_DIR ${CMAKE_SOURCE_DIR}/third_party/ImGuiColorTextEdit)

find_package(OpenGL REQUIRED)

set(PUGL_PATCHED_DIR ${CMAKE_BINARY_DIR}/pugl_patched)
patch_sources(${PUGL_DIR} ${PUGL_PATCHED_DIR} ${CMAKE_SOURCE_DIR}/patches/pugl src/win.c src/x11.c)
add_library(ui_pugl STATIC ${PUGL_DIR}/src/common.c ${PUGL_DIR}/src/internal.c)
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
