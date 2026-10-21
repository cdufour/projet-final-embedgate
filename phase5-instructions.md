# EmbedGate — Phase 5 pas à pas : sur la cible embarquée

**Mercredi 21 et jeudi 22 octobre 2026 · tag `v0.5`**

> Ce document détaille la Phase 5 de l'[agenda](agenda-phases-embedgate.md) :
> il ne demande rien de plus, il découpe le travail en étapes, avec pour
> chacune **quoi faire**, **comment vérifier** et **les pièges**. Le contrat
> ([`squelette/INTERFACES.md`](squelette/INTERFACES.md)) fait foi : en cas de
> contradiction, c'est lui qui a raison, et vous le signalez.

---

## Où vous allez

Le client ne veut pas un PC dans son entrepôt, mais un **petit boîtier** :
une carte ARM, sans écran ni clavier, qui démarre seule à la mise sous
tension et que l'on supervise à distance. Votre passerelle quitte le PC
pour cette carte, ici émulée par QEMU. Le broker et le superviseur restent
sur le PC, comme dans la réalité (le bureau de l'opérateur).

```text
   PC (votre VM Debian)                            carte ARM 32 bits (qemu-system-arm)
   ────────────────────                            ───────────────────────────────────
   broker Mosquitto :1883   ◄──── 10.0.2.2:1883 ────  embedgate-gw  ◄── sim_capteurs
   superviseur.py, commande.py                        (démarrés au boot, sans vous)
   ssh -p 2222 root@localhost   ──────────────────►  serveur SSH (dropbear)
   gdb-multiarch → localhost:2345   ──────────────►  gdbserver
```

Le critère qui résume la phase : **la carte démarre, la passerelle se
lance seule, et le superviseur, sur le PC, voit les mesures de la carte
sans aucune différence.** Le scénario « chambre froide », rejoué sur la
carte, donne les mêmes alarmes qu'en Phase 3.

### Vous connaissez déjà presque tout

Le TP Buildroot du module Linux embarqué (07/10) vous a fait construire
une image, un *external tree*, un *rootfs overlay*, un script de
démarrage `S60hello` et une compilation croisée. C'est exactement ce qu'on
refait ici, pour la passerelle. Trois différences :

| | TP Buildroot (07/10) | EmbedGate (Phase 5) |
|---|---|---|
| La carte | QEMU `virt`, **AArch64** (ARM 64 bits) | QEMU `vexpress-a9`, **ARM 32 bits** (Cortex-A9) |
| Configuration de départ | `qemu_aarch64_virt_defconfig` | `qemu_arm_vexpress_defconfig` |
| Le compilateur de votre code | celui de Buildroot (`aarch64-linux-gcc`) | celui de Debian, **le même qu'en Phase 1** (`arm-linux-gnueabihf-gcc`), via `cmake/arm-linux-gnueabihf.cmake` |

Pourquoi 32 bits ? Parce que votre projet compile pour ARM 32 bits
**depuis le premier jour** : `build-arm/`, les tests sous `qemu-arm`, le
critère `ELF 32-bit ARM` de la Phase 1. On garde cette chaîne, déjà
éprouvée, et on choisit une carte qui sait l'exécuter. La démarche, elle,
ne change pas d'un bit (sans jeu de mots).

### Ce que vous écrivez, ce qui est fourni

| Vous écrivez | Fourni |
|---|---|
| `carte/` : votre *external tree* Buildroot (configuration, *overlay*, script de démarrage) | Buildroot 2026.02.3 (le même qu'au TP) |
| la compilation de `libmosquitto` pour ARM (étape 3 : les commandes sont données) | `cmake/arm-linux-gnueabihf.cmake` (Phase 1) |
| `config/gateway-carte.ini` : la configuration de la passerelle **sur la carte** | |
| `scripts/` : lancer la carte, déployer (noms libres) | |
| le README : la procédure, **rejouable par quelqu'un d'autre** | |
| `passerelle/Version.cpp` : passez à `0.5.0` | |

**Aucun changement dans le code de la passerelle n'est attendu.** Si votre
code est portable (POSIX, C et C++ standards), il tourne sur la carte tel
quel. Si quelque chose casse, c'est une information précieuse : notez-la
dans `JOURNAL.md`.

### Paquets en plus, sur la VM

```bash
sudo apt install qemu-system-arm gdb-multiarch sshpass \
                 build-essential libncurses-dev wget cpio unzip rsync bc file
```

### Rythme conseillé

| Moment | Étapes | Vous êtes dans les temps si… |
|---|---|---|
| mercredi, **première heure** | 0, 1 | la construction de l'image **tourne** (environ 1 h sur 12 cœurs, 2 à 3 h sur une VM de 4 cœurs : on la lance d'abord, on travaille pendant) |
| mercredi matin | 3, 4 | `file build-arm/embedgate-gw` dit ARM ; ctest vert sur ARM ; la passerelle ARM tourne sous `qemu-arm` sur le PC |
| mercredi après-midi | 2, 5 | **16 h** : le binaire ARM, copié sur la carte, démarre à la main et le superviseur voit ses mesures |
| jeudi matin | 6, 7 | la carte redémarre, la passerelle se lance seule ; la chambre froide donne les mêmes alarmes qu'en P3 |
| jeudi après-midi | 8, 9 | la session `gdb` est faite et capturée ; README, checklist, tag `v0.5` |

**Si l'image résiste** (construction en échec, carte qui ne démarre pas,
réseau), signalez-le **dès mercredi midi** : une image de secours est
prévue. On ne perd pas deux jours sur la configuration.

