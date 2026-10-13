# EmbedGate — Phase 2 pas à pas : la chaîne complète, en fin

**Mercredi 14 octobre 2026 · tag `v0.2`**

> Ce document détaille la Phase 2 de l'[agenda](agenda-phases-embedgate.md) :
> il ne demande rien de plus, il découpe le travail en étapes, avec pour
> chacune **quoi faire**, **comment vérifier** et **les pièges**. Le contrat
> ([`squelette/INTERFACES.md`](squelette/INTERFACES.md)) fait foi : en cas de
> contradiction, c'est lui qui a raison, et vous le signalez.

---

## Où vous allez

Votre pilote sait lire la puce. Aujourd'hui, **une mesure fait tout le
trajet**, de la puce jusqu'à l'écran d'un opérateur :

```text
puce  →  pilote C  →  passerelle C++ (embedgate-gw)  →  broker MQTT  →  superviseur Python
```

Le minimum à chaque maillon, mais d'un bout à l'autre. Pas d'alarmes, pas de
journal, pas de threads : c'est la Phase 3. À partir de ce soir, vous ne
ferez plus qu'épaissir une chaîne qui marche.

À la fin de la journée, vous devez pouvoir montrer ceci, dans quatre
terminaux :

```bash
build/sim_capteurs --scenario scenarios/stable.scn          # 1. la puce
build/embedgate-gw --config config/gateway.ini              # 2. votre passerelle
mosquitto_sub -t 'embedgate/gp1/#' -v                       # 3. ce qui circule
# embedgate/gp1/mesure/0 {"ch":0,"nom":"temperature","valeur":4.12,"unite":"degC","ts":1760263351123}
# embedgate/gp1/mesure/1 {"ch":1,"nom":"pression","valeur":1013.21,"unite":"mbar","ts":1760263351123}
# …
python3 superviseur/superviseur.py --config config/gateway.ini   # 4. l'écran de l'opérateur
```

(`gp1` est à remplacer partout par **votre** groupe.)

### Ce que vous écrivez, ce qui est fourni

| Vous écrivez | Fourni (ne pas modifier) |
|---|---|
| `passerelle/` : vos classes C++ (un `.hpp` et un `.cpp` par classe) | `config/gateway.ini` : changez seulement `groupe` |
| `passerelle/main.cpp` : le programme `embedgate-gw` | `passerelle/Version.hpp` (et `Version.cpp`, à passer à `0.2.0`) |
| `superviseur/superviseur.py` | `tests/Verif.hpp` : l'outil de tests C++ du Projet C++ |
| `tests/test_*.cpp` : les tests de vos classes | `tests/test_squelette.cpp` : un exemple |
| | `CMakeLists.txt`, le broker Mosquitto (un paquet Debian) |

**L'organisation de votre code C++ est libre** (noms de classes, de
fichiers). Ce qui est imposé : le nom du programme et son option
`--config`, ses codes de sortie, les topics et le format des messages.

CMake compile tout seul ce que vous ajoutez : tout fichier
`passerelle/*.cpp` (sauf `main.cpp`) entre dans la bibliothèque
`passerelle_lib`, et chaque `tests/test_xxx.cpp` devient le programme
`build/test_xxx`, lancé par `ctest`. Après avoir **créé** un fichier,
relancez `cmake -S . -B build`.

### Les bibliothèques autorisées

Votre dépôt doit compiler tel quel sur une Debian propre, avec **uniquement
les paquets du `README.md` du squelette**, et aussi pour ARM en Phase 5.

