/*
 * test_registres.c — FOURNI, à garder vert. Sert aussi d'EXEMPLE pour vos
 * tests du pilote : on peut tester du code MMIO SANS la puce, en lui donnant
 * pour « adresse de base » un simple tableau en mémoire que le test remplit
 * lui-même (on joue la puce à la main).
 */
#include "verif.h"

#include "embedgate/bus.h"
#include "embedgate/registres.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* Une fausse banque de registres : 0x80 octets = 32 mots de 32 bits. */
static uint32_t banque[EG_TAILLE_BANQUE / 4];

static void carte_des_registres(void)
{
    VERIFIER_EGAL(EG_REG_IRQ_STATUS, 0x0C);
    VERIFIER_EGAL(EG_REG_IRQ_ACK, 0x18);
    VERIFIER_EGAL(EG_REG_DATA(3), 0x2C);
    VERIFIER_EGAL(EG_REG_CALIB(2), 0x48);
    VERIFIER(EG_REG_CALIB(EG_NB_CANAUX - 1) + 4 <= EG_TAILLE_BANQUE);
}

static void lire_et_ecrire_sans_la_puce(void)
{
    /* Le test joue la puce : il « pose » des valeurs dans les registres… */
    EG_ECRIRE32(banque, EG_REG_ID, EG_ID_PUCE);
    EG_ECRIRE32(banque, EG_REG_DATA(0), (uint32_t)-1500);   /* -1,5 °C */
    EG_ECRIRE32(banque, EG_REG_STATUS, EG_STATUS_READY | (7u << EG_STATUS_COMPTEUR_DECALAGE));

    /* … et vérifie ce que le code lit. */
    VERIFIER_EGAL(EG_LIRE32(banque, EG_REG_ID), EG_ID_PUCE);
    VERIFIER_EGAL((int32_t)EG_LIRE32(banque, EG_REG_DATA(0)), -1500);
    VERIFIER_EGAL((EG_LIRE32(banque, EG_REG_STATUS) & EG_STATUS_COMPTEUR_MASQUE)
                      >> EG_STATUS_COMPTEUR_DECALAGE, 7);
    /* Les offsets sont en octets : DATA[0] est le 9e mot (0x20 / 4 = 8). */
    VERIFIER_EGAL(banque[8], (uint32_t)-1500);
}

static void bus_cree_par_la_puce(void)
{
    char chemin[] = "/tmp/eg_test_busXXXXXX";
    int fd = mkstemp(chemin);
    VERIFIER(fd >= 0);
    close(fd);

    void *base = eg_bus_ouvrir(chemin, 1);          /* rôle de la puce */
    VERIFIER(base != NULL);
    if (base) {
        VERIFIER_EGAL(EG_LIRE32(base, EG_REG_ID), 0);   /* banque remise à zéro */
        EG_ECRIRE32(base, EG_REG_CTRL, EG_CTRL_ENABLE);
        void *vue = eg_bus_ouvrir(chemin, 0);       /* rôle du pilote */
        VERIFIER(vue != NULL);
        if (vue) {
            VERIFIER_EGAL(EG_LIRE32(vue, EG_REG_CTRL), EG_CTRL_ENABLE);  /* même mémoire */
            eg_bus_fermer(vue);
        }
        eg_bus_fermer(base);
    }
    unlink(chemin);
}

static void bus_absent(void)
{
    VERIFIER(eg_bus_ouvrir("/tmp/eg_bus_qui_n_existe_pas.bin", 0) == NULL);
}

int main(void)
{
    lancer("carte des registres", carte_des_registres);
    lancer("lire et écrire sans la puce", lire_et_ecrire_sans_la_puce);
    lancer("bus créé par la puce, vu par le pilote", bus_cree_par_la_puce);
    lancer("bus absent", bus_absent);
    return resultat();
}
