# EmbedGate — Phase 3 pas à pas : le cœur de la passerelle

**Jeudi 15 et vendredi 16 octobre 2026 · tag `v0.3` · bilan de mi-parcours vendredi à 16 h**

> Ce document détaille la Phase 3 de l'[agenda](agenda-phases-embedgate.md) :
> il ne demande rien de plus, il découpe le travail en étapes, avec pour
> chacune **quoi faire**, **comment vérifier** et **les pièges**. Le contrat
> ([`squelette/INTERFACES.md`](squelette/INTERFACES.md)) fait foi : en cas de
> contradiction, c'est lui qui a raison, et vous le signalez.

---

## Où vous allez

En Phase 2, votre passerelle faisait passer les mesures, sans rien en
penser. Elle devient maintenant un **surveillant** : elle reçoit vite,
traite à part, décide des alarmes intelligemment, garde une trace de tout,
et s'arrête proprement.

```text
                  thread d'ACQUISITION                    thread de TRAITEMENT
puce → pilote → rappel : dépose la mesure ──► file ──► convertit, publie la mesure (MQTT)
                (rien d'autre)                          moteur d'alarmes ──► journal
                                                                         └─► MQTT (alarmes, pannes)
```

Le critère qui résume la phase : le scénario « chambre froide » (une porte
mal refermée, la température qui monte) produit **exactement** ces quatre
lignes au journal, ni plus, ni moins :

```text
… ALARME ch=3 nom=porte niveau=HAUT valeur=1              (≈ 10 s après START : la porte s'ouvre)
… NORMAL ch=3 nom=porte valeur=0                          (≈ 14 s : elle se referme)
… ALARME ch=0 nom=temperature niveau=HAUT valeur=8.1…     (≈ 21 s : la température dépasse 8 °C)
… NORMAL ch=0 nom=temperature valeur=6.9…                 (≈ 44 s : elle repasse sous 7 °C)
```

(Au début de chaque ligne, l'horodatage ; les valeurs exactes varient un
peu d'un groupe à l'autre.)

C'est ce que vous montrerez en 5 minutes, devant tout le monde, au **bilan
de mi-parcours** du vendredi 16 à 16 h.

### Ce que vous écrivez, ce qui est fourni

| Vous écrivez | Fourni (ne pas modifier) |
|---|---|
| `passerelle/` : le moteur d'alarmes, le journal, la file, les threads | `config/gateway.ini` : seuils et hystérésis (§3 du contrat), utilisés par les vérifications |
| `tests/test_*.cpp` : les tests du moteur (**avant** le moteur), de la file, du journal | `scenarios/chambre-froide.scn`, `panne-capteur.scn`, `rafale.scn` |
| `passerelle/Version.cpp` : passez à `0.3.0` | `CMakeLists.txt` : l'option `-DSANITIZE=thread` est déjà prévue |

Le superviseur Python ne change pas dans cette phase : son tableau de bord
est pour la Phase 4.

### Les bibliothèques autorisées

Celles de la Phase 2, plus la bibliothèque standard du C++ pour les
threads : `std::thread`, `std::mutex`, `std::condition_variable`,
`std::atomic`. Elles suffisent, et elles se compileront pour ARM en Phase 5
sans rien installer. Pour toute autre bibliothèque, demandez avant.

### Rythme conseillé

| Moment | Étapes | Vous êtes dans les temps si… |
|---|---|---|
| jeudi matin | 0, 1 | le test « valeur bruitée autour du seuil » est écrit, puis vert |
| jeudi après-midi | 2, 3, 4 | le journal s'écrit au bon format ; la file passe son test à deux threads |
| vendredi matin | 5, 6, 7 | la chambre froide donne ses 4 lignes ; `Ctrl-C` arrête tout proprement |
| vendredi, avant 16 h | 8, 9 | checklist déroulée, tag `v0.3`, démonstration prête |

**Répartition conseillée dans le binôme.** L'un écrit le moteur d'alarmes,
ses tests et le journal : aucun thread, tout se teste en quelques
millisecondes. L'autre écrit la file, ses tests, puis les deux threads de
`main`. Vous vous rejoignez jeudi soir ou vendredi matin. Changez de rôle
par rapport à la Phase 2.

