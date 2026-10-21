# EmbedGate — Le parcours en 6 phases

**Projet final POEI LLS · EXPLEO · du lundi 12 au lundi 26 octobre 2026**
Binômes (un trinôme) · 10 jours de projet + 1 jour de soutenance

> Ce document vous dit **quoi livrer, et quand**. Le *pourquoi* est dans
> l'énoncé (`projet-final-embedgate-enonce.md`) ; le *comment ça se parle* est dans
> `squelette/INTERFACES.md`, qui fait foi. Le site compagnon montre tout cela
> en schémas : **https://ajc.opusidea.fr/embedgate/**

---

## Les règles du jeu (les mêmes qu'aux projets C et C++)

1. **Six phases, six livrables qui s'exécutent.** Chaque phase se termine
   par quelque chose qu'on lance, et non par du code « presque fini ». Une
   phase est terminée quand tous ses **critères** sont remplis ; je les
   vérifie automatiquement sur votre `main`, sans lire votre code.
2. **Les tags sont des repères, pas des sanctions.** On pose `v0.1`, `v0.2`…
   à la fin de chaque phase. Un tag posé en retard n'est **jamais**
   pénalisé : seul compte l'état de `v1.0`. Les tags servent à voir où vous
   en êtes, à vous comme à moi.
3. **Socle d'abord, extensions ensuite.** Les extensions ne comptent que si
   le socle est complet et fusionné sur `main`.
4. **`main` compile toujours.** On travaille sur des branches, on fusionne
   quand ça passe.
5. **Les tests font partie de chaque phase.** Il n'y a pas de « journée des
   tests » : une phase sans ses tests n'est pas terminée.
6. **Le contrat ne se négocie pas.** Les registres, les topics, le format du
   journal et les noms de programmes sont imposés. À l'intérieur de votre
   code, vous êtes libres.

### Rythme d'une journée

| Heure | Quoi |
|---|---|
| 9h30 – 9h45 | **Point du matin**, par groupe : où on en est, ce qu'on vise aujourd'hui, ce qui bloque |
| journée | travail en autonomie, le formateur circule |
| 17h15 – 17h30 | commit, push, **une ligne dans le journal de bord** (`JOURNAL.md`) |

### Qui fait quoi

