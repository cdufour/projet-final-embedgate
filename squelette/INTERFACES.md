# EmbedGate — Contrat d'interface (IMPOSÉ)

> Ce document **fait foi**. Il fixe tout ce qui se voit de l'extérieur de
> votre code : la puce capteur (sa « datasheet »), les messages MQTT, le
> journal, les noms des programmes et leurs options. À l'intérieur, vous
> organisez votre code comme vous voulez.
>
> Pourquoi un contrat imposé ? Parce qu'en entreprise, on ne choisit ni la
> carte des registres d'une puce, ni le format des messages attendus par
> l'équipe d'à côté. Et parce que c'est ce qui permet de **vérifier votre
> travail automatiquement**, sans lire votre code, à chaque fin de phase.
>
> En cas de doute ou de contradiction : posez la question, ne devinez pas.

---

## 1. La puce capteur (fournie : `outils/sim_capteurs.c`)

Le simulateur **est le matériel**. Vous ne l'écrivez pas, vous ne le
modifiez pas : vous écrivez le **pilote** qui lui parle, à partir de cette
datasheet, comme on le fait avec une vraie puce. Elle est fournie en source
parce qu'elle doit être compilée sur votre machine et pour la carte ARM :
CMake la construit avec le reste (`build/sim_capteurs`). Vous n'avez pas
besoin de lire son code.

La puce expose une banque de registres de 32 bits. Sur une vraie carte, elle
serait à une adresse physique fixe ; ici, elle est projetée en mémoire par
`mmap()` sur un fichier commun (le « bus », par défaut
`/tmp/embedgate_bus.bin`, modifiable par la variable d'environnement
`EG_BUS_PATH`). Les fichiers `include/embedgate/registres.h` et `bus.h/bus.c`
sont fournis.

### 1.1 Carte des registres

