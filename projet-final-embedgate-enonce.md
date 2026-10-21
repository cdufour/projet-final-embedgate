# Projet final — « EmbedGate » : passerelle de supervision embarquée

**POEI — Linux, Langages & Systèmes (LLS · EXPLEO)** · AJC FORMATION
Projet de validation · **10 jours** (du lundi 12 au vendredi 23 octobre 2026) ·
soutenance le lundi 26 octobre · Travail en **autonomie**, en binômes (un trinôme)
Environnement : **VM Debian** · **aucun matériel physique** (l'embarqué est **émulé**)

> **Comment lire les documents du projet.**
> - Cet énoncé dit **pourquoi** et **quoi** : le besoin, l'architecture, les
>   règles, l'évaluation.
> - [`agenda-phases-embedgate.md`](agenda-phases-embedgate.md) dit **quoi
>   livrer, et quand** : six phases, leurs livrables et leurs critères.
> - [`squelette/INTERFACES.md`](squelette/INTERFACES.md) est le **contrat
>   technique** (registres de la puce, messages MQTT, journal, programmes).
>   **Il fait foi.**
> - Le site compagnon montre tout cela en schémas :
>   **https://ajc.opusidea.fr/embedgate/**

---

# Partie A — Le besoin (cahier des charges)

Cette partie est écrite **sans jargon** : c'est ce qu'un client vous
dirait. Gardez-la en tête pendant dix jours, et resservez-vous-en à la
soutenance.

## A.1 À quoi sert l'application ? (le service rendu)

Imaginez un **entrepôt frigorifique** qui conserve des denrées alimentaires.
Si une chambre froide se réchauffe, si une porte reste ouverte, ou si un
moteur de compresseur force anormalement, la marchandise peut être perdue en
quelques heures, pour des milliers d'euros.

Personne ne peut surveiller tous les capteurs en permanence, à la main, jour
et nuit. **EmbedGate est le surveillant automatique** qui le fait à leur
place. L'application rend **trois services** :

1. **Elle surveille en continu** les capteurs des équipements (température,
   pression, courant électrique, ouverture de porte).
2. **Elle alerte immédiatement** dès qu'une valeur sort de la plage normale,
   sans attendre qu'un humain s'en aperçoive.
3. **Elle présente un tableau de bord clair** à l'opérateur, qui voit l'état
   de tout le site d'un coup d'œil et peut agir à distance (recalibrer un
   capteur, réinitialiser un équipement).

En une phrase : **EmbedGate transforme des capteurs éparpillés en une
surveillance centralisée, automatique et réactive.**

> Le scénario « entrepôt frigorifique » est un exemple : le même logiciel
> sert aussi bien pour une salle de serveurs, une serre agricole, une station
> de pompage ou une ligne de production. Les capteurs changent, le service
> reste.

## A.2 Comment ça marche ? (vue simple)

```
   Capteurs                  EmbedGate                    Opérateur
 (sur le terrain)         (le « surveillant »)         (dans le bureau)

  [Température]  ─┐
  [Porte]        ─┤   ┌────────────────────┐        ┌───────────────────┐
  [Moteur]       ─┼──▶│  Écoute les mesures │───────▶│  Tableau de bord   │
  [Pression]     ─┘   │  Compare aux seuils │        │  (état en direct)  │
                      │  Décide s'il y a    │        └───────────────────┘
                      │  un problème        │                 │
                      └─────────┬──────────┘                  │ règle / relance
                                │ anomalie ?                  │ à distance
                                ▼                             │
                      ┌────────────────────┐                 │
                      │  ALERTE immédiate   │◀────────────────┘
                      │ (voyant / message)  │
                      └────────────────────┘
```

Trois étapes, comme une chaîne :

- **Les capteurs** mesurent en permanence (par exemple la température de la
  chambre 1).
- **La passerelle EmbedGate** reçoit ces mesures, les compare à des limites
  définies à l'avance (« la température ne doit pas dépasser 8 °C »), et
  **décide** s'il faut donner l'alerte.
- **L'opérateur** voit tout sur un écran et peut **envoyer des consignes** en
  retour (par exemple recalibrer un capteur qui dérive).

## A.3 Ce que l'application doit faire (les exigences)

### Exigences principales (indispensables)

| # | L'application doit… | Pourquoi c'est important |
|---|---|---|
| E1 | Collecter en continu les mesures de tous les capteurs | C'est la base de la surveillance |
| E2 | Comparer chaque mesure à une plage normale (seuil bas / seuil haut) | Pour savoir si tout va bien |
| E3 | Déclencher une **alerte** dès qu'une valeur est anormale | Réagir avant les dégâts |
| E4 | Ne pas « spammer » : une seule alerte par anomalie, et signaler le retour à la normale | Éviter la fatigue d'alerte |
| E5 | Afficher un **tableau de bord** lisible et à jour | L'opérateur doit tout voir d'un coup d'œil |
| E6 | Permettre d'agir à distance (recalibrer, réinitialiser un capteur) | Corriger sans se déplacer |
| E7 | **Conserver une trace** de tout ce qui se passe (journal) | Comprendre après coup, prouver |
| E8 | S'arrêter et redémarrer proprement, sans perdre le fil | Fiabilité au quotidien |

### Exigences de qualité (comment ça doit être fait)

| # | L'application doit être… | Concrètement |
|---|---|---|
| Q1 | **Fiable** | Elle tourne des heures sans planter ni fuite de mémoire |
| Q2 | **Réactive** | Une anomalie est signalée en quelques instants |
| Q3 | **Paramétrable** | On change les seuils sans réécrire le logiciel |
| Q4 | **Testée** | Son bon fonctionnement est vérifié automatiquement |
| Q5 | **Portable** | Elle fonctionne sur un petit équipement embarqué, pas seulement sur un PC |

### Ce qui n'est PAS demandé (hors périmètre)

Pas de vrai matériel (tout est simulé), pas d'interface web sophistiquée (un
tableau de bord en console suffit), pas de gestion de comptes utilisateurs,
pas d'hébergement dans le cloud. Certains de ces points sont des
**extensions** possibles (voir B.7), pas des obligations.

