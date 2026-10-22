# EmbedGate — Phase 6 pas à pas : finaliser

**Vendredi 23 octobre 2026 · présentation remise à 14 h · tag `v1.0` avant 17 h**

> Ce document détaille la Phase 6 de l'[agenda](agenda-phases-embedgate.md),
> avec un changement : **le dossier de 5 à 10 pages est remplacé par la
> présentation de votre groupe**, à remettre **vendredi à 14 h**. Le reste
> de la journée ne change pas.

---

## Où vous allez

Lundi 26, **l'après-midi**, vous présentez EmbedGate à un jury où siègent des
**opérationnels d'Expleo**. La soutenance se fait **à distance, sur BBB**
(BigBlueButton, la classe virtuelle de la formation) : vous y présentez
en partageant votre écran, et le jury vous rejoint. Ils veulent lire votre présentation
**avant** la soutenance, pour préparer leurs questions. Elle leur est
transmise vendredi après-midi.

Votre présentation fait donc deux choses :

- elle **remplace le dossier** : le jury la lit seul, sans vous, et doit y
  trouver votre architecture, vos choix, vos difficultés, vos résultats
  chiffrés et vos limites ;
- elle **sert de support** lundi, pendant vos 15 minutes de présentation,
  démonstration comprise.

Ces opérationnels sont aussi des ingénieurs avec qui vous pourriez
travailler demain. Ils jugent un groupe, mais ils regardent aussi chacun
de vous.

### La journée

| Heure | Quoi |
|---|---|
| matin | socle complet fusionné sur `main`, critères des phases 1 à 5 repassés, extensions si le temps le permet |
| **14 h** | **présentation remise** (PDF, voir plus bas) |
| 14 h – 15 h | échange de README avec le groupe partenaire |
| après-midi | README corrigé d'après les remarques ; si vous le souhaitez, répétition chronométrée devant un autre groupe |
| **17 h** | **tag `v1.0` poussé**. C'est lui qui est évalué |

**En cas de retard, le lundi 26 au matin sert de tampon** pour l'échange
de README et pour la répétition chronométrée : vous pouvez les y
reporter, en accord avec le groupe partenaire. La remise de la
présentation (vendredi 14 h) et le tag `v1.0` (vendredi 17 h), eux, ne
bougent pas. Seule exception : un tag `v1.0.1` le lundi avant midi, pour
le README seul (voir « Le README testé »).

Ne gardez pas la présentation pour la fin : commencez-la dès jeudi soir,
ou vendredi matin pendant qu'un membre finit le socle.

---

## La présentation

### Une trame suggérée

Voici une trame en 15 diapositives. Un modèle vide la met en forme :
[`soutenance/modele-presentation.pdf`](soutenance/modele-presentation.pdf),
aussi en formats modifiables : `.pptx` (Impress, PowerPoint, Google Slides)
et `.odp` (LibreOffice), dans le même dossier. Selon l'outil, polices et
marges peuvent bouger un peu.
Chaque bloc n'y donne qu'un exemple : tout ce qui est entre `[crochets]`
est à écrire par vous. Ce n'est qu'une proposition : le contenu, la mise
en forme et l'outil sont à vous.

| # | Diapositive | Ce que le jury doit y trouver |
|---|---|---|
| 1 | Couverture | le groupe, ses membres |
| 2 | L'équipe | un micro-CV par membre (voir plus bas) |
| 3 | Le besoin du client | la chambre froide à 3 h du matin, en une phrase |
| 4 | La solution | la chaîne de la puce au superviseur, et la carte ARM |
| 5 | Le cursus dans notre projet | où chaque module du cursus a laissé une trace, avec **votre** preuve |
| 6 | Démonstration | ce que le jury va voir, étape par étape |
| 7 à 10 | Pilote, passerelle, supervision, carte embarquée | un maillon par diapositive, et **vos** choix |
| 11 | Qualité | vos chiffres réels : tests, Valgrind, ThreadSanitizer, rafale |
| 12 | Méthode | branches, tags, qui a porté quoi |
| 13 | Difficultés | trois vrais problèmes de votre `JOURNAL.md` |
| 14 | Au-delà du socle | extensions, limites, ce que vous feriez ensuite |
| 15 | Bilan | une phrase par membre, puis les questions |

