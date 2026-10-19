# EmbedGate — Phase 4 pas à pas : superviser et commander

**Lundi 19 et mardi 20 octobre 2026 · tag `v0.4`**

> Ce document détaille la Phase 4 de l'[agenda](agenda-phases-embedgate.md) :
> il ne demande rien de plus, il découpe le travail en étapes, avec pour
> chacune **quoi faire**, **comment vérifier** et **les pièges**. Le contrat
> ([`squelette/INTERFACES.md`](squelette/INTERFACES.md)) fait foi : en cas de
> contradiction, c'est lui qui a raison, et vous le signalez.

---

## Où vous allez

En Phase 3, votre passerelle est devenue un surveillant : elle décide des
alarmes et garde une trace de tout. Mais personne ne la regarde, et
personne ne peut agir sur elle. Elle devient maintenant **utile à un
humain** :

- l'opérateur voit tout d'un coup d'œil, sur un **tableau de bord** ;
- il **agit à distance** : il recalibre un capteur, réinitialise la puce,
  lance un auto-test, coupe l'acquisition ;
- il sait si la passerelle est **en ligne**, même quand elle a disparu
  brutalement ;
- et si le broker redémarre, tout **reprend seul**.

```text
          superviseur.py (tableau de bord)                commande.py calib 0 -1.5
                    ▲  embedgate/gp1/#                          │ …/cmd          ▲ …/cmd/ack
                    │                                           ▼                │
   ═══════════════════════════════  broker MQTT  ═════════════════════════════════════
                    ▲ mesures, alarmes, pannes, état            │                ▲
                    │                                           ▼                │
              passerelle ── thread réseau : reçoit la commande, la vérifie ──────┘
                    │        thread de la boucle : l'exécute sur la puce, répond
                  puce
   testament : si la passerelle disparaît sans prévenir, le BROKER publie {"etat":"hors_ligne"}
```

Le critère qui résume la phase : **`scripts/demo.sh` rejoue l'incident de
3 h du matin de bout en bout, sans intervention, commande de calibrage
comprise, et affiche `DEMO OK`.**

### Ce que vous écrivez, ce qui est fourni

| Vous écrivez | Fourni (ne pas modifier) |
|---|---|
| `passerelle/` : le testament, la réception et l'exécution des commandes, la reconnexion | le contrat : topics `…/etat`, `…/cmd`, `…/cmd/ack` (§4), commandes (§4.1), ligne `CMD` du journal (§5) |
| `superviseur/superviseur.py` : le tableau de bord ; `superviseur/commande.py` | `config/gateway.ini`, `scenarios/*.scn` |
| `superviseur/test_*.py` : les tests `pytest` ; `tests/test_*.cpp` : les tests des commandes | |
| `scripts/demo.sh` | |
| `passerelle/Version.cpp` : passez à `0.4.0` | |

### Les bibliothèques autorisées