- **MQTT côté C++ : `libmosquitto`** (l'API C), pas `libmosquittopp`.
  Écrire vous-mêmes la classe qui l'enveloppe est justement l'exercice
  (étape 5) : environ 70 lignes.
- **JSON** : à la main (le format est petit et fixé), ou une bibliothèque
  *header-only* copiée dans votre dépôt avec sa licence, son dossier étant
  déclaré `SYSTEM` dans CMake (sinon `-Werror` s'applique à son code).
- **Python** : `paho-mqtt`, `pytest` et la bibliothèque standard.

Pour toute autre bibliothèque, demandez avant.

### Rythme conseillé

| Moment | Étapes | Vous êtes dans les temps si… |
|---|---|---|
| 9 h 45 – 10 h 30 | 0, 1 | vous savez publier et lire un message à la main ; une configuration absente donne le code 2 |
| 10 h 30 – 12 h 30 | 2, 3, 4 | votre `main` **affiche** à l'écran le JSON de chaque mesure (pas encore de MQTT) |
| 13 h 30 – 15 h | 5, 6 | `mosquitto_sub` montre vos 4 topics |
| 15 h – 17 h 15 | 7, 8, 9 | superviseur, tests, checklist, tag `v0.2` |

**Répartition conseillée dans le binôme** : l'un porte la passerelle C++,
l'autre le superviseur Python **dès le matin**, puis les tests. Le
superviseur se développe sans attendre la passerelle : on lui envoie à la
main un message du contrat avec `mosquitto_pub` (étape 7). Vous vous
rejoignez l'après-midi. Changez de rôle par rapport à la Phase 1.

Si à **14 h** rien n'apparaît dans `mosquitto_sub`, dites-le : c'est presque
toujours le topic (groupe oublié), le broker qui ne tourne pas, ou le
thread réseau de `libmosquitto` qui n'est pas démarré (étape 5).

---

## Étape 0 — Le broker, à la main (première demi-heure, sans coder)

**MQTT en une image.** Un tableau d'affichage à rubriques. Le **broker**
(Mosquitto) tient le tableau. Un programme **publie** un message dans une
rubrique (un ***topic***, par exemple `embedgate/gp1/mesure/0`) ; tous ceux
qui sont **abonnés** à cette rubrique le reçoivent. Celui qui publie et
celui qui lit ne se connaissent pas : c'est ce qui permettra de déplacer la
passerelle sur la carte ARM sans toucher au superviseur.

**À faire.**

1. Vérifiez que le broker tourne. Le paquet `mosquitto` l'installe comme un
   service, sur le port 1883 :

```bash
systemctl status mosquitto          # « active (running) »
sudo systemctl start mosquitto      # s'il est arrêté
mosquitto -d                        # machine sans systemd (conteneur) : le lancer à la main
```

2. Jouez avec deux terminaux :

```bash
mosquitto_sub -t 'essai/#' -v            # terminal 1 : on écoute (# = « tout ce qui suit »)
mosquitto_pub -t essai/bonjour -m 42     # terminal 2 : on publie
# le terminal 1 affiche :  essai/bonjour 42
```

3. Dans `config/gateway.ini`, mettez **votre groupe** (`groupe = gp3`…) et
   ne changez rien d'autre. Dans `passerelle/Version.cpp`, passez la
   version à `0.2.0`.

**Pourquoi tout de suite.** `mosquitto_sub -t 'embedgate/gp1/#' -v` est
votre outil de diagnostic de toute la journée : il montre ce que votre
passerelle publie **réellement**. Si vos messages n'apparaissent pas,
essayez `mosquitto_sub -t '#' -v` : ils sont peut-être partis ailleurs.

---

## Étape 1 — Lire la configuration

**À faire.** `embedgate-gw --config FICHIER` lit le fichier `.ini` du
contrat (`INTERFACES.md` §3) et en tire ce dont la Phase 2 a besoin :

- `[passerelle]` : `groupe` (préfixe des topics), `bus` ;
- `[mqtt]` : `hote`, `port` ;
- `[canal.0]` à `[canal.3]` : `nom`, `unite`, `facteur`.

Les seuils (`bas`, `haut`, `hysteresis`) serviront en Phase 3 : vous pouvez
déjà les lire, ce n'est pas exigé aujourd'hui.

Le format est simple : des sections `[nom]`, des lignes `clé = valeur`, des
commentaires qui commencent par `#`, des lignes vides. Pensez à retirer les
espaces autour de la clé et de la valeur.

**L'idée.** Rien n'est écrit « en dur » dans le programme : on change de
broker ou de groupe sans recompiler. Un fichier absent, illisible ou
incomplet est une **erreur de configuration** : un message clair sur la
sortie d'erreur, et le **code de sortie 2**.

**Conseil.** Signalez cette erreur par une **exception** d'un type à vous
(par exemple `ErreurConfig`, qui dérive de `std::runtime_error`). `main`
l'attrapera et en fera le code 2 (étape 6). Une option manquante ou
inconnue sur la ligne de commande donne aussi le code 2.

**Vérifier.**

```bash
build/embedgate-gw --config /nexiste/pas.ini; echo "code=$?"     # un message, code=2
```

**Pièges.**

- `std::stod("8.0abc")` renvoie `8` sans se plaindre : vérifiez que **tout**
  le texte a été lu.
- Un port hors de 1 à 65535, un facteur nul, un groupe vide : mieux vaut
  les refuser ici que chercher une heure pourquoi rien n'arrive.

---

## Étape 2 — La classe qui possède le pilote (RAII)

**À faire.** Une classe C++ (appelons-la `Puce`) qui **possède** votre
pilote C :

- le **constructeur** ouvre la puce ; si elle est absente, il **lève une
  exception** (par exemple `PuceAbsente`) ;
- le **destructeur** remet la puce en veille et ferme le bus ;
- la **copie est interdite** (`Puce(const Puce&) = delete;` et de même pour
  `operator=`) ;
- des méthodes pour démarrer la puce et servir ses interruptions.

Comme en Phase 1, prévoyez un **second constructeur** sur une banque de
registres en mémoire : c'est lui qui rendra la classe testable sans la puce.

**L'image.** Une porte munie d'un groom : elle se referme toute seule,
qu'on sorte en marchant, en courant ou porté par les pompiers. Le
destructeur est appelé quand l'objet sort de sa portée, **quelle que soit
la sortie** : fin normale, `return` anticipé, exception. On ne peut pas
oublier de fermer. Et un objet `Puce` qui existe est forcément une puce
ouverte : il n'y a pas d'état « à moitié ouvert ».

**Ce qui n'est pas négociable.** `main` ne contient **aucun** appel à la
fonction de fermeture de votre pilote. S'il y en a un, la fermeture est
manuelle : elle sera oubliée sur `Ctrl-C` ou sur une erreur, et la puce
restera en marche.

**Vérifier.** Par un test (étape 8) : après la sortie de portée de l'objet,
le bit `ENABLE` de la banque est revenu à 0.

**Pièges.**

- **`undefined reference to pilote_…` à l'édition de liens** : le C++ ne
  retrouve pas vos fonctions C, parce qu'il « décore » les noms. Encadrez
  les déclarations de `pilote.h` ainsi (le fichier fourni `bus.h` montre
  l'exemple) :

```c
#ifdef __cplusplus
extern "C" {
#endif
/* … vos déclarations … */
#ifdef __cplusplus
}
#endif
```

- **Un destructeur ne lève jamais d'exception.** Il n'appelle ici que des
  fonctions C, qui n'en lèvent pas : ignorez leurs codes d'erreur.
- **`pilote.h` doit se compiler en C++** : certaines écritures du C y sont
  refusées (conversion implicite depuis `void *`, par exemple). Si le
  compilateur C++ se plaint de votre en-tête, corrigez l'en-tête.

---

## Étape 3 — Recevoir les mesures : du callback C au C++

**Le problème.** Votre pilote prévient par un **pointeur de fonction C**. La
passerelle voudrait, pour chaque mesure, appeler du code C++ qui connaît la
configuration et la connexion MQTT. Or une méthode de classe ou une lambda
qui capture des variables n'est **pas** un pointeur de fonction C.

**La solution : le contexte.** C'est à cela que sert le `void *contexte`
prévu en Phase 1. Vous donnez au pilote :

- une **fonction ordinaire** (ou une méthode `static`), au format qu'il
  attend ;
- comme contexte, **l'adresse de l'objet C++** à prévenir.

La fonction reconvertit le contexte (`static_cast<MaClasse*>(contexte)`) et
appelle la méthode voulue. On appelle cette petite fonction un
*trampoline* : le C rebondit dessus pour arriver dans le C++.

Vous pouvez aller plus loin (la classe `Puce` accepte une `std::function`,
donc une lambda) : c'est plus souple, pas exigé.

**Piège : une exception ne traverse jamais du code C.** Si le code appelé
depuis le trampoline lève une exception, elle remonterait à travers votre
pilote C, qui ne sait pas la propager : le comportement est indéfini
(souvent un plantage, code 134). Dans le trampoline, encadrez l'appel d'un
`try` / `catch (...)` : notez l'erreur, arrêtez la boucle, et traitez-la
une fois revenus côté C++.

**Vérifier.** Un `main` provisoire qui affiche chaque mesure reçue :

```bash
build/sim_capteurs --scenario scenarios/stable.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini       # 4 canaux qui défilent, comme pilote_demo
```

---

## Étape 4 — Convertir et mettre en forme (JSON)

**À faire.** Pour chaque mesure servie :

1. **convertir** la valeur brute en unité physique : `valeur = brut ×
   facteur` (le facteur du canal, lu dans la configuration). `4120` devient
   `4.12` °C ;
2. **horodater** : `ts` = millisecondes depuis le 1er janvier 1970
   (`std::chrono::system_clock`) ;
3. **écrire le JSON du contrat**, sur une ligne (`INTERFACES.md` §4) :

```json
{"ch":0,"nom":"temperature","valeur":4.12,"unite":"degC","ts":1760263351123}
```

**JSON en une image.** Un formulaire : des cases nommées, et dans chaque
case un texte (entre guillemets) ou un nombre (sans guillemets). Le
superviseur relit ce formulaire sans rien savoir de votre C++.

**Pièges.**

- **`"valeur":"4.12"`** : avec des guillemets, c'est un texte, pas un
  nombre. `ch`, `valeur` et `ts` sont des **nombres**.
- **La virgule décimale** : `4,12` n'est pas du JSON. Sur une machine
  réglée en français, certaines écritures produisent une virgule. Avec un
  flux, imposez la langue neutre : `flux.imbue(std::locale::classic());`.
- **Les chiffres perdus** : par défaut, un flux C++ n'écrit que 6 chiffres
  significatifs, et 1012,995 mbar devient `1013`. Demandez-en davantage
  (`std::setprecision(10)`).
- **`4.1200000000000001`** : arrondissez au millième, le brut n'est pas
  plus précis.
- **Une température négative reste négative** : gardez un `int32_t` jusqu'à
  la conversion.
- **Un `ts` en secondes**, ou depuis le démarrage du programme : il doit
  avoir 13 chiffres aujourd'hui.

**Vérifier.** Votre `main` provisoire affiche le JSON de chaque mesure.
Valeurs attendues avec `stable.scn` : environ `4` (°C), `1013` (mbar),
`500` (mA), `0` (porte). Si vous voyez `4000` ou `1013000`, la conversion
manque. Collez une de vos lignes dans `python3 -m json.tool` : il doit la
réafficher sans erreur.

---

## Étape 5 — La classe qui possède la connexion MQTT (RAII, encore)

**À faire.** Une classe (appelons-la `ClientMqtt`) qui enveloppe
`libmosquitto`, sur le même modèle que `Puce` : le constructeur se connecte
(ou lève une exception, par exemple `BrokerInjoignable`), le destructeur
se déconnecte et libère, la copie est interdite, une méthode publie.

Les fonctions de `libmosquitto` dont vous avez besoin (`#include
<mosquitto.h>`, documentation : `man libmosquitto`) :

| Moment | Fonctions, dans l'ordre |
|---|---|
| une fois par programme | `mosquitto_lib_init()` au début, `mosquitto_lib_cleanup()` à la fin |
| constructeur | `mosquitto_new(identifiant, true, nullptr)`, `mosquitto_connect(m, hote, port, 30)`, `mosquitto_loop_start(m)` |
| publier | `mosquitto_publish(m, nullptr, topic, taille, message, qos, retenu)` |
| destructeur | `mosquitto_disconnect(m)`, `mosquitto_loop_stop(m, false)`, `mosquitto_destroy(m)` |

Chacune renvoie `MOSQ_ERR_SUCCESS` quand tout va bien ;
`mosquitto_strerror(code)` donne le texte d'une erreur.

**Ce qu'il faut comprendre.**

- **`mosquitto_loop_start` lance un thread** de la bibliothèque : c'est lui
  qui envoie réellement les messages sur le réseau, et qui se **reconnecte
  tout seul** si le broker redémarre. Sans lui, `mosquitto_publish` range
  les messages dans une file que personne ne vide.
- **L'identifiant du client doit être unique** sur le broker (par exemple
  `embedgate-gw-gp3`). Deux clients du même nom se déconnectent l'un
  l'autre en boucle.
- **Un échec de publication n'arrête pas la passerelle** : `publier`
  renvoie `false`, vous le comptez, vous continuez. Le broker reviendra.

**Piège : le constructeur qui échoue à mi-chemin.** Si `mosquitto_connect`
échoue, vous levez une exception. Mais le C++ **n'appelle pas le
destructeur** d'un objet dont le constructeur n'a pas fini : ce que
`mosquitto_new` a créé ne serait jamais libéré. Deux solutions : appeler
`mosquitto_destroy` avant de lever l'exception, ou confier le pointeur à un
`std::unique_ptr` muni de `mosquitto_destroy` comme fonction de libération
(un membre déjà construit, lui, est toujours détruit).

**Vérifier.** Broker arrêté, puce lancée :

```bash
sudo systemctl stop mosquitto
build/embedgate-gw --config config/gateway.ini; echo "code=$?"   # un message, code=4
sudo systemctl start mosquitto
```

---

## Étape 6 — Assembler `main` : ordre, signaux, codes de sortie

**À faire.** Le programme imposé par `INTERFACES.md` §6 :

```text
build/embedgate-gw --config config/gateway.ini
```

Dans l'ordre : lire la configuration, créer la puce, créer le client MQTT,
démarrer la puce, puis servir les interruptions à intervalle régulier en
publiant **chaque mesure** sur `embedgate/<groupe>/mesure/<canal>` (QoS 0,
non retenu), jusqu'à ce qu'on demande l'arrêt.

- **La boucle.** La même qu'en Phase 1 : un `timerfd`, un passage du
  service d'interruption à chaque tic, un drapeau d'arrêt. Si elle n'existe
  que dans `pilote_demo.c`, c'est le moment d'en faire une fonction du
  pilote, ou de la récrire dans la passerelle.
- **L'arrêt.** `SIGINT` et `SIGTERM` lèvent un drapeau
  (`volatile std::sig_atomic_t`), installé avec `sigaction`, sans
  `SA_RESTART`. La puce qui s'arrête (`ID` à 0) termine aussi la boucle,
  normalement.
- **Les codes de sortie.**

| Situation | Code |
|---|---|
| arrêt normal (`Ctrl-C`, `SIGTERM`, puce arrêtée) | 0 |
| configuration invalide, ligne de commande invalide | 2 |
| puce absente (bus introuvable ou mauvais `ID`) | 3 |
| broker injoignable au démarrage | 4 |

**L'idée.** Un seul bloc `try` dans `main`, qui construit les trois objets
et fait tourner la boucle ; un `catch` par type d'exception, qui affiche le
message et renvoie le bon code. Les objets sont détruits **automatiquement,
dans l'ordre inverse** de leur création : d'abord la connexion MQTT, puis
la puce. Vous n'écrivez aucun nettoyage. En Phase 5, ces codes diront au
système de la carte pourquoi le service n'a pas démarré.

Tous les messages du programme (démarrage, bilan, erreurs) vont sur la
**sortie d'erreur**.

**Vérifier.**

```bash
build/sim_capteurs --scenario scenarios/stable.scn --silencieux & sleep 1
build/embedgate-gw --config config/gateway.ini &
timeout --foreground 5 mosquitto_sub -t 'embedgate/gp1/#' -v | cut -d' ' -f1 | sort | uniq -c
#   environ 50 messages pour chacun des 4 topics embedgate/gp1/mesure/0 … 3
kill -INT %2; wait %2; echo "code=$?"                # arrêt immédiat, code=0
kill %1
```

| Vous voyez | Ce que ça veut dire |
|---|---|
| 4 topics, environ 50 messages chacun | c'est juste |
| rien, mais `mosquitto_sub -t '#' -v` montre des messages | mauvais topic : groupe oublié, faute de frappe |
| rien du tout | broker arrêté, ou `mosquitto_loop_start` oublié (étape 5) |
| un seul topic | le numéro du canal n'est pas dans le topic |
| dix fois trop de messages | l'acquittement de la Phase 1 est cassé |

---

## Étape 7 — Le superviseur Python minimal

**À faire.** `superviseur/superviseur.py`, lancé ainsi (`INTERFACES.md` §6) :

```text
python3 superviseur/superviseur.py --config config/gateway.ini
```

- il lit le **même** fichier de configuration que la passerelle (module
  `configparser`) : le groupe, l'adresse et le port du broker ;
- il s'abonne à `embedgate/<groupe>/mesure/+` (`+` = « n'importe quel
  canal ») ;
- il décode chaque message (`json.loads`) et **affiche les mesures** sur la
  sortie standard : au moins le nom, la valeur et l'unité de chaque canal.
  Par exemple, une ligne par seconde avec la dernière valeur de chacun :

```text
[11:17:28] temperature 3.711 degC | pression 1013.35 mbar | courant 508.678 mA | porte 0 bool
```

- un message qui ne respecte pas le contrat est **signalé et ignoré** : il
  n'arrête pas le superviseur ;
- `Ctrl-C` l'arrête proprement.

Le vrai tableau de bord (alarmes, états, commandes) est pour la Phase 4.

**Développer sans attendre la passerelle.** Le superviseur lancé, publiez à
la main un message du contrat :

```bash
mosquitto_pub -t embedgate/gp1/mesure/0 \
  -m '{"ch":0,"nom":"temperature","valeur":4.12,"unite":"degC","ts":1760263351123}'
```

**Pièges.**

- **Deux versions de `paho-mqtt`.** Debian 12 fournit la 1.6 ; `pip`
  installe la 2.x, et la création du client a changé entre les deux
  (`AttributeError: CallbackAPIVersion`, ou un avertissement). Écrivez un
  code qui marche avec les deux :

```python
import paho.mqtt.client as mqtt

if hasattr(mqtt, "CallbackAPIVersion"):                                   # paho 2.x
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=identifiant)
else:                                                                     # paho 1.6
    client = mqtt.Client(client_id=identifiant)
```

  La signature de `on_connect` diffère aussi : déclarez-la
  `def on_connect(client, userdata, *args)`.
- **Abonnez-vous dans `on_connect`**, pas juste après `connect()` :
  l'abonnement sera refait tout seul après une reconnexion.
- **Rien ne s'affiche quand la sortie est redirigée** : `print(...,
  flush=True)`.
- **Les fonctions `on_message` tournent dans un autre thread** (celui de
  `loop_start()`). Si le programme principal lit ce qu'elles écrivent,
  protégez ces données par un `threading.Lock`.
- L'identifiant du superviseur doit être différent de celui de la
  passerelle.

---

## Étape 8 — Les tests (à écrire en même temps que le reste)

**À faire.** Des fichiers `tests/test_*.cpp`, avec l'outil fourni
`tests/Verif.hpp` (celui du Projet C++ ; `tests/test_squelette.cpp` montre
l'usage). Chaque maillon se teste **seul**, sans broker ni puce.

**Les tests attendus au minimum.**

- **Configuration** : le fichier de référence est lu correctement ; un
  fichier absent et une clé manquante lèvent votre exception.
- **Conversion et JSON** : une valeur brute donne la bonne valeur physique,
  y compris négative ; le JSON produit est **exactement** celui du contrat
  (comparez la chaîne entière).
- **La classe `Puce`**, sur une banque en mémoire, comme en Phase 1 : une
  banque sans signature est refusée par une exception ; en sortie de portée,
  `ENABLE` est revenu à 0, **y compris quand une exception fait sortir du
  bloc**.

Côté Python, les tests (`pytest`) seront exigés en Phase 4. Vous pouvez
commencer : placez la logique dans des fonctions (décoder un message,
former la ligne affichée) et testez-les dans
`superviseur/test_superviseur.py`, sans broker.

**Ce qu'on ne teste pas ici** : la connexion MQTT réelle. C'est la
vérification de bout en bout (étape 9) qui la couvre.

**Vérifier.**

```bash
ctest --test-dir build --output-on-failure     # tout est vert, Phase 1 comprise
python3 -m pytest -q superviseur/              # si vous avez des tests Python
```

---

## Étape 9 — Bout en bout, puis `v0.2`

**À faire.**

1. Déroulez la checklist ci-dessous.
2. Recompilez pour ARM : le pilote et ses tests doivent **rester verts**.
   La compilation ARM affiche maintenant un avertissement « libmosquitto
   introuvable » et ne construit pas la passerelle : c'est **normal**, la
   bibliothèque MQTT pour ARM sera installée en Phase 5.
3. Fusionnez, posez le tag :

```bash
git switch main && git merge <votre-branche>      # main compile, tests verts
git tag v0.2 && git push origin main v0.2
```

Une ligne dans `JOURNAL.md` ce soir. Le tag est un **repère** : posé jeudi
matin, il n'est pas pénalisé.

---

## Checklist de fin de phase

À dérouler vous-mêmes avant de poser le tag, broker lancé. Ce sont les
critères de l'agenda. Remplacez `gp1` par votre groupe.

```bash
# 1. Compilation (aucun avertissement : -Werror) et tests
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
cmake --build build-arm -j && ctest --test-dir build-arm     # la Phase 1 reste verte sur ARM

