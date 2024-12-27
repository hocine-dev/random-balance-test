#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include "server.h"
#include "client.h" // Inclusion du fichier d'en-tête du client pour les fonctions partagées

#define NOMBRE_FILS 6              // Nombre de processus à créer
#define NOMBRE_VALEURS 60000000000 // Nombre total de valeurs générées par le serveur (60 milliards)
#define TAILLE_MEMOIRE 1000000     // Taille de chaque segment de mémoire partagée
#define CLE_MEMOIRE 1234           // Clé pour la mémoire partagée
#define CLE_SEMAPHORE 5678         // Clé pour le sémaphore
#define PORT_SERVEUR 8080          // Port utilisé pour communiquer avec le client

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

// Fonction pour recevoir des données du client via un socket
void recevoir_donnees_client(int *memoire_partagee) {
    int socket_serveur, socket_client;
    struct sockaddr_in adresse_serveur, adresse_client;
    socklen_t taille_client = sizeof(adresse_client);

    // Créer un socket
    if ((socket_serveur = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur lors de la création du socket");
        exit(1);
    }

    // Configurer l'adresse du serveur
    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(PORT_SERVEUR);
    adresse_serveur.sin_addr.s_addr = INADDR_ANY;

    // Lier le socket à l'adresse du serveur
    if (bind(socket_serveur, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0) {
        perror("Erreur lors du bind");
        exit(1);
    }

    // Écouter les connexions entrantes
    if (listen(socket_serveur, 5) < 0) {
        perror("Erreur lors de l'écoute");
        exit(1);
    }

    // Accepter une connexion du client
    if ((socket_client = accept(socket_serveur, (struct sockaddr *)&adresse_client, &taille_client)) < 0) {
        perror("Erreur lors de l'acceptation de la connexion");
        exit(1);
    }

    // Recevoir les données du client
    if (recv(socket_client, memoire_partagee, TAILLE_MEMOIRE * sizeof(int), 0) < 0) {
        perror("Erreur lors de la réception des données du client");
        exit(1);
    }

    printf("Données reçues du client.\n");
    close(socket_client); // Fermer la connexion
    close(socket_serveur); // Fermer le socket du serveur
}

void calculer_equilibrage(int *memoire_partagee) {
    long long somme = 0;
    for (long i = 0; i < TAILLE_MEMOIRE; i++) {
        somme += memoire_partagee[i];
    }
    printf("Somme totale des valeurs : %lld\n", somme);
}

void executer_serveur() {
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

    // Recevoir les données du client
    recevoir_donnees_client(memoire_partagee);

    // Création des processus fils
    for (int i = 0; i < NOMBRE_FILS; i++) {
        long debut = i * taille_par_fils;
        long fin = (i == NOMBRE_FILS - 1) ? NOMBRE_VALEURS : debut + taille_par_fils;

        if ((fils[i] = fork()) == 0) {
            // Code du fils
            generer_valeurs(memoire_partagee, semaphore, debut, fin);
            shmdt(memoire_partagee); // Détacher la mémoire partagée
            exit(0);
        }
    }

    // Attendre que tous les fils terminent
    for (int i = 0; i < NOMBRE_FILS; i++) {
        waitpid(fils[i], NULL, 0);
    }

    printf("Serveur : Toutes les valeurs ont été générées.\n");

    // Calculer l'équilibrage des résultats
    calculer_equilibrage(memoire_partagee);

    // Nettoyage
    shmdt(memoire_partagee); // Détacher la mémoire partagée
    shmctl(memoire_id, IPC_RMID, NULL); // Supprimer la mémoire partagée
    semctl(semaphore, 0, IPC_RMID); // Supprimer le sémaphore
}

int main() {
    printf("Début du serveur Random Balance Test.\n");

    // Exécuter la logique côté serveur
    printf("Exécution du serveur...\n");
    executer_serveur();

    printf("Fin du serveur Random Balance Test.\n");
    return 0;
}