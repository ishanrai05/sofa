cmake_minimum_required(VERSION 3.1)
project(SceneCreator)

set(SCENECREATOR_MAJOR_VERSION 0)
set(SCENECREATOR_MINOR_VERSION 1)
set(SCENECREATOR_VERSION ${SCENECREATOR_MAJOR_VERSION}.${SCENECREATOR_MINOR_VERSION})

set(HEADER_FILES
    GetAssembledSizeVisitor.h
    GetVectorVisitor.h
    SceneCreator.h
    SceneUtils.h
)

set(SOURCE_FILES
    GetAssembledSizeVisitor.cpp
    GetVectorVisitor.cpp
    SceneCreator.cpp
    SceneUtils.cpp
    initSceneCreator.cpp
)

find_package(SofaSimulation REQUIRED)
find_package(Metis QUIET)

## SceneCreator/config.h
configure_file(config.h.in "${CMAKE_BINARY_DIR}/include/SceneCreator/config.h")
install(FILES "${CMAKE_BINARY_DIR}/include/SceneCreator/config.h" DESTINATION "include/SceneCreator" COMPONENT headers)

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} SofaSimulationGraph)
target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>")
target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>")
target_include_directories(${PROJECT_NAME} PUBLIC "$<INSTALL_INTERFACE:include>")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${SCENECREATOR_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DSOFA_BUILD_SCENECREATOR")
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")

if(Metis_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "-DSOFA_HAVE_METIS")
endif()

## Install rules for the library; CMake package configurations files
sofa_create_package(SceneCreator ${SCENECREATOR_VERSION} SceneCreator SceneCreator)


## Add example project to use sceneCreator
add_subdirectory(sceneCreatorExamples)

## Add test project
if(SOFA_BUILD_TESTS)
    find_package(SofaTest QUIET)
    add_subdirectory(SceneCreator_test)
endif()