---

## Travailler à deux

| | Piste A : la carte | Piste B : le binaire |
|---|---|---|
| Étapes | 1, 2, 6 : l'image, le démarrage de la carte, le démarrage automatique | 3, 4, 8 : `libmosquitto` ARM, la passerelle ARM, le débogage croisé |
| Fichiers | `carte/`, le script qui lance la carte | `config/gateway-carte.ini`, le script de déploiement |
| Se teste seule avec | la carte démarre, `ssh` y entre, `ping 10.0.2.2` répond | la passerelle ARM tourne sous `qemu-arm`, **sur le PC**, et publie |
| Jonction, **mercredi 16 h** | le binaire ARM copié sur la carte démarre à la main (étape 5) | |
| Ensemble, jeudi | la chambre froide sur la carte (étape 7), le README, la checklist | |

**Qui prend quoi.** Celui qui a porté la piste C++ en Phase 4 prend la
piste A, et inversement : chacun aura fait, dans la semaine, une piste
C++ et une piste système. À la soutenance, chacun peut être interrogé sur
n'importe quelle partie, y compris « comment la passerelle démarre-t-elle
sur la carte ? ».

**Trinôme** : une troisième piste, le **débogage croisé** (étape 8), puis
la préparation de ses deux extensions (au choix, voir
[`extensions-instructions.md`](extensions-instructions.md)).

**Les fichiers communs** : `README.md` et `JOURNAL.md`, un seul rédacteur à
la fois (comme en Phase 4). Rien à changer dans `CMakeLists.txt`.

---

## Étape 0 — Version, et lancer ce qui est long

1. Passez la version à `0.5.0` dans `passerelle/Version.cpp`.
2. Ajoutez `build-mosquitto-arm/` à `.gitignore` s'il n'est pas déjà
   couvert par `build*/` (il l'est dans le squelette).
3. **Lancez tout de suite l'étape 1** : la construction de l'image est la
   seule chose vraiment longue de la phase.

---

## Étape 1 — L'image de la carte (piste A)

**L'image.** Buildroot est une **usine** : vous lui donnez une recette (la
*defconfig*), il fabrique tout le reste. Le compilateur pour la carte, le
noyau Linux, le système de fichiers avec ses commandes (BusyBox), et les
paquets demandés. Votre *external tree* (`carte/`) est le **cahier des
charges du client** que vous joignez à la recette standard : il ne
modifie pas Buildroot, il s'y ajoute.

**À faire.** Dans votre dépôt, un dossier `carte/` :

```text
carte/
├── external.desc                       name: EMBEDGATE
├── Config.in                           (vide, ou un commentaire)
├── external.mk                         (vide, ou un commentaire)
├── configs/embedgate_qemu_defconfig    la recette de VOTRE carte
└── overlay/                            fichiers ajoutés tels quels à l'image
    └── etc/init.d/S90embedgate         le démarrage automatique (étape 6)
```

Partez de `configs/qemu_arm_vexpress_defconfig` de Buildroot, et ajoutez :

