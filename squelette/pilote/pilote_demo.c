/*
 * pilote_demo.c — À ÉCRIRE (Phase 1).
 *
 * Programme de démonstration de votre pilote (INTERFACES.md §6) :
 *   build/pilote_demo [--duree s]
 * Il démarre la puce, sert ses interruptions et affiche une ligne par
 * interruption servie :  [irq] ch=<n> brut=<entier>
 *
 * Le code du pilote lui-même va dans d'autres fichiers de pilote/ (par
 * exemple pilote.h / pilote.c) : CMake les compile dans la bibliothèque
 * « pilote », utilisée aussi par vos tests et par la passerelle.
 */
#include <stdio.h>

int main(void)
{
    fprintf(stderr, "pilote_demo : à écrire (Phase 1, voir INTERFACES.md)\n");
    return 1;
}