Vous pouvez réordonner, fusionner, ajouter ou retirer des diapositives.
Visez **12 à 16 diapositives**.

### Personnalisée, pas remplie

C'est la demande explicite d'Expleo. Six groupes ont fait le même projet,
avec le même contrat : ce qui vous distingue, c'est **ce que vous en avez
fait**. Une présentation personnalisée, c'est :

- **vos chiffres**, relevés sur votre dépôt : le nombre de tests, la sortie
  de Valgrind, le débit en rafale. Un chiffre ne figure que si vous pouvez
  le refaire devant le jury ;
- **vos captures** : votre tableau de bord, votre carte qui démarre, votre
  session `gdb`, votre journal d'alarmes ;
- **vos choix**, avec leur raison : pourquoi telle structure, tel
  *pattern*, tel découpage en threads, et ce que vous avez écarté ;
- **vos difficultés**, datées dans votre `JOURNAL.md`, avec la façon dont
  vous les avez trouvées et corrigées ;
- **vos limites**, dites honnêtement : un consultant sait ce que son
  logiciel ne fait pas encore ;
- **votre mise en forme**, si vous le souhaitez : couleurs, ordre,
  schémas. La trame est une proposition.

À éviter : des phrases générales, qui vaudraient pour n'importe quel
groupe, et une diapositive qui ne parle de rien que vous ayez fait. Le
jury lit les six présentations à la suite : il verra tout de suite ce qui
se ressemble.

### La diapositive « L'équipe »

Juste après la couverture, elle présente l'équipe : **un micro-CV par
membre**, côte à côte. Pour chacun :

- prénom, nom, et une photo si vous le souhaitez (facultative) ;
- le parcours avant la formation, en une ligne (études, métier, domaine) ;
- ce qu'il a porté dans EmbedGate (par exemple : « le pilote C et la
  carte ARM ») ;
- une ou deux compétences qu'il met en avant ;
- en une ligne, le poste ou le domaine qu'il vise.

Restez sobres : pas d'adresse, pas de téléphone, pas de date de
naissance. C'est une présentation professionnelle, pas un CV complet.

### Le format

- **PDF**, en 16:9, de moins de 5 Mo (compressez les captures si besoin).
  L'outil est libre : LibreOffice Impress, PowerPoint, Google Slides,
  Canva…
- Le texte doit **se lire sans vous** : des phrases courtes, des titres
  qui disent quelque chose (« Aucune mesure perdue à 500 par seconde »
  plutôt que « Tests »).
- Numérotez les diapositives : le jury s'y référera dans ses questions.

### La remise : vendredi 23, à 14 h

1. Le fichier `soutenance/EmbedGate-gpN.pdf` (votre numéro de groupe à la
   place de `N`), dans votre dépôt.
2. Commit et push sur `main` **avant 14 h**.
3. Un message dans le canal Discord de votre groupe : « présentation
   poussée ».

La version de 14 h est celle que le jury lira. Vous pouvez encore
l'améliorer pour lundi, mais **le jury aura lu
celle-ci** : remettez une version complète, pas un brouillon.

---

## Le reste de la journée

### Le socle sur `main`

Toutes les branches utiles fusionnées, les critères des phases 1 à 5
repassés sur `main` : compilation PC et ARM sans avertissement, tests
verts, `scripts/demo.sh` → `DEMO OK`, la carte qui démarre et la
passerelle qui se lance seule. Les extensions ne viennent qu'ensuite (voir
[`extensions-instructions.md`](extensions-instructions.md)).

### Le README testé par un autre groupe

**Votre README, pas celui du squelette.** Le `README.md` reçu au début
du projet n'était qu'un point de départ. Vendredi, il doit s'adresser à
quelqu'un qui découvre votre projet et contenir tout ce qu'il doit taper,
dans l'ordre. Un seul fichier, à la racine. Un plan qui marche bien :

1. ce que c'est (trois lignes) ;
2. installer (les paquets) ;
3. compiler et tester, pour le PC et pour ARM ;
4. lancer sur le PC et vérifier (`scripts/demo.sh` → `DEMO OK`) ;
5. « Sur la carte embarquée » : la section de l'étape 9 de la Phase 5 ;
6. dépannage : les erreurs que vous avez rencontrées et leur solution.