| Option (dans `make menuconfig`) | Pourquoi |
|---|---|
| *System configuration* → nom d'hôte, bannière, **mot de passe root** | pour `ssh` |
| *System configuration* → *Root filesystem overlay* : `$(BR2_EXTERNAL_EMBEDGATE_PATH)/overlay` | votre script de démarrage |
| *Toolchain* → **Enable C++ support** | `gdbserver` est écrit en C++ |
| *Target packages* → *Networking* → **dropbear** | `ssh` vers la carte, pour déployer |
| *Target packages* → *Debugging* → **gdb**, avec **gdbserver** | le débogage croisé (étape 8) |
| *Target packages* → *Networking* → **mosquitto**, **sans** le broker | `mosquitto_sub` sur la carte, pour tester le réseau (facultatif, pratique) |
| *Filesystem images* → taille de l'ext2 : `128M` | de la place pour vos binaires |
| *Host utilities* → **décocher** *host qemu* | celui de Debian (`qemu-system-arm`) suffit, et le construire coûte cher |

```bash
mkdir -p ~/br-lab && cd ~/br-lab           # si Buildroot 2026.02.3 n'y est pas déjà (TP du 07/10)
wget https://buildroot.org/downloads/buildroot-2026.02.3.tar.gz && tar xzf buildroot-2026.02.3.tar.gz
cd buildroot-2026.02.3
make O=~/br-lab/build-carte BR2_EXTERNAL=~/<votre-dépôt>/carte qemu_arm_vexpress_defconfig
make O=~/br-lab/build-carte menuconfig     # les options du tableau
make O=~/br-lab/build-carte savedefconfig BR2_DEFCONFIG=~/<votre-dépôt>/carte/configs/embedgate_qemu_defconfig
make O=~/br-lab/build-carte                # long : lancez-le dans tmux
```

**Pièges.**

- **Jamais en root** (TP, partie 1). Ni `sudo make`.
- **Le dossier de construction hors du dépôt** (`~/br-lab/build-carte`) :
  plusieurs Go, qui n'ont rien à faire dans Git. Seul `carte/` est
  versionné.
- **La `defconfig` est la seule chose à garder** (TP, partie 8) : avec
  elle et `carte/`, n'importe qui reconstruit la même image. Vérifiez-le
  en la relisant : elle doit tenir en une quarantaine de lignes.
- **Ne gardez pas la carte 64 bits du TP** : votre `embedgate-gw` est un
  binaire 32 bits.
- Le premier `make` télécharge les sources (noyau, gcc…) : s'il échoue sur
  un téléchargement, relancez-le, il reprend où il en était.

**Vérifier.**

```bash
ls ~/br-lab/build-carte/images/            # zImage  vexpress-v2p-ca9.dtb  rootfs.ext2 …
```

---

## Étape 2 — Démarrer la carte (piste A)

**À faire.** Un script qui démarre la carte (le TP vous en a fait écrire
un, `run-qemu.sh`). La commande, pour cette carte :

```bash
qemu-system-arm -M vexpress-a9 -smp 1 -m 256 \
    -kernel ~/br-lab/build-carte/images/zImage \
    -dtb ~/br-lab/build-carte/images/vexpress-v2p-ca9.dtb \
    -drive file=~/br-lab/build-carte/images/rootfs.ext2,if=sd,format=raw \
    -append "console=ttyAMA0,115200 rootwait root=/dev/mmcblk0" \
    -net nic,model=lan9118 \
    -net user,hostfwd=tcp:127.0.0.1:2222-:22,hostfwd=tcp:127.0.0.1:2345-:2345 \
    -nographic
```

**Le réseau, en une image.** QEMU place la carte derrière une **box
internet** imaginaire. La carte a l'adresse `10.0.2.15` ; la box,
c'est-à-dire votre PC, a l'adresse **`10.0.2.2`**, vue de la carte. Depuis
le PC, on n'atteint pas la carte directement : on ouvre des **portes**
(`hostfwd`). Le port 2222 du PC mène au port 22 (SSH) de la carte, le
2345 au port 2345 (pour `gdbserver`).

**Pièges.**

- **`127.0.0.1` sur la carte, c'est la carte elle-même**, pas le PC. Le
  broker du PC, vu de la carte, est en `10.0.2.2`.
