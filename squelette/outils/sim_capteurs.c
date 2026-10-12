/*
 * sim_capteurs.c — FOURNI, ne pas modifier : c'est le MATÉRIEL.
 *
 * Joue le rôle de la puce capteur décrite dans INTERFACES.md. Vous n'avez pas
 * à lire ce fichier pour faire le projet : la datasheet (INTERFACES.md) suffit,
 * comme avec une vraie puce. Il est fourni en source pour être compilé sur
 * votre machine et pour la cible ARM.
 *
 * Usage :
 *   build/sim_capteurs [--scenario FICHIER.scn] [--vite N] [--bus CHEMIN] [--silencieux]
 *
 * Comportement (INTERFACES.md §1.3) :
 *   - au démarrage : banque à zéro, STATUS.READY, puis ID (écrit en dernier) ;
 *   - en veille tant que CTRL.ENABLE = 0 ; le temps du scénario ne s'écoule
 *     que pendant que ENABLE = 1 (il démarre donc quand le pilote démarre) ;
 *   - à chaque période : DATA[n] = mesure + CALIB[n] pour chaque canal qui
 *     n'est pas en panne, puis bit n de IRQ_STATUS si IRQ_ENABLE le permet ;
 *   - IRQ_STATUS n'est écrit QUE par la puce : une écriture du pilote y est
 *     écrasée. On acquitte en écrivant 1 sur le bit n de IRQ_ACK ;
 *   - RESET et SELFTEST sont traités puis effacés par la puce ;
 *   - à l'arrêt (fin du scénario, Ctrl-C, SIGTERM) : ID et STATUS repassent
 *     à 0, comme une puce qu'on débranche.
 */
#include "embedgate/bus.h"
#include "embedgate/registres.h"

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------------ */
/* Scénario                                                                  */
/* ------------------------------------------------------------------------ */

typedef enum { A_FIXE, A_RAMPE, A_BRUIT, A_PANNE, A_RETABLIT } action_t;

static const char *const NOM_ACTION[] = { "fixe", "rampe", "bruit", "panne", "retablit" };
static const char *const NOM_CANAL[EG_NB_CANAUX] = { "temperature", "pression", "courant", "porte" };

typedef struct {
    uint32_t t_ms;
    unsigned canal;
    action_t action;
    double   v1, v2;
} evenement_t;

typedef struct {
    unsigned     periode_ms;
    unsigned     duree_s;      /* 0 = sans fin */
    uint32_t     graine;
    evenement_t *ev;
    size_t       nb, cap;
} scenario_t;

typedef struct {
    double   base;              /* consigne sans bruit */
    int      rampe;
    double   r_depart, r_cible;
    uint32_t r_t0, r_duree_ms;
    double   bruit;
    int      panne;
} canal_t;

static void ajouter(scenario_t *sc, evenement_t e)
{
    if (sc->nb == sc->cap) {
        sc->cap = sc->cap ? 2 * sc->cap : 32;
        evenement_t *p = realloc(sc->ev, sc->cap * sizeof *p);
        if (!p) { perror("sim_capteurs"); exit(EXIT_FAILURE); }
        sc->ev = p;
    }
    /* Insertion triée et stable : à instant égal, l'ordre du fichier est gardé. */
    size_t i = sc->nb++;
    while (i > 0 && sc->ev[i - 1].t_ms > e.t_ms) {
        sc->ev[i] = sc->ev[i - 1];
        --i;
    }
    sc->ev[i] = e;
}

static void scenario_defaut(scenario_t *sc)
{
    sc->periode_ms = 100;
    sc->duree_s = 0;
    sc->graine = 1;
    const evenement_t base[] = {
        { 0, 0, A_FIXE, 4.0, 0 },    { 0, 0, A_BRUIT, 0.3, 0 },
        { 0, 1, A_FIXE, 1013.0, 0 }, { 0, 1, A_BRUIT, 1.0, 0 },
        { 0, 2, A_FIXE, 500.0, 0 },  { 0, 2, A_BRUIT, 10.0, 0 },
        { 0, 3, A_FIXE, 0.0, 0 },
    };
    for (size_t i = 0; i < sizeof base / sizeof base[0]; ++i)
        ajouter(sc, base[i]);
}

static void erreur_scn(const char *fichier, unsigned ligne, const char *msg)
{
    fprintf(stderr, "%s:%u : %s\n", fichier, ligne, msg);
    exit(2);
}