# 2. Les codes de sortie
build/embedgate-gw --config /nexiste/pas.ini; echo "code=$?"      # code=2
pkill sim_capteurs; rm -f /tmp/embedgate_bus.bin
build/embedgate-gw --config config/gateway.ini; echo "code=$?"    # code=3 (pas de puce)
build/sim_capteurs --scenario scenarios/stable.scn --silencieux & sleep 1
sed 's/^port.*/port = 1999/' config/gateway.ini > /tmp/sans-broker.ini
build/embedgate-gw --config /tmp/sans-broker.ini; echo "code=$?"  # code=4 (pas de broker sur ce port)

# 3. La chaîne complète
build/embedgate-gw --config config/gateway.ini &
timeout --foreground 5 mosquitto_sub -t 'embedgate/gp1/#' -v | cut -d' ' -f1 | sort | uniq -c
#   4 lignes, environ 50 chacune : embedgate/gp1/mesure/0 … 3
mosquitto_sub -t 'embedgate/gp1/mesure/+' -C 4 | python3 -m json.tool --json-lines
#   4 messages réaffichés sans erreur : ce sont des JSON valides
timeout 4 python3 superviseur/superviseur.py --config config/gateway.ini
#   les 4 mesures à l'écran, température ≈ 4

# 4. Arrêt propre
kill -INT %2; wait %2; echo "code=$?"                             # code=0
kill %1