- **La console de la carte est dans le terminal** : `Ctrl-C` y arrête un
  programme de la carte, pas QEMU. Pour quitter QEMU : `Ctrl-a` puis `x`.
- **Les fichiers déposés sur la carte vivent dans `rootfs.ext2`.** Ils
  survivent à un redémarrage de la carte, mais pas à une reconstruction
  de l'image (`make`) : redéployez alors (étape 5).

**Vérifier.**

```bash
# console de la carte : login root, le mot de passe choisi
ping -c 2 10.0.2.2                          # le PC répond
mosquitto_sub -h 10.0.2.2 -t '#' -C 1 -W 5  # si mosquitto est sur la carte : le broker du PC répond
# sur le PC :
ssh -p 2222 root@localhost                  # on entre sur la carte
```

---

## Étape 3 — `libmosquitto` pour ARM (piste B)

**Le problème.** La passerelle a besoin de `libmosquitto`. Sur le PC,
`apt` l'a installée pour le PC (x86). Pour la carte, il faut une
`libmosquitto` **compilée pour ARM**, et **statique** : depuis la Phase 1,
`cmake/arm-linux-gnueabihf.cmake` produit des binaires statiques, qui
emportent tout avec eux et n'ont besoin de rien sur la carte. Or Debian
ne fournit pas de `libmosquitto` statique pour ARM. On la compile depuis
ses sources, avec **votre** chaîne, une fois pour toutes.

**À faire.** Ces commandes, à mettre dans un script (`scripts/…`) pour
que le README puisse y renvoyer :

```bash
mkdir -p build-mosquitto-arm && cd build-mosquitto-arm
wget https://mosquitto.org/files/source/mosquitto-2.0.21.tar.gz && tar xzf mosquitto-2.0.21.tar.gz
cmake -S mosquitto-2.0.21 -B compil \
      -DCMAKE_TOOLCHAIN_FILE=$PWD/../cmake/arm-linux-gnueabihf.cmake \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/install \
      -DWITH_STATIC_LIBRARIES=ON -DWITH_PIC=ON -DWITH_TLS=OFF -DWITH_CJSON=OFF \
      -DWITH_BROKER=OFF -DWITH_APPS=OFF -DWITH_CLIENTS=OFF -DWITH_PLUGINS=OFF \
      -DDOCUMENTATION=OFF -DWITH_LIB_CPP=OFF
cmake --build compil -j && cmake --install compil
cp install/lib/libmosquitto_static.a install/lib/libmosquitto.a
cd ..
```

**Pièges.**

- **La version 2.0.21**, pas la 2.0.22 : celle-ci réclame OpenSSL même
  sans TLS, et la configuration échoue (« OpenSSL::SSL … not found »).
- **`-DWITH_TLS=OFF`** : sans lui, il faudrait aussi OpenSSL pour ARM.
  Le broker du PC n'utilise pas TLS.
- **`build-mosquitto-arm/` ne se versionne pas** (`build*/` dans
  `.gitignore`) : seul le script qui le produit va dans Git.

---

## Étape 4 — La passerelle pour ARM, essayée sur le PC (piste B)

**À faire.** Recompilez `build-arm/` en indiquant à CMake où est la
`libmosquitto` ARM. La passerelle et ses tests C++, absents de
`build-arm/` jusqu'ici, apparaissent :

```bash
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake \
      -DMOSQUITTO_INCLUDE_DIR=$PWD/build-mosquitto-arm/install/include \
      -DMOSQUITTO_LIBRARY=$PWD/build-mosquitto-arm/install/lib/libmosquitto.a
cmake --build build-arm -j
ctest --test-dir build-arm                  # TOUS vos tests, C et C++, sous qemu-arm
file build-arm/embedgate-gw                 # ELF 32-bit LSB executable, ARM, … statically linked
```

**L'astuce qui fait gagner l'après-midi : essayer le binaire ARM sur le
PC, avant la carte.** `qemu-arm` (Phase 1) exécute un programme ARM sur
le PC. Si la passerelle ARM marche sous `qemu-arm`, tout problème sur la
carte viendra de la carte (réseau, chemins), pas du binaire :

