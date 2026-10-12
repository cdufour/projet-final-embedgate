# EmbedGate — Phase 1 pas à pas : le pilote C

**Lundi 12 et mardi 13 octobre 2026 · tag `v0.1`**

> Ce document détaille la Phase 1 de l'[agenda](agenda-phases-embedgate.md) :
> il ne demande rien de plus, il découpe le travail en étapes, avec pour
> chacune **quoi faire**, **comment vérifier** et **les pièges**. Le contrat
> ([`squelette/INTERFACES.md`](squelette/INTERFACES.md)) fait foi : en cas de
> contradiction, c'est lui qui a raison, et vous le signalez.

---

## Où vous allez

Vous recevez une **puce capteur** (le simulateur `sim_capteurs`, fourni) et
sa **datasheet** (`INTERFACES.md` §1). Comme pour une vraie carte, vous
écrivez le **pilote** qui la fait fonctionner : la trouver, la démarrer, lire
ses mesures, servir ses interruptions, voir ses pannes.

À la fin du mardi, vous devez pouvoir montrer ceci :

```bash
build/sim_capteurs --scenario scenarios/stable.scn &
build/pilote_demo --duree 5
# [irq] ch=0 brut=4076
# [irq] ch=1 brut=1013210
# [irq] ch=2 brut=498550
# [irq] ch=3 brut=0
# …  environ 200 lignes : 4 canaux × 10 mesures par seconde × 5 secondes
```

et la même chose avec le binaire **ARM**, sous `qemu-arm`.

### Ce que vous écrivez, ce qui est fourni

| Vous écrivez | Fourni (ne pas modifier) |
|---|---|
| `pilote/` : votre pilote (par exemple `pilote.h` et `pilote.c`) | `outils/sim_capteurs.c` : la puce |
| `pilote/pilote_demo.c` : le programme de démonstration | `include/embedgate/registres.h` : la carte des registres en C |
| `tests/test_*.c` : les tests de votre pilote | `include/embedgate/bus.h`, `commun/bus.c` : l'accès au bus |
| | `tests/verif.h` : l'outil de tests ; `tests/test_registres.c` : un exemple |
| | `scenarios/*.scn`, `CMakeLists.txt`, `cmake/` |

**L'API de votre pilote est libre** (noms de fonctions, structures). Ce qui
est imposé : les registres, et ce qu'affiche `pilote_demo`.

CMake compile tout seul ce que vous ajoutez : tout fichier `pilote/*.c`
(sauf `pilote_demo.c`) entre dans la bibliothèque `pilote`, et chaque
`tests/test_xxx.c` devient le programme `build/test_xxx`, lancé par `ctest`.
Après avoir **créé** un fichier, relancez `cmake -S . -B build`.

### Rythme conseillé

| Moment | Étapes | Vous êtes dans les temps si… |
|---|---|---|
| Lundi matin | 0, 1, 2, 3 | la puce affiche « ENABLE : début du scénario » quand vous lancez votre programme |
| Lundi après-midi | 4, 5 | des lignes `[irq]` défilent, au bon rythme (environ 40 par seconde) |
| Mardi matin | 6, 7, 9 | `Ctrl-C` arrête proprement, la panne est détectée, vos tests sont verts |
| Mardi après-midi | 8, 10, 11 | toute la checklist finale passe, tag `v0.1` posé |

Répartition possible dans le binôme : l'un porte le pilote, l'autre écrit
les tests **en même temps** (étape 9) et `pilote_demo`. Chacun relit le
travail de l'autre avant la fusion sur `main`.

Si vous n'avez **aucune ligne `[irq]` lundi soir**, dites-le au point du
mardi matin : c'est presque toujours l'étape 3 ou l'étape 5.

---

## Étape 0 — Installer, créer le dépôt, compiler deux fois (première heure)

**À faire.**

1. Créez le dépôt Git de votre groupe, copiez-y le contenu de `squelette/`,
   faites un premier commit sur `main`. Donnez l'accès au formateur.
2. Installez les paquets (commande dans `squelette/README.md`).
3. Compilez et testez **pour le PC**, puis **pour ARM** :

