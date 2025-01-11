#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <arpa/inet.h>

#include <sys/wait.h>

#include <sys/mman.h>

#include <semaphore.h>

#include <fcntl.h>

#include <math.h>

#include <time.h>



#define PORT 8080

#define MAX_RAND 32767

#define MAX_FILS 6  // Nombre de processus enfants du serveur

#define MILLIARD 1000000000LL

#define NOMBRE_DE_VALEURS (600 * MILLIARD)  // Nombre totale de valeurs a generé

#define NVPF  (0.01 * MILLIARD)  // Nombre totale de valeurs a generé par les processus fils



// Générateur Linéaire Congruent (GLC)

unsigned int glc(unsigned int seed) {

    static unsigned int a = 1664525, c = 1013904223, m = 0xFFFFFFFF;

    return (a * seed + c) % m;

}



void coef_variation(int *tab) {

    long double somme = 0.0;

    double moyenne = 0.0;

    float variance = 0.0, ecart_type = 0.0, coef_variation = 0.0;



    for (int i = 0; i < MAX_RAND; i++) {

        somme += tab[i];

    }

    moyenne = somme / MAX_RAND;



    for (int i = 0; i < MAX_RAND; i++) {

        variance += (tab[i] - moyenne) * (tab[i] - moyenne);

    }

    variance /= MAX_RAND;

    ecart_type = sqrt(variance);

    coef_variation = ecart_type / moyenne;



    printf("[Serveur] Coefficient de variation final : %f\n", coef_variation);

    if (coef_variation < 0.05) {

        printf("[Serveur] La distribution est équilibrée.\n");

    } else {

        printf("[Serveur] La distribution n'est pas équilibrée.\n");

    }

}



int main() {

    printf("[Serveur] Démarrage du serveur...\n");



    int server_fd, new_socket;

    struct sockaddr_in address;

    int addrlen = sizeof(address);



    // Création du tableau global partagé avec mmap

    int *tab_global = mmap(NULL, MAX_RAND * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (tab_global == MAP_FAILED) {

        perror("Erreur mmap");

        exit(EXIT_FAILURE);

    }

    memset(tab_global, 0, MAX_RAND * sizeof(int));



    sem_t *sem = sem_open("semaphore", O_CREAT | O_EXCL, 0644, 1);

    sem_unlink("semaphore");



    printf("[Serveur] Création des processus enfants pour générer des données...\n");



    // Création des processus enfants du serveur pour générer des données

  for (int i = 0; i < MAX_FILS; i++) {

    if (fork() == 0) {

        printf("[Processus Serveur %d] Génération des nombres aléatoires...\n", i + 1);



        // Initialisation de la graine pour rand() avec srand

        srand(time(NULL) + getpid());  // Utilisation de time(NULL) et getpid() pour garantir une graine unique par processus



        int *tab_local = calloc(MAX_RAND, sizeof(int));



        for (int j = 0; j < NVPF; j++) {

            unsigned int random_value = rand() % MAX_RAND;  // Utilisation de rand() pour générer un nombre aléatoire

            tab_local[random_value]++;

        }



        sem_wait(sem);

        for (int k = 0; k < MAX_RAND; k++) {

            tab_global[k] += tab_local[k];

        }

        sem_post(sem);



        printf("[Processus Serveur %d] Fusion terminée.\n", i + 1);

        free(tab_local);

        munmap(tab_global, MAX_RAND * sizeof(int));

        exit(0);

    }

}





    for (int i = 0; i < MAX_FILS; i++) {

        wait(NULL);

    }



    printf("[Serveur] Tous les processus enfants ont terminé la génération des données.\n");



    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {

        perror("Erreur de création du socket");

        exit(EXIT_FAILURE);

    }



    address.sin_family = AF_INET;

    address.sin_addr.s_addr = INADDR_ANY;

    address.sin_port = htons(PORT);



    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {

        perror("Erreur de liaison (bind)");

        exit(EXIT_FAILURE);

    }

    if (listen(server_fd, 3) < 0) {

        perror("Erreur d'écoute");

        exit(EXIT_FAILURE);

    }



    printf("[Serveur] En attente de connexion...\n");



    while (1) {

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {

            perror("Erreur d'acceptation");

            exit(EXIT_FAILURE);

        }



        printf("[Serveur] Connexion acceptée. Réception des données...\n");



        int *tab_recu = malloc(MAX_RAND * sizeof(int));

        recv(new_socket, tab_recu, MAX_RAND * sizeof(int), 0);

        printf("[Serveur] Données reçues du client.\n");



        // Affichage des 10 premières valeurs du tableau reçu

        printf("[Serveur] Échantillon des 10 premières valeurs du tableau reçu :\n");

        for (int i = 0; i < 10; i++) {

            printf("tab_recu[%d] = %d\n", i, tab_recu[i]);

        }



        printf("[Serveur] Fusion des données du client avec le tableau global...\n");

        sem_wait(sem);

        for (int i = 0; i < MAX_RAND; i++) {

            tab_global[i] += tab_recu[i];

        }

        sem_post(sem);

        printf("[Serveur] Fusion terminée.\n");



        // Affichage des 10 premières valeurs du tableau global après fusion

        printf("[Serveur] Échantillon des 10 premières valeurs du tableau global après fusion :\n");

        for (int i = 0; i < 10; i++) {

            printf("tab_global[%d] = %d\n", i, tab_global[i]);

        }



        free(tab_recu);

        close(new_socket);



        coef_variation(tab_global);

    }



    close(server_fd);

    munmap(tab_global, MAX_RAND * sizeof(int));

    return 0;

}