## A.4 Qui utilise l'application ? (les rôles)

- **L'opérateur de surveillance** : regarde le tableau de bord, reçoit les
  alertes, envoie des consignes. C'est l'utilisateur principal.
- **Le technicien de maintenance** : intervient quand une alerte signale une
  panne réelle.
- **Le responsable de site** : consulte le journal pour comprendre un
  incident passé.

## A.5 Un exemple concret, de bout en bout

1. Il est 3 h du matin. La chambre froide n° 1 est à 4 °C, tout va bien : le
   tableau de bord affiche un état **vert**.
2. Un joint de porte cède : la température monte doucement… 6 °C… 8,5 °C.
3. À 8,5 °C, EmbedGate **détecte le dépassement du seuil** (8 °C) et
   déclenche une **alerte immédiate** : la ligne passe au **rouge** sur le
   tableau de bord, l'événement est **inscrit au journal**.
4. L'opérateur, prévenu, envoie une consigne ou dépêche un technicien.
5. Une fois la porte réparée, la température redescend ; EmbedGate signale
   le **retour à la normale** et repasse au vert.

Sans EmbedGate, personne n'aurait rien vu avant l'ouverture, le matin : trop
tard. **C'est exactement la valeur du service.** Cet incident est fourni
sous forme de scénario rejouable (`scenarios/chambre-froide.scn`) : vous le
ferez tourner des dizaines de fois.

## A.6 Comment saura-t-on que c'est réussi ? (critères d'acceptation)

Le projet est réussi si, à la soutenance :

- on voit les mesures **s'afficher et se mettre à jour** en direct (E1, E5) ;
- on peut **provoquer une anomalie** et voir l'**alerte se déclencher** tout
  de suite (E2, E3) ;
- l'alerte **ne se répète pas** en boucle et le **retour à la normale** est
  signalé (E4) ;