```bash
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure        # 2 tests fournis : verts

cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake
cmake --build build-arm -j
ctest --test-dir build-arm                        # les mêmes tests, sous qemu-arm
file build-arm/sim_capteurs                       # ELF 32-bit LSB executable, ARM
```

4. Lancez la puce une fois, pour la voir : `build/sim_capteurs --scenario
   scenarios/stable.scn`. Elle annonce qu'elle est **en veille** : elle
   attend votre pilote. `Ctrl-C` pour l'arrêter.

**Pourquoi tout de suite.** Un paquet manquant (compilateur ARM,
`qemu-user`) se découvre en cinq minutes aujourd'hui, ou en deux heures le
jour où vous en aurez besoin. Si une de ces commandes échoue, signalez-le
immédiatement.

**Organisation Git.** `main` doit toujours compiler. Travaillez sur des
branches (une par personne ou par étape), fusionnez quand ça compile et que
les tests passent, après relecture par l'autre membre.

---

## Étape 1 — Lire la datasheet (30 minutes, sans clavier)

**À faire.** Lisez `INTERFACES.md` **§1** (la puce : registres, canaux,
comportement) et **§2** (les scénarios), ainsi que `registres.h`. Sur le
site compagnon, les sections « La puce » et « Une interruption, pas à pas » montrent la
même chose en schémas : https://ajc.opusidea.fr/embedgate/

**Vérifiez que vous savez répondre**, à voix haute, à votre binôme :

- À quoi sert le registre `ID` ? Que vaut-il quand la puce est arrêtée ?
- Que faut-il écrire, et dans quel ordre, pour que la puce se mette à
  mesurer **et** à prévenir ?
- Comment la puce signale-t-elle une mesure neuve sur le canal 2 ? Comment
  lui dit-on qu'on l'a lue ?
- Que vaut `DATA[0]` quand il fait 4,12 °C ? Et −2,5 °C ?
- Que devient un canal en panne ?

Si une réponse vous manque, vous n'êtes pas prêts à coder : relisez, ou
demandez.

---

## Étape 2 — Trouver la puce (le « probe »)

**À faire.** Une fonction d'ouverture qui :

- ouvre le bus avec `eg_bus_ouvrir(chemin, 0)` (`bus.h`) ; `NULL` veut dire
  que la puce n'est pas lancée ;
- lit `ID` et le compare à `EG_ID_PUCE` ;
- renvoie un **code d'erreur** distinct pour chaque cas (bus introuvable,
  mauvaise signature). Et une fonction de fermeture, qui libère le bus.

Rangez l'état du pilote (au moins l'adresse de base du bus) dans une
structure que vous passez à vos fonctions.

**Deux conseils de conception** qui vous feront gagner du temps ensuite :

- **Aucun `printf` ni `exit` dans le pilote.** Il renvoie des codes ; c'est
  `pilote_demo` qui décide du message et du code de sortie. La passerelle
  C++ de la Phase 2 réutilisera ce pilote tel quel.
- **Prévoyez une seconde façon d'« ouvrir »** : sur une banque de registres
  déjà en mémoire (un simple pointeur), sans passer par le bus. C'est ce
  qui rendra vos tests possibles sans la puce (étape 9).

**Vérifier.** Sans lancer la puce :

```bash
rm -f /tmp/embedgate_bus.bin
build/pilote_demo; echo "code=$?"
# un message compréhensible sur la sortie d'erreur, et un code ≠ 0
```

**Piège.** `Segmentation fault` (code 139) : vous utilisez le pointeur
rendu par `eg_bus_ouvrir` sans avoir vérifié qu'il n'est pas `NULL`.

---

## Étape 3 — Mettre en marche et arrêter : manipuler des bits

**À faire.** Deux fonctions :

- **démarrer** : autoriser les interruptions des canaux voulus
  (`IRQ_ENABLE`, un bit par canal), **puis** mettre le bit `ENABLE` de
  `CTRL` à 1 ;
- **arrêter** : remettre `ENABLE` à 0, **sans toucher aux autres bits** de
  `CTRL`.

Utilisez les macros fournies `EG_LIRE32` et `EG_ECRIRE32`, et les
constantes de `registres.h` (`EG_REG_CTRL`, `EG_CTRL_ENABLE`…). Jamais de
nombre « magique » comme `0x08` dans votre code.

