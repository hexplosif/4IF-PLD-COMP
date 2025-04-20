# PLD COMPILATION - Compilateur C vers Assembleur Multi-Cible
Hexanôme : 4221

Membres :
- NGO Truong Son
- SCHLEE Adam
- FAKRONI Mohamed
- SUN Jixiang
- VU Thi Tho
- HUYNH Huu Thanh Tu
- FORERO GUTIERREZ Santiago

## Description Générale

Ce projet implémente un compilateur capable de traduire un sous-ensemble significatif du langage C en code assembleur pour deux architectures (x86-64, ARM64). Le compilateur gère un large éventail de fonctionnalités du C, de la gestion des types de base aux structures de contrôle complexes, en passant par l'analyse statique pour assurer la robustesse du code généré. Le processus de compilation s'appuie sur une Représentation Intermédiaire (IR) pour faciliter l'optimisation et le reciblage.

## Fonctionnalités Implémentées

Notre compilateur prend en charge les caractéristiques suivantes du langage C :

### Types de Données et Constantes
* **Types de base :**
    * `int` (entier signé 32 bits)
    * `float` (flottant double précision)
    * `char` (caractère)
    * `void` (principalement comme type de retour de fonction)
* **Conversions implicites :** Gestion des conversions automatiques entre les types numériques (par exemple, `int` vers `float`).
* **Constantes :**
    * Entières (ex : `123`, `-5`)
    * Caractères (ex : `'a'`, `'\n'`)

### Variables et Portée
* **Déclaration :** Variables locales et globales. Possibilité de déclarer des variables n'importe où dans un bloc (style C++ / C99).
* **Initialisation :** Possibilité d'initialiser une variable lors de sa déclaration (ex : `int count = 0;`).
* **Portée (Scope) :** Gestion correcte de la portée des variables grâce aux blocs `{ ... }`.
* **Masquage (Shadowing) :** Support du masquage de variables (une variable locale peut masquer une variable globale ou d'un bloc englobant).
* **Affectation :** Opérateur d'affectation simple (`=`) et opérateurs d'affectation composée (`+=`, `-=` etc.). L'affectation est une expression qui retourne une valeur.

### Opérateurs
* **Arithmétiques :** Addition (`+`), Soustraction (`-`), Multiplication (`*`).
* **Comparaison :** Égalité (`==`), Différence (`!=`), Inférieur (`<`), Supérieur (`>`).
* **Logiques (évaluation paresseuse) :** OU logique (`||`), ET logique (`&&`).
* **Logiques bit-à-bit :** OU (`|`), ET (`&`), OU exclusif (`^`).
* **Unaires :** Négation logique (`!`), Moins unaire (`-`).
* **Incrémentation / Décrémentation :** Préfixe et postfixe (`++var`, `var++`, `--var`, `var--`).

### Structures de Contrôle
* **Conditionnelles :** `if`, `else`.
* **Boucles :** `while`, `for`.
* **Blocs :** Utilisation de `{ ... }` pour délimiter les blocs d'instructions et les portées.

### Fonctions
* **Définition :** Définition de fonctions avec paramètres typés.
* **Types de retour :** Support des types de retour `int` et `void`.
* **Appel :** Appels de fonctions avec passage de paramètres.
* **Retour :** Instruction `return` permettant de retourner une valeur (pour les fonctions `int`) ou de terminer l'exécution (pour `void`). Le `return expression;` peut apparaître n'importe où dans le corps de la fonction.

### Structures de Données
* **Tableaux :** Support des tableaux à une dimension.
* **Chaînes de caractères :** Représentées classiquement comme des tableaux de `char` (`char[]`).
* **Structures :** Définition et utilisation de `struct`.

### Analyse Statique et Vérifications
* **Cohérence des appels de fonction :** Vérification du nombre et (potentiellement) du type des paramètres lors des appels.
* **Gestion des déclarations :**
    * Vérification qu'une variable utilisée a bien été déclarée au préalable.
    * Vérification qu'une variable n'est pas déclarée plusieurs fois dans la même portée.
* **Utilisation des variables :** Vérification qu'une variable déclarée est effectivement utilisée dans le code (aide à détecter le code mort ou les erreurs).

### Gestion du Code Source et Compilation
* **Fichier unique :** Le compilateur traite un seul fichier source `.c`.
* **Commentaires :** Les commentaires C sont correctement ignorés.
* **Représentation Intermédiaire (IR) :** Le code source est traduit en une IR avant la génération du code final.
* **Génération de code :**
    * Production de code assembleur.
    * Reciblage vers plusieurs architectures : x86-64, ARM64.


## Navigation dans le Code

Le code source est organisé de manière modulaire pour une meilleure maintenabilité :

- `CodeGenVisitor.cpp` / `CodeGenVisitor.h` : Responsable de la traversée de l'AST (Arbre Syntaxique Abstrait) et de la génération de la Représentation Intermédiaire ou du code assembleur final. Contient la logique de traduction des différentes constructions du langage.
- `SymbolTable.cpp` / `SymbolTable.h` : Implémente la table des symboles pour gérer les informations sur les variables (type, portée, adresse mémoire/registre).
- `main.cpp` : Point d'entrée du programme. Gère les arguments de la ligne de commande, lance l'analyse lexicale/syntaxique et initie le processus de compilation (visite de l'AST).


## Gestion de Projet

### Organisation
Les tâches ont été définies et réparties entre les membres de l'équipe, en utilisant un tableau collaboratif pour suivre l'avancement.


## Compilation et Exécution

### Compilation du Projet
Utilisation de `make` pour compiler le projet :
```sh
cd compiler/
make
```

### Exécution
Pour compiler un fichier source :
```sh
./ifcc fichier.c -o fichier.s
```

Pour assembler et exécuter le programme généré :
```sh
gcc fichier.s -o fichier.out
./fichier.out
```

Pour afficher les valeurs retourné dans le shell :
```sh
echo $?
```
## Tests

Pour lancer les tests :
```sh
python3 ifcc-test.py testfiles/
```

!!! Important : Pour le test 43 `43_getchar`, il faut rentrer 2 caractères car il y a deux getchar() (un pour gcc et un pour ifcc)