Ce qui ne sert plus est à retirer ou à réécrire : le titre « squelette du
projet », la section « Vérifier une phase », le tableau « fourni / à
écrire », la ligne « 2 tests fournis ». `INTERFACES.md` reste à part (le
contrat) et le README peut y renvoyer. Vos choix techniques et vos
difficultés vont dans la présentation, l'historique dans `JOURNAL.md`.

C'est la preuve d'un critère de l'énoncé : votre procédure est
**rejouable par quelqu'un d'autre**. De 14 h à 15 h, chaque groupe suit
le README de son groupe partenaire, **à la lettre**, sur sa propre VM, et
sans aide de ses auteurs.

**Les paires :**

| Groupe | teste le README de |
|---|---|
| gp1 | gp2 |
| gp2 | gp1 |
| gp3 | gp4 |
| gp4 | gp3 |
| gp5 | gp6 |
| gp6 | gp5 |

**L'accès.** Dès jeudi, donnez au groupe partenaire l'accès en lecture à
votre dépôt GitLab (*Manage › Members*, rôle *Reporter*). Vérifiez avec
lui qu'il peut cloner : à 14 h, il sera trop tard.

**Le périmètre :**

1. cloner le dépôt et installer ce que le README demande ;
2. compiler pour le PC et pour ARM, et lancer les tests ;
3. `scripts/demo.sh` → `DEMO OK` ;
4. déployer leurs binaires sur **votre** carte, en suivant leur procédure.

Ne reconstruisez pas leur image Buildroot : c'est plusieurs heures.

**Le retour.** À 15 h, envoyez au groupe partenaire, par écrit, chaque
endroit où vous avez bloqué : l'étape, la commande, ce que vous attendiez
et ce qui s'est passé. Vous corrigez ensuite votre propre README d'après
le retour que vous avez reçu.

En retard vendredi ? Reportez l'échange au lundi matin, en accord avec le
groupe partenaire. Vous corrigez alors votre README après le tag `v1.0` :
posez un tag **`v1.0.1` le lundi avant midi**, qui ne modifie **que le
`README.md`**. Tout autre changement après `v1.0` ne sera pas pris en
compte.

### La répétition chronométrée (facultative)

C'est une suggestion, pas une obligation. Les groupes qui le souhaitent
s'entendent entre eux pour répéter l'un devant l'autre, dans les
conditions de lundi :

- **15 minutes**, démonstration comprise (5 à 7 minutes) ;
- **sur BBB**, en partage d'écran, comme lundi : vérifiez le son, la
  fenêtre partagée et la taille de la police dans les terminaux ;
- puis 10 minutes de questions du groupe spectateur, posées à **chacun**,
  y compris sur les briques qu'il n'a pas portées.

Le groupe spectateur chronomètre et note ce qui n'était pas clair. Vous
corrigerez votre présentation d'après ses remarques. Pas le temps
vendredi ? Le lundi matin convient aussi.

### Le tag `v1.0`

Avant 17 h, sur `main` : `passerelle/Version.cpp` annonce `1.0.0`, le `JOURNAL.md`
est à jour, le tag `v1.0` est poussé.

---

## Checklist de fin de phase

- [ ] socle complet fusionné sur `main`, critères des phases 1 à 5 repassés
- [ ] extensions (au moins deux, au choix, pour le trinôme)
- [ ] `soutenance/EmbedGate-gpN.pdf` poussé sur `main` **avant 14 h**, et annoncé sur Discord
- [ ] la présentation se comprend sans vous, et chaque diapositive parle de ce que **vous** avez fait
- [ ] la diapositive « L'équipe », avec un micro-CV par membre
- [ ] des chiffres et des captures de **votre** dépôt
- [ ] accès en lecture au dépôt donné au groupe partenaire **dès jeudi**
- [ ] README réécrit pour un utilisateur (plus rien du squelette qui ne serve)
- [ ] README suivi par le groupe partenaire, retour écrit reçu, README corrigé
- [ ] `passerelle/Version.cpp` annonce `1.0.0`, tag `v1.0` poussé **avant 17 h**, `JOURNAL.md` tenu
