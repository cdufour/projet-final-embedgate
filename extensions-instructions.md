# EmbedGate — Les extensions pas à pas

> Ce document détaille les extensions de l'[agenda](agenda-phases-embedgate.md)
> (tableau « Extensions »). Il ne change rien au socle : les critères des
> phases et le contrat ([`squelette/INTERFACES.md`](squelette/INTERFACES.md))
> restent ceux d'origine. Une extension **ajoute** à la passerelle, elle ne
> modifie jamais ce que le contrat impose.

---

## Les règles communes

1. **Elles ne comptent que si le socle est complet et fusionné sur `main`**
   au tag `v1.0` (énoncé, B.7). Une extension brillante sur un socle
   incomplet ne rapporte rien.
2. **Quand commencer.** Une extension peut démarrer dès que la phase en
   cours est **validée sur `main`**, et seulement les extensions marquées
   « dès la P3 » ou « dès la P4 » dans le tableau ci-dessous. Elle ne doit
   jamais retarder la phase suivante.
3. **Une branche par extension.** `main` reste vert : après la fusion, tous
   les critères des phases déjà faites passent toujours.
4. **La configuration de référence ne change pas.** Une extension qui a
   besoin d'un réglage ajoute une clé **facultative** ; si elle est absente,
   la passerelle se comporte exactement comme avant. Pour vos essais :
   une copie (`config/gateway-e4.ini`…).
5. **Un porteur, un relecteur.** Le relecteur relit avant la fusion **et
   ajoute au moins un test**. À la soutenance, chacun peut être interrogé
   sur l'extension de l'autre.
6. **Montrable en 2 minutes.** Chaque extension a, dans le `README.md`, une
   section « Extension Ex » : comment la lancer, ce qu'on doit voir.

| Code | Extension | Dès | Durée indicative |
|---|---|---|---|
| E1 | Bus CAN : un 5ᵉ canal venu d'un nœud distant | la P3 | 1,5 à 2 jours |
| E2 | Tableau de bord web | la P4 | 1 jour |
| E3 | Historisation SQLite et export CSV | la P3 | 1 jour |
| E4 | Alarme temporisée | la P3 | une demi-journée |
| E5 | Qualité : CI locale, `cppcheck`, couverture, Doxygen | la P3 | une demi-journée à 1 jour |
| E6 | Paquet Debian, service `systemd` | la P5 | 1 jour |

**Le trinôme** réalise au moins **deux extensions, au choix**.

Toutes sont détaillées ci-dessous : E4, E3, E5, puis E1, E2, E6.

---

## E4 — L'alarme temporisée

**Le besoin.** Un livreur ouvre la porte de la chambre froide 30 secondes :
aujourd'hui, la passerelle déclenche une alarme à chaque livraison.
Résultat : l'opérateur ne regarde plus les alarmes de porte (la « fatigue
d'alerte » de l'énoncé). On veut qu'une anomalie ne devienne une alarme
que si elle **dure** plus de N secondes.

**La règle.**

- Une nouvelle clé **facultative** par canal : `delai` (secondes, réel).
  Absente ou `0` : comportement actuel, alarme immédiate.
- Avec `delai = N` : l'anomalie (dépassement de `haut` ou de `bas`) doit
  durer **N secondes sans interruption** avant l'événement `ALARME`.
- Si la valeur redevient normale avant N secondes : **rien** au journal,
  ni `ALARME` ni `NORMAL`. Si elle redevient anormale ensuite, le compte
  repart de zéro.
- L'`ALARME` est horodatée à l'instant où le délai est atteint (le `ts` de
  la mesure qui le franchit). Le retour à la normale, lui, n'est pas
  temporisé : `NORMAL` dès que la valeur repasse le seuil (hystérésis
  comprise), comme aujourd'hui.
- Une **panne** pendant le décompte l'annule (une valeur figée n'est pas
  une anomalie, comme en Phase 3).

