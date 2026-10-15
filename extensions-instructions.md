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
| **E1** | Bus CAN : un 5ᵉ canal venu d'un nœud distant | la P3 | 1,5 à 2 jours (trinôme : obligatoire) |
| E2 | Tableau de bord web | la P4 | 1 jour |
| **E3** | Historisation SQLite et export CSV | la P3 | 1 jour |
| **E4** | Alarme temporisée | la P3 | une demi-journée |
| **E5** | Qualité : CI locale, `cppcheck`, couverture, Doxygen | la P3 | une demi-journée à 1 jour |
| E6 | Paquet Debian, service `systemd` | la P5 | 1 jour |

E3, E4 et E5 sont détaillées ci-dessous. E1, E2 et E6 le seront avant la
date où elles deviennent possibles.

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

## E1, E2, E6

Détaillées avant la date où elles deviennent possibles. En attendant, la
description de l'agenda fait foi :

- **E1 — Bus CAN** : un nœud capteur distant envoie ses mesures sur un bus
  CAN virtuel (SocketCAN, `vcan0`) ; la passerelle les intègre comme un
  5ᵉ canal (mesures, alarmes, tableau de bord).
- **E2 — Tableau de bord web** : la page se met à jour en direct pendant la
  démonstration.
- **E6 — Paquet Debian et service `systemd`** :
  `apt install ./embedgate.deb`, puis `systemctl status embedgate`.
