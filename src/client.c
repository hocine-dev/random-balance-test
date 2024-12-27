#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include "client.h"

#define NOMBRE_FILS 6              // Nombre de processus à créer
#define NOMBRE_VALEURS 60000000000 // Nombre total de valeurs générées par le client (60 milliards)
#define TAILLE_MEMOIRE 1000000     // Taille de chaque segment de mémoire partagée
#define CLE_MEMOIRE 1234           // Clé pour la mémoire partagée
#define CLE_SEMAPHORE 5678         // Clé pour le sémaphore
#define PORT_SERVEUR 8080          // Port utilisé pour communiquer avec le serveur
#define IP_SERVEUR "127.0.0.1"     // Adresse IP du serveur

// Structure pour encapsuler les sémaphores
int initialiser_semaphore(int cle) {
    int semaphore = semget(cle, 1, IPC_CREAT | 0666);
    if (semaphore < 0) {
        perror("Erreur lors de la création du sémaphore");
        exit(1);
    }
    semctl(semaphore, 0, SETVAL, 1); // Initialiser à 1
    return semaphore;
}

// Fonction pour verrouiller (P)
void verrouiller_semaphore(int semaphore) {
    struct sembuf operation = {0, -1, 0};
    semop(semaphore, &operation, 1);
}

// Fonction pour déverrouiller (V)
void deverrouiller_semaphore(int semaphore) {
    struct sembuf operation = {0, 1, 0};
    semop(semaphore, &operation, 1);
}

// Fonction pour générer des valeurs aléatoires
void generer_valeurs(int *memoire_partagee, int semaphore, long debut, long fin) {
    srand(time(NULL) + getpid()); // Graine unique par processus
    for (long i = debut; i < fin; i++) {
        int valeur_aleatoire = rand() % 1000000000 + 1;

        // Accès protégé à la mémoire partagée
        verrouiller_semaphore(semaphore);
        memoire_partagee[i % TAILLE_MEMOIRE] = valeur_aleatoire; // Stocker dans la mémoire partagée
        deverrouiller_semaphore(semaphore);
    }
}

// Fonction pour envoyer des données au serveur via un socket
void envoyer_donnees_serveur(int *memoire_partagee) {
    int socket_client;
    struct sockaddr_in adresse_serveur;

    // Créer un socket
    if ((socket_client = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur lors de la création du socket");
        exit(1);
    }

    // Configurer l'adresse du serveur
    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(PORT_SERVEUR);
    adresse_serveur.sin_addr.s_addr = inet_addr(IP_SERVEUR);

    // Se connecter au serveur
    if (connect(socket_client, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0) {
        perror("Erreur lors de la connexion au serveur");
        exit(1);
    }

    // Envoyer les données de la mémoire partagée au serveur
    if (send(socket_client, memoire_partagee, TAILLE_MEMOIRE * sizeof(int), 0) < 0) {
        perror("Erreur lors de l'envoi des données au serveur");
        exit(1);
    }

    printf("Données envoyées au serveur.\n");
    close(socket_client); // Fermer la connexion
}

void executer_client() {
    int memoire_id, semaphore;
    int *memoire_partagee;
    pid_t fils[NOMBRE_FILS];
    long taille_par_fils = NOMBRE_VALEURS / NOMBRE_FILS;

    // Création de la mémoire partagée
    memoire_id = shmget(CLE_MEMOIRE, TAILLE_MEMOIRE * sizeof(int), IPC_CREAT | 0666);
    if (memoire_id < 0) {
        perror("Erreur lors de la création de la mémoire partagée");
        exit(1);
    }

    // Attachement à la mémoire partagée
    memoire_partagee = (int *)shmat(memoire_id, NULL, 0);
    if (memoire_partagee == (int *)-1) {
        perror("Erreur lors de l'attachement à la mémoire partagée");
        exit(1);
    }

    // Initialisation du sémaphore
    semaphore = initialiser_semaphore(CLE_SEMAPHORE);

    // Création des processus fils
    for (int i = 0; i < NOMBRE_FILS; i++) {
        long debut = i * taille_par_fils;
        long fin = (i == NOMBRE_FILS - 1) ? NOMBRE_VALEURS : debut + taille_par_fils;

        if ((fils[i] = fork()) == 0) {
            // Code du fils
            generer_valeurs(memoire_partagee, semaphore, debut, fin);
            envoyer_donnees_serveur(memoire_partagee); // Envoyer les données au serveur
            shmdt(memoire_partagee); // Détacher la mémoire partagée
            exit(0);
        }
    }

    // Attendre que tous les fils terminent
    for (int i = 0; i < NOMBRE_FILS; i++) {
        waitpid(fils[i], NULL, 0);
    }

    printf("Client : Toutes les valeurs ont été générées et envoyées au serveur.\n");

    // Nettoyage
    shmdt(memoire_partagee); // Détacher la mémoire partagée
    shmctl(memoire_id, IPC_RMID, NULL); // Supprimer la mémoire partagée
    semctl(semaphore, 0, IPC_RMID); // Supprimer le sémaphore
}