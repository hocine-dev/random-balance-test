# Random Balance Test

## Description

Le projet **Random Balance Test** a pour objectif de tester l'équilibre des valeurs générées par la fonction `rand()` en C dans un intervalle de 1 à 1 milliard. En générant 1 milliard de nombres aléatoires, nous vérifions si ces valeurs sont uniformément réparties, afin d'évaluer l'efficacité de la fonction `rand()` dans un grand intervalle.

Le projet utilise deux ordinateurs en parallèle, communiquant via des **sockets** et exécutant des tâches en **multithreading** pour optimiser l'exécution et rendre le test plus représentatif.

## But du projet

L'objectif principal de ce projet est de :

- Tester l'équilibre des valeurs générées par la fonction `rand()` sur un large intervalle [1, 1 milliard].
- Exécuter le test sur deux machines via des **sockets** pour améliorer la précision et réduire le temps d'exécution.
- Utiliser **le multithreading** pour répartir les tâches de génération de nombres entre plusieurs threads afin de maximiser les performances.

## Prérequis

Avant de commencer, assurez-vous d'avoir les éléments suivants :

- Un compilateur C/C++ (ex. GCC).
- Une machine avec un système d'exploitation compatible pour exécuter des programmes en C.
- Une connexion réseau entre deux ordinateurs pour la communication via des **sockets**.
- La bibliothèque **pthread** pour le multithreading.

## Installation

1. Clonez ce dépôt :

   ```bash
   git clone https://github.com/ton-utilisateur/random-balance-test.git
   cd random-balance-test

   ```

2. Compilez le projet avec GCC ou un autre compilateur compatible :

```bash
make
```

3. Sur chaque machine, lancez le programme correspondant (serveur ou client).

Lancer le serveur (Machine 1) :
```bash
./random_balance_test_server
```


Lancer le client (Machine 2) :
```bash
./random_balance_test_client
```

Structure du projet
Le projet est organisé comme suit :

```bash
random-balance-test/
├── README.md          # Ce fichier
├── src/               # Code source
│   ├── main.c         # Programme principal pour générer et envoyer des nombres
│   ├── generator.c    # Code pour générer les nombres
│   ├── generator.h    # Header pour le générateur de nombres
│   ├── server.c       # Code pour le serveur qui reçoit et traite les données
│   └── server.h       # Header pour le serveur
├── data/              # Dossier pour stocker les résultats ou fichiers de test
│   └── results.txt    # Résultats ou fichiers de logs générés
└── Makefile           # Fichier pour construire le projet
```



Le serveur et le client échangeront les données via des sockets pour effectuer le test. Le programme génère et analyse 1 milliard de valeurs aléatoires, puis compare leur répartition pour évaluer l'équilibre de la fonction rand().

Résultats attendus
Après avoir exécuté le programme, les résultats seront stockés dans un fichier log ou affichés à l'écran. Les valeurs doivent être réparties de manière relativement uniforme dans l'intervalle de 1 à 1 milliard. Toute distribution irrégulière pourrait indiquer un problème avec la fonction rand().

Contribuer
Les contributions sont les bienvenues ! Si tu souhaites contribuer, ouvre une pull request ou signale des problèmes via les issues.
