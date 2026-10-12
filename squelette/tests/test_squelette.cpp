// test_squelette.cpp — FOURNI : vérifie que le C++ voit le C (pilote, registres).
#include "Verif.hpp"

#include <string>

#include "Version.hpp"
#include "embedgate/registres.h"

void version_lisible()
{
    VERIFIER(!std::string(embedgate::version()).empty());
}

void registres_visibles_en_cpp()
{
    VERIFIER_EGAL(EG_ID_PUCE, 0xE9B0DA7Au);
    VERIFIER_EGAL(EG_NB_CANAUX, 4u);
}

int main()
{
    verif::lancer("version lisible", version_lisible);
    verif::lancer("registres visibles en C++", registres_visibles_en_cpp);
    return verif::resultat();
}
