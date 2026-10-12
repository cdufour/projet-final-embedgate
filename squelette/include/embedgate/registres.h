/*
 * registres.h — FOURNI, ne pas modifier.
 *
 * Carte des registres de la puce capteur EmbedGate (voir INTERFACES.md, §1).
 * Ce fichier est partagé par la puce (outils/sim_capteurs.c) et par votre
 * pilote : c'est la « datasheet » traduite en C.
 *
 * Sur une vraie carte, ces registres seraient à une adresse physique fixe.
 * Ici, ils sont projetés en mémoire par mmap() sur un fichier commun (le
 * « bus ») : voir bus.h. Pour votre pilote, cela ne change rien : il lit et
 * écrit des mots de 32 bits à des offsets fixes à partir d'une adresse de base.
 */
#ifndef EMBEDGATE_REGISTRES_H
#define EMBEDGATE_REGISTRES_H

#include <stdint.h>

/* Nombre de canaux de la puce. */
#define EG_NB_CANAUX        4u

/* Canaux (INTERFACES.md §1.2). */
#define EG_CANAL_TEMPERATURE 0u   /* milli-degré Celsius   */
#define EG_CANAL_PRESSION    1u   /* milli-millibar        */
#define EG_CANAL_COURANT     2u   /* micro-ampère          */
#define EG_CANAL_PORTE       3u   /* 0 fermée, 1 ouverte   */

/* ---- Offsets des registres, en octets (INTERFACES.md §1.1) ------------- */
#define EG_REG_ID           0x00u  /* R    signature EG_ID_PUCE                 */
#define EG_REG_STATUS       0x04u  /* R    READY, FAULT, compteur               */
#define EG_REG_CTRL         0x08u  /* R/W  ENABLE, RESET, SELFTEST              */
#define EG_REG_IRQ_STATUS   0x0Cu  /* R    bit n : donnée neuve sur le canal n  */
#define EG_REG_IRQ_ENABLE   0x10u  /* R/W  bit n : IT du canal n autorisée      */
#define EG_REG_FAULT_MAP    0x14u  /* R    bit n : canal n en panne             */
#define EG_REG_IRQ_ACK      0x18u  /* W1C  écrire 1 sur le bit n acquitte le canal n */
#define EG_REG_DATA_BASE    0x20u  /* R    DATA[n] à 0x20 + 4n (int32)          */
#define EG_REG_CALIB_BASE   0x40u  /* R/W  CALIB[n] à 0x40 + 4n (int32)         */

#define EG_REG_DATA(n)      (EG_REG_DATA_BASE  + 4u * (uint32_t)(n))
#define EG_REG_CALIB(n)     (EG_REG_CALIB_BASE + 4u * (uint32_t)(n))

/* Taille de la banque de registres (octets). */
#define EG_TAILLE_BANQUE    0x80u

/* Signature lue dans ID quand la puce est présente et initialisée. */
#define EG_ID_PUCE          0xE9B0DA7Au

/* ---- Champs de STATUS --------------------------------------------------- */
#define EG_STATUS_READY       (1u << 0)
#define EG_STATUS_FAULT       (1u << 1)
#define EG_STATUS_COMPTEUR_DECALAGE 8u
#define EG_STATUS_COMPTEUR_MASQUE   (0xFFu << EG_STATUS_COMPTEUR_DECALAGE)

/* ---- Champs de CTRL ----------------------------------------------------- */
#define EG_CTRL_ENABLE        (1u << 0)
#define EG_CTRL_RESET         (1u << 1)
#define EG_CTRL_SELFTEST      (1u << 2)

/*
 * Accès MMIO. `volatile` est indispensable : la valeur d'un registre peut
 * changer à tout moment « sous les pieds » du programme (c'est la puce qui
 * l'écrit). Sans volatile, le compilateur a le droit de ne lire qu'une fois
 * et de réutiliser la valeur, ou de supprimer une écriture qu'il juge inutile.
 */
#define EG_LIRE32(base, off) \
    (*(volatile uint32_t *)((volatile uint8_t *)(base) + (off)))

#define EG_ECRIRE32(base, off, val) \
    (*(volatile uint32_t *)((volatile uint8_t *)(base) + (off)) = (uint32_t)(val))

#endif /* EMBEDGATE_REGISTRES_H */