Chaque composant a un **porteur**, qui le connaît par cœur, et un
**relecteur** (l'autre membre du binôme), qui relit avant chaque fusion.
Changez de rôle d'une phase à l'autre : à la soutenance, le jury peut
interroger n'importe lequel d'entre vous sur n'importe quelle partie.
`git shortlog -sn` doit montrer un travail réparti.

Trinôme : un troisième porteur prend le superviseur Python et les scripts
de vérification, et le groupe réalise **une extension de plus** que les
binômes.

---

## Vue d'ensemble

| Phase | Jours | Dates | Ce qu'on peut lancer à la fin | Tag |
|---|---|---|---|---|
| **P1** Le pilote C | J1 – J2 | lun. 12 – mar. 13 | `pilote_demo` affiche les interruptions de la puce, sur PC et sur ARM émulé | `v0.1` |
| **P2** La chaîne complète, en fin | J3 | mer. 14 | une mesure part de la puce et arrive à l'écran du superviseur, via MQTT | `v0.2` |
| **P3** Le cœur de la passerelle | J4 – J5 | jeu. 15 – ven. 16 | le scénario « chambre froide » produit exactement les bonnes alarmes au journal | `v0.3` |
| **P4** Superviser et commander | J6 – J7 | lun. 19 – mar. 20 | `scripts/demo.sh` rejoue l'incident de bout en bout, commandes comprises : `DEMO OK` | `v0.4` |
| **P5** Sur la cible embarquée | J8 – J9 | mer. 21 – jeu. 22 | la passerelle tourne sur la carte ARM émulée et le superviseur, sur le PC, la voit | `v0.5` |
| **P6** Finaliser | J10 | ven. 23 | socle complet sur `main`, extensions, présentation, répétition facultative | `v1.0` |
| Soutenance | — | lun. 26 | devant le jury | |

Le vendredi 16 au soir, on fait un **bilan de mi-parcours** : chaque groupe
montre son `v0.3` en 5 minutes.

---

## P1 — Le pilote C · J1 – J2 (lundi 12, mardi 13)

**L'idée.** Vous recevez une « puce capteur » (le simulateur) et sa
datasheet. Comme pour une vraie carte, vous écrivez le pilote qui la fait
fonctionner : la démarrer, lire ses mesures, servir ses interruptions.

**Fourni** (dossier `squelette/`). Le build (`CMakeLists.txt`, compilation
croisée ARM), `registres.h` et le bus, la puce (`outils/sim_capteurs.c`,
compilée avec le reste), les scénarios, la configuration, les outils de
tests (`verif.h`, `Verif.hpp`) et `INTERFACES.md`.

**À faire.**
- Créer le dépôt à partir du squelette ; vérifier que tout compile, sur PC
  **et** en compilation croisée ARM (**faites-le dès la première heure**).
- Le pilote C (`pilote/`), avec une API à vous : ouvrir, vérifier `ID`,
  démarrer, lire un canal, écrire un calibrage, reset, auto-test, détecter
  un canal en panne.
- Les **interruptions simulées** : une boucle cadencée par `timerfd` lit
  `IRQ_STATUS`, appelle un *callback* par canal concerné, puis acquitte
  dans `IRQ_ACK` (W1C).
- `pilote_demo`, qui affiche une ligne `[irq] ch=… brut=…` par
  interruption.
- Les tests du pilote **sans la puce** : ils travaillent sur une banque de
  registres en mémoire, que le test remplit lui-même.

**Livrable vérifiable.**

```bash
cmake -B build && cmake --build build      # 0 warning
ctest --test-dir build                     # tests du pilote : tous verts
build/sim_capteurs --scenario scenarios/stable.scn &
build/pilote_demo --duree 5                # ~200 lignes [irq], 4 canaux
build-arm/pilote_demo --duree 5            # le même, binaire ARM sous qemu-arm
```

**Critères (vérifiés en fin de phase).** Compilation sans warning · tests verts ·
les 4 canaux servis à la bonne cadence · `pilote_demo` sans la puce lancée
→ message clair et code de sortie ≠ 0 · le binaire ARM est bien un binaire
ARM (`file`) et tourne sous `qemu-arm` · avec `panne-capteur.scn`, le canal
1 cesse d'être servi, et `pilote_demo` affiche `[panne] ch=1` puis
`[retabli] ch=1`, une fois chacun.

**Piège n° 1.** Le W1C : on acquitte en écrivant **1** dans `IRQ_ACK`, pas
en effaçant le bit dans `IRQ_STATUS` (la puce l'écraserait). Et
`volatile`, sans quoi le compilateur peut « optimiser » vos lectures de
registres.

---

## P2 — La chaîne complète, en fin · J3 (mercredi 14)

**L'idée.** Avant d'enrichir quoi que ce soit, on fait passer **une mesure
de bout en bout** : puce → pilote → démon C++ → MQTT → superviseur Python.
Le minimum, mais d'un bout à l'autre. Ainsi, l'assemblage n'est plus une
surprise de fin de projet : à partir d'aujourd'hui, on ne fait
qu'épaissir une chaîne qui marche.

**À faire.**
- `embedgate-gw` en C++ : une classe qui **possède** le pilote C (RAII :
  ouvert dans le constructeur, fermé dans le destructeur, non copiable).
- Une classe qui enveloppe la connexion `libmosquitto` de la même façon.
- Publier chaque mesure sur `embedgate/<gp>/mesure/<ch>`, au format du
  contrat.
- `superviseur.py` minimal : s'abonne et affiche les mesures reçues.

**Livrable vérifiable.**

```bash
mosquitto_sub -t 'embedgate/gp1/#' -v      # les 4 canaux défilent, JSON valide
python3 superviseur/superviseur.py --config config/gateway.ini
```

**Critères (vérifiés en fin de phase).** Les 4 topics de mesure reçus · chaque
message est un JSON valide avec les bons champs · les valeurs sont en unité
physique (≈ 4,0 °C, pas 4000) · `Ctrl-C` arrête le démon avec le code 0.

---

## P3 — Le cœur de la passerelle · J4 – J5 (jeudi 15, vendredi 16)

**L'idée.** La passerelle devient un vrai démon : elle reçoit vite, traite
à part, décide des alarmes intelligemment, garde une trace de tout, et
s'arrête proprement.

**À faire.**
- **Deux threads au moins** : l'acquisition (interruptions) dépose les
  mesures dans une **file thread-safe** ; le traitement les en retire.
  Recevoir ne doit jamais attendre le traitement.
- Le **moteur d'alarmes** : seuils bas/haut avec **hystérésis**, une seule
  alarme par anomalie, un événement de retour à la normale. Interchangeable
  (pattern *Strategy*) ; les abonnés (journal, MQTT) sont prévenus sans que
  le moteur les connaisse (pattern *Observer*).
- Le **journal** (format imposé), la **configuration** lue au démarrage
  (code 2 si invalide), l'**arrêt propre** sur `SIGTERM`/`SIGINT`.
- Les pannes de capteur : événements `PANNE` / `RETABLI`.
- Les tests, **écrits avant le code** pour le moteur d'alarmes :
  commencez par le test « une valeur bruitée autour du seuil ne déclenche
  qu'une seule alarme ».

**Livrable vérifiable.**

```bash
build/sim_capteurs --scenario scenarios/chambre-froide.scn &
build/embedgate-gw --config config/gateway.ini   # Ctrl-C une fois la puce arrêtée (60 s)
grep -E 'ALARME|NORMAL' embedgate.log
```

On doit lire **exactement**, dans cet ordre : alarme porte vers 10 s,
retour à la normale de la porte vers 14 s, alarme température vers 21 s,
retour à la normale vers 44 s. Ni plus, ni moins.

**Critères (vérifiés en fin de phase).** Le journal du scénario « chambre froide »
est exact · `panne-capteur.scn` donne une `PANNE` puis un `RETABLI` ·
Valgrind : 0 fuite sur 30 s · ThreadSanitizer : aucune *data race* (build
`-DSANITIZE=thread`) · `rafale.scn` (10 ms) tient 30 s sans perte ni
blocage · aucun `new`/`delete` nu dans le code.

**Vendredi 16, 16 h : bilan de mi-parcours.** 5 minutes par groupe : on
lance le scénario devant tout le monde.

---

## P4 — Superviser et commander · J6 – J7 (lundi 19, mardi 20)

**L'idée.** La passerelle devient utile à un humain : il voit tout d'un coup
d'œil, et il peut agir à distance. L'incident de 3 h du matin se rejoue
alors **de bout en bout, sans intervention**.

**À faire.**
- Le **tableau de bord** : une ligne par capteur (valeur, unité, état
  vert/rouge, panne), les dernières alarmes. Une console qui se rafraîchit
  suffit ; une page web est une extension.
- Les **commandes** (`calib`, `reset`, `selftest`, `acquisition`) : le
  superviseur publie sur `…/cmd`, la passerelle agit sur la puce et répond
  sur `…/cmd/ack`. Une commande invalide reçoit un `ack` en erreur et ne
  fait jamais tomber la passerelle.
- L'**état en ligne / hors ligne** grâce au testament MQTT (*Last Will*).
  Les alarmes sont publiées **retenues** : un superviseur lancé en retard
  voit tout de suite la situation.
- La **robustesse** : broker arrêté puis relancé (la passerelle se
  reconnecte), JSON malformé, canal inexistant.
- `pytest` pour le superviseur ; `scripts/demo.sh`, qui lance tout,
  rejoue `chambre-froide.scn`, envoie une commande de calibrage et vérifie
  les résultats.

**Livrable vérifiable.**

```bash
scripts/demo.sh                                   # → DEMO OK
python3 superviseur/commande.py --config config/gateway.ini calib 0 -1.5
# → ack ok, et la température affichée baisse de 1,5 °C
```

**Critères (vérifiés en fin de phase).** `demo.sh` affiche `DEMO OK` · l'`ack`
correspond à l'`id` envoyé · le calibrage se voit dans les mesures · une
commande invalide renvoie `ok:false` et le démon tourne toujours · tuer le
démon (`kill -9`) fait passer `…/etat` à `hors_ligne` · broker relancé →
les mesures reprennent sans redémarrer le démon · `pytest` vert.

---

## P5 — Sur la cible embarquée · J8 – J9 (mercredi 21, jeudi 22)

> *Les détails de cette phase seront ajustés le vendredi 9 octobre selon ce
> qui aura été fait dans le module Linux embarqué (architecture, image
> Buildroot). Le principe ne change pas.*

**L'idée.** Le client ne veut pas un PC dans son entrepôt, mais un petit
boîtier. Vous déployez la passerelle sur une carte ARM, ici émulée par
QEMU, et vous la supervisez depuis le PC, comme dans la réalité.

**À faire.**
- Compiler en croisé `pilote_demo` et `embedgate-gw`, avec `libmosquitto`
  pour ARM.
- Démarrer la carte (QEMU + image Buildroot) ; y copier la passerelle, la
  puce ARM fournie et la configuration.
- **Démarrage automatique** au boot (script d'init de la cible).
- La passerelle, sur la cible, publie vers le broker du PC hôte : le
  superviseur, sur le PC, ne voit aucune différence.
- Une session de **débogage croisé** : `gdbserver` sur la cible, `gdb` sur
  le PC, un point d'arrêt dans le service d'interruption. Captures dans le
  rapport.

**Livrable vérifiable.**

```bash
file build-arm/embedgate-gw                 # ELF 32-bit ARM
# carte démarrée : la passerelle se lance seule au boot
mosquitto_sub -t 'embedgate/gp1/etat'       # sur le PC : {"etat":"en_ligne"}
python3 superviseur/superviseur.py --config config/gateway.ini   # mesures venues de la carte
```

**Critères (vérifiés en fin de phase, et en démonstration).** Binaires ARM ·
démarrage automatique au boot · mesures de la carte reçues sur le PC ·
`chambre-froide.scn` sur la carte → mêmes alarmes qu'en P3 · procédure de
déploiement écrite dans le README et rejouable par quelqu'un d'autre.

**Si une étape résiste** (image, réseau), signalez-le dès le mercredi midi :
une image de secours est prévue. On ne perd pas deux jours sur la
configuration.

---

## P6 — Finaliser · J10 (vendredi 23)

**Matin.** Socle complet fusionné sur `main`. Extensions si le temps le
permet (voir ci-dessous). Les critères des phases 1 à 5 repassés
sur `main`.

**Après-midi.**
- `README.md` : installer, compiler, lancer, déployer. Testé par un autre
  groupe (échange de README à 14 h).
- La **présentation** du groupe, en PDF, **remise à 14 h** : architecture,
  choix, difficultés, résultats chiffrés, limites, et un micro-CV par
  membre. Le détail est dans [`phase6-instructions.md`](phase6-instructions.md).
- **Répétition chronométrée** de la soutenance devant un autre groupe :
  facultative, entre groupes volontaires.
- **Tag `v1.0` avant 17 h.** C'est lui qui est évalué.

---

## Extensions (seulement une fois le socle sur `main`)

Le détail de chaque extension (règles communes, quand la commencer, ce
qu'on montre) est dans [`extensions-instructions.md`](extensions-instructions.md).

| Code | Extension | Ce qu'on peut montrer |
|---|---|---|
| E1 | Bus CAN : un nœud capteur distant envoie ses mesures sur un bus CAN virtuel (SocketCAN, `vcan0`) ; la passerelle les intègre comme un 5ᵉ canal | `cansend` / `candump`, la mesure CAN apparaît au tableau de bord et déclenche ses alarmes |
| E2 | Tableau de bord web (mise à jour en direct) | navigateur ouvert pendant la démo |
| E3 | Historisation SQLite + export CSV | courbe de l'incident reconstituée après coup |
| E4 | Alarme temporisée : la porte ouverte n'alarme qu'après N secondes | scénario « livraison » sans fausse alerte |
| E5 | Qualité : CI locale, `cppcheck`, couverture `gcov`, Doxygen | rapports générés par une seule commande |
| E6 | Paquet Debian de la passerelle, service `systemd` sur le PC | `apt install ./embedgate.deb` puis `systemctl status` |

Le trinôme réalise au moins **deux extensions, au choix**.

---

## Ce qui est évalué à la fin

L'état du dépôt au tag **`v1.0`** (les vérifications des 5 phases
repassées), le journal de bord, la présentation et la soutenance. La grille
détaillée est dans l'énoncé (partie B.6).
Seule exception : un tag `v1.0.1` le lundi avant midi, qui ne modifie que
le `README.md` (voir [`phase6-instructions.md`](phase6-instructions.md)).