static void scenario_lire(scenario_t *sc, const char *fichier)
{
    FILE *f = fopen(fichier, "r");
    if (!f) {
        fprintf(stderr, "sim_capteurs : impossible d'ouvrir %s : %s\n", fichier, strerror(errno));
        exit(2);
    }
    sc->periode_ms = 100;
    sc->duree_s = 0;
    sc->graine = 1;

    char buf[512];
    unsigned ligne = 0;
    while (fgets(buf, sizeof buf, f)) {
        ++ligne;
        char *diese = strchr(buf, '#');
        if (diese) *diese = '\0';

        char mot[32], action[32];
        double t, v1, v2;
        unsigned u, canal;
        char reste;

        char *p = buf;
        while (isspace((unsigned char)*p)) ++p;
        if (*p == '\0') continue;

        if (sscanf(p, "%31s", mot) == 1 && isalpha((unsigned char)mot[0])) {
            if (sscanf(p, "%31s %u %c", mot, &u, &reste) != 2)
                erreur_scn(fichier, ligne, "directive attendue : periode|duree|graine <entier>");
            if (strcmp(mot, "periode") == 0) {
                if (u < 1 || u > 10000) erreur_scn(fichier, ligne, "periode entre 1 et 10000 ms");
                sc->periode_ms = u;
            } else if (strcmp(mot, "duree") == 0) {
                sc->duree_s = u;
            } else if (strcmp(mot, "graine") == 0) {
                sc->graine = (uint32_t)u;
            } else {
                erreur_scn(fichier, ligne, "directive inconnue (periode, duree, graine)");
            }
            continue;
        }

        int n = sscanf(p, "%lf %u %31s %lf %lf %c", &t, &canal, action, &v1, &v2, &reste);
        if (n < 3) erreur_scn(fichier, ligne, "attendu : <t en s> <canal> <action> [valeurs]");
        if (t < 0) erreur_scn(fichier, ligne, "instant négatif");
        if (canal >= EG_NB_CANAUX) erreur_scn(fichier, ligne, "canal inexistant (0 à 3)");

        evenement_t e = { (uint32_t)llround(t * 1000.0), canal, A_FIXE, 0, 0 };
        int attendu;
        if      (strcmp(action, "fixe") == 0)     { e.action = A_FIXE;     attendu = 1; }
        else if (strcmp(action, "rampe") == 0)    { e.action = A_RAMPE;    attendu = 2; }
        else if (strcmp(action, "bruit") == 0)    { e.action = A_BRUIT;    attendu = 1; }
        else if (strcmp(action, "panne") == 0)    { e.action = A_PANNE;    attendu = 0; }
        else if (strcmp(action, "retablit") == 0) { e.action = A_RETABLIT; attendu = 0; }
        else erreur_scn(fichier, ligne, "action inconnue (fixe, rampe, bruit, panne, retablit)");

        if (n - 3 != attendu) erreur_scn(fichier, ligne, "mauvais nombre de valeurs pour cette action");
        if (attendu >= 1) e.v1 = v1;
        if (attendu >= 2) e.v2 = v2;
        if (e.action == A_RAMPE && e.v2 <= 0) erreur_scn(fichier, ligne, "durée de rampe > 0 attendue");
        if (e.action == A_BRUIT && e.v1 < 0) erreur_scn(fichier, ligne, "amplitude de bruit >= 0 attendue");
        ajouter(sc, e);
    }
    fclose(f);
}

/* ------------------------------------------------------------------------ */
/* Mesures                                                                   */
/* ------------------------------------------------------------------------ */

/* Bruit déterministe : ne dépend que de la graine, du numéro d'acquisition et
   du canal. Deux exécutions du même scénario donnent les mêmes mesures. */