Celles des phases précédentes : `libmosquitto` (l'API C) et la
bibliothèque standard du C++ ; pour le JSON, votre code ou une bibliothèque
*header-only* copiée dans le dépôt (instructions de la Phase 2). En Python :
`paho-mqtt`, `pytest` et la bibliothèque standard. Une console qui se
rafraîchit suffit pour le tableau de bord : pas de `curses` obligatoire, pas
de framework web (une page web, c'est l'extension E2).

### Rythme conseillé

| Moment | Étapes | Vous êtes dans les temps si… |
|---|---|---|
| lundi matin | 0, 1, 2 | `kill -9` de la passerelle fait passer `…/etat` à `hors_ligne` ; un `mosquitto_pub` sur `…/cmd` reçoit un `ack`, même `"ok":false` |
| lundi après-midi | 3, 5 | `calib 0 -1.5` fait baisser la température affichée ; le tableau de bord montre états et alarmes |
| mardi matin | 4, 6, 7 | broker relancé : tout reprend ; `commande.py` affiche l'`ack` ; `pytest` est vert |
| mardi après-midi | 8, 9 | `demo.sh` affiche `DEMO OK` ; checklist, tag `v0.4` |

---

## Travailler à deux

Cette phase se découpe naturellement en **deux pistes**, qui ne se touchent
qu'en un point fixé par le contrat : les topics `…/cmd` et `…/cmd/ack`.

| | Piste A : côté humain (Python) | Piste B : côté passerelle (C++) |
|---|---|---|
| Étapes | 5, 6, 7 : tableau de bord, `commande.py`, `pytest` | 1 à 4 : testament, commandes reçues, exécutées, reconnexion |
| Fichiers | `superviseur/*.py` | `passerelle/` (nouveaux fichiers pour les commandes), `tests/test_*.cpp` |
| Se teste seule avec | `mosquitto_pub` qui joue la passerelle (un faux `ack`) | `mosquitto_pub` qui joue le superviseur (une fausse commande) |
| Ensemble, mardi | `scripts/demo.sh`, puis la checklist | |

**Qui prend quoi.** Si l'un de vous a peu écrit dans `passerelle/` en
semaine 1 (`git shortlog -sn -- passerelle/`), c'est l'occasion : il prend
la piste B, l'autre relit et ajoute un test. À la soutenance, chacun peut
être interrogé sur n'importe quelle partie.

**Trinôme** : une troisième piste, la **robustesse** (étapes 1 et 4 :
testament, reconnexion), séparée des commandes (étapes 2 et 3), plus
`demo.sh`.

**Lundi, première heure : le commit de jonction.** Avant de vous séparer,
créez ensemble les **fichiers vides** des nouvelles classes (déclarations,
corps vides), branchez-les dans `main.cpp`, vérifiez que tout compile, et
fusionnez sur `main`. Ensuite, chacun ne travaille plus que dans **ses**
fichiers. C'est le moyen le plus simple d'éviter les conflits.

**Les fichiers communs** ont un responsable pour la phase :

| Fichier | Qui y touche |
|---|---|
| `passerelle/main.cpp` | celui qui porte la piste B. L'autre écrit ses classes dans ses fichiers, et donne les 3 lignes de branchement dans sa *merge request* |
| `CMakeLists.txt` | rien à faire : tout nouveau `passerelle/*.cpp` et tout `tests/test_*.cpp` sont compilés automatiquement |
| `JOURNAL.md` | un seul l'écrit, le soir, après avoir fait le point avec l'autre : deux ajouts au même endroit, c'est un conflit assuré |
| `config/gateway.ini` | personne : faites une copie pour vos essais |

**La routine Git, à deux.** Fusionnez **petit et souvent** : une branche
vit une demi-journée, pas deux jours.

```bash
git switch main && git pull                  # chaque matin, et avant chaque nouvelle branche
git switch -c p4/commandes                   # une branche courte, un sujet
git add passerelle/Commandes.cpp && git commit -m "commandes : refus du JSON invalide"
git push -u origin p4/commandes              # puis une merge request sur GitLab, relue par l'autre
git merge main                               # si main a avancé entre-temps : on le fusionne DANS sa branche
```

- **Fusionnez (`merge`), ne réécrivez pas l'historique** (`rebase`,
  `cherry-pick`, `push --force`) sur une branche que l'autre utilise : c'est
  ce qui fabrique des commits en double. Un historique avec des fusions est
  parfaitement lisible.
- **Un conflit** : ouvrez le fichier, gardez ce qu'il faut des deux
  versions, supprimez les marques `<<<<<<<`, `=======`, `>>>>>>>`,
  recompilez, relancez les tests, puis `git add` et `git commit`.
- **Une erreur ?** Sur une branche, presque tout se rattrape :
  `git reflog` montre où vous étiez avant, et `git switch -c secours <ligne>`
  le récupère. Demandez avant d'effacer quoi que ce soit.

---

## Étape 0 — Version et commit de jonction (lundi, première heure)

**À faire.**

- Passez la version à `0.4.0` dans `passerelle/Version.cpp`.
- Créez les fichiers des classes de la phase, vides, et branchez-les (voir
  « Travailler à deux »). Par exemple : une classe pour les commandes
  (lecture, vérification, exécution, réponse), et les ajouts à votre classe
  MQTT (testament, abonnement).
- Écrivez **le premier test** : « un JSON invalide reçoit un `ack`
  `"ok":false` et ne lève pas d'exception ». Comme en Phase 3 : il décrit le
  comportement voulu avant le code.

---

## Étape 1 — L'état en ligne et le testament MQTT