**L'idée.** Un registre est une rangée de 32 interrupteurs. Pour en
changer un seul : **lire** le registre, **modifier** le bit avec un masque
(`|` pour mettre à 1, `& ~` pour mettre à 0), **réécrire**. L'**ordre**
compte : si vous mettez en marche avant d'autoriser les interruptions, les
premières mesures arrivent sans prévenir.

**Vérifier.** La puce dans un terminal, votre programme dans un autre :
la puce affiche « ENABLE : début du scénario » au moment où vous démarrez.

**Pièges.**

- Rien ne se passe côté puce : `ENABLE` n'est pas écrit (ou pas dans `CTRL`).
- La puce mesure mais ne prévient jamais : `IRQ_ENABLE` oublié.
- Pourquoi `volatile` ? La valeur d'un registre change « dans le dos » du
  programme. Sans `volatile`, le compilateur a le droit de ne lire qu'une
  fois et de réutiliser l'ancienne valeur. Les macros fournies s'en
  occupent : ne les contournez pas avec un pointeur ordinaire.

---

## Étape 4 — Lire une mesure à la demande

**À faire.** Une fonction qui lit `DATA[n]` et rend la valeur brute du
canal *n*, et refuse un numéro de canal invalide (≥ `EG_NB_CANAUX`).
Profitez-en pour écrire les petites fonctions de lecture d'état : canaux en
panne (`FAULT_MAP`), compteur d'acquisitions (bits 8 à 15 de `STATUS` :
masquer, puis décaler).

**Piège : le signe.** Un registre contient 32 bits non signés ; une
température peut être négative. La mesure doit être rendue dans un
**`int32_t`**. Si −2,5 °C vous donne 4 294 964 796, vous avez gardé un
type non signé.

**Vérifier.** Affichez provisoirement les 4 canaux : environ `4000`
(4 °C), `1013000` (1013 mbar), `500000` (500 mA), `0` (porte fermée).

---

## Étape 5 — Le service d'interruption : le cœur de la phase

**À faire.** Une fonction qui fait **un passage** du service
d'interruption :

1. lire `IRQ_STATUS` **une fois** : c'est la liste des canaux qui ont une
   mesure neuve ;
2. pour chacun de ces canaux, lire `DATA[n]` et appeler un ***callback***
   fourni par l'appelant, avec le canal et la valeur ;
3. **acquitter** ces canaux, et eux seuls, en écrivant leurs bits à **1**
   dans **`IRQ_ACK`** ;
4. renvoyer le nombre de mesures servies.

**Le callback.** Le pilote ne sait pas ce qu'on fera des mesures (les
afficher aujourd'hui, les publier demain). L'appelant lui donne donc un
pointeur de fonction, et un pointeur « contexte » (`void *`) que le pilote
lui repasse tel quel. C'est le mécanisme du module « pointeurs et
callbacks ». Le contexte servira dès la Phase 2 : prévoyez-le maintenant.

**L'image.** Une rangée de voyants « courrier », un par boîte aux lettres.
Le facteur (la puce) allume le voyant quand il dépose une lettre. Vous
passez, vous distribuez le courrier des boîtes allumées, puis vous appuyez
sur le bouton « distribué » de **ces boîtes-là**. Le voyant ne s'éteint que
par ce bouton.

**Piège n° 1 de la phase : le W1C.** On acquitte en écrivant **1** dans
**`IRQ_ACK`** (*write 1 to clear*). On n'écrit **jamais** dans
`IRQ_STATUS` : il est en lecture seule, la puce écraserait votre écriture.
Et on n'acquitte **que** les bits qu'on vient de servir : un canal qui
s'allume pendant que vous travaillez sera servi au passage suivant.

**Séparez ce passage de la boucle** de l'étape 6 : une fonction qui ne
fait qu'un passage se teste sans horloge, en quelques lignes (étape 9).

