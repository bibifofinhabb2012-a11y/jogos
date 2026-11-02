cmake_minimum_required(VERSION 3.20)
project(SimuladorDeVida CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(simulador-de-vida src/main.cpp src/jogo.cpp src/mapa.cpp src/audio.cpp src/fala.cpp)
target_include_directories(simulador-de-vida PRIVATE externo)

if(WIN32)
  # No Windows, baixa e compila SDL2, OpenAL Soft (som 3D com HRTF) e Tolk (fala pelo leitor de tela).
  include(FetchContent)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)
  set(SDL2_DISABLE_SDL2MAIN OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(sdl2 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-2.30.9 GIT_SHALLOW TRUE)
  set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
  set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
  set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(openal GIT_REPOSITORY https://github.com/kcat/openal-soft.git GIT_TAG 1.23.1 GIT_SHALLOW TRUE)
  FetchContent_Declare(tolk GIT_REPOSITORY https://github.com/dkager/tolk.git GIT_TAG master GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(sdl2 openal)
  FetchContent_GetProperties(tolk)
  if(NOT tolk_POPULATED)
    FetchContent_Populate(tolk)
  endif()
  add_library(Tolk SHARED
    ${tolk_SOURCE_DIR}/src/Tolk.cpp
    ${tolk_SOURCE_DIR}/src/ScreenReaderDriverJAWS.cpp ${tolk_SOURCE_DIR}/src/ScreenReaderDriverNVDA.cpp
    ${tolk_SOURCE_DIR}/src/ScreenReaderDriverSA.cpp ${tolk_SOURCE_DIR}/src/ScreenReaderDriverSNova.cpp
    ${tolk_SOURCE_DIR}/src/ScreenReaderDriverWE.cpp ${tolk_SOURCE_DIR}/src/ScreenReaderDriverZT.cpp
    ${tolk_SOURCE_DIR}/src/ScreenReaderDriverSAPI.cpp
    ${tolk_SOURCE_DIR}/src/fsapi.c ${tolk_SOURCE_DIR}/src/wineyes.c ${tolk_SOURCE_DIR}/src/zt.c)
  target_compile_definitions(Tolk PRIVATE _EXPORTING UNICODE _UNICODE)
  target_include_directories(Tolk PUBLIC ${tolk_SOURCE_DIR}/src)
  target_link_libraries(Tolk PRIVATE User32 Ole32 OleAut32)
  target_compile_options(simulador-de-vida PRIVATE /utf-8)
  target_compile_definitions(simulador-de-vida PRIVATE UNICODE _UNICODE)
  target_link_libraries(simulador-de-vida PRIVATE SDL2::SDL2main SDL2::SDL2 OpenAL::OpenAL Tolk)
  set_target_properties(simulador-de-vida PROPERTIES WIN32_EXECUTABLE TRUE)
  # junta tudo numa pasta só: o jogo, as DLLs e os sons
  add_custom_command(TARGET simulador-de-vida POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE:OpenAL::OpenAL> $<TARGET_FILE:Tolk> $<TARGET_FILE_DIR:simulador-de-vida>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${tolk_SOURCE_DIR}/libs/x64/nvdaControllerClient64.dll ${tolk_SOURCE_DIR}/libs/x64/SAAPI64.dll $<TARGET_FILE_DIR:simulador-de-vida>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/sons $<TARGET_FILE_DIR:simulador-de-vida>/sons)
else()
  # Linux, para testar: usa as bibliotecas do sistema
  find_package(SDL2 REQUIRED)
  find_package(OpenAL REQUIRED)
  target_include_directories(simulador-de-vida PRIVATE ${SDL2_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR})
  target_link_libraries(simulador-de-vida PRIVATE ${SDL2_LIBRARIES} ${OPENAL_LIBRARY})
endif()