- on peut **envoyer une consigne à distance** et en observer l'effet (E6) ;
- il existe un **journal** consultable des événements (E7) ;
- l'application **démarre et s'arrête proprement** (E8) ;
- les **tests automatiques passent** au vert (Q4) ;
- l'application **fonctionne aussi sur la cible embarquée émulée** (Q5).

---

# Partie B — Le projet technique

## B.1 Objectif

Concevoir, développer, tester et documenter une **passerelle de supervision
pour objets connectés** (*IoT gateway*), entièrement **simulée sur la VM
Debian**. Le projet mobilise l'ensemble de la formation : Linux et shell,
**C bas niveau** (registres, MMIO), **temps réel** (interruptions, tâches,
file inter-tâches), **C++ moderne**, **Python** et MQTT, **tests** et TDD,
**débogage** (Valgrind, gdb, sanitizers), **Git**, **compilation croisée**
et **Linux embarqué** (QEMU, Buildroot).

L'embarqué est reproduit sans matériel grâce à trois mécanismes :

- une **puce capteur simulée**, **fournie** (`outils/sim_capteurs.c`) : elle
  joue le rôle du matériel et rejoue des scénarios (température, pression,
  courant, porte ; bruit, rampes, pannes) ;
- une **banque de registres** projetée en mémoire par `mmap()`, que votre
  pilote manipule comme du **MMIO** réel, à partir d'une « datasheet » ;
- une **cible ARM émulée** par QEMU, pour la compilation croisée et le
  déploiement.

## B.2 Architecture

```
                        VM DEBIAN (hôte de développement)
 ┌───────────────────────────────────────────────────────────────────────┐
 │   [C bas niveau]                [C++ moderne]              [Python]     │
 │  ┌───────────────┐  rappels    ┌────────────────────┐   MQTT ┌────────┐ │
 │  │ Pilote de la   │──────────▶ │  Passerelle         │─────▶ │ Super-  │ │
 │  │ puce (MMIO,    │            │  embedgate-gw       │ pub/  │ viseur  │ │
 │  │ interruptions) │            │  (threads, file,    │ sub   │ + com-  │ │
 │  └───────────────┘            │  alarmes, journal)  │◀───── │ mandes  │ │
 │        ▲ registres             └────────────────────┘        └────────┘ │
 │  ┌───────────────┐                      │                        ▲      │
 │  │ Puce capteur   │            ┌────────────────────┐            │      │
 │  │ simulée        │            │ Broker MQTT         │────────────┘      │
 │  │ (FOURNIE)      │            │ (Mosquitto)         │                   │
 │  └───────────────┘            └────────────────────┘                   │
 └───────────────────────────────────────────────────────────────────────┘
                                    │  compilation croisée
                                    ▼
                     ┌──────────────────────────────┐
                     │   CIBLE ARM émulée (QEMU)      │
                     │   image Buildroot + pilote et  │
                     │   passerelle déployés          │
                     └──────────────────────────────┘
```

### Ce que vous réalisez