**L'image.** Un alpiniste laisse au refuge une enveloppe : « si je ne suis
pas revenu à 18 h, prévenez les secours ». S'il revient, il la reprend. S'il
ne revient pas, c'est **le refuge** qui l'ouvre, pas lui. Le **testament
MQTT** (*Last Will*), c'est cette enveloppe : un message que la passerelle
confie au broker **en se connectant**, et que **le broker** publiera à sa
place si elle disparaît sans dire au revoir (plantage, `kill -9`, câble
arraché).

**À faire** (contrat §4, topic `embedgate/<gp>/etat`, QoS 1, **retenu**) :

1. **Avant** `mosquitto_connect` : `mosquitto_will_set` avec
   `{"etat":"hors_ligne"}`, QoS 1, retenu.
2. **Une fois connecté** : publier `{"etat":"en_ligne"}`, QoS 1, retenu.
3. **À l'arrêt normal** (Ctrl-C, `SIGTERM`, fin de la puce) : publier
   **vous-même** `{"etat":"hors_ligne"}`, puis vous déconnecter.

**Pièges.**

- **Le testament déclaré après la connexion** ne sert à rien : il fait
  partie du message de connexion. Pas d'erreur, juste un `kill -9` qui
  laisse `en_ligne` affiché.
- **Un départ annoncé ne déclenche pas le testament.** Quand la passerelle
  se déconnecte proprement, le broker considère qu'elle a dit au revoir :
  il jette l'enveloppe. D'où le point 3.
- **Publier `en_ligne` juste après `mosquitto_connect`** marche… une fois.
  Après une reconnexion (étape 4), personne ne le republie. Publiez-le dans
  le **rappel de connexion** (`mosquitto_connect_callback_set`), appelé à
  chaque connexion.

**Vérifier.**

```bash
mosquitto_sub -t 'embedgate/gp1/etat' -v &          # laissez tourner dans un coin
build/sim_capteurs --scenario scenarios/stable.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini &    # → {"etat":"en_ligne"}
sleep 3; pkill -9 embedgate-gw                      # → {"etat":"hors_ligne"}, en moins d'une seconde
```

---

## Étape 2 — Recevoir les commandes

**À faire.**

- **S'abonner** à `embedgate/<gp>/cmd` (QoS 1), **dans le rappel de
  connexion**, comme `en_ligne`.
- Brancher un **rappel de message** (`mosquitto_message_callback_set`) : il
  reçoit le topic et le contenu (des octets, pas une chaîne terminée par
  `\0` : utilisez la longueur).
- **Lire le JSON et le vérifier** (contrat §4.1) : un objet ; un `"id"`
  texte ; un `"cmd"` parmi `calib`, `reset`, `selftest`, `acquisition` ; pour
  `calib`, un `"ch"` entier de 0 à 3 et un `"decalage"` nombre ; pour
  `acquisition`, un `"marche"` booléen.
- **Répondre** sur `embedgate/<gp>/cmd/ack`, QoS 1, **non retenu** :
  `{"id":"7","ok":true,"detail":""}`. Une commande invalide reçoit
  `"ok":false` et un `detail` lisible (« canal inexistant : 7 »,
  « commande inconnue : vidange », « JSON invalide… »), avec **le même id**
  quand on a pu le lire.
- **Une ligne au journal** par commande (contrat §5) :
  `… CMD id=7 cmd=calib ok=1`.

**La règle d'or : rien de ce qui arrive du réseau ne doit faire tomber la
passerelle.** N'importe qui peut publier n'importe quoi sur `…/cmd` : du
texte, un JSON tronqué, un canal 99, un message vide de 0 octet.

**Pièges.**

- **Une exception qui sort du rappel de message** : il est appelé par
  `libmosquitto`, du **C**, qui ne sait pas la faire remonter. Résultat :
  `terminate`, la passerelle tombe. Attrapez **tout** dans le rappel.
- **`std::stoi`, `std::stod`** lèvent une exception sur « abc » ; un `double`
  `1e30` converti en `unsigned` est un comportement indéfini. Vérifiez
  avant de convertir.
- **Le rappel tourne dans le thread réseau de `libmosquitto`**, pas dans
  les vôtres. Ce qu'il touche est partagé : voir l'étape 3. Le journal est
  déjà protégé par un mutex si vous avez suivi la Phase 3 ; sinon, c'est le
  moment.
- **`"id":7`** (un nombre) n'est pas `"id":"7"` (un texte) : le contrat dit
  texte.
- **L'`ack` retenu** : un superviseur qui arrive plus tard recevrait la
  réponse à une commande qu'il n'a jamais envoyée.