**Si jeudi à 17 h le moteur n'est pas testé, dites-le.** C'est la pièce
maîtresse de la démonstration du vendredi. Faites-le passer avant les
threads : le moteur peut d'abord être appelé directement dans la boucle de
la Phase 2, et déménager dans le thread de traitement ensuite.

---

## Étape 0 — Le premier test, avant le moteur (jeudi, première heure)

**À faire.** Créez `tests/test_alarmes.cpp` et écrivez **d'abord** le test
que l'agenda demande : « une valeur bruitée autour du seuil ne déclenche
qu'une seule alarme ». Avec les seuils de la température (`haut = 8.0`,
`hysteresis = 1.0`), donnez au moteur une suite comme :

```text
7.9  8.1  7.8  8.2  7.75  8.3  7.6  8.05     → une seule alarme HAUT
```

Puis d'autres valeurs qui redescendent : tant qu'elles restent au-dessus de
7,0, aucun retour à la normale ; la première sous 7,0 en donne **un**.

Le test ne compile pas encore : c'est normal, la classe n'existe pas. En
l'écrivant, vous décidez comment le moteur s'utilise : ce qu'on lui donne
(un canal, une valeur, une heure) et comment il annonce ce qu'il a décidé.

**Pourquoi d'abord.** Le test décrit le comportement voulu **avant** qu'on
soit tenté de l'adapter à ce que le code fait. Committez-le seul, **sur
votre branche** (il ne compile pas encore, `main` doit toujours compiler) :
votre historique Git montrera qu'il est arrivé avant le moteur.

Passez aussi la version à `0.3.0` dans `passerelle/Version.cpp`.

---

## Étape 1 — Le moteur d'alarmes et sa règle à seuils

**La règle du contrat** (`INTERFACES.md` §3) : une alarme haute se
déclenche quand la valeur **dépasse** `haut`, et ne retombe que quand la
valeur repasse **sous** `haut − hysteresis`. Même chose, en miroir, pour
`bas`.

**L'image.** Un thermostat de chauffage. S'il se coupait à 20 °C pile et se
rallumait à 19,99 °C, il claquerait toutes les secondes. Il a donc **deux**
seuils, et entre les deux, il reste dans l'état où il est. Ici : on entre
en alarme au-dessus de 8 °C, on n'en sort que sous 7 °C. La mesure, qui
tremble de ±0,3 °C, ne suffit plus à faire clignoter l'alarme.

**À faire.**

- Une **règle** qui, à partir de la valeur et de l'état actuel du canal
  (`NORMAL`, `HAUT` ou `BAS`), donne le nouvel état.
- Un **moteur** qui garde l'état de **chaque canal**, applique la règle à
  chaque mesure, et n'annonce un événement que quand l'état **change** :
  `ALARME` (avec le niveau `HAUT` ou `BAS`) ou `NORMAL`.
- L'événement porte le canal, son nom, le niveau, **la valeur et l'heure de
  la mesure** qui a décidé.

**Le pattern *Strategy* (imposé par l'agenda).** La règle est une **pièce
interchangeable**, comme la lame d'un tournevis : une interface (une classe
abstraite avec une méthode virtuelle pure), et la règle à seuils qui en
hérite. Demain, une règle « la porte n'alarme qu'après 30 s ouverte »
(extension E4) se branchera sans toucher au moteur. Le minimum : une
interface et une implémentation. À l'oral, on vous demandera comment
brancher une autre règle.

**Les tests à ajouter** (dans `test_alarmes.cpp`) :

- le retour à la normale n'arrive que **sous** `haut − hysteresis` ;
- le seuil lui-même ne déclenche pas (8,0 ne « dépasse » pas 8,0) ;
- le seuil bas, en miroir ;
- la porte (`haut = 0.5`, `hysteresis = 0`) : 1 donne une alarme, 0 la
  retombée ;
- les canaux sont indépendants : une alarme sur l'un ne change rien aux
  autres.

**Pièges.**

- **Annoncer l'alarme à chaque mesure** au-dessus du seuil, au lieu d'une
  fois au changement d'état : des centaines de lignes au journal.