**Vérifier.** Avec une boucle provisoire (un passage toutes les 10 ms, peu
importe comment pour l'instant) et `stable.scn` :

```bash
build/pilote_demo --duree 5 | grep -c '^\[irq\]'
```

| Vous voyez | Ce que ça veut dire |
|---|---|
| environ 200 lignes, 50 par canal | c'est juste |
| beaucoup plus (≈ 2 000) | vous n'acquittez pas, ou pas dans le bon registre : la même mesure est resservie à chaque passage |
| nettement moins | vos passages sont plus lents que la puce (100 ms) : des mesures sont écrasées avant d'être lues |
| 0 | `ENABLE` ou `IRQ_ENABLE` manquant (étape 3) |

---

## Étape 6 — Le métronome (`timerfd`) et l'arrêt propre

**À faire.** Une boucle qui appelle le service d'interruption à intervalle
régulier, jusqu'à ce qu'on demande l'arrêt.

- **La cadence** : un `timerfd` (`timerfd_create`, `timerfd_settime`). Un
  `read()` sur ce descripteur **dort** jusqu'au prochain tic. Choisissez une
  période nettement plus courte que celle de la puce (elle mesure toutes les
  100 ms dans `stable.scn`).
- **L'arrêt** : `Ctrl-C` (`SIGINT`) et `SIGTERM` lèvent un **drapeau**
  (`volatile sig_atomic_t`), que la boucle consulte à chaque tour. Le
  gestionnaire de signal ne fait **rien d'autre**. Installez-le avec
  `sigaction`.
- **La puce qui disparaît** : quand elle s'arrête, elle remet `ID` à 0.
  Votre boucle doit le voir et s'arrêter proprement.
- **`--duree s`** : `pilote_demo` s'arrête seul au bout de *s* secondes.

**Pourquoi pas `sleep()` ?** Une boucle « travailler puis dormir 10 ms »
dérive : elle ne compte pas le temps passé à travailler. Le `timerfd`
tient la période, c'est le noyau qui compte. C'est le même schéma qu'en
FreeRTOS : une tâche périodique, un traitement court, un acquittement.

**Pièges.**

- **`EINTR`** : si le signal arrive pendant que `read()` dort, `read()`
  échoue avec `errno == EINTR`. Ce n'est **pas** une erreur : retournez
  voir le drapeau.
- Un `printf` ou un `exit` dans le gestionnaire de signal : interdit (ces
  fonctions ne sont pas sûres à cet endroit).
- Le `timerfd` est un descripteur : fermez-le en sortant.

**Vérifier.**

```bash
build/pilote_demo --duree 5; echo "code=$?"     # s'arrête seul après 5 s, code 0
build/pilote_demo                               # puis Ctrl-C : arrêt immédiat, code 0
```

À l'arrêt, laissez la puce en veille (`ENABLE` à 0).

---

## Étape 7 — Pannes, calibrage, reset, auto-test

**À faire.**

- **Pannes.** À chaque passage, comparez `FAULT_MAP` à ce que vous aviez
  vu la fois précédente, et ne signalez que les **changements**, par un
  second callback : « le canal 1 tombe en panne », « le canal 1 revient ».
- **Calibrage.** Écrire un décalage (signé, en unité brute) dans
  `CALIB[n]`. Il servira en Phase 4.
- **Reset et auto-test.** Mettre le bit `RESET` (ou `SELFTEST`) de `CTRL` à
  1, puis **attendre que la puce le remette à 0** : c'est sa façon de dire
  « fait ». Avec un **délai maximal** : si elle ne répond pas, on rend un
  code d'erreur, on ne reste pas bloqué. Après l'auto-test, le résultat est
  dans `STATUS.FAULT`.

**L'idée à retenir.** Signaler un **changement d'état**, pas l'état à
chaque passage. Vous retrouverez la même idée pour les alarmes en Phase 3.

**Vérifier.**

```bash
build/sim_capteurs --scenario scenarios/panne-capteur.scn --vite 4 --silencieux &
build/pilote_demo --duree 10 | grep -v '^\[irq\]'
# [panne] ch=1
# [retabli] ch=1          une seule fois chacun
```

Pendant la panne, le canal 1 ne doit plus produire de ligne `[irq]`.

---

## Étape 8 — `pilote_demo`, la preuve visible

**À faire.** Le programme imposé par `INTERFACES.md` §6 :

```text
build/pilote_demo [--duree s]
```

- sur la **sortie standard**, et rien d'autre : une ligne
  `[irq] ch=<n> brut=<entier>` par interruption servie ; `[panne] ch=<n>` et
  `[retabli] ch=<n>`, une seule fois chacune ;
- sur la **sortie d'erreur** : tous les autres messages (démarrage, bilan,
  erreurs) ;
