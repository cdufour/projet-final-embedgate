/*
 * verif.h — FOURNI : le même mini-outil de tests que Verif.hpp, pour le C.
 *
 * Chaque fichier tests/test_xxx.c est un PETIT PROGRAMME : son main() lance
 * des fonctions de test, puis renvoie 0 si tout est vert, 1 sinon. CMake en
 * fait l'exécutable build/test_xxx et le déclare à ctest.
 *
 *   #include "verif.h"
 *
 *   static void acquittement(void)
 *   {
 *       ...
 *       VERIFIER(pilote_pret(&p));                 // condition vraie
 *       VERIFIER_EGAL(lire_ack(banque), 0x1);      // deux entiers égaux
 *   }
 *
 *   int main(void)
 *   {
 *       lancer("acquittement", acquittement);
 *       return resultat();
 *   }
 *
 * Un échec affiche le fichier, la ligne et, pour VERIFIER_EGAL, les deux
 * valeurs (en décimal et en hexadécimal : pratique pour des registres).
 * Les vérifications suivantes continuent.
 *
 * LANCER :
 *   ctest --test-dir build --output-on-failure   tous les fichiers de tests
 *   ./build/test_pilote                           un seul
 */
#ifndef EMBEDGATE_VERIF_H
#define EMBEDGATE_VERIF_H

#include <stdio.h>

static int verif_nb_echecs = 0;
static int verif_nb_tests = 0;

static void verif_echec_egal(const char *fichier, int ligne, const char *ta, const char *tb,
                             long long a, long long b)
{
    ++verif_nb_echecs;
    fprintf(stderr, "  ECHEC %s:%d : %s == %s\n         obtenu %lld (0x%llX), attendu %lld (0x%llX)\n",
            fichier, ligne, ta, tb, a, (unsigned long long)a, b, (unsigned long long)b);
}

#define VERIFIER(condition)                                                    \
    do {                                                                       \
        if (!(condition)) {                                                    \
            ++verif_nb_echecs;                                                 \
            fprintf(stderr, "  ECHEC %s:%d : %s\n", __FILE__, __LINE__, #condition); \
        }                                                                      \
    } while (0)

#define VERIFIER_EGAL(a, b)                                                    \
    do {                                                                       \
        long long verif_a_ = (long long)(a), verif_b_ = (long long)(b);        \
        if (verif_a_ != verif_b_)                                              \
            verif_echec_egal(__FILE__, __LINE__, #a, #b, verif_a_, verif_b_);  \
    } while (0)

static void lancer(const char *nom, void (*test)(void))
{
    int avant = verif_nb_echecs;
    ++verif_nb_tests;
    test();
    printf("%s %s\n", verif_nb_echecs == avant ? "[ OK ]" : "[ECHEC]", nom);
}

static int resultat(void)
{
    printf("%d test(s), %d vérification(s) en échec\n", verif_nb_tests, verif_nb_echecs);
    return verif_nb_echecs == 0 ? 0 : 1;
}

#endif /* EMBEDGATE_VERIF_H */
