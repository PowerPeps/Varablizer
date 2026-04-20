GUIDE Varablizer

Varablizer est un langage haut niveau compilé en bytecode et exécuté par une machine virtuelle dédiée. Le projet couvre l'ensemble du pipeline : lexing, parsing, AST, résolution de types, génération de code et exécution. Cette architecture propose une base compacte, conçue pour être étendue et maintenue au fil du temps sans réécriture majeure. Chaque couche est isolée, ce qui permet d'intervenir sur un composant sans affecter les autres.

La syntaxe complète du langage Varablizer (.vz) est décrite dans le fichier `features.vz` à la racine du dépôt.

Vue d'ensemble

Le dépôt contient trois blocs principaux :

- le compilateur : transforme du code source `.vz` en instructions bytecode ;
- la VM : conçue pour exécuter du bitcode (.vbin), elle fonctionne en LIFO (last in, first out) avec une pile d'évaluation typée, des frames d'appel et un tas adressable par octet ;
- artisan : un utilitaire pour les développeurs, accompagné d'une suite de fichiers de test pour valider les comportements de base.

Le compilateur

Les sections suivantes détaillent chaque étape de la compilation.

1) Lexer

Le lexer lit le texte source et le découpe en tokens. Il reconnaît les nombres (décimaux et hexadécimaux), les littéraux de caractères, les identifiants, les mots-clés, les opérateurs et les séparateurs. Il filtre les espaces et les commentaires puis fournit un flux ordonné de tokens au parser.

Exemple : la ligne `b_8 int x = 10;` produit les tokens suivants.

```
[TYPE_PREFIX: b_8] [TYPE: int] [IDENT: x] [ASSIGN: =] [NUMBER: 10] [SEMICOLON: ;]
```

2) Parser

Le parser est de type recursive descent : il consomme les tokens et reconstruit la structure logique du programme. Il gère le nombre d'arguments et la priorité des opérateurs (multiplication avant addition, etc.), les appels de fonction, les expressions ternaires, et toutes les constructions de contrôle (if, for, while, do/while). Le résultat est un AST (arbre syntaxique).

Exemple : l'expression `a + b * c` produit l'arbre suivant.

```
      (+)
      / \
     a   (*)
         / \
        b   c
```

La multiplication est évaluée en premier grâce à sa priorité supérieure.

3) AST (Abstract Syntax Tree)

L'AST est composé de noeuds pour les littéraux, variables, opérations, appels, déclarations, blocs et fonctions. Chaque noeud conserve une référence de position (ligne et colonne dans le fichier source), utilisée lors de la génération des messages d'erreur. L'AST sépare les responsabilités : les passes suivantes ne travaillent pas sur du texte mais sur une structure arborescente.

4) Résolution de types

Après parsing, une passe assigne un `vtype` à chaque expression et variable : tailles différentes (8/16/32/64 bits), pointeur vs non-pointeur, signed/unsigned. C'est dans cette passe que sont résolues les conversions nécessaires entre types (élargissement d'un 8 bits vers un 64 bits, ou troncature inverse) pour que la génération de code produise des opérations cohérentes.

Exemple : dans l'expression `b_8 int a + b_64 int b`, la valeur de `a` est élargie à 64 bits avant l'addition.

5) Table des symboles et pré-scan

La table des symboles gère les portées locales et les fonctions. Avant la génération du bytecode, le compilateur effectue un pré-scan pour collecter les signatures et réserver les emplacements locaux : ceci permet de connaître l'adresse d'entrée de chaque fonction et le nombre de slots locaux nécessaires.

6) Génération de code (CodeGen)

L'AST est transformé en une suite d'instructions pour la VM.

- les instructions sont produites via la fonction `emit(opcode, operand)` qui ajoute des entrées dans le vecteur d'instructions ;
- pour les sauts ou appels vers des cibles dont l'adresse n'est pas encore connue, des emplacements temporaires sont réservés et des références sont enregistrées ;
- après la génération, la passe `resolve_labels()` remplace ces références par les adresses finales.

Le CodeGen calcule aussi les offsets des variables locales, installe les instructions de réserve de frame (FRAME) et compose l'opérande de `CALL` avec les métadonnées nécessaires (nombre d'arguments, taille des locaux, etc.).

Exemple : la fonction suivante

```c
int function add(b_8 int x, b_8 int y) {
    return x + y;
}
```

produit les instructions

```
FRAME       2
LOAD_LOCAL  0       // x
LOAD_LOCAL  1       // y
ADD
RET
```

