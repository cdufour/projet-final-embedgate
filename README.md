# Projet final — EmbedGate

Projet de validation du cursus LLS / EXPLEO (AJC Formation), du 12 au
23 octobre 2026, soutenance le 26 : une passerelle de supervision pour
équipements connectés. Elle surveille des capteurs, donne l'alerte en cas
d'anomalie, affiche un tableau de bord et tourne sur une cible ARM émulée.

## Contenu

| Élément | Rôle |
| --- | --- |
| [`projet-final-embedgate-enonce.md`](projet-final-embedgate-enonce.md) | L'énoncé : le besoin, l'architecture, les règles, la grille d'évaluation. À lire en entier avant de commencer |
| [`agenda-phases-embedgate.md`](agenda-phases-embedgate.md) | Le parcours en 6 phases : quoi livrer, quand, et les critères de chaque phase |
| [`squelette/`](squelette/) | Le point de départ de votre projet, à copier dans le dépôt de votre groupe |
| [`squelette/INTERFACES.md`](squelette/INTERFACES.md) | Le contrat technique (registres, MQTT, journal, programmes). Il fait foi |
| [`phase1-instructions.md`](phase1-instructions.md) | La Phase 1 pas à pas : les étapes, comment vérifier chacune, les pièges, la checklist de fin de phase |
| [`phase2-instructions.md`](phase2-instructions.md) | La Phase 2 pas à pas : de la puce au superviseur, les bibliothèques autorisées, les pièges, la checklist de fin de phase |
| [`groupes.txt`](groupes.txt) | La composition des groupes |
| [EmbedGate en schémas](https://ajc.opusidea.fr/embedgate/) | Le site compagnon : le projet expliqué en schémas (source : [`site/`](site/)) |

D'autres ressources pourront être ajoutées ici pendant le projet.

## Démarrer

```bash
git clone https://github.com/cdufour/projet-final-embedgate.git
cp -r projet-final-embedgate/squelette/. <votre-depot-embedgate>/
cd <votre-depot-embedgate>
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
```

La suite (paquets à installer, compilation pour ARM, lancement de la puce)
est dans [`squelette/README.md`](squelette/README.md).

Pour récupérer plus tard les ajouts de ce dépôt : `git pull` dans
`projet-final-embedgate/`.

## Donner accès au formateur

Le dépôt de votre groupe est sur GitLab. Ajoutez-y le formateur avec le rôle
**Reporter** (*Manage → Members → Invite members*) :

- compte GitLab : **`@cdufour1`** (nom affiché : « Opusidea ») ;
- attention, `cdufour` tout court est une autre personne sur GitLab (c'est
  le nom du compte **GitHub** du formateur, celui de ce dépôt).

Puis **envoyez l'adresse de votre dépôt au formateur** (une par groupe) :
GitLab ne le prévient pas quand vous l'ajoutez.
