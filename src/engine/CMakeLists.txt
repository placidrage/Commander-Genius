
option(NOTYPESAVE "Enable this if you don't want to type names for your savesgames" no)

include_directories(${SDL_INCLUDE_DIR})

add_subdirectory(core)
add_subdirectory(keen)
add_subdirectory(refkeen)

# Move this to somewhere else
add_subdirectory(unzip)


if(DOWNLOADER)
    add_definitions(-DDOWNLOADER)
endif()


set(CXXSRC CGameLauncher.cpp
           downloadgui.cpp
           gamedownloader.cpp)


set(HSRC CGameLauncher.h
         gamedownloader.h)

add_library(engine ${CXXSRC}
                   ${HSRC})

target_link_libraries(engine 
                      refkeen 
                      engine_keen_galaxy  
		      engine_keen_vorticon
		      engine_keen_dreams
                      engine_core                       
                      cg_dialog
                      unzip)

