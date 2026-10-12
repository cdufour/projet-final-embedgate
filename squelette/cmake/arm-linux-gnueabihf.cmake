# Fichier de chaîne de compilation FOURNI : compiler pour ARM 32 bits.
#
#   cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake
#   cmake --build build-arm -j
#   ctest --test-dir build-arm        # les tests tournent sous qemu-arm
#
# Paquets Debian : gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf qemu-user
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Bibliothèques et en-têtes : ceux de la cible, jamais ceux du PC.
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Binaires statiques : ils tournent sous qemu-arm sans autre réglage.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")

# ctest lance les tests ARM à travers l'émulateur.
set(CMAKE_CROSSCOMPILING_EMULATOR qemu-arm)