static uint32_t melanger(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

static double bruit(uint32_t graine, uint32_t k, unsigned canal)
{
    uint32_t h = melanger(graine ^ melanger(k * 4u + canal + 0x9e3779b9U));
    return (double)h / 4294967295.0 * 2.0 - 1.0;   /* [-1, 1] */
}

static double consigne(canal_t *c, uint32_t t_ms)
{
    if (c->rampe) {
        double f = (double)(t_ms - c->r_t0) / (double)c->r_duree_ms;
        if (f >= 1.0) {
            c->rampe = 0;
            c->base = c->r_cible;
        } else {
            return c->r_depart + (c->r_cible - c->r_depart) * f;
        }
    }
    return c->base;
}

static void appliquer(canal_t *c, const evenement_t *e, int bavard)
{
    switch (e->action) {
    case A_FIXE:     c->base = e->v1; c->rampe = 0; break;
    case A_RAMPE:
        c->r_depart = consigne(c, e->t_ms);
        c->r_cible = e->v1;
        c->r_t0 = e->t_ms;
        c->r_duree_ms = (uint32_t)llround(e->v2 * 1000.0);
        c->rampe = 1;
        break;
    case A_BRUIT:    c->bruit = e->v1; break;
    case A_PANNE:    c->panne = 1; break;
    case A_RETABLIT: c->panne = 0; break;
    }
    if (bavard) {
        printf("[puce] t=%6.1f s  canal %u (%s) : %s", e->t_ms / 1000.0, e->canal,
               NOM_CANAL[e->canal], NOM_ACTION[e->action]);
        if (e->action == A_FIXE || e->action == A_BRUIT) printf(" %g", e->v1);
        if (e->action == A_RAMPE) printf(" vers %g en %g s", e->v1, e->v2);
        printf("\n");
        fflush(stdout);
    }
}

/* ------------------------------------------------------------------------ */
/* Boucle principale                                                         */
/* ------------------------------------------------------------------------ */

static volatile sig_atomic_t g_arret = 0;
static void sur_signal(int sig) { (void)sig; g_arret = 1; }

static double maintenant_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static volatile uint32_t *reg(void *base, uint32_t off)
{
    return (volatile uint32_t *)((volatile uint8_t *)base + off);
}

static void usage(FILE *f)
{
    fprintf(f,
        "Usage : sim_capteurs [--scenario FICHIER.scn] [--vite N] [--bus CHEMIN] [--silencieux]\n"
        "  --scenario  rejoue un scénario (sans : valeurs nominales bruitées, sans fin)\n"
        "  --vite N    déroule le scénario N fois plus vite (ex. 4)\n"
        "  --bus       chemin du bus (défaut : $EG_BUS_PATH ou " EG_BUS_DEFAUT ")\n"
        "  --silencieux  n'affiche pas les événements du scénario\n");
}

int main(int argc, char **argv)
{
    const char *fichier = NULL, *chemin_bus = NULL;
    double vite = 1.0;
    int bavard = 1;

    static const struct option opts[] = {
        { "scenario",   required_argument, NULL, 's' },
        { "vite",       required_argument, NULL, 'v' },
        { "bus",        required_argument, NULL, 'b' },
        { "silencieux", no_argument,       NULL, 'q' },
        { "aide",       no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };
    int o;
    while ((o = getopt_long(argc, argv, "s:v:b:qh", opts, NULL)) != -1) {
        switch (o) {
        case 's': fichier = optarg; break;
        case 'v': vite = atof(optarg); break;
        case 'b': chemin_bus = optarg; break;
        case 'q': bavard = 0; break;
        case 'h': usage(stdout); return 0;
        default:  usage(stderr); return 2;
        }
    }
    if (optind < argc || !(vite > 0.0 && vite <= 1000.0)) {
        usage(stderr);
        return 2;
    }

    scenario_t sc = { 0 };
    if (fichier) scenario_lire(&sc, fichier);
    else         scenario_defaut(&sc);

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = sur_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    void *base = eg_bus_ouvrir(chemin_bus, 1);
    if (!base) {
        fprintf(stderr, "sim_capteurs : bus inaccessible : %s\n", strerror(errno));
        return 1;
    }

    volatile uint32_t *r_status = reg(base, EG_REG_STATUS);
    volatile uint32_t *r_ctrl   = reg(base, EG_REG_CTRL);
    volatile uint32_t *r_irq    = reg(base, EG_REG_IRQ_STATUS);
    volatile uint32_t *r_irq_en = reg(base, EG_REG_IRQ_ENABLE);
    volatile uint32_t *r_fault  = reg(base, EG_REG_FAULT_MAP);
    volatile uint32_t *r_ack    = reg(base, EG_REG_IRQ_ACK);

    *r_status = EG_STATUS_READY;
    __atomic_store_n(reg(base, EG_REG_ID), EG_ID_PUCE, __ATOMIC_RELEASE);  /* en dernier */

    if (bavard) {
        printf("[puce] prête : ID=0x%08X, période %u ms, %s%s, vitesse x%g\n",
               EG_ID_PUCE, sc.periode_ms, fichier ? "scénario " : "valeurs nominales",
               fichier ? fichier : "", vite);
        printf("[puce] en veille jusqu'à CTRL.ENABLE = 1\n");
        fflush(stdout);
    }

    canal_t canaux[EG_NB_CANAUX];
    memset(canaux, 0, sizeof canaux);
    size_t prochain_ev = 0;
    uint32_t k = 0;                 /* numéro d'acquisition */
    uint32_t en_attente = 0;        /* IRQ_STATUS, tenu par la puce seule */
    uint8_t compteur = 0;
    double pas_ms = sc.periode_ms / vite;
    double echeance = 0.0;          /* instant réel de la prochaine acquisition */
    int en_marche = 0;
    int fin = 0;

    while (!g_arret && !fin) {
        /* 1. Acquittements : chaque bit à 1 dans IRQ_ACK efface le bit de IRQ_STATUS. */
        uint32_t ack = __atomic_exchange_n(r_ack, 0u, __ATOMIC_ACQ_REL);
        en_attente &= ~ack;

        /* 2. Commandes du registre CTRL. */
        uint32_t ctrl = *r_ctrl;
        if (ctrl & EG_CTRL_RESET) {
            for (unsigned c = 0; c < EG_NB_CANAUX; ++c) {
                *reg(base, EG_REG_DATA(c)) = 0;
                *reg(base, EG_REG_CALIB(c)) = 0;
            }
            en_attente = 0;
            __atomic_fetch_and(r_ctrl, ~EG_CTRL_RESET, __ATOMIC_ACQ_REL);
            if (bavard) { printf("[puce] RESET traité\n"); fflush(stdout); }
        }
        if (ctrl & EG_CTRL_SELFTEST) {
            uint32_t st = *r_status & ~EG_STATUS_FAULT;
            if (*r_fault) st |= EG_STATUS_FAULT;
            *r_status = st;
            __atomic_fetch_and(r_ctrl, ~EG_CTRL_SELFTEST, __ATOMIC_ACQ_REL);
            if (bavard) { printf("[puce] SELFTEST : %s\n", *r_fault ? "défaut" : "ok"); fflush(stdout); }
        }

        /* 3. Acquisition périodique, seulement si ENABLE. */
        int enable = (ctrl & EG_CTRL_ENABLE) != 0;
        double t_reel = maintenant_ms();
        if (enable && !en_marche) {
            echeance = t_reel;          /* reprise : pas de rattrapage */
            if (bavard && k == 0) { printf("[puce] ENABLE : début du scénario\n"); fflush(stdout); }
        }
        en_marche = enable;

        if (enable && t_reel >= echeance) {
            uint32_t t_ms = k * sc.periode_ms;
            while (prochain_ev < sc.nb && sc.ev[prochain_ev].t_ms <= t_ms) {
                const evenement_t *e = &sc.ev[prochain_ev++];
                appliquer(&canaux[e->canal], e, bavard);
            }

            uint32_t irq_en = *r_irq_en;
            uint32_t pannes = 0;
            for (unsigned c = 0; c < EG_NB_CANAUX; ++c) {
                canal_t *cn = &canaux[c];
                double v = consigne(cn, t_ms) + cn->bruit * bruit(sc.graine, k, c);
                if (cn->panne) { pannes |= 1u << c; continue; }
                int32_t brut = (c == EG_CANAL_PORTE) ? (int32_t)lround(v)
                                                     : (int32_t)lround(v * 1000.0);
                int32_t calib = (int32_t)*reg(base, EG_REG_CALIB(c));
                *reg(base, EG_REG_DATA(c)) = (uint32_t)(brut + calib);
                if (irq_en & (1u << c))
                    en_attente |= 1u << c;
            }
            *r_fault = pannes;
            ++compteur;
            uint32_t st = EG_STATUS_READY | ((uint32_t)compteur << EG_STATUS_COMPTEUR_DECALAGE);
            if (pannes) st |= EG_STATUS_FAULT;
            *r_status = st;

            ++k;
            echeance += pas_ms;
            if (sc.duree_s && t_ms >= sc.duree_s * 1000u) fin = 1;
        }

        /* 4. IRQ_STATUS reflète l'état de la puce (une écriture du pilote est écrasée). */
        __atomic_store_n(r_irq, en_attente, __ATOMIC_RELEASE);

        struct timespec dodo = { 0, 1000000L };   /* 1 ms */
        nanosleep(&dodo, NULL);
    }

    /* Débranchement : le pilote doit voir que la puce a disparu. */
    *r_status = 0;
    *r_irq = 0;
    __atomic_store_n(reg(base, EG_REG_ID), 0u, __ATOMIC_RELEASE);
    eg_bus_fermer(base);
    free(sc.ev);
    if (bavard) {
        if (fin) printf("[puce] fin du scénario (%u s)\n", sc.duree_s);
        else     printf("[puce] arrêt\n");
        fflush(stdout);
    }
    return 0;
}