**Ce qu'on attend de l'architecture.** C'est le test grandeur nature de
votre pattern *Strategy* : la nouvelle règle se **branche** sans modifier
le moteur. L'idéal : une règle temporisée qui **enveloppe** la règle à
hystérésis existante (elle lui délègue la décision « anormal ou non » et
n'ajoute que le décompte). Le moteur et ses abonnés ne voient aucune
différence.

**Le temps.** Le décompte se fait avec le `ts` des mesures, **jamais** avec
l'horloge ni un `sleep` ou un thread de minuterie. C'est ce qui rend la
règle testable en une milliseconde : un test fabrique des mesures à
`ts = 0, 100, 200…` et vérifie l'instant de l'alarme.

**Les tests, d'abord.**

1. anomalie de 5 s avec `delai = 30` : aucun événement ;
2. anomalie de 45 s : une seule `ALARME`, à `t0 + 30 s` exactement, puis
   un `NORMAL` à la fin de l'anomalie ;
3. anomalie de 20 s, retour normal 1 s, anomalie de 20 s : aucun événement
   (le compte est reparti de zéro) ;
4. `delai = 0` ou absent : tous les tests du moteur de la Phase 3 passent
   sans changement ;
5. panne pendant le décompte : aucune alarme.

**Le scénario.** Ajoutez `scenarios/livraison.scn` :

```text
# livraison.scn : une livraison normale, puis une porte oubliée.
periode 100
duree   90
graine  7

# t(s)  canal  action   valeurs
0       0      fixe     4.0
0       0      bruit    0.3
0       1      fixe     1013
0       2      fixe     500
0       3      fixe     0
10      3      fixe     1            # livraison : porte ouverte 5 s
15      3      fixe     0
30      3      fixe     1            # porte oubliée...
75      3      fixe     0            # ... refermée 45 s plus tard
```

**Ce qu'on montre.** Avec une copie de la configuration où le canal 3 a
`delai = 30` :

```bash
build/sim_capteurs --scenario scenarios/livraison.scn &
build/embedgate-gw --config config/gateway-e4.ini
grep -E 'ALARME|NORMAL' embedgate.log
```

Exactement **deux** lignes : `ALARME ch=3` vers 60 s, `NORMAL ch=3` vers
75 s. Avec la configuration de référence (sans `delai`), le même scénario
en donne **quatre** : c'est la démonstration du gain.

**Pièges.**

- La configuration de référence n'a pas de `delai` : la chambre froide
  doit toujours donner l'alarme porte **à 10 s** (critère de la Phase 3).
- Un décompte **par canal** : deux canaux en anomalie en même temps ne
  partagent rien.
- **Pas de `--vite` pour cette démonstration.** La puce accélère le
  scénario, mais le `ts` des mesures reste l'heure réelle : avec
  `--vite 3`, la porte oubliée ne reste ouverte que 15 vraies secondes,
  moins que le délai de 30 s, et l'alarme n'arrive jamais. Le scénario
  dure 90 s : c'est le prix de la démonstration.
- `delai` illisible (`delai = abc`) ou négatif : erreur de configuration,
  code 2, comme pour les seuils.

---

## E3 — Historisation SQLite et export CSV

**Le besoin.** Le lendemain d'un incident, le client veut **la courbe** :
quand la température a commencé à monter, combien de temps la porte est
restée ouverte. Le journal ne garde que les événements ; MQTT ne garde
rien. Vous enregistrez tout dans une base SQLite, puis vous en extrayez un
CSV qu'un tableur sait tracer.

**La bibliothèque.** SQLite (`sudo apt install libsqlite3-dev sqlite3`),
son API C (`sqlite3_open`, `sqlite3_prepare_v2`, `sqlite3_step`…). Elle
est autorisée pour cette extension.

**À faire.**

1. Une clé **facultative** dans `[passerelle]` : `historique = embedgate.db`.
   Absente : pas d'historique, la passerelle se comporte comme avant.
   Fichier impossible à ouvrir : code 2.
2. Un **troisième abonné** : il enregistre chaque mesure et chaque
   événement du moteur (*Observer* : le moteur ne le connaît pas plus que
   le journal ou MQTT). Le schéma, imposé pour pouvoir comparer :

   ```sql
   CREATE TABLE IF NOT EXISTS mesure    (ts INTEGER, ch INTEGER, valeur REAL);
   CREATE TABLE IF NOT EXISTS evenement (ts INTEGER, type TEXT, ch INTEGER,
                                         niveau TEXT, valeur REAL);
   ```

   `ts` en millisecondes, comme partout ; `type` vaut `ALARME`, `NORMAL`,
   `PANNE` ou `RETABLI` ; `niveau` est `NULL` quand il n'a pas de sens.
3. **L'export** : une commande qui écrit un CSV, par exemple
   `scripts/export_csv.py --db embedgate.db --canal 0 > temperature.csv`
   (en Python, le module `sqlite3` est fourni d'office ; en C++ si vous
   préférez). Première ligne : `heure,canal,nom,valeur` ; l'heure au
   format ISO du journal (UTC, millisecondes, `Z`).

**Les tests.**

- l'abonné, sur une base **en mémoire** (`":memory:"`) : 3 mesures et
  une alarme envoyées, puis relues par une requête ;
- l'export, sur une petite base fabriquée par le test : en-tête, format de
  l'heure, point décimal (jamais de virgule) ;
- base absente de la configuration : rien n'est créé.

**Ce qu'on montre.** Après `chambre-froide.scn` :

```bash
sqlite3 embedgate.db "SELECT ch, COUNT(*) FROM mesure GROUP BY ch"   # ≈ 600 par canal
sqlite3 embedgate.db "SELECT * FROM evenement"                       # les 4 événements du journal
scripts/export_csv.py --db embedgate.db --canal 0 > temperature.csv
```

Puis `temperature.csv` ouvert dans LibreOffice Calc : la montée vers
11 °C, la redescente, et les instants de l'alarme et du retour à la
normale lus dans la table `evenement`.

**Pièges.**

- **La lenteur.** Par défaut, SQLite écrit sur le disque à **chaque**
  insertion (plusieurs millisecondes). Sous `rafale.scn` (400 mesures par
  seconde), le thread de traitement prend du retard et la file déborde :
  le critère « rafale sans perte » de la Phase 3 tombe. Remèdes :
  requête **préparée une fois** et réutilisée, insertions **groupées dans
  une transaction** (`BEGIN` … `COMMIT` toutes les secondes, ou toutes les
  N mesures), éventuellement `PRAGMA journal_mode=WAL`. Mesurez : la
  rafale doit rester sans perte **avec** l'historique activé.
- **Un seul thread touche la base** : celui du traitement. Pas de
  connexion partagée entre threads.
- **Aucun `new` / `delete`, aucune fuite** : `sqlite3*` et `sqlite3_stmt*`
  se libèrent dans un destructeur (RAII, ou `std::unique_ptr` avec une
  fonction de libération). Valgrind doit rester propre.
- **L'arrêt** : la dernière transaction est validée avant `STOP`, sinon les
  dernières secondes de l'incident sont perdues.
- Le fichier `.db` est un résultat, pas une source : dans `.gitignore`.

---

## E5 — Qualité : CI locale, `cppcheck`, couverture, Doxygen

**Le besoin.** Prouver la qualité au lieu de l'affirmer : à la soutenance,
« notre moteur d'alarmes est testé à 95 % » vaut mieux que « on a bien
testé ». Et ne jamais fusionner un `main` cassé.

**Les outils** : `sudo apt install cppcheck gcovr doxygen graphviz`.

**À faire : une seule commande**, `scripts/qualite.sh`, qui part de zéro
et enchaîne :

1. **La CI locale** : compilation dans des dossiers neufs, puis `ctest`
   pour les trois versions : normale, `-DSANITIZE=address`,
   `-DSANITIZE=thread` ;
2. **`cppcheck`** sur **votre** code (`pilote/`, `passerelle/`, `include/`,
   pas `outils/` qui est fourni) :

   ```bash
   cppcheck --enable=warning,style,performance --std=c++17 --inline-suppr \
            --error-exitcode=1 -I include pilote passerelle
   ```

   Objectif : 0 avertissement. Un avertissement que vous jugez faux se
   neutralise **dans le code**, à la ligne concernée, avec un commentaire
   qui dit pourquoi (`// cppcheck-suppress … : raison`) ;
3. **La couverture** : une option CMake `-DCOUVERTURE=ON` (désactivée par
   défaut) qui ajoute `--coverage`, un `ctest`, puis
   `gcovr -r . --html-details qualite/couverture.html` ;
4. **Doxygen** : un `Doxyfile` ; les en-têtes publics (`include/`,
   `passerelle/*.hpp`) documentés, sans avertissement ;
5. **Un résumé**, et un code de sortie 0 seulement si tout est bon.

**Ce qu'on montre.**

```bash
scripts/qualite.sh
# CI     : PC, ASan, TSan : tous les tests verts
# cppcheck : 0 avertissement
# couverture : 84 % des lignes (moteur d'alarmes : 96 %)
# Doxygen : 0 avertissement → qualite/doc/html/index.html
# QUALITE OK
```

Objectifs chiffrés : **moteur d'alarmes ≥ 90 %** des lignes couvertes,
l'ensemble ≥ 70 %. Notez les chiffres dans `JOURNAL.md` : ils serviront au
dossier (« résultats chiffrés »).

**Et la CI GitLab ?** Facultative. Si vous en faites une, elle appelle
**le même** `scripts/qualite.sh` (une seule vérité), sur les merge
requests et sur `main`, et reste légère : l'offre gratuite de GitLab
limite les minutes de calcul.

**Pièges.**

- L'option de couverture ne doit **rien** changer à la compilation
  normale : `-Werror` reste, la version par défaut reste sans `--coverage`.
- Les rapports (`qualite/`, dossiers `build-*`) sont générés : dans
  `.gitignore`, jamais commités.
- Viser 100 % de couverture n'a pas de sens ; viser **les branches qui
  comptent** en a : le seuil franchi, l'hystérésis, la panne pendant une
  alarme, la configuration invalide.
- Un rapport `cppcheck` corrigé en masse la veille de la soutenance est un
  risque : corrigez au fil de l'eau, chaque correction avec son test.

---

## E1 — Bus CAN : un 5ᵉ canal venu d'un nœud distant

**Le besoin.** Le client ajoute une sonde sur le **quai de chargement**,
à 30 mètres de la passerelle. Trop loin pour la puce : la sonde est un
petit calculateur autonome, un **nœud**, qui envoie ses mesures sur un
**bus CAN**, comme les calculateurs d'une voiture, d'un train ou d'un
avion. La passerelle doit l'intégrer comme un **5ᵉ canal** : mesures sur
MQTT, alarmes, pannes, tableau de bord, exactement comme les quatre
canaux de la puce.

**Le bus.** Linux en fournit un **virtuel**, `vcan0` (SocketCAN), et les
outils `can-utils` : `cansend` pour y écrire, `candump` pour l'écouter.

```bash
sudo apt install can-utils
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0
candump vcan0                    # dans un terminal : on voit passer les trames
cansend vcan0 120#A00F0000       # dans un autre : une trame d'identifiant 0x120, 4 octets
```

**La messagerie CAN.** Sur un bus, il n'y a pas de JSON : une trame, c'est
un **identifiant** et **au plus 8 octets**. Le format des trames du nœud
est le vôtre, **documenté dans le README** comme le ferait l'équipe qui
publie la messagerie d'un calculateur. À défaut, prenez celui-ci :

| Champ | Valeur |
|---|---|
| Identifiant | `0x120` (standard, 11 bits) |
| Longueur | 4 octets |
| Octets 0 à 3 | la mesure brute, `int32`, petit-boutiste (octet de poids faible en premier) |
| Unité brute | milli-degré Celsius, comme le canal 0 |
| Cadence | une trame toutes les 100 ms |

Exemple : `120#A00F0000` → `0x00000FA0` = 4000 → **4,0 °C**.

**À faire.**

1. **Une section facultative** dans la configuration, et le canal qui va
   avec :

   ```ini
   [can]
   interface   = vcan0
   identifiant = 0x120
   # secondes sans trame avant PANNE ch=4
   silence     = 2.0

   [canal.4]
   nom = quai
   unite = degC
   facteur = 0.001
   bas = -5.0
   haut = 12.0
   hysteresis = 1.0
   ```

   Sans `[can]`, la passerelle se comporte exactement comme avant, sans
   socket ni thread de plus. `[can]` sans `[canal.4]` : erreur de
   configuration, code 2. Interface introuvable (`vcan0` pas créée) :
   message clair et **code 3**, comme une puce absente, car c'est du
   matériel qui manque.
2. **La réception** : une socket CAN (`socket(PF_CAN, SOCK_RAW, CAN_RAW)`,
   `bind` sur l'interface, un filtre sur l'identifiant), lue par un
   **thread de réception**. Chaque trame valide devient une mesure du
   canal 4, horodatée à la réception (`ts` en millisecondes depuis 1970,
   comme les autres), et part dans **la même file** que les mesures de la
   puce. Le moteur d'alarmes et ses abonnés ne voient aucune différence :
   ils reçoivent une mesure de plus.
3. **La panne** : un nœud distant ne « fige » pas sa valeur, il **se tait**
   (câble coupé, nœud éteint). Plus aucune trame depuis `silence`
   secondes : `PANNE ch=4` ; la trame suivante : `RETABLI ch=4`. Une seule
   fois chacune, comme pour la puce.
4. **Les commandes** : la passerelle n'a pas de registre `CALIB` sur le
   nœud. `calib` sur le canal 4 reçoit un `ack` `"ok":false` avec un
   `detail` clair. Vous pouvez aussi appliquer le décalage dans la
   passerelle : au choix, mais documenté.
5. **Le nœud** : un script, par exemple `scripts/noeud_can.sh`, qui joue la
   sonde avec `cansend`. Cette fonction encode une valeur brute :

   ```bash
   envoyer() {                      # envoyer <brut> : une trame 0x120, int32 petit-boutiste
     local h; printf -v h '%08X' $(( $1 & 0xFFFFFFFF ))
     cansend vcan0 "120#${h:6:2}${h:4:2}${h:2:2}${h:0:2}"
   }
   ```

   Le scénario du script : 10 s à 4,0 °C, 10 s à 15,0 °C, 5 s à 4,0 °C,
   5 s de silence, puis 5 s à 4,0 °C, une trame toutes les 100 ms.

**Les tests, d'abord**, tous **sans `vcan0`** : la socket est derrière une
interface, comme le client MQTT de la Phase 4, et un faux bus fournit les
trames.

1. le décodage : `A0 0F 00 00` → 4,0 °C ; `24 FA FF FF` → −1,5 °C (valeur
   négative) ;
2. une trame d'un autre identifiant, ou d'une autre longueur : ignorée,
   aucune mesure ;
3. le silence, avec un instant fourni par le test (jamais un vrai `sleep`) :
   1,9 s sans trame → rien ; 2,1 s → **une** `PANNE` ; la trame suivante →
   **un** `RETABLI` ;
4. sans `[can]` : tous les tests des phases précédentes passent sans
   changement.

**Ce qu'on montre.**

```bash
build/sim_capteurs &                                  # la puce, valeurs nominales sans fin
build/embedgate-gw --config config/gateway-e1.ini &
candump vcan0 &                                       # les trames qui passent
scripts/noeud_can.sh
grep 'ch=4' embedgate.log
```

Quatre lignes : `ALARME ch=4 nom=quai niveau=HAUT` vers 10 s, `NORMAL ch=4`
vers 20 s, `PANNE ch=4` vers 27 s (25 s + 2 s de silence), `RETABLI ch=4`
vers 30 s. Pendant ce temps, le superviseur affiche une 5ᵉ ligne, `quai`,
qui passe en alarme, puis en panne.

**Pièges.**

- **Les « 4 canaux » écrits en dur** : `std::array<…, 4>`, `ch < 4`, une
  liste fixe dans le superviseur. C'est le vrai sujet de l'extension :
  tout ce qui suppose 4 canaux doit suivre la configuration.
- **Deux producteurs, une file** : le thread des interruptions et le
  thread CAN y écrivent en même temps. Votre file est-elle vraiment sûre
  à plusieurs producteurs ? TSan le dira.
- **L'arrêt** : un `read()` bloquant sur la socket ne rend jamais la main,
  et la passerelle ne s'arrête plus sur `SIGTERM`. Attendez avec un délai
  (`poll()` à 100 ms, ou `SO_RCVTIMEO`), et regardez à chaque tour s'il
  faut s'arrêter. C'est aussi ce tour qui mesure le silence.
- **Le temps du silence** se mesure avec `std::chrono::steady_clock`, qui
  ne recule jamais, et pas avec l'heure système, qui peut sauter.
- **L'ordre des octets** : décodez avec des décalages
  (`d[0] | d[1] << 8 | …`, sur un `uint32_t` converti en `int32_t` à la
  fin), pas avec un `memcpy` : c'est portable, et ça se teste.
- **La socket** se ferme dans un destructeur (RAII) : Valgrind doit
  rester propre.
- **`vcan0` disparaît au redémarrage** de la VM : un petit script la
  recrée. `modprobe vcan` demande `sudo`, et le noyau de WSL ne le
  fournit pas toujours : travaillez sur la VM Debian.
- **Sur la carte**, ce n'est pas demandé : il faudrait `vcan` dans le
  noyau Buildroot et `can-utils` dans l'image. C'est un bonus possible,
  pas une attente.

---

## E2 — Tableau de bord web

**Le besoin.** L'opérateur ne veut pas de terminal : il veut **une page
dans son navigateur**, et le client veut l'afficher sur l'écran de
l'entrepôt. La page se met à jour **en direct**, sans qu'on la recharge.

**Les bibliothèques.** La bibliothèque standard de Python
(`http.server`, `json`, `threading`) et `paho-mqtt`, déjà utilisés. Pas de
framework web (Flask, Django…). Côté page : HTML, CSS et JavaScript
simples, **rien chargé d'internet** : un réseau d'usine est souvent coupé
du monde.

**À faire.**

1. Un programme `superviseur/web.py --config config/gateway.ini
   [--port 8080]` : il s'abonne à `embedgate/<gp>/#`, comme
   `superviseur.py`, et sert la page.
2. Deux adresses :
   - `GET /` → la page (par exemple `superviseur/web/index.html`) ;
   - `GET /etat` → l'état courant, en JSON : la passerelle (`en_ligne` ou
     `hors_ligne`) et, pour chaque canal, nom, valeur, unité, `ts`, état
     d'alarme et niveau, panne.
