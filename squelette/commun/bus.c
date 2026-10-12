/*
 * bus.c — FOURNI, ne pas modifier. Voir include/embedgate/bus.h.
 */
#include "embedgate/bus.h"
#include "embedgate/registres.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void *eg_bus_ouvrir(const char *chemin, int creer)
{
    if (chemin == NULL) {
        chemin = getenv(EG_BUS_ENV);
        if (chemin == NULL)
            chemin = EG_BUS_DEFAUT;
    }

    int fd = open(chemin, O_RDWR | (creer ? O_CREAT : 0), 0666);
    if (fd < 0)
        return NULL;

    if (creer && ftruncate(fd, (off_t)EG_TAILLE_BANQUE) != 0) {
        int e = errno;
        close(fd);
        errno = e;
        return NULL;
    }

    /* Un bus plus petit que la banque n'est pas une puce EmbedGate. */
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < (off_t)EG_TAILLE_BANQUE) {
        close(fd);
        errno = ENODEV;
        return NULL;
    }

    void *base = mmap(NULL, EG_TAILLE_BANQUE, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);
    int e = errno;
    close(fd);              /* la projection reste valide après close() */
    if (base == MAP_FAILED) {
        errno = e;
        return NULL;
    }
    if (creer)
        memset(base, 0, EG_TAILLE_BANQUE);
    return base;
}

void eg_bus_fermer(void *base)
{
    if (base != NULL)
        munmap(base, EG_TAILLE_BANQUE);
}
