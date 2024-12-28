#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <string.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <pthread.h>

#include <time.h>

#include <sys/wait.h>



#define PORT 12345

#define NOMBRE_DE_PROCESSUS 6

#define BUFFER_SIZE 256



// Structure pour stocker les valeurs générées et reçues

typedef struct {

    int valeurs_generees[NOMBRE_DE_PROCESSUS];

    int valeurs_reçues[NOMBRE_DE_PROCESSUS];

    pthread_mutex_t mutex;

} serveur_data;



serveur_data data;



// Fonction pour générer une valeur aléatoire

void *generation_valeurs(void *arg) {

    srand(time(NULL) + getpid());  // Initialiser la graine de manière unique



    for (int i = 0; i < NOMBRE_DE_PROCESSUS; i++) {

        int valeur = rand() % 100;  // Générer une valeur aléatoire

        pthread_mutex_lock(&data.mutex);  // Verrouiller l'accès aux données partagées

        data.valeurs_generees[i] = valeur;

        pthread_mutex_unlock(&data.mutex);  // Déverrouiller l'accès aux données partagées



        printf("Serveur (PID: %d) a généré la valeur : %d\n", getpid(), valeur);

        sleep(1);  // Attendre une seconde avant de générer la suivante

    }



    return NULL;

}



// Fonction pour recevoir les données du client

void *recevoir_donnees(void *arg) {

    int server_fd, client_fd;

    struct sockaddr_in server_addr, client_addr;

    socklen_t addr_len = sizeof(client_addr);

    char buffer[BUFFER_SIZE];



    // Création du socket

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {

        perror("Échec de la création du socket");

        exit(1);

    }



    // Configuration de l'adresse du serveur

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr = INADDR_ANY;

    server_addr.sin_port = htons(PORT);



    // Lier le socket à l'adresse

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {

        perror("Échec du binding");

        exit(1);

    }



    // Mise en écoute du serveur

    if (listen(server_fd, 5) < 0) {

        perror("Erreur d'écoute");

        exit(1);

    }



    printf("Serveur en attente de connexion...\n");



    // Attente d'une connexion client

    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {

        perror("Erreur d'acceptation");

        exit(1);

    }



    printf("Client connecté, en attente des données...\n");



    // Attendre que le client envoie une demande pour générer des valeurs

    while (1) {

        memset(buffer, 0, sizeof(buffer));  // Nettoyage du buffer

        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);  // Recevoir les données

        if (n <= 0) {

            if (n == 0) {

                printf("Connexion fermée par le client\n");

            } else {

                perror("Erreur de réception");

            }

            break;  // Sortir de la boucle si la connexion est fermée ou une erreur se produit

        }



        // Affichage du message reçu

        printf("Message reçu du client : %s\n", buffer);



        // Vérifier si le client a envoyé la commande pour commencer la génération

        if (strcmp(buffer, "generer") == 0) {

            // Lancer le thread pour générer les valeurs

            pthread_t thread_generation;

            if (pthread_create(&thread_generation, NULL, generation_valeurs, NULL) != 0) {

                perror("Erreur lors de la création du thread de génération");

                exit(1);

            }



            // Attendre que le thread de génération se termine

            pthread_join(thread_generation, NULL);

        }



        // Sauvegarder la valeur reçue dans la structure partagée

        pthread_mutex_lock(&data.mutex);

        int index = 0;

        while (index < NOMBRE_DE_PROCESSUS && data.valeurs_reçues[index] != 0) {

            index++;

        }

        data.valeurs_reçues[index] = atoi(buffer);  // Convertir la chaîne en entier

        pthread_mutex_unlock(&data.mutex);



        // Affichage de la valeur reçue

        printf("Valeur reçue du client : %s\n", buffer);

    }



    // Fermeture des sockets

    close(client_fd);

    close(server_fd);

    return NULL;

}



int main() {

    pthread_t thread_reception;



    // Initialisation du mutex

    pthread_mutex_init(&data.mutex, NULL);



    // Initialiser les tableaux avec des valeurs par défaut

    memset(data.valeurs_generees, 0, sizeof(data.valeurs_generees));

    memset(data.valeurs_reçues, 0, sizeof(data.valeurs_reçues));



    // Lancer un thread pour recevoir les données des clients

    if (pthread_create(&thread_reception, NULL, recevoir_donnees, NULL) != 0) {

        perror("Erreur lors de la création du thread de réception");

        exit(1);

    }



    // Attendre que le thread de réception se termine

    pthread_join(thread_reception, NULL);



    // Afficher toutes les valeurs générées et reçues

    printf("\nValeurs générées par le serveur :\n");

    for (int i = 0; i < NOMBRE_DE_PROCESSUS; i++) {

        printf("%d ", data.valeurs_generees[i]);

    }

    printf("\n");



    printf("Valeurs reçues du client :\n");

    for (int i = 0; i < NOMBRE_DE_PROCESSUS; i++) {

        if (data.valeurs_reçues[i] != 0) {

            printf("%d ", data.valeurs_reçues[i]);

        }

    }

    printf("\n");



    // Détruire le mutex

    pthread_mutex_destroy(&data.mutex);



    return 0;

}