# 5. Pas de fuite mémoire
build/sim_capteurs --scenario scenarios/stable.scn --silencieux & sleep 1
timeout --foreground -s INT 5 valgrind --leak-check=full build/embedgate-gw --config config/gateway.ini 2>&1 \
  | grep -E "definitely lost|indirectly lost|ERROR SUMMARY"
kill %1                                                           # 0 bytes, 0 errors
```

Pour Valgrind, regardez « definitely lost » et « indirectly lost » : les
deux à 0. Quelques octets « still reachable » laissés par une bibliothèque
du système sont sans importance.

- [ ] `config/gateway.ini` porte votre groupe, et rien d'autre n'y a changé
- [ ] aucun avertissement à la compilation ; la Phase 1 est toujours verte, sur PC et sur ARM
- [ ] au moins un fichier de tests C++ à vous, tout est vert
- [ ] le pilote et la connexion MQTT sont chacun possédés par un objet non copiable ; `main` ne ferme rien à la main
- [ ] codes de sortie : 2 (configuration), 3 (puce absente), 4 (broker injoignable)
- [ ] les 4 topics `embedgate/<gp>/mesure/<ch>`, environ 10 messages par seconde chacun
- [ ] chaque message est un JSON valide, avec les champs `ch`, `nom`, `valeur`, `unite`, `ts`
- [ ] les valeurs sont en unité physique (≈ 4 °C, pas 4000)
- [ ] le superviseur affiche les mesures
- [ ] `Ctrl-C` : arrêt immédiat de la passerelle, code 0
- [ ] Valgrind : aucune erreur, aucune fuite
- [ ] `main` à jour, tag `v0.2` poussé, `JOURNAL.md` tenu

---

## Si vous êtes bloqués

| Symptôme | Où regarder |
|---|---|
| `undefined reference to pilote_…` | étape 2 : `extern "C"` autour des déclarations de `pilote.h` |
| `mosquitto.h: No such file` | paquet `libmosquitto-dev` ; puis relancez `cmake -S . -B build` |
| la passerelle n'est pas compilée (« libmosquitto introuvable ») | sur le PC : même paquet ; sur ARM : normal jusqu'à la Phase 5 |
| code 4 alors que le broker devrait tourner | étape 0 : `systemctl status mosquitto` ; l'hôte et le port de la configuration |
| `mosquitto_sub` ne montre rien | étape 6 : le tableau des symptômes ; essayez `-t '#'` |
| la commande de comptage (`timeout 5 mosquitto_sub …` suivie de `sort` et `uniq -c`) n'affiche rien, alors que sans `sort` les messages défilent | dans un terminal, `timeout` arrête aussi les commandes placées après lui dans la ligne : écrivez `timeout --foreground 5 …` |
| des valeurs comme `4000` ou `1013000` | étape 4 : le facteur |
| le superviseur ignore tous les messages | étape 4 : `valeur` entre guillemets, ou virgule décimale |
| la passerelle et le superviseur se déconnectent sans arrêt | deux clients avec le même identifiant (étapes 5 et 7) |
| `AttributeError: CallbackAPIVersion` | étape 7 : les deux versions de `paho-mqtt` |
| plantage (code 134, `terminate called…`) | étape 3 : une exception traverse le pilote C ; ou une exception que `main` n'attrape pas |
| `Ctrl-C` ne rend pas la main | étape 6 : le drapeau, `sigaction` sans `SA_RESTART`, `EINTR` |
| la puce reste en marche après l'arrêt de la passerelle | étape 2 : le destructeur de `Puce` |
| des mesures incohérentes, des lignes en double | une seule puce et une seule passerelle à la fois : `pkill sim_capteurs; pkill embedgate-gw` |

Pour demander de l'aide, dites ce que vous **attendiez** et ce qui s'est
**réellement** passé, avec la commande et sa sortie : c'est le plus court
chemin vers la réponse.
