># PLD COMPILATION
Hexanôme : 4221

## Description des Fonctionnalités Implémentées

Ce projet implémente un compilateur permettant de traduire un sous-ensemble du langage C en code assembleur x86-64.

### Fonctionnalités principales :
- **Déclarations de variables** : Gestion des variables locales et globales.
- **Affectations** : Affectation de valeurs aux variables.
- **Opérations arithmétiques** : Addition, soustraction, multiplication, division.
- **Opérations logiques et bit à bit** : AND, OR, XOR, NOT.
- **Comparaisons** : Égalité, inégalités.
- **Gestion des scopes** : Portée des variables et allocations mémoires par pile.
- **Génération de code assembleur** : Compilation vers un fichier assemblé exécutable.

## Navigation dans le Code

Le code est organisé dans différents fichiers pour assurer la modularité :

- `CodeGenVisitor.cpp` : Génération du code assembleur.
- `CodeGenVisitor.h` : Déclaration des fonctions du visiteur.
- `SymbolTable.cpp` et `SymbolTable.h` : Gestion des tables de symboles et des variables.
- `main.cpp` : Point d'entrée du compilateur.

### Points clés dans `CodeGenVisitor.cpp` :
- `visitProg` : Point d'entrée de la génération du code, gestion du prologue et épilogue.
- `visitBlock` : Gestion des blocs `{}` et des scopes de variables.
- `visitDecl_stmt` : Déclaration et initialisation des variables.
- `visitAssign_stmt` : Affectation des variables.
- `visitReturn_stmt` : Gestion des instructions `return`.
- `visitAddSubExpression`, `visitMulDivExpression` : Gestion des opérations mathématiques.
- `visitComparisonExpression` : Gestion des comparaisons.

## Gestion de Projet

### Organisation 
Toutes les tâches du sujet ont été recopié dans un Google Slide sur lequel on attribue chaque tâche à un binôme.

### Avancement passé
- Mise en place de l'architecture du compilateur.
- Implémentation des bases du `CodeGenVisitor`.
- Support des opérations arithmétiques et des variables.
- Gestion des scopes et allocation mémoire.
- Génération du code assembleur minimal exécutable.

### Étapes à venir
- Implémentation de l'IR
- Ajout du support des structures de contrôle (if, while, for).
- Support des autres fonctionnalités prioritaires ainsi que certainne fonctionnalisté secondaires (tableaux, +=, ++, ect...)
- Ajout de tests unitaires et d'intégration.

## Compilation et Exécution

### Compilation
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

Pour lancer le test 43 `43_getchar`, il faut rentrer 2 caractères car il y a deux getchar() (un pour gcc et un pour ifcc)