| # | Composant | Langage | En bref |
|---|---|---|---|
| 1 | **Pilote de la puce** (`pilote/`) | C | lit et écrit les registres, sert les **interruptions simulées** (`timerfd`), expose une API C propre ; `pilote_demo` |
| 2 | **Passerelle** `embedgate-gw` (`passerelle/`) | C++17 | possède le pilote (RAII), collecte dans un thread, traite dans un autre (file thread-safe), **moteur d'alarmes** à hystérésis (*Strategy*, *Observer*), journal, configuration, arrêt propre, publication MQTT, commandes |
| 3 | **Superviseur** (`superviseur/`) | Python | s'abonne au broker, affiche le tableau de bord, envoie les commandes (calibrage, reset, auto-test) |
| 4 | **Tests** (`tests/`, `superviseur/`) | C, C++, Python | tests unitaires de chaque composant, écrits **avec** le code (et avant, pour le moteur d'alarmes) |
| 5 | **Déploiement embarqué** | shell, Buildroot | compilation croisée ARM, exécution sur la cible QEMU, démarrage automatique, débogage croisé |
| 6 | **Démonstration** `scripts/demo.sh` | shell | rejoue l'incident de 3 h du matin de bout en bout |

### Ce qui est fourni (dossier `squelette/`)

La puce simulée, `registres.h` et le bus, les scénarios, la configuration de
référence, le système de build (CMake, PC et ARM), les outils de tests
(`verif.h`, `Verif.hpp`) et le contrat `INTERFACES.md`. Le détail est dans
`squelette/README.md`. **Le contrat ne se négocie pas** : registres, topics
MQTT, format du journal, noms et options des programmes sont imposés. À
l'intérieur de votre code, vous êtes libres.

## B.3 Compétences validées (lien avec le programme)

| Bloc / module du programme | Où c'est validé dans le projet |
|---|---|
| Commandes Linux, shell script | `scripts/demo.sh`, lancement, exploitation du journal (`grep`, `awk`) |
| Administration et architecture Linux | démon, signaux, codes de sortie, démarrage automatique sur la cible |
| Git | dépôt versionné, branches, relectures, tags de fin de phase |
| Programmation C et C bas niveau | pilote, MMIO, champs de bits, *callbacks*, `volatile` |
| Temps réel | interruptions simulées, tâches, file inter-tâches, cadence tenue |
| Tests unitaires (TDD) | tests C, C++ et Python ; tests du moteur d'alarmes écrits avant lui |
| C++ / C++ moderne | RAII, threads, pointeurs intelligents, lambdas, *design patterns* |
| Python | superviseur, MQTT, `pytest` |
| Débogage | Valgrind, gdb, AddressSanitizer, ThreadSanitizer |
| Linux embarqué / Buildroot / ARM | compilation croisée, image cible, QEMU, débogage croisé |
| Savoir-être et soutenance | journal de bord, présentation, démonstration devant un jury |

## B.4 Organisation : six phases

Le détail (ce qu'il faut faire, le livrable et les critères de chaque
phase) est dans [`agenda-phases-embedgate.md`](agenda-phases-embedgate.md).
En résumé :

| Phase | Jours | Dates | Ce qu'on peut lancer à la fin | Tag |
|---|---|---|---|---|
| **P1** Le pilote C | J1 – J2 | lun. 12 – mar. 13 | `pilote_demo` affiche les interruptions de la puce, sur PC et sur ARM émulé | `v0.1` |
| **P2** La chaîne complète, en fin | J3 | mer. 14 | une mesure va de la puce à l'écran du superviseur, via MQTT | `v0.2` |
| **P3** Le cœur de la passerelle | J4 – J5 | jeu. 15 – ven. 16 | « chambre froide » produit exactement les bonnes alarmes au journal | `v0.3` |
| **P4** Superviser et commander | J6 – J7 | lun. 19 – mar. 20 | `scripts/demo.sh` rejoue l'incident, commandes comprises | `v0.4` |
| **P5** Sur la cible embarquée | J8 – J9 | mer. 21 – jeu. 22 | la passerelle tourne sur la carte ARM émulée | `v0.5` |
| **P6** Finaliser | J10 | ven. 23 | socle sur `main`, extensions, présentation, soutenance répétée | `v1.0` |
| Soutenance | | lun. 26 | devant le jury | |

Chaque phase se termine par **quelque chose qui s'exécute**, et par une
liste de **critères vérifiables**. Une phase est terminée quand tous ses
critères sont remplis ; le formateur les vérifie automatiquement sur votre
`main`, sans lire votre code.

## B.5 Livrables

1. **Le dépôt Git du groupe**, à l'état du tag **`v1.0`** : c'est lui qui est
   évalué. Historique lisible, travail réparti (`git shortlog -sn`), tags de
   fin de phase.
2. **Le code** : pilote C, passerelle C++, superviseur Python, leurs tests,
   `scripts/demo.sh`.
3. **Un build reproductible** : `cmake` pour le PC et pour ARM, sans
   avertissement.
4. **La cible embarquée** : procédure de compilation croisée et de
   déploiement, écrite dans le README et rejouable par quelqu'un d'autre.
5. **La documentation** : `README.md` (installer, compiler, lancer,
   déployer), `JOURNAL.md` (journal de bord, une ligne par jour au moins),
   **présentation du projet**, en PDF, remise le vendredi 23 à 14 h
   (architecture, choix, difficultés, résultats chiffrés, limites, et
   l'équipe).
6. **La soutenance**, avec une **démonstration en direct** de la chaîne
   complète.

## B.6 Grille d'évaluation

| Critère | Détail |
|---|---|
| **Fonctionnalité de bout en bout** | chaîne puce → pilote → passerelle → MQTT → superviseur, commandes comprises |
| **C bas niveau et MMIO** | registres, champs de bits, acquittement, API propre |
| **Temps réel** | interruptions simulées, cadence, file inter-tâches, aucune perte sous charge |
| **C++ / C++ moderne** | RAII, threads, pointeurs intelligents, *design patterns* |
| **Python et MQTT** | superviseur, tableau de bord, commandes, robustesse |
| **Tests et TDD** | tests de chaque composant, démarche, tests verts |
| **Débogage et qualité** | Valgrind, sanitizers, gdb, aucune fuite, aucune *data race* |
| **Embarqué (Buildroot, QEMU, compilation croisée)** | binaires ARM, exécution sur la cible, démarrage automatique |
| **Git et documentation** | historique, README, journal de bord, présentation |
| **Soutenance et savoir-être** | clarté, démonstration, posture de consultant |

La **pondération** des critères vous sera communiquée ultérieurement.

Les **tags** de fin de phase sont des **repères** : un tag posé en retard
n'est jamais pénalisé. Seul compte l'état de `v1.0`.

## B.7 Extensions (seulement une fois le socle sur `main`)

Les extensions ne comptent que si le **socle est complet et fusionné sur
`main`**. Elles sont décrites dans l'agenda :

| Code | Extension |
|---|---|
| E1 | Bus CAN : un nœud capteur distant sur un bus CAN virtuel (SocketCAN), intégré comme un 5ᵉ canal |
| E2 | Tableau de bord web, mis à jour en direct |
| E3 | Historisation SQLite et export CSV |
| E4 | Alarme temporisée (la porte n'alarme qu'après N secondes) |
| E5 | Qualité : CI locale, `cppcheck`, couverture `gcov`, Doxygen |
| E6 | Paquet Debian de la passerelle, service `systemd` |

Le trinôme réalise au moins **deux extensions, au choix**.

## B.8 Règles

- **Autonomie** : le formateur circule et dépanne. **Point du matin** par
  groupe (où on en est, ce qu'on vise, ce qui bloque) ; **bilan de
  mi-parcours** le vendredi 16 (5 minutes par groupe sur `v0.3`).
- **`main` compile toujours.** On travaille sur des branches, on fusionne
  quand ça passe. Commits réguliers et atomiques.
- **Un porteur et un relecteur** par composant ; on change de rôle d'une
  phase à l'autre. À la soutenance, le jury peut interroger n'importe qui
  sur n'importe quelle partie.
- **Les tests font partie de chaque phase** : aucun composant ne reste sans
  test sans justification.
- **Journal de bord** quotidien (`JOURNAL.md`) : décisions, difficultés,
  solutions. Il est évalué.
- Tout est **reproductible sur la VM Debian**, sans matériel.
- En cas de doute sur le contrat : **posez la question, ne devinez pas.**

## B.9 Prérequis de la VM (à vérifier la première heure)

```bash
sudo apt install cmake gcc g++ gdb valgrind \
                 gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf qemu-user file \
                 libmosquitto-dev mosquitto mosquitto-clients \
                 python3-paho-mqtt python3-pytest
```

Puis les trois vérifications de `squelette/README.md` (compiler et tester
sur PC, compiler et tester pour ARM, lancer la puce). Si l'une échoue,
signalez-le tout de suite : on ne perd pas une demi-journée sur
l'installation. Les outils de la Phase 5 (QEMU système, image Buildroot)
seront précisés avant le mercredi 21.

---

> **AJC FORMATION** — 33 avenue Sainte-Foy, 92200 Neuilly-sur-Seine · www.ajc-formation.fr
> Projet final POEI LLS
