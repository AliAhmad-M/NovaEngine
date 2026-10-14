get_filename_component(NOVA_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

# Find the triplet folder (x64-windows, x64-windows-static, ...) inside vcpkg_installed
file(GLOB _nova_triplets LIST_DIRECTORIES true "${NOVA_ROOT}/vcpkg_installed/*")
foreach(_t IN LISTS _nova_triplets)
    if(IS_DIRECTORY "${_t}" AND EXISTS "${_t}/include")
        set(NOVA_VCPKG_PREFIX "${_t}")
        list(APPEND CMAKE_PREFIX_PATH "${_t}")
        break()
    endif()
endforeach()

if(NOT NOVA_VCPKG_PREFIX)
    message(FATAL_ERROR "No vcpkg packages found in ${NOVA_ROOT}/vcpkg_installed")
endif()

include(CMakeFindDependencyMacro)

# ---- vcpkg packages (names match what vcpkg prints in its usage text) ----
find_dependency(glad CONFIG)
find_dependency(glfw3 CONFIG)
find_dependency(glm CONFIG)
find_dependency(Freetype)
find_dependency(OpenAL CONFIG)
find_dependency(Jolt CONFIG)
find_dependency(nlohmann_json CONFIG)
find_dependency(box2d CONFIG)
find_dependency(imgui CONFIG)
find_dependency(imguizmo CONFIG)

# stb is header-only with no CMake config
find_path(NOVA_STB_INCLUDE_DIR "stb_image.h")

get_filename_component(NOVA_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

# ---- Nova modules ----
set(_NOVA_MODULES Core Platform Graphics Physics World Runtime)

foreach(m IN LISTS _NOVA_MODULES)
    if(NOT TARGET Nova::${m})
        add_library(Nova::${m} STATIC IMPORTED)
        set_target_properties(Nova::${m} PROPERTIES
            IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
            IMPORTED_LOCATION_RELEASE "${NOVA_ROOT}/lib/Release/${m}.lib"
            IMPORTED_LOCATION_DEBUG   "${NOVA_ROOT}/lib/Debug/${m}.lib"
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            INTERFACE_INCLUDE_DIRECTORIES "${NOVA_ROOT}/include"
        )
    endif()
endforeach()

# ---- All third-party deps, bundled into one target ----
if(NOT TARGET Nova::ThirdParty)
    add_library(Nova::ThirdParty INTERFACE IMPORTED)
    set_property(TARGET Nova::ThirdParty PROPERTY INTERFACE_LINK_LIBRARIES
        glad::glad
        glfw
        glm::glm
        Freetype::Freetype
        OpenAL::OpenAL
        Jolt::Jolt
        nlohmann_json::nlohmann_json
        box2d::box2d
        imgui::imgui
        imguizmo::imguizmo
        opengl32 gdi32 user32 shell32 winmm
    )
    if(NOVA_STB_INCLUDE_DIR)
        set_property(TARGET Nova::ThirdParty APPEND PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES "${NOVA_STB_INCLUDE_DIR}")
    endif()
endif()

# ---- Module dependency edges (my guess; adjust if you get LNK2019) ----
set_property(TARGET Nova::Core     PROPERTY INTERFACE_LINK_LIBRARIES)
set_property(TARGET Nova::Platform PROPERTY INTERFACE_LINK_LIBRARIES Nova::Core)
set_property(TARGET Nova::Physics  PROPERTY INTERFACE_LINK_LIBRARIES Nova::Core)
set_property(TARGET Nova::Graphics PROPERTY INTERFACE_LINK_LIBRARIES
    Nova::Platform Nova::Core)
set_property(TARGET Nova::World    PROPERTY INTERFACE_LINK_LIBRARIES
    Nova::Graphics Nova::Physics Nova::Platform Nova::Core)
set_property(TARGET Nova::Runtime  PROPERTY INTERFACE_LINK_LIBRARIES
    Nova::World Nova::Physics Nova::Graphics Nova::Platform Nova::Core)

# ---- The one target the game links ----
if(NOT TARGET Nova::Engine)
    add_library(Nova::Engine INTERFACE IMPORTED)
    set_property(TARGET Nova::Engine PROPERTY INTERFACE_LINK_LIBRARIES
        Nova::Runtime Nova::ThirdParty)
endif()

set(Nova_FOUND TRUE)