```bash
qemu-arm build-arm/sim_capteurs --scenario scenarios/chambre-froide.scn --vite 4 --silencieux &
sleep 1; qemu-arm build-arm/embedgate-gw --config config/gateway.ini
grep -E 'ALARME|NORMAL' embedgate.log                          # les 4 lignes de la Phase 3
```

**Pièges.**

- **L'avertissement du lien sur `getaddrinfo`** (« Using 'getaddrinfo' in
  statically linked applications… ») : il est normal avec un binaire
  statique. Il dit qu'un **nom** de machine ne pourra pas être résolu sur
  la carte. Conséquence pratique : dans la configuration de la carte,
  donnez le broker par son **adresse IP** (`10.0.2.2`), jamais par un nom
  (`localhost`, le nom du PC).
- **CMake garde ses réponses en cache** : si vous aviez configuré
  `build-arm/` sans `libmosquitto`, la ligne ci-dessus suffit à
  l'informer. En cas de doute : `rm -rf build-arm` et on recommence.
- **Un test qui passe sur PC et échoue sous `qemu-arm`** : c'est une vraie
  information (taille d'un `long`, alignement, ordre des octets, délai trop
  serré sous émulation). Corrigez le code ou le test, pas le contrat.

---

## Étape 5 — Déployer sur la carte, et lancer à la main (jonction)

**À faire.**

1. **`config/gateway-carte.ini`** : une copie de `config/gateway.ini`, avec
   deux différences seulement. Le broker est le PC vu de la carte,
   `hote = 10.0.2.2`. Le journal a un chemin absolu, par exemple
   `/opt/embedgate/embedgate.log`. Mêmes seuils, même groupe.
2. **Un script de déploiement** qui copie dans `/opt/embedgate` de la
   carte : `embedgate-gw`, `sim_capteurs`, `pilote_demo`, la configuration
   (sous le nom `gateway.ini`) et `scenarios/`. Les binaires partent
   **allégés** de leurs symboles de débogage (`strip`) : la passerelle
   passe de 3,5 Mo à 1,3 Mo, et la copie de plus de deux minutes à dix
   secondes (le réseau de la carte émulée est lent). Les symboles restent
   dans `build-arm/`, où `gdb` les lira (étape 8). Pour tout envoyer d'un
   coup, une archive `tar` qui passe par `ssh` :

```bash
SSH="sshpass -p <mot de passe> ssh -p 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@localhost"
mkdir -p build-arm/deploiement/scenarios
for f in embedgate-gw sim_capteurs pilote_demo; do
    arm-linux-gnueabihf-strip -o build-arm/deploiement/$f build-arm/$f
done
cp config/gateway-carte.ini build-arm/deploiement/gateway.ini
cp scenarios/*.scn build-arm/deploiement/scenarios/
tar cf - -C build-arm/deploiement . | $SSH 'mkdir -p /opt/embedgate && tar xf - -C /opt/embedgate'
```

3. **Sur la carte, à la main** :

```bash
cd /opt/embedgate
./sim_capteurs --silencieux &
sleep 1; ./embedgate-gw --config gateway.ini
```

