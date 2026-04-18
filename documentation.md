GUIDE Varablizer

Si vous tombez sur ce dépôt, bienvenue : on écrit un langage minimal, on le compile en bytecode, et on l’exécute dans une VM qu’on peut modifier à la volée.

Pourquoi ce projet est intéressant
Parce qu’il rassemble l’essentiel sans trop de couches : lexing, parsing, AST, résolution de types, génération de code, et une VM basée sur une pile. C’est parfait pour apprendre en touchant au code — ajouter un opcode, modifier la gestion des frames, ou tester une optimisation basique prend peu de temps et donne un feedback immédiat.

Vue d’ensemble du contenu
Le dépôt contient trois blocs principaux :

- le compilateur : transforme du code source `.vz` en instructions bytecode ;
- la VM : exécute le bytecode, gère les appels, la pile et un tas simple ;
- des outils et tests : une CLI pratique (artisan) et une série de fichiers de test pour valider les comportements de base.

Le compilateur, point par point (avec un peu plus de détail)
Je détaille ici les parties qui vous intéresseront si vous voulez toucher la compilation.

1) Lexer
Le lexer lit le texte et le découpe en tokens. Il reconnaît les nombres (décimaux et hexadécimaux), les littéraux de caractères, les identifiants, les mots-clés, les opérateurs et les séparateurs. Il enlève les espaces et les commentaires et fournit un flux ordonné de tokens au parser. Si vous voulez modifier la syntaxe (par exemple ajouter un nouvel opérateur), c’est souvent ici qu’il faut commencer.

2) Parser
Le parser est de type « recursive descent » : il consomme les tokens et reconstruit la structure logique du programme. Il gère l’arité et la priorité des opérateurs (multiplication avant addition, etc.), les appels de fonction, les expressions ternaires, et toutes les constructions de contrôle (if, for, while, do/while). Le résultat est un AST (arbre syntaxique) riche en information.

3) AST (Abstract Syntax Tree)
L’AST est composé de nœuds pour les littéraux, variables, opérations, appels, déclarations, blocs et fonctions. Chaque nœud conserve sa position source (utile pour produire des messages d’erreur clairs). L’AST facilite la séparation des responsabilités : les passes suivantes ne travaillent pas sur du texte mais sur une structure directe.

4) Résolution de types
Après parsing, une passe assigne un `vtype` à chaque expression et variable : tailles différentes (8/16/32/64 bits), pointeur vs non-pointeur, signed/unsigned. Cette passe décide aussi des conversions nécessaires entre types pour que la génération de code produise des opérations cohérentes (promotion et troncature).

5) Table des symboles et pré-scan
La table des symboles gère les portées locales et les fonctions. Avant d’émettre le code, le compilateur effectue un pré-scan pour collecter les signatures et réserver les emplacements locaux : ceci permet de connaître l’adresse d’entrée de chaque fonction et le nombre de slots locaux nécessaires.

6) Génération de code (CodeGen)
C’est l’étape où l’AST devient une suite d’instructions pour la VM. Quelques points pratiques :

- on émet les instructions via une fonction `emit(opcode, operand)` qui ajoute des entrées dans le vecteur d’instructions ;
- pour les sauts ou appels vers des cibles inconnues, on émet des emplacements à patcher et on enregistre des références à résoudre plus tard ;
- après la génération, une passe `resolve_labels()` remplace ces références par les adresses finales.

Le CodeGen s’occupe aussi de calculer les offsets des variables locales, d’installer les instructions de réserve de frame (FRAME) et de composer l’opérande de `CALL` avec les métadonnées nécessaires (nombre d’arguments, taille des locaux, etc.). Si vous voulez comprendre comment une fonction source devient des instructions d’appel/retour, c’est ici qu’il faut regarder.

7) La façade `Compiler`
La couche publique est simple : elle offre `compile(source)` qui produit un programme en mémoire, et `compile_file(in, out)` qui écrit un `.vbin`. Les erreurs sont signalées avec des messages incluant la position source quand c’est possible.

La VM en bref
La VM est volontairement minimale mais complète : pile d’évaluation typée, frames d’appel, un tas byte-addressable et des handlers d’opcodes répartis par groupes (math, stack, locals, heap, call, etc.). Les handlers sont dispatchés via une table, ce qui rend l’ajout ou la désactivation d’un groupe d’opcodes relativement simple.

Le format binaire
Chaque instruction est stockée comme un octet (opcode) suivi d’un entier 64 bits (opérande). Simple, pragmatique, et facile à déboguer avec le disassembleur fourni.

Démarrage rapide
Si vous voulez tester rapidement :

1. Générer le projet avec CMake dans un répertoire `build`.
2. Construire les cibles `varablizer` (VM) et `artisan` (outil CLI).

Commandes utiles (PowerShell) :

```powershell
cmake -S . -B build
cmake --build build --config Debug --target varablizer
cmake --build build --config Debug --target artisan
```

Pour lancer la suite de tests fournie :

```powershell
.\tests\run_tests.ps1
```

Idées pour contribuer (rapide)
- Ajouter un opcode et son handler ; utiliser le générateur d’opcodes pour maintenir la table et le disassembleur.
- Améliorer la gestion du tas (le projet utilise un bump allocator simple pour l’instant).
- Écrire des tests unitaires ciblés pour le lexer, le parser et le codegen.

Souhaitez-vous que j’ajoute : un exemple pas-à-pas montrant la compilation d’une petite fonction, une génération automatique de `OPCODES.md`, ou la création d’un mini-tutoriel pour écrire un `.vz` simple ? Dites laquelle et je la crée.
Exemple pas-à-pas : compiler et exécuter une petite fonction

Voici un exemple concret et rapide pour illustrer le flux complet : écrire une petite fonction, la compiler en `.vbin` puis l’exécuter.

1) Écrire le fichier source
Créez un fichier `tests/example_add.vz` (ou `example.vz`) contenant :

```c
// Exemple : fonction add et appel
int function add(b_8 int x, b_8 int y) {
	return x + y;
}

void function main() {
	pf(add(10, 20)); // attend 30
}
```

2) Compiler en `.vbin`
Utilisez l’outil `artisan` fourni par le projet (ou appelez directement la façade `Compiler`). Depuis la racine du dépôt, la commande PowerShell :

```powershell
.
tools\artisan\artisan.exe compile tests\example_add.vz -o tests\example_add.vbin
```

Remarque : si vous avez construit `artisan` dans `build`, adaptez le chemin vers l’exécutable.

3) Exécuter le binaire
Lancez la VM sur le `.vbin` généré. Deux possibilités :

- lancer directement l’exécutable `varablizer.exe` :

```powershell
.
build\varablizer.exe tests\example_add.vbin
```

- ou utiliser `artisan execute` pour trouver automatiquement le binaire construit :

```powershell
.
tools\artisan\artisan.exe execute tests\example_add.vbin
```

4) Résultat attendu
La sortie attendue affichera la valeur calculée par `add(10, 20)` :

```
30
```

Notes et diagnostics rapides
- Si la compilation échoue, vérifiez le message d’erreur renvoyé par `artisan compile` : il contient généralement la position source (ligne/colonne) et la nature de l’erreur (lexicale, syntaxique, type, etc.).
- Pour déboguer le bytecode, vous pouvez regarder le dossier `src/execute/generated` qui contient un disassembleur auto-généré et la table des handlers. On peut aussi afficher l’AST ou insérer des logs dans `CodeGen` pour voir les instructions émises.

Souhaitez-vous que je crée réellement `tests/example_add.vz` dans le dépôt et exécute la compilation ici (si vous voulez que je commit/ajoute le fichier) ?