| Offset | Nom | Accès | Contenu |
|---|---|---|---|
| `0x00` | `ID` | R | Signature de la puce : `0xE9B0DA7A` |
| `0x04` | `STATUS` | R | bit 0 `READY` · bit 1 `FAULT` (au moins un canal en panne) · bits 8-15 compteur d'acquisitions (modulo 256) |
| `0x08` | `CTRL` | R/W | bit 0 `ENABLE` (acquisition en marche) · bit 1 `RESET` · bit 2 `SELFTEST` |
| `0x0C` | `IRQ_STATUS` | R | bit *n* = donnée neuve sur le canal *n* (interruption en attente) |
| `0x10` | `IRQ_ENABLE` | R/W | bit *n* = interruption du canal *n* autorisée |
| `0x14` | `FAULT_MAP` | R | bit *n* = canal *n* en panne (sa valeur `DATA` est figée, plus d'interruption) |
| `0x18` | `IRQ_ACK` | W1C | écrire 1 sur le bit *n* acquitte l'interruption du canal *n* ; les bits à 0 n'ont aucun effet |
| `0x20` + 4·*n* | `DATA[n]` | R | mesure brute du canal *n* (`int32`, unités ci-dessous), calibrage déjà appliqué |
| `0x40` + 4·*n* | `CALIB[n]` | R/W | décalage de calibrage du canal *n* (`int32`, mêmes unités brutes), ajouté par la puce |

### 1.2 Canaux

| Canal | Grandeur | Unité brute | Facteur vers l'unité physique |
|---|---|---|---|
| 0 | température de la chambre | milli-degré Celsius | × 0,001 → `degC` |
| 1 | pression du circuit | milli-millibar | × 0,001 → `mbar` |
| 2 | courant du compresseur | micro-ampère | × 0,001 → `mA` |
| 3 | porte | 0 = fermée, 1 = ouverte | × 1 → `bool` |

### 1.3 Comportement de la puce

- Au démarrage : banque à 0, `STATUS.READY = 1`, puis `ID` (écrit en
  dernier). La puce est **en veille** tant que `CTRL.ENABLE = 0`, et le
  temps du scénario ne s'écoule que pendant que `ENABLE = 1` : il démarre
  donc quand votre pilote démarre la puce.
- À chaque période d'acquisition, si `ENABLE = 1` : pour chaque canal qui
  n'est pas en panne, la puce écrit `DATA[n]`, puis met le bit *n* de
  `IRQ_STATUS` à 1 **si** le bit *n* de `IRQ_ENABLE` est à 1.
- `RESET` : la puce remet `DATA`, `CALIB` et `IRQ_STATUS` à 0, puis efface
  elle-même le bit `RESET`.
- `SELFTEST` : la puce vérifie ses canaux, met `STATUS.FAULT` à jour, puis
  efface elle-même le bit `SELFTEST`.
- Le pilote acquitte une interruption en écrivant **1** sur le bit concerné
  de `IRQ_ACK` (*write 1 to clear*, comme le registre `ICR` des STM32).
  Écrire `0x1` acquitte le canal 0 sans toucher aux autres. `IRQ_STATUS`
  est en lecture seule : une écriture du pilote y est écrasée par la puce.
  Une interruption non acquittée reste en attente ; si une nouvelle mesure
  arrive entre-temps, `DATA[n]` est simplement remplacée.
- À l'arrêt (fin du scénario, `Ctrl-C`, `SIGTERM`), la puce remet `ID` et
  `STATUS` à 0, comme une puce qu'on débranche : votre pilote peut le
  détecter.

### 1.4 Lancer la puce

```bash
build/sim_capteurs --scenario scenarios/stable.scn
build/sim_capteurs --scenario scenarios/chambre-froide.scn --vite 4   # 4x plus vite
```

Sans `--scenario`, la puce produit des valeurs nominales bruitées sans fin.
Elle s'arrête proprement sur `Ctrl-C` / `SIGTERM`, et à la fin du scénario
si celui-ci a une durée. La version ARM est produite par la compilation
croisée (`build-arm/sim_capteurs`).

---

## 2. Les scénarios (`scenarios/*.scn`)

Un scénario décrit, seconde par seconde, ce que « vivent » les capteurs. Il
est **déterministe** : rejoué deux fois avec la même graine, il produit
exactement les mêmes mesures. C'est ce qui rend vos tests reproductibles.

```text
# chambre-froide.scn : un joint de porte cède pendant la nuit
periode 100          # ms entre deux acquisitions
duree   60           # s, puis la puce s'arrête (0 = sans fin)
graine  42

# t(s)  canal  action   valeurs
0       0      fixe     4.0
0       0      bruit    0.3          # ± 0,3 °C autour de la consigne
0       1      fixe     1013
0       2      fixe     500
0       3      fixe     0
10      3      fixe     1            # la porte s'ouvre
14      3      fixe     0            # elle se referme... mais le joint a cédé
10      0      rampe    11.0  20     # la température monte vers 11 °C en 20 s
36      0      rampe    4.0   14     # réparation : retour à 4 °C en 14 s
```

| Action | Valeurs | Effet |
|---|---|---|
| `fixe` | *v* | la grandeur vaut *v* (unité physique) |
| `rampe` | *cible* *durée* | variation linéaire jusqu'à *cible* en *durée* secondes |
| `bruit` | *a* | ajoute un bruit pseudo-aléatoire dans [−*a*, +*a*] |
| `panne` | — | le canal tombe en panne (`FAULT_MAP`, plus d'IT, `DATA` figée) |
| `retablit` | — | le canal refonctionne |

Scénarios fournis : `stable.scn`, `chambre-froide.scn`, `panne-capteur.scn`,
`rafale.scn` (période de 10 ms : test de charge). Vous pouvez en écrire
d'autres pour vos tests.

---

## 3. La configuration (`config/gateway.ini`)

Le format est imposé ; un exemple complet est fourni. Les seuils ci-dessous
sont ceux **utilisés par les vérifications** : ne les changez pas dans le
fichier fourni (faites-en une copie pour vos essais).

```ini
[passerelle]
groupe    = gp1                 # gp1 ... gp6 : préfixe de vos topics MQTT
journal   = embedgate.log       # chemin du journal (section 5)
bus       = /tmp/embedgate_bus.bin

[mqtt]
hote = 127.0.0.1
port = 1883

[canal.0]
nom = temperature
unite = degC
facteur = 0.001
bas = 0.0
haut = 8.0
hysteresis = 1.0

# [canal.1] pression : bas 1000 / haut 1025 / hysteresis 2
# [canal.2] courant  : bas 0 / haut 900 / hysteresis 20
# [canal.3] porte    : bas -1 / haut 0.5 / hysteresis 0
```

**Hystérésis** : une alarme « haute » se déclenche quand la valeur dépasse
`haut`, et ne retombe que quand la valeur repasse sous `haut − hysteresis`.
Même chose, en miroir, pour `bas`. Sans cela, une valeur bruitée qui
hésite autour du seuil déclencherait des dizaines d'alarmes. Règle
pratique : l'hystérésis doit dépasser deux fois l'amplitude du bruit
(ici ±0,3 °C, d'où 1,0).

---

## 4. Les messages MQTT

Chaque groupe préfixe ses topics par son identifiant (`gp1`…`gp6`) : vous
pourrez ainsi partager un même broker sans vous gêner. Les messages sont du
**JSON sur une ligne**, encodé en UTF-8. `ts` est un horodatage en
millisecondes depuis le 1er janvier 1970 (UTC).

| Topic | Sens | QoS | Retenu | Contenu |
|---|---|---|---|---|
| `embedgate/<gp>/mesure/<ch>` | passerelle → | 0 | non | `{"ch":0,"nom":"temperature","valeur":4.12,"unite":"degC","ts":1760263351123}` |
| `embedgate/<gp>/alarme/<ch>` | passerelle → | 1 | **oui** | `{"ch":0,"nom":"temperature","etat":"ALARME","niveau":"HAUT","valeur":8.31,"ts":…}` · retour : `"etat":"NORMAL","niveau":null` |
| `embedgate/<gp>/panne/<ch>` | passerelle → | 1 | **oui** | `{"ch":1,"panne":true,"ts":…}` · rétabli : `"panne":false` |
| `embedgate/<gp>/etat` | passerelle → | 1 | **oui** | `{"etat":"en_ligne"}` à la connexion · `{"etat":"hors_ligne"}` par **testament MQTT** (*Last Will*) |
| `embedgate/<gp>/cmd` | → passerelle | 1 | non | voir 4.1 |
| `embedgate/<gp>/cmd/ack` | passerelle → | 1 | non | `{"id":"7","ok":true,"detail":""}` |

Les alarmes, pannes et l'état sont **retenus** : un superviseur qui démarre
après coup voit immédiatement la situation courante.

### 4.1 Commandes

| Commande | JSON envoyé sur `…/cmd` | Effet attendu sur la puce |
|---|---|---|
| calibrer | `{"id":"7","cmd":"calib","ch":0,"decalage":-1.5}` | `CALIB[0]` = −1,5 °C en unité brute, soit −1500 |
| réinitialiser | `{"id":"8","cmd":"reset"}` | bit `CTRL.RESET` |
| auto-test | `{"id":"9","cmd":"selftest"}` | bit `CTRL.SELFTEST`, puis `ack` avec `"detail"` = résultat |
| marche / arrêt | `{"id":"10","cmd":"acquisition","marche":false}` | bit `CTRL.ENABLE` |

Toute commande reçoit **un** `ack` portant le même `id`, avec `"ok":false`
et un `detail` lisible si elle est invalide (canal inexistant, JSON
malformé…). Une commande invalide ne doit jamais arrêter la passerelle.

---

## 5. Le journal

Un événement par ligne, en texte, dans le fichier indiqué par la
configuration. Les lignes commencent par l'horodatage ISO 8601 en UTC, puis
le type d'événement, puis des paires `clé=valeur`.

```text
2026-10-15T09:12:00.004Z START version=0.3.0 groupe=gp1
2026-10-15T09:12:10.103Z ALARME ch=3 nom=porte niveau=HAUT valeur=1
2026-10-15T09:12:14.101Z NORMAL ch=3 nom=porte valeur=0
2026-10-15T09:12:21.502Z ALARME ch=0 nom=temperature niveau=HAUT valeur=8.06
2026-10-15T09:12:44.000Z NORMAL ch=0 nom=temperature valeur=6.97
2026-10-15T09:12:50.000Z CMD id=7 cmd=calib ok=1
2026-10-15T09:12:55.000Z PANNE ch=1 nom=pression
2026-10-15T09:13:05.000Z RETABLI ch=1 nom=pression
2026-10-15T09:13:10.220Z STOP raison=signal
```

Types d'événement : `START`, `STOP`, `ALARME`, `NORMAL`, `PANNE`, `RETABLI`,
`CMD`, `ERREUR`. Les mesures ordinaires ne vont **pas** au journal (elles
partent sur MQTT).

---

## 6. Les programmes et leurs options

| Programme | Langage | Lancement | Sortie attendue |
|---|---|---|---|
| `build/pilote_demo` | C | `build/pilote_demo [--duree s]` | sur la sortie standard, une ligne par interruption servie : `[irq] ch=<n> brut=<entier>` ; une ligne `[panne] ch=<n>` quand un canal tombe en panne, `[retabli] ch=<n>` quand il revient (une seule fois chacune) ; les autres messages sur la sortie d'erreur |
| `build/embedgate-gw` | C++ | `build/embedgate-gw --config config/gateway.ini` | tourne jusqu'à `SIGINT` / `SIGTERM`, puis s'arrête proprement |
| `superviseur/superviseur.py` | Python | `python3 superviseur/superviseur.py --config config/gateway.ini` | tableau de bord en direct |
| `superviseur/commande.py` | Python | `python3 superviseur/commande.py --config config/gateway.ini calib 0 -1.5` | affiche l'`ack` ; code de sortie 0 si `ok`, 1 sinon |
| `scripts/demo.sh` | shell | `scripts/demo.sh` | rejoue `chambre-froide.scn` de bout en bout, affiche `DEMO OK` ou `DEMO ECHEC` |

**Codes de sortie de `embedgate-gw`** : 0 arrêt normal · 2 configuration
invalide · 3 puce absente (`ID` incorrect ou bus introuvable) · 4 broker
injoignable au démarrage.

---

## 7. Ce qui est fourni / ce que vous écrivez

| Fourni (ne pas modifier) | Vous écrivez |
|---|---|
| `outils/sim_capteurs.c` (la puce) | le pilote C (`pilote/`) et ses tests |
| `include/embedgate/registres.h`, `bus.h`, `commun/bus.c` | le démon C++ (`passerelle/`) et ses tests |
| `scenarios/*.scn`, `config/gateway.ini` | le superviseur Python (`superviseur/`) et ses tests |
| `CMakeLists.txt` de départ, `cmake/arm-linux-gnueabihf.cmake` | `scripts/demo.sh`, le déploiement embarqué |
| `tests/Verif.hpp` (le même qu'au projet C++), `tests/verif.h` (sa version C) | le README, le journal de bord |