- puce absente : un message clair, un code de sortie différent de 0 ;
- arrêt normal (`--duree` écoulée, `Ctrl-C`, puce arrêtée) : code 0.

**Respectez le format à l'espace près** : ces lignes sont comptées
automatiquement. `[IRQ] canal 0 = 4076` ne sera pas reconnu.

**Piège.** Si vous redirigez la sortie vers un fichier ou un `grep`, elle
est mise en tampon : en cas d'arrêt brutal, des lignes manquent. Terminez
proprement (c'est l'étape 6), ou videz le tampon (`fflush(stdout)`).

---

## Étape 9 — Les tests, sans la puce (à écrire en même temps que le reste)

**À faire.** Un fichier `tests/test_pilote.c` (ou plusieurs), avec l'outil
fourni `tests/verif.h`. Le principe est montré dans
`tests/test_registres.c` : **le test joue la puce**. Il déclare un tableau
en mémoire, y pose des valeurs avec `EG_ECRIRE32`, attache votre pilote à
ce tableau (la seconde façon d'ouvrir, étape 2), appelle une fonction du
pilote, puis vérifie ce qu'elle a lu, rappelé et écrit.

Exemple de scénario de test, en français : « `ID` est bon, `DATA[2]` vaut
498000, `IRQ_STATUS` vaut `0b0100`. J'appelle un passage du service
d'interruption. Mon callback doit avoir été appelé une fois, avec le canal 2
et 498000 ; `IRQ_ACK` doit valoir `0b0100` ; `IRQ_STATUS` ne doit pas avoir
été modifié. »

**Les tests attendus au minimum.**

- une banque sans la bonne signature est refusée ;
- démarrer écrit `IRQ_ENABLE` et `ENABLE` ; arrêter n'efface **que**
  `ENABLE` ;
- une mesure négative reste négative ;
- le service d'interruption sert **les bons canaux**, avec les bonnes
  valeurs, et **rien** quand rien n'est en attente ;
- l'acquittement va dans **`IRQ_ACK`**, sur les **bons bits** ;
- une panne est signalée une fois à l'arrivée, une fois au retour ;
- un reset auquel la « puce » ne répond pas rend une erreur au bout du
  délai (dans un test, personne ne remet le bit à 0 : c'est justement le
  cas à tester).

**Pourquoi sans la puce.** En entreprise, on écrit et on teste le pilote
avant d'avoir la carte. Et certains cas ne se provoquent pas à volonté sur
la vraie puce : deux canaux en attente au même instant, une puce muette.

**Vérifier.**

```bash
ctest --test-dir build --output-on-failure     # tout est vert
./build/test_pilote                            # le détail, test par test
```

---

## Étape 10 — Le même code sur ARM

**À faire.** Recompilez pour ARM et lancez votre démonstration ARM, la puce
(version PC) tournant à côté :

```bash
cmake --build build-arm -j
ctest --test-dir build-arm                                   # vos tests, en ARM
file build-arm/pilote_demo                                   # … ARM …
build/sim_capteurs --scenario scenarios/stable.scn --silencieux &
qemu-arm build-arm/pilote_demo --duree 3 | grep -c '^\[irq\]'   # ≈ 120
```

**L'idée.** Le même code source, sans une ligne changée, donne un
programme pour un autre processeur. S'il ne compile pas pour ARM, il
dépend d'un détail du PC (taille d'un `long`, par exemple) : utilisez les
types de `<stdint.h>` (`uint32_t`, `int32_t`).

Ne gardez pas cette étape pour la fin : recompilez pour ARM **chaque
jour**.

---

## Étape 11 — Fusionner, poser `v0.1`

**À faire.**

```bash
git switch main && git merge <votre-branche>      # main compile, tests verts
git tag v0.1 && git push origin main v0.1
```

