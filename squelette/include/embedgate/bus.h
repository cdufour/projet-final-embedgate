/*
 * bus.h — FOURNI, ne pas modifier.
 *
 * Le « bus » simule le câblage entre le processeur et la puce : un fichier de
 * EG_TAILLE_BANQUE octets, projeté en mémoire partagée par mmap(). Le pointeur
 * obtenu se comporte comme l'adresse de base d'un périphérique : on y lit et
 * écrit les registres avec EG_LIRE32 / EG_ECRIRE32 (registres.h).
 *
 * Chemin du fichier : argument `chemin` s'il n'est pas NULL, sinon la
 * variable d'environnement EG_BUS_PATH, sinon EG_BUS_DEFAUT.
 */
#ifndef EMBEDGATE_BUS_H
#define EMBEDGATE_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#define EG_BUS_ENV     "EG_BUS_PATH"
#define EG_BUS_DEFAUT  "/tmp/embedgate_bus.bin"

/*
 * Ouvre le bus et le projette en mémoire.
 *   creer = 1 : rôle de la PUCE (crée le fichier et remet la banque à zéro) ;
 *   creer = 0 : rôle du PILOTE (le fichier doit exister : la puce est lancée).
 * Renvoie l'adresse de base de la banque, ou NULL (errno renseigné, rien
 * n'est affiché : c'est à l'appelant de produire un message clair).
 */
void *eg_bus_ouvrir(const char *chemin, int creer);

/* Libère la projection. Sans effet si base vaut NULL. */
void eg_bus_fermer(void *base);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDGATE_BUS_H */
