cmake_minimum_required(VERSION 3.7.2)


set(SIMPLE_EXAMPLE_SOURCE
   main.cpp
   TestScene.h
   TestScene.cpp
)


project(SimpleExample)
add_executable(SimpleExample ${SIMPLE_EXAMPLE_SOURCE})
target_link_libraries(SimpleExample PUBLIC Renderer Gui Scene AssetLoader)

set_target_properties(SimpleExample PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)




