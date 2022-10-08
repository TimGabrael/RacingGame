cmake_minimum_required(VERSION 3.8)
project(RacingGameProj)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(3rd-party/glfw)
add_subdirectory(3rd-party/assimp)
add_subdirectory(3rd-party/glad)
add_subdirectory(3rd-party/NFDriver)

set(ENGINE_FILES "src/Graphics/Renderer.h" "src/Graphics/Renderer.cpp"   "src/Util/Math.h" "src/Util/Assets.h" "src/Graphics/Scene.h" "src/Graphics/Scene.cpp" "src/GameState.h" "src/Util/Assets.cpp" "src/Graphics/ModelInfo.h" "src/Graphics/ModelInfo.cpp" "src/Util/Utility.h" "src/Graphics/GLIncludes.h" "src/GameState.cpp" "src/Graphics/Camera.h" "src/Graphics/Camera.cpp" "src/Physics/Physics.h" "src/Physics/Physics.cpp" "src/Audio/WavFile.cpp" "src/Audio/AudioManager.cpp")


add_executable(RacingGame "src/main.cpp" ${ENGINE_FILES} "src/Game/GameManager.cpp" "src/Game/Entitys.cpp" "src/Physics/Controller.h" "src/Physics/Controller.cpp")


set(PHYSX_LIBRARIES
	PhysXExtensions
	PhysX
	PhysXPvdSDK
	PhysXVehicle
	PhysXCharacterKinematic
	PhysXCooking
	PhysXCommon
	PhysXFoundation
)

#TO MAKE THIS WORK I ADDED THE FOLLOWING WARNING DISABLES INTO THE CMAKELISTS.TXT FILE OF PHYSX/WINDOWS
#/wd5219 /wd5045 /wd5220 /wd5038 /wd4464 /wd4365 /wd5204 /wd5243 /wd26495 /wd4264 /wd4866
set(PHYSX_ROOT_DIR D:/SDKs/PhysX-4.0.0/physx)
set(TARGET_BUILD_PLATFORM "windows")
set(PX_BUILDSNIPPETS OFF CACHE BOOL "Generate the snippets")
set(PX_BUILDPUBLICSAMPLES OFF CACHE BOOL "Generate the samples projects")
set(PX_GENERATE_STATIC_LIBRARIES OFF CACHE BOOL "Generate static libraries")
set(PX_FLOAT_POINT_PRECISE_MATH OFF CACHE BOOL "Float point precise math")
set(NV_USE_STATIC_WINCRT OFF CACHE BOOL "Use the statically linked windows CRT")
set(NV_USE_DEBUG_WINCRT OFF CACHE BOOL "Use the debug version of the CRT")
set(PXSHARED_PATH ${PHYSX_ROOT_DIR}/../pxshared)
set(PXSHARED_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CMAKEMODULES_VERSION "1.27")
set(CMAKEMODULES_PATH ${PHYSX_ROOT_DIR}/../externals/cmakemodules)
set(PX_OUTPUT_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/externals/physx)
set(PX_OUTPUT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/externals/physx)

target_include_directories(RacingGame PRIVATE D:/SDKs/PhysX-4.0.0/physx/include ${ZLIB_INCLUDE_DIR})
add_subdirectory(${PHYSX_ROOT_DIR}/compiler/public extern/physx)

target_include_directories(RacingGame PRIVATE 3rd-party/assimp/include 3rd-party/glfw/include 3rd-party/glm 3rd-party/NFDriver/include 3rd-party/rectpack2D/src)
target_link_libraries(RacingGame PRIVATE glfw assimp::assimp ${GLAD_LIBRARIES} NFDriver ${PHYSX_LIBRARIES} zlibstatic)


set_target_properties(RacingGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)