**Vérifier**, la passerelle lancée :

```bash
mosquitto_sub -t 'embedgate/gp1/cmd/ack' -v &
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m 'pas du json'                                   # ok:false
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m '{"id":"3","cmd":"vidange"}'                    # ok:false, id 3
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m '{"id":"4","cmd":"calib","ch":7,"decalage":1}'  # ok:false, id 4
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -n                                                 # message vide : ok:false
pgrep embedgate-gw                         # toujours là
grep CMD embedgate.log
```

---

## Étape 3 — Exécuter les commandes sur la puce

**À faire.** Votre pilote de la Phase 1 sait déjà tout faire :

| Commande | Pilote | Attention |
|---|---|---|
| `calib` | écrire `CALIB[ch]` | le contrat donne le décalage en **unité physique** (−1,5 °C) ; la puce le veut en **unité brute** : −1,5 / 0,001 = **−1500** (facteur du canal, §3) |
| `reset` | `CTRL.RESET`, attendre que la puce l'efface | la puce remet aussi les calibrages à 0 |
| `selftest` | `CTRL.SELFTEST`, attendre, lire `STATUS.FAULT` | `"ok":true` (l'auto-test a eu lieu), et le résultat dans `detail` : « aucun défaut » ou « défaut… » |
| `acquisition` | `CTRL.ENABLE` à 1 ou 0 | à l'arrêt, plus aucune mesure ; le temps du scénario s'arrête aussi (§1.3) |

L'`ack` part **après** l'exécution, et dit ce qui s'est réellement passé :
une puce qui ne répond pas au reset donne `"ok":false`.

**Le point délicat : qui touche la puce ?** La commande arrive dans le
thread réseau. Pendant ce temps, votre thread d'acquisition lit et acquitte
les registres toutes les quelques millisecondes. Deux threads sur le même
matériel, c'est la question de la Phase 3 (l'étagère de la poste), appliquée
à la puce. Deux façons de s'en sortir :

- **la simple** : un **mutex** pris par le thread d'acquisition autour de
  chaque service d'interruption, et par la commande autour de son accès à
  la puce. Si votre boucle est `pilote_boucle_irq` (du C, qui sert les
  interruptions elle-même), ce n'est pas possible sans la modifier : voyez
  la suivante ;
- **la rigoureuse** : la puce n'a **qu'un seul maître**, le thread
  d'acquisition. Le rappel réseau ne fait que **déposer** la commande
  vérifiée dans une petite file (votre `FileAttente` de la Phase 3), et le
  thread d'acquisition la vide **entre deux services**, exécute, répond.
  Avec `pilote_boucle_irq`, ajoutez au pilote un troisième rappel, appelé à
  chaque tick après le service des interruptions (trois lignes de C). C'est
  aussi le seul moment où il y a quelque chose à faire quand l'acquisition
  est arrêtée : sans interruption, le rappel de mesure n'est plus jamais
  appelé, et la commande `acquisition marche` ne serait jamais exécutée.

**Les tests** : donnez à votre classe de commandes la puce sous la forme
d'une **interface** (calibrer, réinitialiser, auto-tester, marche / arrêt),
comme vous avez donné une fonction « publier » à l'abonné MQTT en Phase 3.
Le test lui passe une fausse puce qui note ce qu'on lui demande, et vérifie
que `calib 0 -1.5` demande bien `-1500` ; un canal 7 n'appelle rien ; une
puce muette donne un `ack` en erreur.

**Vérifier.**

```bash
mosquitto_sub -t 'embedgate/gp1/mesure/0' &                                                    # ≈ 4.0 °C
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m '{"id":"7","cmd":"calib","ch":0,"decalage":-1.5}'  # ≈ 2.5 °C
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m '{"id":"8","cmd":"reset"}'                        # ≈ 4.0 °C de nouveau
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m '{"id":"10","cmd":"acquisition","marche":false}'  # plus rien
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m '{"id":"11","cmd":"acquisition","marche":true}'   # ça repart
```

---

## Étape 4 — La reconnexion au broker

**À faire.** `mosquitto_loop_start` (Phase 2) se reconnecte déjà tout seul
quand le broker revient. Votre travail est de **tout remettre en place** à
chaque reconnexion : le broker a tout oublié de vous (session « propre »).
D'où le rappel de connexion des étapes 1 et 2 : il **republie `en_ligne`**
et **se réabonne** à `…/cmd`. Pendant la coupure, une publication qui
échoue est comptée, jamais fatale (déjà le cas en Phase 2).