- **Appliquer l'hystérésis au déclenchement** (alarme à `haut +
  hysteresis`) : une seule alarme, mais trois secondes trop tard. Le
  contrat dit : on **déclenche** à `haut`, on **retombe** à `haut −
  hysteresis`.
- **Un seul état pour tous les canaux** : la porte qui se referme
  « éteint » l'alarme température.
- **Comparer des valeurs brutes aux seuils** : les seuils sont en unité
  physique (8.0 °C, pas 8000).

---

## Étape 2 — Le journal, premier abonné du moteur

**Le pattern *Observer* (imposé par l'agenda).** Le moteur annonce ce qui
se passe, comme un crieur public : il ne sait pas qui écoute. Il garde une
liste d'**abonnés** (une interface avec une méthode du genre
`surEvenement(const Evenement&)`), et les prévient tous à chaque
événement. Aujourd'hui, deux abonnés : le journal et MQTT. Le moteur ne
doit connaître ni l'un ni l'autre par son nom.

**À faire.** Une classe `Journal`, abonnée au moteur, qui écrit dans le
fichier indiqué par la clé `journal` de la configuration, au format du
contrat (`INTERFACES.md` §5) :

```text
2026-10-15T09:12:00.004Z START version=0.3.0 groupe=gp1
2026-10-15T09:12:21.502Z ALARME ch=0 nom=temperature niveau=HAUT valeur=8.06
2026-10-15T09:12:44.000Z NORMAL ch=0 nom=temperature valeur=6.97
2026-10-15T09:13:10.220Z STOP raison=signal
```

- une ligne par événement : horodatage, type, puis des paires `clé=valeur`
  séparées par un espace ;
- `START` au démarrage, `STOP` à l'arrêt, toujours en **dernière** ligne ;
- les mesures ordinaires ne vont **pas** au journal (elles partent sur
  MQTT).

**Pièges.**

- **L'heure** : en **UTC**, avec les **millisecondes** et le `Z` final.
  L'heure locale décale tout d'une ou deux heures ; le changement d'heure du
  25 octobre rendrait le journal incohérent.
- **Le fichier ouvert en écriture simple** est vidé à chaque démarrage :
  l'historique de la veille disparaît. Ouvrez-le **en ajout**.
- **Les lignes qui restent en mémoire** : un flux C++ garde les octets un
  moment avant de les écrire. Pendant que la passerelle tourne, `grep` ne
  voit rien, et un `kill -9` les perd. Videz le flux après chaque ligne
  (`std::flush` ou `std::endl`).
- **Les nombres** s'écrivent comme dans le JSON de la Phase 2 : un point
  décimal, jamais une virgule ; `1`, pas `1.000000`. Réutilisez la même
  fonction.
- Un journal impossible à ouvrir (dossier inexistant, droits) est une
  erreur de configuration : code 2.

**Les tests.** Le format d'une ligne, **à l'octet près**, pour chaque type
d'événement ; l'horodatage d'un instant connu (par exemple 0 ms donne
`1970-01-01T00:00:00.000Z`). Pour que ce soit testable, donnez l'heure au
journal au lieu de la lui faire lire.

---

## Étape 3 — MQTT, second abonné : alarmes et pannes

**À faire.** Un second abonné publie chaque événement (`INTERFACES.md` §4) :

| Topic | QoS | Retenu | Contenu |
|---|---|---|---|
| `embedgate/<gp>/alarme/<ch>` | 1 | **oui** | `{"ch":0,"nom":"temperature","etat":"ALARME","niveau":"HAUT","valeur":8.31,"ts":…}` |
| | | | retour : `"etat":"NORMAL","niveau":null` |
| `embedgate/<gp>/panne/<ch>` | 1 | **oui** | `{"ch":1,"panne":true,"ts":…}` · rétabli : `"panne":false` |

**Message retenu, en une image.** Le broker garde le **dernier** message de
chaque topic retenu et le remet aussitôt à tout nouvel abonné, comme un
panneau « porte ouverte » qu'on laisse affiché. Un superviseur lancé en
retard voit tout de suite qu'une alarme est en cours. Dans
`mosquitto_publish`, c'est le dernier argument (`retain = true`).

**Pièges.**

- **`"niveau":"null"` ou `"niveau":""`** : c'est un texte. Le JSON veut
  `null`, **sans guillemets**.
- **QoS 1 pour les alarmes et les pannes**, 0 pour les mesures : c'est
  l'avant-dernier argument de `mosquitto_publish`.
- **Tester sans broker** : si l'abonné reçoit une fonction « publier »
  (une `std::function`) plutôt qu'un `ClientMqtt`, le test lui en donne une
  qui range les messages dans un vecteur, et vérifie le topic, la QoS, le
  drapeau « retenu » et le JSON exact.

**Vérifier**, une fois la chaîne assemblée (étape 5) :

```bash
mosquitto_sub -t 'embedgate/gp1/alarme/#' -v      # pendant la chambre froide
```

---

## Étape 4 — La file thread-safe

**L'image.** Une poste. Au **guichet**, un agent reçoit les colis : il ne
fait jamais attendre la file des clients. Dans le **bureau**, un autre les
ouvre, les trie, remplit les registres. Entre les deux, une **étagère** : le
guichetier y pose, le trieur y prend. La file, c'est l'étagère.

**À faire.** Une classe de file (un *template*, ou une file de votre type de
mesure) protégée par un `std::mutex`, avec trois opérations :

- **déposer** : prend le verrou, ajoute, rend le verrou, puis **réveille**
  le consommateur (`notify_one`) ;
- **retirer** : attend qu'il y ait quelque chose, sans consommer de
  processeur, grâce à une `std::condition_variable` ; renvoie l'élément,
  ou « rien » (`std::optional` vide) quand la file est fermée **et** vide ;
- **fermer** : plus aucun dépôt accepté, et **tous** les consommateurs qui
  attendent sont réveillés (`notify_all`).

**Pièges.**

- **Un accès à la file sans le verrou**, même pour une simple lecture
  (`size()`, `empty()`) : c'est une *data race*, deux threads qui touchent
  la même donnée au même instant. Le programme marche… presque toujours, et
  plante une fois sur mille. ThreadSanitizer (étape 8) les trouve.
- **`wait` sans condition** : `cv.wait(verrou)` seul peut se réveiller sans
  raison (la norme l'autorise), et `front()` sur une file vide plante.
  Écrivez `cv.wait(verrou, [&]{ return !file.empty() || fermee; })` : la
  condition est revérifiée à chaque réveil.
- **Oublier de réveiller à la fermeture** : le consommateur dort pour
  toujours devant une étagère vide, et `Ctrl-C` ne termine jamais le
  programme.
- **Fermer en jetant ce qui reste** : les dernières alarmes seraient
  perdues. À la fermeture, le consommateur vide d'abord la file.

**Une file sans limite** est acceptée pour cette phase. Sur une carte à la
mémoire limitée, on la borne, et on **compte** ce qu'on refuse quand elle
est pleine : déposer ne doit jamais faire attendre le thread d'acquisition.

**Les tests** (`tests/test_file_attente.cpp`) :

- ordre : premier déposé, premier retiré ;
- fermée : on retire ce qui reste, puis « rien » ;
- `fermer()` réveille un consommateur qui attend dans un autre thread (sans
  quoi le test ne se termine jamais : c'est le signe de l'erreur) ;
- un thread dépose 100 000 nombres, un autre les retire : aucun perdu,
  aucun dans le désordre.

---

## Étape 5 — Deux threads dans `main`

**À faire.**

- Le **thread d'acquisition** sert les interruptions, comme en Phase 2. Son
  rappel de mesure ne fait **qu'une chose** : déposer dans la file le canal,
  la **valeur brute** et l'**heure de la mesure**. Ni conversion, ni JSON,
  ni réseau, ni disque.
- Le **thread de traitement** retire de la file, convertit, publie la
  mesure sur MQTT (comme en Phase 2), puis la donne au moteur d'alarmes.
- L'un des deux peut être le thread principal de `main` ; l'autre est un
  `std::thread`.

**Pourquoi.** L'agenda le dit : « recevoir ne doit jamais attendre le
traitement ». Chaque milliseconde passée dans le rappel (un disque lent, un
réseau encombré) est une milliseconde pendant laquelle la puce n'est pas
servie, et la mesure suivante écrase la précédente. La file absorbe les
à-coups.

**Pièges.**

- **L'heure de l'événement** est celle de la **mesure**, prise dans le
  rappel, et non celle où le traitement a eu le temps de s'en occuper.
- **Un `std::thread` encore joignable qui est détruit**, c'est
  `std::terminate` : `terminate called without an active exception`, code
  134, pas de `STOP` au journal. Tout `std::thread` est `join()` avant sa
  destruction, **y compris quand une exception sort du bloc** : confiez le
  `join` à un objet RAII (fermer la file, puis `join`, dans son
  destructeur).
- **Une exception qui sort d'un thread** termine aussi le programme. Dans
  la fonction du thread, attrapez tout, notez l'erreur (`ERREUR` au
  journal), et demandez l'arrêt.
- **L'ordre de destruction** : les objets de `main` sont détruits dans
  l'ordre **inverse** de leur création. Un abonné détruit avant le moteur
  qui le prévient encore, c'est un plantage à l'arrêt. Déclarez le journal
  et l'abonné MQTT **avant** le moteur.
- **Aucun `new` / `delete` dans votre code** (critère de l'agenda) :
  `std::make_unique`, des conteneurs, des objets par valeur. Le seul
  `delete` admis est celui de `= delete` (copie interdite).

**Vérifier.** La chaîne de la Phase 2 marche toujours (les mesures
défilent dans `mosquitto_sub`), et la chambre froide écrit ses alarmes :

```bash
rm -f embedgate.log
build/sim_capteurs --scenario scenarios/chambre-froide.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini       # 60 s
grep -E 'ALARME|NORMAL' embedgate.log                # 4 lignes, ni plus, ni moins
```

Pour aller plus vite pendant la mise au point : `--vite 4` sur la puce (le
scénario dure 15 s, les instants sont divisés par 4).

---

## Étape 6 — L'arrêt propre

**L'image.** Fermer la poste le soir : on ferme le guichet, on réveille le
trieur qui s'est assoupi devant l'étagère vide, il finit les colis
restants, on attend qu'il ait fini, on note l'heure de fermeture au
registre, on éteint.

**À faire.** Sur `SIGINT` (`Ctrl-C`) ou `SIGTERM`, dans cet ordre :

1. la boucle d'acquisition s'arrête ;
2. la file est **fermée** (ce qui réveille le traitement) ;
3. le traitement vide la file, puis sa boucle se termine ;
4. `join` ;
5. `STOP` au journal ;
6. code de sortie 0, en moins de 3 secondes.

Quand la puce s'arrête d'elle-même (fin du scénario), les deux
comportements sont acceptés : la passerelle s'arrête seule, avec `STOP` au
journal, ou elle attend un `Ctrl-C`.

**Le point délicat : à quel thread va le signal ?** Avec plusieurs threads,
le noyau délivre un signal à **n'importe lequel** de ceux qui ne le bloquent
pas : y compris votre thread de traitement, ou le thread réseau de
`libmosquitto`. Deux façons de s'en sortir :

- **la simple** : un drapeau d'arrêt `std::atomic<bool>`, levé par le
  gestionnaire de signal, et relu par la boucle d'acquisition à **chaque**
  tic du `timerfd`. Quel que soit le thread qui reçoit le signal, la boucle
  le voit au tic suivant, quelques millisecondes plus tard ;
- **la rigoureuse** : bloquer `SIGINT` et `SIGTERM` (`pthread_sigmask`)
  **avant** de créer le moindre thread, et les débloquer seulement dans le
  thread de la boucle. Les threads créés ensuite héritent du blocage :
  seul le thread de la boucle reçoit le signal, et son `read()` sur le
  `timerfd` est interrompu aussitôt.

**Pièges.**

- **Un `bool` ordinaire** lu par un thread et écrit par un autre : *data
  race* (ThreadSanitizer la signale), et en `-O2` la boucle peut ne
  **jamais** voir le changement. `std::atomic<bool>`, ou le masque de
  signaux.
- **Le gestionnaire de signal ne fait presque rien** : il lève le drapeau.
  Pas d'écriture au journal, pas de `notify`, pas de mutex : tout cela se
  fait dans `main`, après la boucle.

**Vérifier.**

```bash
rm -f embedgate.log
build/sim_capteurs --scenario scenarios/stable.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini & sleep 3
kill -TERM $!; wait $!; echo "code=$?"        # aussitôt : code=0
tail -1 embedgate.log                         # … STOP raison=signal
pkill sim_capteurs
```

---

## Étape 7 — Les pannes de capteur

**À faire.** Votre pilote de la Phase 1 sait déjà signaler qu'un canal
tombe en panne et qu'il revient. En Phase 2, ce rappel n'était branché à
rien : branchez-le. Le rappel dépose l'information dans la file, comme une
mesure ; le traitement la passe au moteur, qui prévient ses abonnés :

```text
2026-10-15T09:12:55.000Z PANNE ch=1 nom=pression
2026-10-15T09:13:05.000Z RETABLI ch=1 nom=pression
```

et sur `embedgate/<gp>/panne/1` (retenu) : `{"ch":1,"panne":true,"ts":…}`,
puis `"panne":false`.

**Une panne n'est pas une alarme.** La valeur d'un canal en panne est
figée : elle ne déclenche ni ne fait retomber aucune alarme.

**Piège.** Signaler la panne à **chaque** tic tant qu'elle dure : une seule
`PANNE`, un seul `RETABLI`, comme pour les alarmes.

**Vérifier.**

```bash
rm -f embedgate.log
build/sim_capteurs --scenario scenarios/panne-capteur.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini       # 30 s
grep -E 'PANNE|RETABLI|ALARME|NORMAL' embedgate.log  # PANNE ch=1 vers 5 s, RETABLI ch=1 vers 15 s, rien d'autre
```

---

## Étape 8 — Les outils : Valgrind, ThreadSanitizer, la rafale

Trois vérifications que l'œil ne fait pas. Corrigez ce qui en sort avant
de poser le tag.

**Valgrind** (fuites mémoire), comme en Phase 2 : voir la checklist.

**ThreadSanitizer** (« TSan ») surveille le programme pendant qu'il tourne
et signale chaque accès à une donnée partagée sans protection, avec les
**deux** endroits en conflit. Il faut une compilation à part, dans un
autre dossier :

```bash
cmake -S . -B build-tsan -DSANITIZE=thread && cmake --build build-tsan -j
ctest --test-dir build-tsan                  # un test qui trouve une data race échoue
```

Un rapport commence par `WARNING: ThreadSanitizer: data race`, puis montre
deux piles d'appels : « Write of size … by thread T1 » et « Previous read …
by main thread ». Cherchez dans chacune la première ligne qui est dans
**votre** code : ce sont les deux accès à protéger par le **même** mutex,
ou à remplacer par un `std::atomic`.

**La rafale** : `rafale.scn` produit une mesure toutes les **10 ms**, sur
les 4 canaux, pendant 30 s, soit 3 000 par canal. La passerelle doit tout
publier, sans bloquer, et s'arrêter proprement ensuite.

**Pièges.**

- **Servir les interruptions au rythme de la puce ne suffit pas.** Si votre
  `timerfd` tique toutes les 10 ms, la moindre irrégularité fait arriver
  deux mesures entre deux tics : la seconde écrase la première. Servez
  nettement plus vite que la puce ne produit (quelques millisecondes).
- **« TSan : unexpected memory mapping »**, ou TSan qui refuse de démarrer :
  ce n'est pas votre code, c'est le noyau de la machine. Lancez
  `sudo sysctl vm.mmap_rnd_bits=28`, puis recommencez.
- **Un traitement lent dans le rappel d'interruption** (MQTT, journal) ne
  se voit souvent pas avec `stable.scn` sur un PC rapide : c'est la rafale
  qui le révèle.

---

## Étape 9 — La checklist, `v0.3`, la démonstration

**À faire.**

1. Déroulez la checklist ci-dessous.
2. Recompilez pour ARM : le pilote et ses tests doivent **rester verts**
   (la passerelle n'y est toujours pas construite : c'est la Phase 5).
3. Fusionnez, posez le tag :

```bash
git switch main && git merge <votre-branche>      # main compile, tests verts
git tag v0.3 && git push origin main v0.3
```

4. Préparez **5 minutes** pour le bilan du vendredi à 16 h : la chambre
   froide lancée en direct (avec `--vite 4`, elle dure 15 s), le journal et
   ses 4 lignes, les alarmes dans `mosquitto_sub`. Prévoyez qui parle et
   qui tape.

Une ligne dans `JOURNAL.md` chaque soir. Le tag est un **repère** : posé
lundi matin, il n'est pas pénalisé.

---

## Checklist de fin de phase

À dérouler vous-mêmes avant de poser le tag, broker lancé. Ce sont les
critères de l'agenda. Remplacez `gp1` par votre groupe. Pour ne pas
mélanger les essais, le journal est effacé avant chacun.

```bash
# 1. Compilation (aucun avertissement : -Werror) et tests
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
cmake --build build-arm -j && ctest --test-dir build-arm     # la Phase 1 reste verte sur ARM