**Vérifier, sur le PC** (c'est la jonction de 16 h) :

```bash
mosquitto_sub -t 'embedgate/gp1/etat' -C 1                        # {"etat":"en_ligne"}
python3 superviseur/superviseur.py --config config/gateway.ini    # les mesures… de la carte
python3 superviseur/commande.py --config config/gateway.ini calib 0 -1.5   # ack ok : la commande traverse le réseau
```

**Pièges.**

- **`Exec format error`** sur la carte : vous avez copié le binaire du PC
  (`build/`) au lieu de celui de la carte (`build-arm/`).
- **Code de sortie 4** (« broker injoignable ») : `hote` pointe vers
  `127.0.0.1` (la carte elle-même), ou le broker du PC ne tourne pas.
- **Code 3** (« puce absente ») : la puce n'est pas lancée **sur la
  carte**. La puce et la passerelle partagent un fichier en mémoire
  (`/tmp/embedgate_bus.bin`) : elles doivent tourner sur la même machine.
- **Le superviseur ne voit rien** alors que la passerelle tourne : le
  groupe de `gateway-carte.ini` est-il le vôtre ?
- **`tar: invalid option -- 'z'`** sur la carte : le `tar` de BusyBox ne
  sait pas décompresser. Envoyez une archive non compressée (`tar cf`,
  `tar xf`).
- **Juste après un redéploiement, `…/etat` dit `hors_ligne`** : c'est le
  message de l'arrêt de l'ancienne passerelle, encore retenu. Le
  `en_ligne` de la nouvelle arrive dans les secondes qui suivent
  (`mosquitto_sub -t … -v` en continu, plutôt que `-C 1`).

---

## Étape 6 — Le démarrage automatique (piste A)

**L'image.** Une carte dans un entrepôt n'a personne pour taper
`./embedgate-gw`. À la mise sous tension, elle doit tout démarrer seule,
et elle doit le faire **même si le réseau ou le broker arrivent après
elle**.

**À faire.** `carte/overlay/etc/init.d/S90embedgate` (exécutable), sur le
modèle de votre `S60hello` du TP : au boot, BusyBox lance
`/etc/init.d/S??*` dans l'ordre, avec l'argument `start` ; à l'arrêt, avec
`stop`.

- **`start`** : lancer la puce en tâche de fond, attendre une seconde,
  lancer la passerelle en tâche de fond. Gardez leurs numéros de processus
  (`$!`) dans `/var/run/…pid`.
- **`stop`** : envoyer `SIGTERM` à la passerelle (elle publie `hors_ligne`
  et écrit `STOP` : c'est votre Phase 3 et votre Phase 4), puis à la puce.
- **Si rien n'est encore déployé**, `start` le dit et s'arrête sans
  erreur : la carte doit démarrer même vide.

**Le piège central : la course au démarrage.** Au boot, la passerelle peut
démarrer avant que le réseau soit prêt, ou alors que le broker du PC est
arrêté. Elle s'arrête alors avec le **code 4** (contrat §6). Votre script
doit **réessayer** : par exemple une boucle, en tâche de fond, qui relance
la passerelle toutes les 2 secondes tant qu'elle sort avec le code 4, et
s'arrête sur tout autre code (0 : arrêt normal ; 2 ou 3 : une vraie erreur,
inutile d'insister). Une fois connectée, c'est la reconnexion de la Phase 4
qui prend le relais.

**Pièges.**

- **Le script n'est pas exécutable** : il est ignoré, sans message.
  `chmod +x`, **avant** le `git add` (Git garde ce droit).
- **Le script écrit avec des fins de ligne Windows** (CRLF) :
  `/bin/sh^M: not found`. Écrivez-le sur la VM.
- **Lancer la passerelle au premier plan** dans `start` : le démarrage de
  la carte reste bloqué sur elle, et la console n'arrive jamais.
- **Le nouveau script ne change pas la carte qui tourne** : il est dans
  l'*overlay*, donc dans l'image. Reconstruisez l'image (`make` : quelques
  secondes cette fois, seul le système de fichiers est refait), puis
  redéployez (étape 5), puis redémarrez la carte.

**Vérifier.**

```bash
# console de la carte :
reboot                                       # ou : quitter QEMU et relancer la carte
# sur le PC, sans rien toucher sur la carte :
mosquitto_sub -t 'embedgate/gp1/etat' -v     # hors_ligne à l'arrêt, puis en_ligne ≈ 20 s après le reboot
# broker arrêté pendant le boot, puis relancé : la passerelle finit par passer en_ligne
```

---

## Étape 7 — La chambre froide, sur la carte (ensemble)

**À faire.** Sur la carte : arrêter le service, rejouer le scénario, lire
le journal.

```bash
/etc/init.d/S90embedgate stop
cd /opt/embedgate && rm -f embedgate.log
./sim_capteurs --scenario scenarios/chambre-froide.scn --vite 4 --silencieux &
sleep 1; ./embedgate-gw --config gateway.ini
grep -E 'ALARME|NORMAL' embedgate.log
```

Les **4 mêmes événements qu'en Phase 3**, dans le même ordre. Les valeurs
peuvent varier au millième près, pas les événements.

Et le tableau de bord, sur le PC, pendant ce temps, montre les alarmes
venues de la carte.

**Une question à vous poser** (elle viendra à la soutenance) : la carte
est émulée, donc **plus lente** qu'une vraie. La passerelle tient-elle la
cadence ? Les compteurs affichés à l'arrêt (mesures reçues, perdues, file
au plus…) répondent. Notez-les dans `JOURNAL.md` : c'est un résultat
chiffré pour le dossier.

