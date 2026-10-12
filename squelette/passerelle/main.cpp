// main.cpp — À ÉCRIRE (Phase 2) : le programme embedgate-gw.
//   build/embedgate-gw --config config/gateway.ini
// Le code de la passerelle va dans d'autres fichiers de passerelle/ : CMake
// les compile dans la bibliothèque passerelle_lib, utilisée aussi par vos tests.
#include <iostream>

#include "Version.hpp"

int main()
{
    std::cerr << "embedgate-gw " << embedgate::version()
              << " : à écrire (Phase 2, voir INTERFACES.md)\n";
    return 1;
}
