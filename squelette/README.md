# EmbedGate — squelette du projet

Point de départ de votre dépôt. Avant toute chose, lisez `INTERFACES.md` :
c'est le contrat (registres, MQTT, journal, programmes) et il fait foi.
L'agenda des phases et le site compagnon (https://ajc.opusidea.fr/embedgate/)
disent quoi livrer et quand.

## Première heure (Phase 1)

```bash
# Paquets (VM Debian)
sudo apt install cmake gcc g++ gdb valgrind \
                 gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf qemu-user file \
                 libmosquitto-dev mosquitto mosquitto-clients \
                 python3-paho-mqtt python3-pytest

# 1. Compiler et tester sur le PC
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure        # 2 tests fournis : verts

# 2. Compiler et tester pour la carte ARM (tests lancés sous qemu-arm)
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake
cmake --build build-arm -j
ctest --test-dir build-arm
file build-arm/sim_capteurs                       # ELF 32-bit LSB executable, ARM

# 3. Lancer la puce : elle attend que votre pilote mette CTRL.ENABLE à 1
build/sim_capteurs --scenario scenarios/stable.scn
```

Si ces trois étapes passent, votre environnement est prêt. Sinon, signalez-le
tout de suite : on ne perd pas une demi-journée sur l'installation.

## Ce qu'il y a dedans

| Chemin | Rôle | |
|---|---|---|
| `INTERFACES.md` | le contrat | fourni |
| `outils/sim_capteurs.c` | la puce (le matériel simulé) | fourni, ne pas modifier |
| `include/embedgate/`, `commun/bus.c` | registres et bus | fourni, ne pas modifier |
| `scenarios/*.scn` | ce que vivent les capteurs | fourni (ajoutez les vôtres) |
| `config/gateway.ini` | configuration de référence | fourni (changez seulement `groupe`) |
| `tests/verif.h`, `tests/Verif.hpp` | outils de tests C et C++ | fourni |
| `tests/test_registres.c` | exemple : tester du code MMIO **sans** la puce | fourni, à garder vert |
| `cmake/arm-linux-gnueabihf.cmake` | compilation croisée ARM | fourni |
| `pilote/` | votre pilote C (P1) | **à écrire** |
| `passerelle/` | votre passerelle C++ (P2, P3) | **à écrire** |
| `superviseur/` | votre superviseur Python (P2, P4) | **à écrire** |
| `scripts/demo.sh` | la démonstration de bout en bout (P4) | **à écrire** |

CMake compile automatiquement tout nouveau fichier : `pilote/*.c` dans la
bibliothèque `pilote`, `passerelle/*.cpp` dans `passerelle_lib`, et chaque
`tests/test_xxx.c` ou `.cpp` devient un programme de test `build/test_xxx`.

## Vérifier une phase

Chaque phase de l'agenda se termine par une liste de **critères
vérifiables** (« Critères » et « Livrable vérifiable »). Vérifiez-les
vous-mêmes, commande par commande, avant de poser le tag. Le formateur
repasse ces critères automatiquement sur votre `main`, sans lire votre code.
Une phase est terminée quand tous ses critères sont remplis.

## Les variantes de compilation utiles

```bash
cmake -S . -B build-asan -DSANITIZE=address    # accès mémoire illégaux
cmake -S . -B build-tsan -DSANITIZE=thread     # data races (Phase 3)
```