# 2. Configuration invalide : code 2
build/embedgate-gw --config /nexiste/pas.ini; echo "code=$?"                 # code=2
sed 's/^haut *= *8.0$/haut = abc/' config/gateway.ini > /tmp/seuil-faux.ini
build/embedgate-gw --config /tmp/seuil-faux.ini; echo "code=$?"              # code=2

# 3. La chambre froide : LE critère de la phase (60 s)
rm -f embedgate.log
build/sim_capteurs --scenario scenarios/chambre-froide.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini; echo "code=$?"   # s'arrête seule (ou Ctrl-C) ; code=0
grep -E 'ALARME|NORMAL' embedgate.log
#   exactement 4 lignes : porte ≈ 10 s et ≈ 14 s après START, température ≈ 21 s et ≈ 44 s
mosquitto_sub -t 'embedgate/gp1/alarme/#' -v -C 2
#   2 messages RETENUS, reçus aussitôt : "etat":"NORMAL","niveau":null, canaux 0 et 3

# 4. Arrêt propre sur SIGTERM
rm -f embedgate.log
build/sim_capteurs --scenario scenarios/stable.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini & sleep 3
kill -TERM $!; wait $!; echo "code=$?"                        # aussitôt : code=0
tail -1 embedgate.log                                         # … STOP raison=…
pkill sim_capteurs