---

## Étape 8 — Le débogage croisé (piste B)

**L'image.** Déboguer un programme qui tourne **sur une autre machine**,
d'une autre architecture. Sur la carte, `gdbserver` est un **petit agent
sur place** : il tient le programme, l'arrête, lit sa mémoire, mais il ne
comprend rien au code. Sur le PC, `gdb-multiarch` est **l'expert à
distance** : il a le binaire avec ses symboles (les noms des fonctions et
des variables), et pilote l'agent par le réseau.

**À faire.**

```bash
# sur la carte (la puce tourne déjà ; le service est arrêté) :
gdbserver :2345 /opt/embedgate/embedgate-gw --config /opt/embedgate/gateway.ini

# sur le PC, avec LE MÊME binaire (build-arm/, compilé en Debug, avec ses symboles) :
gdb-multiarch build-arm/embedgate-gw
(gdb) target remote localhost:2345
(gdb) break <votre fonction de service d'interruption>
(gdb) continue
(gdb) backtrace                       # qui l'a appelée, depuis main
(gdb) info threads                    # les threads de la passerelle
(gdb) print *<votre pilote>           # son état
(gdb) x/12xw <adresse de la banque>   # les registres de la puce, en mémoire
(gdb) next                            # pas à pas : lecture de IRQ_STATUS, acquittement…
```

**Le livrable** : une **capture** (texte copié, ou image) de la session,
avec le point d'arrêt atteint, la pile d'appels et la banque de registres
lue en mémoire. Elle ira dans le dossier de projet (Phase 6), avec deux
phrases d'explication.

**Pièges.**

- **`gdb` du PC (x86) au lieu de `gdb-multiarch`** : il ne comprend pas
  le binaire ARM.
- **Donner à `gdb` un autre binaire** que celui qui tourne sur la carte
  (recompilé depuis) : les adresses ne correspondent plus, les points
  d'arrêt tombent n'importe où. Redéployez avant de déboguer.
- **Le point d'arrêt arrête la passerelle**, mais pas la puce : la puce
  continue d'écrire des mesures. En reprenant (`continue`), vous verrez
  des interruptions en attente. C'est normal, et c'est même une bonne
  question pour la soutenance (qu'est-ce qui se perd pendant un arrêt ?).
- **Le broker voit la passerelle disparaître** pendant un long arrêt
  (pas de réponse à ses messages de vie) : au bout d'un moment, il publie
  le **testament** (`hors_ligne`). C'est la Phase 4 qui fonctionne.

---

## Étape 9 — Le README, la checklist, `v0.5`

**Le README** reçoit une section « Sur la carte embarquée », **rejouable
par quelqu'un d'autre** sur une VM propre (c'est un critère, et l'échange
de README avec un autre groupe, vendredi à 14 h, le vérifiera) :

1. les paquets à installer ;
2. construire l'image (les commandes, avec `BR2_EXTERNAL`) ;
3. compiler `libmosquitto` ARM et la passerelle ARM ;
4. démarrer la carte ; déployer ;
5. vérifier depuis le PC ;
6. rejouer la chambre froide ; déboguer.

Puis :

```bash
git switch main && git merge <votre-branche>      # main compile, tests verts, PC et ARM
git tag v0.5 && git push origin main v0.5
```

Une ligne dans `JOURNAL.md` chaque soir. Le tag est un **repère** : posé
vendredi matin, il n'est pas pénalisé.

---

## Checklist de fin de phase

À dérouler vous-mêmes avant de poser le tag. Ce sont les critères de
l'agenda. Remplacez `gp1` par votre groupe.

```bash
# 1. PC : rien n'a régressé
cmake --build build -j && ctest --test-dir build && python3 -m pytest superviseur/
scripts/demo.sh                                                   # DEMO OK

# 2. Binaires ARM, tests ARM
cmake --build build-arm -j && ctest --test-dir build-arm          # tous verts, passerelle comprise
file build-arm/embedgate-gw build-arm/pilote_demo                 # ELF 32-bit … ARM

# 3. La carte démarre seule, la passerelle avec
#    (carte démarrée, binaires déployés, aucune commande tapée sur la carte)
mosquitto_sub -t 'embedgate/gp1/etat' -C 1                        # {"etat":"en_ligne"}
python3 superviseur/superviseur.py --config config/gateway.ini    # les mesures de la carte
python3 superviseur/commande.py --config config/gateway.ini selftest; echo "code=$?"   # code=0

# 4. Sur la carte : chambre froide → les 4 événements de la Phase 3
#    (étape 7)

# 5. Arrêt de la carte (poweroff) : hors_ligne côté PC
```

- [ ] `passerelle/Version.cpp` annonce `0.5.0`
- [ ] tous les tests verts, sur PC **et sous `qemu-arm`**, passerelle comprise ; `DEMO OK` sur le PC
- [ ] `file` dit `ELF 32-bit … ARM` pour `pilote_demo` et `embedgate-gw`
- [ ] l'image se reconstruit à partir de `carte/` (la `defconfig` et l'*overlay* sont dans Git)
- [ ] la carte démarre, la passerelle se lance **seule**, même si le broker arrive après
- [ ] le superviseur, sur le PC, montre les mesures de la carte ; une commande reçoit son `ack`
- [ ] `chambre-froide.scn` sur la carte : les mêmes alarmes qu'en Phase 3
- [ ] une session `gdbserver` / `gdb-multiarch` capturée (point d'arrêt dans le service d'interruption)
- [ ] le README décrit le déploiement, rejouable par un autre groupe
- [ ] `main` à jour, tag `v0.5` poussé, `JOURNAL.md` tenu