Une ligne dans `JOURNAL.md` chaque soir : ce qui est fait, ce qui a coincé,
comment vous vous en êtes sortis. `git shortlog -sn` doit montrer le travail
des deux membres.

Le tag est un **repère** : posé mercredi matin, il n'est pas pénalisé.
Mais ne commencez pas la Phase 2 sur un pilote qui ne passe pas la
checklist ci-dessous.

---

## Checklist de fin de phase

À dérouler vous-mêmes avant de poser le tag. Ce sont les critères de
l'agenda.

```bash
# 1. Compilation (aucun avertissement : -Werror) et tests, PC puis ARM
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake
cmake --build build-arm -j && ctest --test-dir build-arm
file build-arm/pilote_demo                              # ARM

# 2. Sans la puce : un message, pas un plantage
rm -f /tmp/embedgate_bus.bin
build/pilote_demo --duree 1; echo "code=$?"             # message clair, code ≠ 0

# 3. Avec la puce : la cadence, sur PC et sur ARM
build/sim_capteurs --scenario scenarios/stable.scn --silencieux &
build/pilote_demo --duree 5 | grep -c '^\[irq\]'        # ≈ 200
build/pilote_demo --duree 5 | grep -c 'ch=2 '           # ≈ 50
qemu-arm build-arm/pilote_demo --duree 3 | grep -c '^\[irq\]'   # ≈ 120
kill %1

# 4. La panne
build/sim_capteurs --scenario scenarios/panne-capteur.scn --vite 4 --silencieux &
build/pilote_demo --duree 10 | grep -v '^\[irq\]'       # [panne] ch=1 puis [retabli] ch=1
kill %1 2>/dev/null

# 5. Pas de fuite mémoire
build/sim_capteurs --scenario scenarios/stable.scn --silencieux &
valgrind --leak-check=full build/pilote_demo --duree 3 2>&1 | grep -E "in use at exit|ERROR SUMMARY"
kill %1                                                 # 0 bytes in use, 0 errors
```

- [ ] `pilote/` contient votre pilote, sans `printf` ni `exit`
- [ ] aucun avertissement à la compilation, sur PC et sur ARM
- [ ] vos tests du pilote existent et sont verts, sur PC et sur ARM
- [ ] `pilote_demo` : format exact, environ 50 lignes par canal en 5 s
- [ ] sans la puce : message clair, code ≠ 0, pas de plantage
- [ ] `Ctrl-C` : arrêt immédiat et propre, code 0
- [ ] `panne-capteur.scn` : `[panne] ch=1` puis `[retabli] ch=1`, une fois chacun
- [ ] Valgrind : aucune erreur, aucune fuite
- [ ] `main` à jour, tag `v0.1` poussé, `JOURNAL.md` tenu

---

## Si vous êtes bloqués

| Symptôme | Où regarder |
|---|---|
| `pilote_demo` dit que la puce est absente alors qu'elle tourne | la puce et le pilote utilisent-ils le même bus ? (variable `EG_BUS_PATH`, chemin par défaut `/tmp/embedgate_bus.bin`) |
| la puce reste « en veille » | étape 3 : `ENABLE` |
| aucune ligne `[irq]` | étape 3 : `IRQ_ENABLE` ; étape 5 : lisez-vous bien `IRQ_STATUS` ? |
| dix fois trop de lignes | étape 5 : l'acquittement (`IRQ_ACK`, écrire 1) |
| des valeurs énormes (4 294 96…) | étape 4 : `int32_t` |
| `Ctrl-C` ne fait rien, ou il faut appuyer deux fois | étape 6 : le drapeau, `EINTR`, `sigaction` sans `SA_RESTART` |
| le programme ne s'arrête pas à la fin d'un scénario | étape 6 : `ID` repasse à 0 quand la puce s'arrête |
| ça marche sur PC, pas sur ARM | étape 10 : types de taille fixe ; avertissements du compilateur ARM |
| des mesures incohérentes, des lignes en double | une seule puce à la fois sur la machine : `pkill sim_capteurs`, puis relancez-en une |

Pour demander de l'aide, dites ce que vous **attendiez** et ce qui s'est
**réellement** passé, avec la commande et sa sortie : c'est le plus court
chemin vers la réponse.
