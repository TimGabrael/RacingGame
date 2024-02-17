cmake_minimum_required(VERSION 3.8)
project(RacingGameProj)
set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)




set(IMGUI_FILES "3rd-party/imgui/imgui.cpp" "3rd-party/imgui/imgui_demo.cpp" "3rd-party/imgui/imgui_draw.cpp" "3rd-party/imgui/imgui_tables.cpp" "3rd-party/imgui/imgui_widgets.cpp" "3rd-party/imgui/backends/imgui_impl_glfw.cpp" "3rd-party/imgui/backends/imgui_impl_opengl3.cpp")

set(ENGINE_FILES "engine/Graphics/Renderer.h" "engine/Graphics/Renderer.cpp"   "engine/Util/Math.h" "engine/Util/Assets.h" "engine/Graphics/Scene.h" "engine/Graphics/Scene.cpp" "engine/GameState.h" "engine/Util/Assets.cpp" "engine/Graphics/ModelInfo.h" "engine/Graphics/ModelInfo.cpp" "engine/Util/Utility.h" "engine/Graphics/GLIncludes.h" "engine/GameState.cpp" "engine/Graphics/Camera.h" "engine/Graphics/Camera.cpp" "engine/Physics/Physics.h" "engine/Physics/Physics.cpp" "engine/Audio/WavFile.cpp" "engine/Audio/AudioManager.cpp" "engine/Physics/Controller.h" "engine/Physics/Controller.cpp" "engine/Graphics/Renderable.h" "engine/Graphics/Renderable.cpp" "engine/Audio/miniaudio.h" ${IMGUI_FILES})
set(ENGINE_INCLUDES D:/vcpkg/packages/physx_x64-windows/include ${ZLIB_INCLUDE_DIR} 3rd-party/glm 3rd-party/rectpack2D/src 3rd-party/imgui 3rd-party/imgui/backends engine/)
set(ENGINE_LIBRARYS glfw assimp::assimp glad::glad Xinput)



add_executable(RacingGame "src/main.cpp" ${ENGINE_FILES} "src/GameManager.cpp" "src/Entitys.cpp" "src/GameManager.h" "src/Entitys.h")



target_include_directories(RacingGame PRIVATE ${ENGINE_INCLUDES} D:/vcpkg/packages/zlib_x64-windows-static/include)
target_link_libraries(RacingGame PRIVATE ${ENGINE_LIBRARYS} D:/vcpkg/packages/zlib_x64-windows-static/lib/zlib.lib unofficial::omniverse-physx-sdk::sdk Mfplat.lib)


set_target_properties(RacingGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)


# Optional: import the defined target to copy over the GPU acceleration libraries (3rd party provided by NVIDIA)
if(TARGET unofficial::omniverse-physx-sdk::gpu-library)
    if(UNIX)
        # Add rpath setting to find .so libraries on unix based systems
        set_target_properties(RacingGame PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$ORIGIN"
            )
    endif()
    add_custom_command(TARGET RacingGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-library>
        $<TARGET_FILE_DIR:RacingGame>)
    if(WIN32)
        add_custom_command(TARGET RacingGame POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-device-library>
            $<TARGET_FILE_DIR:RacingGame>)
    endif()
else()
    message(WARNING "GPU acceleration library target not defined - GPU acceleration will NOT be available!")
endif()

