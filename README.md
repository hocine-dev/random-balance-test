# Random Balance Test

## Description

Le projet **Random Balance Test** a pour objectif de tester l'équilibre des valeurs générées par la fonction `rand()` en C dans un intervalle de 1 à 1 milliard. En générant 3 milliards de nombres aléatoires, nous vérifions si ces valeurs sont uniformément réparties, afin d'évaluer l'efficacité de la fonction `rand()` dans un grand intervalle.

Le projet utilise deux ordinateurs en parallèle, communiquant via des **sockets** pour optimiser l'exécution et rendre le test plus représentatif.

## But du projet

L'objectif principal de ce projet est de :

- Tester l'équilibre des valeurs générées par la fonction `rand()` sur un large intervalle [1, 1 milliard].
- Exécuter le test sur deux machines via des **sockets** pour améliorer la précision et réduire le temps d'exécution.
- Répartir les tâches de génération, afin de maximiser les performances.

## Prérequis

Avant de commencer, assurez-vous d'avoir les éléments suivants :

- Un compilateur C (ex. GCC).
- Une machine avec un système d'exploitation compatible pour exécuter des programmes en C.
- Une connexion réseau entre deux ordinateurs pour la communication via des **sockets**.

## Installation

1. Clonez ce dépôt :

   ```bash
   git clone https://github.com/ton-utilisateur/random-balance-test.git
   cd random-balance-test

2.Compilez le projet avec GCC ou un autre compilateur compatible :
 ```bash
    make
3.Sur chaque machine, lancez le programme correspondant (serveur ou client).

3.1.Lancer le serveur (Machine 1) :
 ```bash
./server_program

3.2.Lancer le client (Machine 2) :
 ```bash
./client_program

4.Structure du projet
Le projet est organisé comme suit :
random-balance-test/
├── .gitignore
├── .vscode/
│   └── tasks.json
├── LICENSE
├── Makefile
├── README.md
├── src/
│   ├── client.c
│   └── server.c

5.Utilisation
Serveur : Le serveur génère des nombres aléatoires et les stocke dans un tableau partagé. Il attend ensuite les données du client pour les fusionner et calculer le coefficient de variation.

Client : Le client génère également des nombres aléatoires, les stocke dans un tableau partagé, puis envoie ces données au serveur.

Communication : Les deux programmes communiquent via des sockets pour échanger les données générées.

Évaluation : Le serveur calcule le coefficient de variation des valeurs combinées pour déterminer si la distribution est équilibrée.

6.Résultats attendus
Après avoir exécuté le programme, les résultats seront affichés à l'écran. Les valeurs doivent être réparties de manière relativement uniforme dans l'intervalle de 1 à 1 milliard. Toute distribution irrégulière pourrait indiquer un problème avec la fonction rand().

7.Contribuer
Les contributions sont les bienvenues ! Si vous souhaitez contribuer, ouvrez une pull request ou signalez des problèmes via les issues.

8.Licence
Ce projet est sous licence MIT. Voir le fichier LICENSE pour plus de détails.