3. La page interroge `/etat` toutes les 500 ms (`fetch`) et se redessine :
   une carte par canal, **rouge** en alarme, **grise** en panne, et un
   bandeau « passerelle hors ligne » quand le testament est arrivé.
   L'`EventSource` (le serveur pousse les mises à jour) est une
   amélioration possible, pas une attente.
4. **Pour aller plus loin** : des boutons de commande (`POST /cmd` →
   publication sur `…/cmd`, puis affichage de l'`ack`).

**Ce qu'on attend de l'architecture.** L'état du tableau de bord (ce qui
reçoit les messages MQTT et retient la situation) existe déjà dans votre
`superviseur.py` de la Phase 4. **Réutilisez-le** : une seule classe
« modèle », deux vues, la console et la page. Si tout est mélangé dans
`superviseur.py`, commencez par en sortir cette classe, tests verts avant
et après.

**Autre voie acceptée.** Activer les WebSockets de Mosquitto
(`listener 9001` + `protocol websockets` dans sa configuration) et faire
parler la page **directement** au broker avec une bibliothèque MQTT en
JavaScript, copiée dans le dépôt. Dites-le dans le README : la
configuration du broker fait alors partie de l'installation.

**Les tests** (`pytest`, sans broker).

- le modèle : des messages fabriqués par le test (mesure, alarme, panne,
  état) donnent le bon JSON d'état ; un JSON malformé est ignoré sans
  exception ;