7) La façade Compiler

La couche publique expose `compile(source)` qui produit un programme en mémoire, et `compile_file(in, out)` qui écrit un `.vbin`. Les erreurs sont signalées avec un message incluant la position source (ligne, colonne) et la nature de l'erreur.

La VM

La pile d'évaluation est typée, les frames d'appel gèrent les contextes de fonction, et le tas fonctionne par allocation linéaire (bump allocator). Les handlers d'opcodes sont répartis par groupes : MATH, STACK, LOCALS, HEAP, CALL, FLOW, BITWISE, COMPARE, IO, CONTROL, DEBUG.

Les handlers sont dispatchés via une table indexée par la valeur de l'opcode. Pour activer ou désactiver un groupe entier, il suffit de modifier la macro correspondante dans `opcodes_config.h`. Par exemple, passer `ENABLE_BITWISE` de 1 à 0 désactive tous les opcodes bitwise à la compilation, sans toucher au reste du code.

Le format binaire

Chaque instruction est stockée ainsi : un octet (opcode) suivi d'un blob de 64 bits (opérande). L'opérande n'est pas un entier signé, c'est un champ brut dont l'interprétation dépend de l'opcode (adresse, valeur immédiate, métadonnées de frame, etc.). Le disassembleur fourni dans `src/execute/generated` permet d'inspecter le contenu d'un `.vbin`.

Build et exécution

1. Générer le projet avec CMake dans un répertoire `build`.
2. Construire les cibles `varablizer` (VM) et `artisan`.

```powershell
cmake -S . -B build
cmake --build build --config Debug --target varablizer
cmake --build build --config Debug --target artisan
```

Pour lancer la suite de tests (script .ps1, Windows uniquement) :

```powershell
.\tests\run_tests.ps1
```

Exemple complet : compilation et exécution

Fichier `tests/example_add.vz` :

```c
int function add(b_8 int x, b_8 int y) {
    return x + y;
}

void function main() {
    pf(add(10, 20));
}
```

Compilation :

```powershell
.\build\artisan.exe compile tests\example_add.vz -o tests\example_add.vbin
```

Exécution :

```powershell
.\build\varablizer.exe tests\example_add.vbin
```

Sortie :

```
30
```

Optimisation des opcodes existants

Le jeu d'opcodes actuel est déjà étendu (plus de 70 opcodes répartis en 11 groupes). L'objectif n'est pas d'en ajouter systématiquement, mais d'optimiser ceux qui existent : réduire les instructions redondantes, fusionner des séquences fréquentes, ou améliorer les handlers existants.

Création d'un opcode : exemple concret

Pour illustrer le mécanisme, voici la création d'un opcode PICK qui dépile le troisième élément de la pile.

Etape 1 : générer le squelette avec artisan.

```powershell
.\build\artisan.exe make:opcode PICK 0x70 STACK
```

Cette commande crée le handler dans `src/execute/opcodes/stack.h` et met à jour la table des opcodes.

Etape 2 : implémenter le handler dans `stack.h`.

```cpp
/* [[Opcode::PICK, 0x70, Group::STACK]] */
static void h_pick(assembly& vm) noexcept
{
    if (vm.eval_.size() < 3)
    {
        vm.halted_ = true;
        return;
    }
    auto [val, type] = vm.eval_.pick(2);
    vm.eval_.remove(2);
    vm.eval_.push(val, type);
}
```

Etape 3 : reconstruire le projet. Le générateur d'opcodes met automatiquement à jour l'enum, la table de dispatch et le disassembleur.

```powershell
cmake --build build --config Debug --target varablizer
```

Exemple .vz : tri à bulles

```c
void function sort(b_8 int* arr, b_8 int n) {
    for (b_8 int i = 0, i < n - 1, i++) {
        for (b_8 int j = 0, j < n - i - 1, j++) {
            if (arr[j] > arr[j + 1]) {
                b_8 int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

void function main() {
    b_8 int[5] data = [5, 3, 1, 4, 2];
    sort(&data[0], 5);
    for (b_8 int i = 0, i < 5, i++) {
        pf(data[i]);
    }
}
```

Sortie attendue :

```
1
2
3
4
5
```

Diagnostics

En cas d'erreur de compilation, `artisan compile` renvoie un message contenant la position source (ligne/colonne) et la nature de l'erreur (lexicale, syntaxique, type, etc.). Le dossier `src/execute/generated` contient un disassembleur auto-généré et la table des handlers, utiles pour inspecter le bytecode produit.
