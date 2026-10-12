# Squelette FOURNI du projet EmbedGate. Vous pouvez le compléter (nouvelles
# cibles, options), pas le dégrader : les noms des programmes sont imposés
# par INTERFACES.md §6, et les vérifications en dépendent.
#
# ============================================================================
#     cmake -S . -B build              # préparer (une fois) dans build/
#     cmake --build build -j           # compiler
#     ctest --test-dir build --output-on-failure
#
# Pour la carte ARM (même code, autre compilateur) :
#     cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake
#     cmake --build build-arm -j
# ============================================================================
cmake_minimum_required(VERSION 3.18)
project(embedgate VERSION 0.1 LANGUAGES C CXX)

# C11 et C++17, sans extensions propres à gcc. _GNU_SOURCE rend visibles les
# fonctions POSIX/Linux (timerfd, sigaction, clock_gettime…) malgré -std=c11.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_definitions(_GNU_SOURCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# -DSANITIZE=address  → AddressSanitizer (+ UBSan) : accès mémoire illégaux
# -DSANITIZE=thread   → ThreadSanitizer : accès concurrents non protégés (P3)
#   cmake -S . -B build-tsan -DSANITIZE=thread
set(SANITIZE "" CACHE STRING "Sanitizer : address | thread | vide")

find_package(Threads REQUIRED)

# Zéro warning : chaque avertissement devient une erreur.
function(eg_options cible)
  target_compile_options(${cible} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Werror)
  if(SANITIZE STREQUAL "address")
    target_compile_options(${cible} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${cible} PRIVATE -fsanitize=address,undefined)
  elseif(SANITIZE STREQUAL "thread")
    target_compile_options(${cible} PRIVATE -fsanitize=thread)
    target_link_options(${cible} PRIVATE -fsanitize=thread)
  endif()
endfunction()

# --- La puce (FOURNIE) ------------------------------------------------------
add_executable(sim_capteurs outils/sim_capteurs.c commun/bus.c)
target_include_directories(sim_capteurs PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(sim_capteurs PRIVATE m)
eg_options(sim_capteurs)

# --- Le pilote C (P1) : tout pilote/*.c sauf pilote_demo.c ------------------
file(GLOB PILOTE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/pilote/*.c)
list(REMOVE_ITEM PILOTE_SOURCES ${CMAKE_SOURCE_DIR}/pilote/pilote_demo.c)
add_library(pilote STATIC commun/bus.c ${PILOTE_SOURCES})
target_include_directories(pilote PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/pilote)
eg_options(pilote)

add_executable(pilote_demo pilote/pilote_demo.c)
target_link_libraries(pilote_demo PRIVATE pilote)
eg_options(pilote_demo)

# --- La passerelle C++ (P2, P3) : tout passerelle/*.cpp sauf main.cpp -------
# Elle a besoin de libmosquitto (client MQTT en C) :
#   sudo apt install libmosquitto-dev mosquitto mosquitto-clients
# Sans libmosquitto, la passerelle et ses tests ne sont PAS construits, mais
# le reste (puce, pilote, tests C) l'est : c'est le cas normal de la
# compilation ARM avant la Phase 5.
find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
find_library(MOSQUITTO_LIBRARY mosquitto)
if(MOSQUITTO_INCLUDE_DIR AND MOSQUITTO_LIBRARY)
  set(EG_PASSERELLE ON)
  file(GLOB_RECURSE PASSERELLE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/passerelle/*.cpp)
  list(REMOVE_ITEM PASSERELLE_SOURCES ${CMAKE_SOURCE_DIR}/passerelle/main.cpp)
  add_library(passerelle_lib STATIC ${PASSERELLE_SOURCES})
  target_include_directories(passerelle_lib PUBLIC ${CMAKE_SOURCE_DIR}/passerelle ${MOSQUITTO_INCLUDE_DIR})
  target_link_libraries(passerelle_lib PUBLIC pilote Threads::Threads ${MOSQUITTO_LIBRARY})
  eg_options(passerelle_lib)

  add_executable(embedgate-gw passerelle/main.cpp)
  target_link_libraries(embedgate-gw PRIVATE passerelle_lib)
  eg_options(embedgate-gw)
else()
  set(EG_PASSERELLE OFF)
  message(WARNING "libmosquitto introuvable : la passerelle (embedgate-gw) et ses "
                  "tests ne sont pas construits. Normal pour la cible ARM avant la "
                  "Phase 5 ; sur le PC : sudo apt install libmosquitto-dev")
endif()

# --- Tests : chaque tests/test_xxx.c ou .cpp devient build/test_xxx ---------
#     Les tests C utilisent tests/verif.h, les tests C++ tests/Verif.hpp.
enable_testing()
file(GLOB TESTS_C CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/test_*.c)
file(GLOB TESTS_CPP CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/test_*.cpp)
foreach(source ${TESTS_C})
  get_filename_component(nom ${source} NAME_WE)
  add_executable(${nom} ${source})
  target_link_libraries(${nom} PRIVATE pilote)
  eg_options(${nom})
  add_test(NAME ${nom} COMMAND ${nom})
endforeach()
if(EG_PASSERELLE)
  foreach(source ${TESTS_CPP})
    get_filename_component(nom ${source} NAME_WE)
    add_executable(${nom} ${source})
    target_link_libraries(${nom} PRIVATE passerelle_lib)
    eg_options(${nom})
    add_test(NAME ${nom} COMMAND ${nom})
  endforeach()
endif()