# 5. Les pannes, sous Valgrind (30 s)
rm -f embedgate.log
build/sim_capteurs --scenario scenarios/panne-capteur.scn --silencieux & sleep 1
timeout --foreground -s INT 45 valgrind --leak-check=full build/embedgate-gw --config config/gateway.ini 2>&1 \
  | grep -E "definitely lost|indirectly lost|ERROR SUMMARY"   # 0 bytes, 0 errors
grep -E 'PANNE|RETABLI|ALARME|NORMAL' embedgate.log           # PANNE ch=1 puis RETABLI ch=1, rien d'autre

# 6. La rafale : une mesure toutes les 10 ms (30 s)
rm -f embedgate.log
timeout --foreground 40 mosquitto_sub -t 'embedgate/gp1/mesure/+' -v > /tmp/rafale.txt &
build/sim_capteurs --scenario scenarios/rafale.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini; echo "code=$?"   # s'arrête seule (ou Ctrl-C) ; code=0
wait
cut -d' ' -f1 /tmp/rafale.txt | sort | uniq -c                # environ 3000 par canal, 2940 au moins
grep -c ALARME embedgate.log                                  # 0

# 7. ThreadSanitizer : aucune data race
cmake -S . -B build-tsan -DSANITIZE=thread && cmake --build build-tsan -j
ctest --test-dir build-tsan                                   # tous verts
build/sim_capteurs --scenario scenarios/rafale.scn --silencieux & sleep 1
timeout --foreground -s INT 10 build-tsan/embedgate-gw --config config/gateway.ini 2>&1 \
  | grep -c "WARNING: ThreadSanitizer"                        # 0