- le serveur, démarré sur le port `0` (le système choisit un port libre)
  dans un thread : `GET /etat` → 200, `Content-Type: application/json`,
  JSON valide ; `GET /` → 200 ; `GET /inconnu` → 404.

**Ce qu'on montre.**

```bash
python3 superviseur/web.py --config config/gateway.ini &
scripts/demo.sh
```

Le navigateur ouvert sur `http://localhost:8080` : la porte passe au rouge
vers 10 s, la température monte puis passe au rouge vers 21 s, et tout
redevient normal (instants du scénario : si votre `demo.sh` l'accélère,
divisez d'autant). Puis `kill -9` de la passerelle : le bandeau « hors
ligne » apparaît en moins d'une seconde après le testament.

**Pièges.**

- **Les threads** : `paho` appelle vos fonctions depuis son thread réseau,
  le serveur HTTP (`ThreadingHTTPServer`) répond depuis d'autres threads.
  L'état partagé est protégé par un `threading.Lock`, et `/etat` renvoie
  une **copie**.
- **Les messages retenus** arrivent dès l'abonnement : c'est ce qui permet
  à la page d'afficher les alarmes en cours dès son lancement. Ne remettez
  pas l'état à zéro à la connexion.
- **L'écoute** se fait sur `127.0.0.1` par défaut. Une option
  `--hote 0.0.0.0` permet de voir la page depuis une autre machine (par
  exemple, la page servie par la VM, vue depuis le PC hôte).
- **Le port déjà pris** : message clair, code de sortie ≠ 0.
- **Le cache** : renvoyez `Cache-Control: no-store` sur `/etat`.
- **L'arrêt** sur `Ctrl-C` : arrêt du serveur, puis `loop_stop()`.
- `superviseur.py` et `commande.py` sont des programmes du contrat : ils
  marchent toujours, sans changement de comportement.

---

## E6 — Paquet Debian et service `systemd`

**Le besoin.** Chez le client, personne ne compile. On lui livre **un
fichier** : `apt install ./embedgate.deb`, et la passerelle est installée,
démarrée, relancée si elle tombe, et redémarrée avec la machine.
C'est la livraison d'un logiciel Linux industriel, sur le PC (la carte
Buildroot n'a ni `dpkg` ni `systemd`).

**À faire.**

1. **Le paquet.** Un script, par exemple `scripts/paquet.sh`, qui part de
   zéro : compilation **neuve en Release, sans sanitizer**
   (`-DCMAKE_BUILD_TYPE=Release`, dans `build-paquet/`), copie des
   fichiers dans une arborescence, puis
   `dpkg-deb --build --root-owner-group`. CPack (`cpack -G DEB`) est
   accepté aussi. Le contenu :

   ```text
   DEBIAN/control                          nom, version, dépendances
   DEBIAN/conffiles                        /etc/embedgate/gateway.ini
   DEBIAN/postinst, prerm, postrm          utilisateur, activation du service
   usr/bin/embedgate-gw
   usr/lib/embedgate/sim_capteurs          la puce simulée (voir 3)
   etc/embedgate/gateway.ini               la configuration installée
   lib/systemd/system/embedgate.service
   lib/systemd/system/embedgate-puce.service
   ```

   Le fichier `control`, par exemple :

   ```text
   Package: embedgate
   Version: 1.0.0
   Architecture: amd64
   Maintainer: gp1 <prenom.nom@exemple.fr>
   Depends: libmosquitto1, adduser
   Recommends: mosquitto
   Description: EmbedGate, passerelle de supervision d'une chambre froide
    Lit la puce capteur, décide des alarmes et publie sur MQTT.
   ```

   La version suit celle de `passerelle/Version.cpp`.
2. **La configuration installée** est une **copie** de la configuration de
   référence, avec des chemins absolus :
   `journal = /var/log/embedgate/embedgate.log`. Le fichier fourni
   `config/gateway.ini` ne change pas.
3. **Les services.** `embedgate.service` lance la passerelle :

   ```ini
   [Unit]
   Description=EmbedGate : passerelle de supervision
   After=network-online.target mosquitto.service embedgate-puce.service
   Wants=network-online.target embedgate-puce.service
   StartLimitIntervalSec=0

   [Service]
   User=embedgate
   ExecStart=/usr/bin/embedgate-gw --config /etc/embedgate/gateway.ini
   Restart=always
   RestartSec=3
   RestartPreventExitStatus=2
   LogsDirectory=embedgate

   [Install]
   WantedBy=multi-user.target
   ```

   Vos codes de sortie prennent ici tout leur sens. Avec 3 (puce absente)
   ou 4 (broker injoignable), systemd relance la passerelle toutes les
   3 s jusqu'à ce que le matériel ou le réseau soit là. Avec 2
   (configuration invalide), il ne relance pas : relancer ne réparerait
   rien.

   `embedgate-puce.service` lance `sim_capteurs` sans scénario (valeurs
   nominales sans fin), avec le même `User=embedgate`. Il **tient lieu de
   matériel** : sur un vrai boîtier, la puce est soudée, et ce service
   n'existerait pas. Dites-le dans le README.
4. **Les scripts du paquet** :
   - `postinst` crée l'utilisateur système
     (`adduser --system --group --no-create-home embedgate`), puis
     `systemctl daemon-reload` et `systemctl enable --now` sur les deux
     services ;
   - `prerm` les arrête et les désactive ;
   - `postrm`, sur `purge`, supprime l'utilisateur et `/var/log/embedgate`.

**Les tests.** Un script, par exemple `scripts/test_paquet.sh`, sans rien
installer :

- `dpkg-deb -c` : les fichiers attendus sont dans le paquet ;
- `dpkg-deb -I` : `Depends` contient `libmosquitto1` ;
- `ldd` sur le binaire du paquet : aucune bibliothèque de sanitizer
  (`libasan`, `libtsan`) ;
- `systemd-analyze verify` sur les deux fichiers `.service` : aucun
  avertissement.

Puis, une fois, à la main sur la VM, le cycle complet : installation,
désinstallation, purge. Notez-le dans `JOURNAL.md`.

**Ce qu'on montre.**

```bash
scripts/paquet.sh
sudo apt install ./build-paquet/embedgate_1.0.0_amd64.deb
systemctl status embedgate                     # active (running)
mosquitto_sub -v -t 'embedgate/gp1/#'          # les mesures arrivent
sudo kill -9 $(pidof embedgate-gw)             # …/etat : hors_ligne, puis en_ligne 3 s plus tard
journalctl -u embedgate -n 20                  # la relance, vue par systemd
sudo systemctl stop embedgate
tail -1 /var/log/embedgate/embedgate.log       # STOP raison=signal
sudo apt remove embedgate                      # plus de service ni de binaire ; la configuration reste
sudo apt purge embedgate                       # plus rien
```

**Pièges.**

- **`apt install embedgate.deb`, sans `./`**, fait chercher le paquet
  dans les dépôts Debian : le `./` est obligatoire.
- **Un journal en chemin relatif** s'écrit dans le dossier courant du
  service, `/`, où l'utilisateur `embedgate` n'a pas le droit d'écrire :
  la passerelle échoue au démarrage. D'où les chemins absolus de la
  configuration installée.
- **Le bus partagé** : la puce et la passerelle doivent voir le **même**
  `/tmp`. N'ajoutez pas `PrivateTmp=yes` à un seul des deux services
  (sinon : puce absente, code 3, relance sans fin), et lancez-les sous le
  même utilisateur (le fichier du bus est créé en `0644`).
- **Un binaire de développement** (Debug, ASan, TSan) dans le paquet :
  c'est le rôle de la compilation neuve en Release, et du test `ldd`.
- **L'arrêt** : `systemctl stop` envoie `SIGTERM`. Si la passerelle ne
  s'arrête pas, systemd attend 90 s puis la tue. Le symptôme : un `stop`
  qui « gèle ».
- **La configuration modifiée** par le client est préservée à la mise à
  jour : c'est le rôle de `conffiles`. Ne la régénérez pas dans
  `postinst`.
- `build-paquet/` et les `.deb` sont des résultats : dans `.gitignore`.
- `systemd` doit tourner : c'est le cas sur la VM Debian, pas toujours
  sous WSL.
- **Pour aller plus loin** : un paquet `armhf` construit depuis
  `build-arm/`, installable sur une carte sous Debian (Raspberry Pi OS…),
  pas sur la carte Buildroot.