---

## Si vous êtes bloqués

| Symptôme | Où regarder |
|---|---|
| `make` de Buildroot échoue au téléchargement | relancez `make` : il reprend où il en était |
| la construction de l'image dure des heures | normal sur une petite VM (2 à 3 h sur 4 cœurs) ; si elle n'est pas finie mercredi midi, demandez l'image de secours |
| le déploiement dure plusieurs minutes | étape 5 : binaires allégés avec `arm-linux-gnueabihf-strip` ? |
| `make` refuse de tourner (« you should not run Buildroot as root ») | étape 1 : jamais en root |
| CMake ARM : « OpenSSL::SSL … not found » en compilant mosquitto | étape 3 : version 2.0.21, et `-DWITH_TLS=OFF` |
| `build-arm/` ne contient toujours pas `embedgate-gw` | étape 4 : `-DMOSQUITTO_INCLUDE_DIR` et `-DMOSQUITTO_LIBRARY` donnés ? message « libmosquitto introuvable » ? |
| la carte reste muette après « Starting kernel » | étape 2 : `-dtb` donné ? console `ttyAMA0` ? image construite à partir de `qemu_arm_vexpress_defconfig` (pas la carte 64 bits) ? |
| `ssh -p 2222` : « Connection refused » | étape 2 : dropbear dans l'image ? `hostfwd` dans la commande QEMU ? la carte a-t-elle fini de démarrer ? |
| `ssh` : « Host key verification failed » | la carte a changé de clé après une reconstruction : `-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null` |
| sur la carte : `Exec format error` | étape 5 : c'est le binaire du PC ; prenez celui de `build-arm/` |
| sur la carte : `not found`, alors que le fichier est là | binaire non statique, ou script avec des fins de ligne Windows |
| la passerelle sort avec le code 4 sur la carte | étape 5 : `hote = 10.0.2.2` ? broker lancé sur le PC ? |
| la passerelle sort avec le code 3 sur la carte | étape 5 : la puce doit tourner **sur la carte** |
| après `reboot`, rien ne démarre | étape 6 : script exécutable ? dans l'*overlay* ? image reconstruite ? `ls -l /etc/init.d/` sur la carte |
| démarre à la main, jamais au boot | étape 6 : la course au démarrage ; réessayer sur le code 4 |
| `gdb` : « Remote 'g' packet reply is too long » ou registres absurdes | étape 8 : `gdb-multiarch`, pas `gdb` |
| `gdb` : les points d'arrêt ne tombent pas au bon endroit | étape 8 : le binaire sur la carte n'est pas celui que lit `gdb` |
| un test passe sur PC, échoue sous `qemu-arm` | étape 4 : une vraie différence d'architecture, ou un délai trop serré sous émulation |

Pour demander de l'aide, dites ce que vous **attendiez** et ce qui s'est
**réellement** passé, avec la commande et sa sortie : c'est le plus court
chemin vers la réponse.
