cmake_minimum_required(VERSION 3.8)
project(RacingGameProj)
set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)




set(IMGUI_FILES "3rd-party/imgui/imgui.cpp" "3rd-party/imgui/imgui_demo.cpp" "3rd-party/imgui/imgui_draw.cpp" "3rd-party/imgui/imgui_tables.cpp" "3rd-party/imgui/imgui_widgets.cpp" "3rd-party/imgui/backends/imgui_impl_glfw.cpp" "3rd-party/imgui/backends/imgui_impl_opengl3.cpp")

set(ENGINE_FILES "src/Graphics/Renderer.h" "src/Graphics/Renderer.cpp"   "src/Util/Math.h" "src/Util/Assets.h" "src/Graphics/Scene.h" "src/Graphics/Scene.cpp" "src/GameState.h" "src/Util/Assets.cpp" "src/Graphics/ModelInfo.h" "src/Graphics/ModelInfo.cpp" "src/Util/Utility.h" "src/Graphics/GLIncludes.h" "src/GameState.cpp" "src/Graphics/Camera.h" "src/Graphics/Camera.cpp" "src/Physics/Physics.h" "src/Physics/Physics.cpp" "src/Audio/WavFile.cpp" "src/Audio/AudioManager.cpp" "src/Physics/Controller.h" "src/Physics/Controller.cpp" ${IMGUI_FILES})
set(ENGINE_INCLUDES D:/vcpkg/packages/physx_x64-windows/include ${ZLIB_INCLUDE_DIR} 3rd-party/glm 3rd-party/rectpack2D/src 3rd-party/imgui 3rd-party/imgui/backends src/)
set(ENGINE_LIBRARYS glfw assimp::assimp glad::glad Xinput)



add_executable(RacingGame "example/main.cpp" ${ENGINE_FILES} "example/GameManager.cpp" "example/Entitys.cpp" "src/Graphics/Renderable.h" "src/Graphics/Renderable.cpp")



target_include_directories(RacingGame PRIVATE ${ENGINE_INCLUDES})
target_link_libraries(RacingGame PRIVATE ${ENGINE_LIBRARYS} zlib unofficial::omniverse-physx-sdk::sdk Mfplat.lib)


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