pkill sim_capteurs

# 8. Aucun new / delete nu
grep -rnwE 'new|delete' passerelle/ | grep -v '= *delete'     # rien (hors commentaires)
```

Pour Valgrind, regardez « definitely lost » et « indirectly lost » : les
deux à 0. Si aucune de ces deux lignes n'apparaît, Valgrind a écrit « All
heap blocks were freed » : c'est encore mieux.

- [ ] `passerelle/Version.cpp` annonce `0.3.0` (la ligne `START` du journal le montre)
- [ ] aucun avertissement à la compilation ; les Phases 1 et 2 sont toujours vertes, sur PC et sur ARM
- [ ] un fichier de tests du moteur d'alarmes, dont « une valeur bruitée autour du seuil ne déclenche qu'une seule alarme » ; un test de la file à deux threads ; tout est vert
- [ ] le moteur ne connaît ni le journal ni MQTT (Observer) ; la règle est interchangeable (Strategy)
- [ ] configuration invalide : code 2
- [ ] chambre froide : exactement 4 lignes `ALARME` / `NORMAL`, aux bons instants ; `START` en tête, `STOP` en dernière ligne
- [ ] les mêmes 4 événements sur `embedgate/<gp>/alarme/<ch>`, retenus, `"niveau":null` au retour
- [ ] chaque ligne du journal au format du contrat : UTC, millisecondes, `Z`
- [ ] `SIGTERM` : arrêt en moins de 3 s, code 0, `STOP` en dernière ligne
- [ ] `panne-capteur.scn` : une `PANNE` puis un `RETABLI` du canal 1, aucune alarme
- [ ] Valgrind : aucune erreur, aucune fuite
- [ ] `rafale.scn` : environ 3000 messages par canal, aucun blocage, arrêt propre
- [ ] ThreadSanitizer : aucune *data race*, ni dans les tests ni dans la passerelle
- [ ] aucun `new` / `delete` dans `passerelle/`
- [ ] `main` à jour, tag `v0.3` poussé, `JOURNAL.md` tenu, démonstration de 5 minutes prête

---

## Si vous êtes bloqués

| Symptôme | Où regarder |
|---|---|
| des dizaines d'alarmes température au lieu d'une | étape 1 : hystérésis absente, ou alarme annoncée à chaque mesure au lieu de chaque changement d'état |
| une seule alarme température, mais vers 24 s au lieu de 21 s | étape 1 : l'hystérésis est appliquée au déclenchement ; on déclenche à `haut` |
| l'alarme température retombe quand la porte se referme | étape 1 : un état par canal |
| les instants du journal sont décalés d'une ou deux heures | étape 2 : l'heure doit être en UTC |
| `grep` ne trouve rien pendant que la passerelle tourne | étape 2 : le flux n'est pas vidé après chaque ligne |
| le journal de la veille a disparu | étape 2 : ouvrir le fichier en ajout |
| `"niveau":"null"` ou `"niveau":""` | étape 3 : `null` sans guillemets |
| un superviseur lancé après coup ne voit pas l'alarme | étape 3 : message non retenu |
| `Ctrl-C` ne termine plus la passerelle | étape 4 : `fermer()` sans `notify_all` ; étape 6 : drapeau `bool` ordinaire, ou signal reçu par un autre thread |
| `terminate called without an active exception`, code 134 | étape 5 : un `std::thread` détruit sans `join` |
| plantage à l'arrêt, Valgrind « Invalid read » | étape 5 : un abonné détruit avant le moteur (ordre de déclaration dans `main`) |
| plantage rare, impossible à reproduire | étape 4 : accès à la file sans verrou, ou `wait` sans condition ; lancez TSan |
| `WARNING: ThreadSanitizer: data race` | étape 8 : lire les deux piles, protéger les deux accès par le même mutex |
| TSan : « unexpected memory mapping » | étape 8 : `sudo sysctl vm.mmap_rnd_bits=28` |
| aucune `PANNE` au journal | étape 7 : le rappel de panne du pilote est-il branché ? |
| plusieurs `PANNE` de suite | étape 7 : n'annoncer que le changement |
| moins de 2940 messages par canal sous rafale | étape 8 : servir les interruptions plus souvent que toutes les 10 ms ; rien de lent dans le rappel |
| la commande de la checklist n'affiche rien | il manque `--foreground` après `timeout` (correctif de la Phase 2) |
| des événements en double, des instants incohérents | une seule puce et une seule passerelle à la fois : `pkill sim_capteurs; pkill embedgate-gw` ; et `rm -f embedgate.log` avant chaque essai |

Pour demander de l'aide, dites ce que vous **attendiez** et ce qui s'est
**réellement** passé, avec la commande et sa sortie : c'est le plus court
chemin vers la réponse.