**Le piège le plus vicieux de la phase.** Sans réabonnement, après une
relance du broker, **les mesures repartent** (publier ne demande pas
d'abonnement), le tableau de bord se remet à bouger… et les commandes ne
sont plus jamais reçues. Tout a l'air de marcher.

**Vérifier.**

```bash
sudo systemctl stop mosquitto; sleep 5; sudo systemctl start mosquitto
mosquitto_sub -t 'embedgate/gp1/mesure/+' -C 4      # les mesures reviennent (quelques secondes)
python3 superviseur/commande.py --config config/gateway.ini selftest   # et les commandes passent
```

---

## Étape 5 — Le tableau de bord

**À faire** (`superviseur/superviseur.py`, agenda) : une console qui se
rafraîchit, par exemple chaque seconde, avec :

- **une ligne par capteur** : nom, dernière valeur et unité, **état** (ok
  en vert, alarme en rouge, panne) ;
- l'**état de la passerelle** : en ligne, hors ligne, ou inconnu ;
- les **dernières alarmes** (heure, capteur, ALARME ou NORMAL, valeur).

Un seul abonnement suffit : `embedgate/<gp>/#`. Les alarmes, les pannes et
l'état sont **retenus** : lancé en retard, le tableau de bord doit montrer
**aussitôt** la situation.

**Séparez la logique de l'affichage.** Une classe (ou un dictionnaire) qui
tient la situation et la met à jour message par message, et une fonction
qui la transforme en texte. Ni l'une ni l'autre ne touche au réseau : c'est
ce que vous testerez avec `pytest` (étape 7), sans broker.

**Pièges.**

- **Le rappel `on_message` de paho tourne dans un autre thread** que votre
  boucle d'affichage : le même verrou qu'en Phase 2 (`threading.Lock`).
- **Un message hors contrat** (champ manquant, JSON invalide) : un
  avertissement sur la sortie d'erreur, et on continue. Un `KeyError` qui
  arrête le superviseur, c'est l'opérateur aveugle.
- **Les couleurs** (`\033[31m` rouge, `\033[32m` vert, `\033[0m` fin) et
  l'effacement de l'écran (`\033[H\033[2J`) seulement dans un terminal
  (`sys.stdout.isatty()`) : redirigé vers un fichier, ils deviennent des
  caractères parasites.
- **`"niveau":null`** au retour à la normale : en Python, c'est `None`.

**Vérifier.** Lancez la chambre froide (`--vite 4`) et la passerelle,
attendez 8 secondes, puis lancez le tableau de bord **en retard** :
l'alarme de température (active de 5 à 11 s à cette vitesse) apparaît dès
la première seconde. (Le broker ne retient que le **dernier** message de
chaque topic : de la porte, le tableau de bord ne verra que son retour à la
normale.)

---

## Étape 6 — `commande.py`

**À faire** (contrat §6) :

```bash
python3 superviseur/commande.py --config config/gateway.ini calib 0 -1.5
python3 superviseur/commande.py --config config/gateway.ini reset
python3 superviseur/commande.py --config config/gateway.ini selftest
python3 superviseur/commande.py --config config/gateway.ini acquisition arret
```

Il affiche l'`ack`, et sort avec le code **0** si `"ok":true`, **1** sinon
(refus, ou pas de réponse en quelques secondes : la passerelle est-elle
lancée ?).

**L'ordre compte.**

1. Se connecter, s'abonner à `…/cmd/ack`, et **attendre que le broker
   confirme l'abonnement** (`on_subscribe`).
2. Seulement alors, publier la commande, avec un **id unique**.
3. Attendre l'`ack` qui porte **cet id**.

**Pièges.**

- **Publier avant d'être abonné** : si la passerelle répond plus vite que
  l'abonnement ne s'établit, l'`ack`, non retenu, est perdu. Rare sur la
  VM, fréquent en Phase 5, quand la passerelle est sur la carte.
- **Prendre le premier `ack` venu** : un autre superviseur peut commander
  au même moment. Comparez l'id.
- **Vérifier le canal dans `commande.py`** : ne le faites pas. C'est le
  travail de la passerelle, et c'est ce qui permet de tester son refus
  (`calib 9 1` → `"ok":false`, code 1).

---

## Étape 7 — `pytest`

**À faire.** Des tests sans broker, dans `superviseur/test_*.py` :

- **la situation** : une mesure s'affiche ; une `ALARME` passe le canal au
  rouge, son `NORMAL` le remet au vert ; une panne, puis son rétablissement ;
  `en_ligne`, puis `hors_ligne` ;
- **la robustesse** : un message hors contrat ne change rien et ne fait
  pas planter ;
- **`commande.py`** : chaque commande donne le JSON exact du contrat ; un
  `ack` d'un autre id est ignoré.

```bash
python3 -m pytest superviseur/        # tout vert
```

---

## Étape 8 — `scripts/demo.sh`

**L'idée** (agenda) : l'incident de 3 h du matin se rejoue **de bout en
bout, sans intervention**. Un seul script, qui lance tout, agit, vérifie et
conclut :

1. lancer la puce (`chambre-froide.scn`, avec `--vite` pour ne pas attendre
   60 s), la passerelle, le tableau de bord (sortie dans un fichier) ;
2. attendre `en_ligne` ;
3. envoyer une **commande de calibrage** avec `commande.py`, et vérifier
   son code ;
4. laisser le scénario se dérouler (la passerelle s'arrête seule à la fin
   de la puce, ou le script l'arrête) ;
5. **vérifier**, au moins : les 4 événements du journal dans l'ordre, la
   ligne `CMD … ok=1`, le calibrage visible dans les mesures,
   `hors_ligne` à l'arrêt ;
6. afficher **`DEMO OK`** (code 0), ou **`DEMO ECHEC : la raison`** (code 1).

**Pièges.**

- **Un `DEMO OK` sans vérification** : si rien dans votre script ne peut
  afficher `DEMO ECHEC`, il ne prouve rien.
- **Des processus oubliés** : à la fin, et surtout en cas d'échec, tuez ce
  que vous avez lancé (`trap 'kill …' EXIT`). Sinon, la démo suivante voit
  deux puces.
- **Modifier `config/gateway.ini`** : faites une copie (le journal dans
  `/tmp`, par exemple).
- **Le calibrage** décale l'alarme de température (la puce lit 1,5 °C de
  moins) : elle arrive plus tard. C'est normal, et c'est même une bonne
  vérification.
- **Mesurer le calibrage** sur une température stable : avant l'ouverture
  de la porte (10 s de scénario), par exemple la moyenne de quelques mesures
  avant l'`ack`, et de quelques mesures après.

---

## Étape 9 — La checklist, `v0.4`

1. Déroulez la checklist ci-dessous.
2. Recompilez pour ARM : le pilote et ses tests restent verts.
3. Fusionnez, posez le tag :

```bash
git switch main && git merge <votre-branche>      # main compile, tests verts
git tag v0.4 && git push origin main v0.4
```

Une ligne dans `JOURNAL.md` chaque soir. Le tag est un **repère** : posé
mercredi matin, il n'est pas pénalisé.

---

## Checklist de fin de phase

À dérouler vous-mêmes avant de poser le tag, broker lancé. Ce sont les
critères de l'agenda. Remplacez `gp1` par votre groupe.

```bash
# 1. Compilation (aucun avertissement) et tests
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
python3 -m pytest superviseur/
cmake --build build-arm -j && ctest --test-dir build-arm         # la Phase 1 reste verte sur ARM

# 2. LE critère de la phase
scripts/demo.sh                                                  # DEMO OK

# 3. Une commande, son ack, son effet
build/sim_capteurs --scenario scenarios/stable.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini & sleep 2
mosquitto_sub -t 'embedgate/gp1/mesure/0' -C 3                   # ≈ 4 °C
python3 superviseur/commande.py --config config/gateway.ini calib 0 -1.5; echo "code=$?"   # ack ok, code=0
mosquitto_sub -t 'embedgate/gp1/mesure/0' -C 3                   # ≈ 2.5 °C

# 4. Commandes invalides : ok:false, la passerelle tourne toujours
python3 superviseur/commande.py --config config/gateway.ini calib 9 1; echo "code=$?"      # ok:false, code=1
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m 'pas du json'
mosquitto_pub -t 'embedgate/gp1/cmd' -q 1 -m '{"id":"3","cmd":"vidange"}'
pgrep embedgate-gw                                               # toujours là
grep CMD embedgate.log                                           # une ligne par commande

# 5. Broker relancé : tout reprend, sans redémarrer la passerelle
sudo systemctl stop mosquitto; sleep 5; sudo systemctl start mosquitto; sleep 5
mosquitto_sub -t 'embedgate/gp1/mesure/+' -C 4                   # les mesures reviennent
python3 superviseur/commande.py --config config/gateway.ini selftest; echo "code=$?"       # code=0

# 6. Le testament
pkill -9 embedgate-gw; sleep 1
mosquitto_sub -t 'embedgate/gp1/etat' -C 1                       # {"etat":"hors_ligne"}
pkill sim_capteurs

# 7. La Phase 3 tient toujours (chambre froide : 4 lignes, comme avant)
```

- [ ] `passerelle/Version.cpp` annonce `0.4.0`
- [ ] aucun avertissement ; tous les tests verts, C, C++ et `pytest` ; la Phase 1 verte sur ARM
- [ ] `scripts/demo.sh` affiche `DEMO OK`, et il sait afficher `DEMO ECHEC`
- [ ] l'`ack` porte l'`id` envoyé ; `commande.py` : code 0 si ok, 1 sinon
- [ ] le calibrage se voit dans les mesures
- [ ] une commande invalide renvoie `"ok":false` et la passerelle tourne toujours
- [ ] `kill -9` de la passerelle : `…/etat` passe à `hors_ligne` ; arrêt normal : aussi
- [ ] broker relancé : les mesures **et les commandes** reprennent, sans redémarrer la passerelle
- [ ] le tableau de bord montre l'état de chaque capteur, les pannes, les dernières alarmes et l'état de la passerelle, même lancé en retard
- [ ] les critères de la Phase 3 sont toujours remplis
- [ ] `main` à jour, tag `v0.4` poussé, `JOURNAL.md` tenu

---

## Si vous êtes bloqués

| Symptôme | Où regarder |
|---|---|
| `kill -9` : `…/etat` reste `en_ligne` | étape 1 : testament déclaré avant `mosquitto_connect` ? retenu ? |
| Ctrl-C : le tableau de bord reste « en ligne » | étape 1 : publiez `hors_ligne` vous-même à l'arrêt normal |
| aucun `ack`, rien au journal | étape 2 : abonnement à `…/cmd` fait ? rappel de message branché **avant** le démarrage du thread réseau ? |
| la passerelle tombe sur `mosquitto_pub -m 'pas du json'` | étape 2 : une exception sort du rappel de message ; attrapez tout |
| `ack` reçu dans `mosquitto_sub`, mais `commande.py` dit « pas de réponse » | étape 2 : id identique, en texte ? topic exact `…/cmd/ack` ? |
| `calib 0 -1.5` : ack ok, mais la température ne bouge pas | étape 3 : décalage converti en unité brute (−1500) ? |
| `acquisition marche` ne relance jamais l'acquisition | étape 3 : la commande attend un rappel de mesure qui ne vient plus ; exécutez-la à chaque tick |
| broker relancé : les mesures reviennent, mais plus les commandes | étape 4 : réabonnement dans le rappel de connexion |
| broker relancé : la passerelle s'arrête | étape 4 : une publication en échec ne doit jamais être fatale |
| le tableau de bord lancé en retard ne montre pas l'alarme | étape 5 : abonné à `embedgate/<gp>/#` ? (et la passerelle publie-t-elle bien en retenu ?) |
| le superviseur s'arrête sur `KeyError` | étape 5 : un message hors contrat est ignoré, pas fatal |
| des `^[[31m` dans le fichier de sortie | étape 5 : couleurs seulement si `isatty()` |
| `commande.py` rate un `ack` de temps en temps | étape 6 : attendre la confirmation de l'abonnement avant de publier |
| `demo.sh` : deux puces, événements en double | étape 8 : `trap` pour tout arrêter ; avant de relancer, `pkill sim_capteurs; pkill embedgate-gw` |
| `demo.sh` : l'alarme de température arrive plus tard qu'en P3 | étape 8 : c'est le calibrage, c'est normal |
| TSan : data race au reset | étape 3 : deux threads touchent la puce ; un mutex, ou un seul maître |

Pour demander de l'aide, dites ce que vous **attendiez** et ce qui s'est
**réellement** passé, avec la commande et sa sortie : c'est le plus court
chemin vers la réponse.
