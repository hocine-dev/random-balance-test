#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <arpa/inet.h>

#include <string.h>

#include <sys/mman.h>

#include <semaphore.h>

#include <fcntl.h>

#include <time.h>

#include <sys/wait.h>



#define MAX_RAND 32767

#define MAX_FILS 6  // Nombre de processus enfants

#define PORT 8080

#define MILLIARD 1000000000LL

#define NOMBRE_DE_VALEURS (600 * MILLIARD)  // Nombre totale de valeurs a generé

#define NVPF  (0.01 * MILLIARD)  // Nombre totale de valeurs a generé par les processus fils

#define CHUNKSIZE 50000000 // Taille du chunk



// Générateur Linéaire Congruent (GLC)

unsigned int glc(unsigned int seed) {

    static unsigned int a = 1664525, c = 1013904223, m = 0xFFFFFFFF;

    return (a * seed + c) % m;

}



void envoyer_donnees(int *tab) {

    int sock = 0;

    struct sockaddr_in serv_addr;



    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

        perror("Erreur de création du socket");

        exit(EXIT_FAILURE);

    }

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons(PORT);



    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {

        perror("Adresse non valide");

        exit(EXIT_FAILURE);

    }



    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {

        perror("Erreur de connexion");

        exit(EXIT_FAILURE);

    }



    printf("[Client] Envoi du tableau global au serveur...\n");

    send(sock, tab, MAX_RAND * sizeof(int), 0);

    printf("[Client] Données envoyées au serveur avec succès.\n");

    close(sock);

}



int main() {

    printf("[Client] Début du programme client...\n");



    int *tab_global = mmap(NULL, MAX_RAND * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (tab_global == MAP_FAILED) {

        perror("Erreur mmap");

        exit(EXIT_FAILURE);

    }

    memset(tab_global, 0, MAX_RAND * sizeof(int));



    sem_t *sem = sem_open("semaphore", O_CREAT | O_EXCL, 0644, 1);

    sem_unlink("semaphore");



    printf("[Client] Création des processus enfants...\n");



    for (int i = 0; i < MAX_FILS; i++) {

    if (fork() == 0) {

        printf("[Processus Fils %d] Génération des nombres aléatoires...\n", i + 1);

        

        // Initialisation de la graine pour rand()

        srand(time(NULL) + getpid());  // Utilisation de time(NULL) et getpid() pour garantir une graine unique par processus



        int *tab_local = calloc(MAX_RAND, sizeof(int));

        int chunk_count = 0;  // Compteur de valeurs générées



        for (int j = 0; j < NVPF; j++) {

            unsigned int random_value = rand() % MAX_RAND;  // Utilisation de rand() pour générer un nombre aléatoire

            tab_local[random_value]++;

            chunk_count++;



            // Fusionner lorsque le chunk de 50 millions est atteint

            if (chunk_count >= CHUNKSIZE) {

                printf("[Processus Fils %d] Fusion après %d valeurs...\n", i + 1, chunk_count);

                sem_wait(sem);

                for (int k = 0; k < MAX_RAND; k++) {

                    tab_global[k] += tab_local[k];

                }

                sem_post(sem);



                // Réinitialiser le tableau local pour le prochain chunk

                memset(tab_local, 0, MAX_RAND * sizeof(int));

                chunk_count = 0;  // Réinitialiser le compteur de valeurs générées

            }

        }



        // Fusion finale pour les valeurs restantes

        if (chunk_count > 0) {

            printf("[Processus Fils %d] Fusion finale après %d valeurs...\n", i + 1, chunk_count);

            sem_wait(sem);

            for (int k = 0; k < MAX_RAND; k++) {

                tab_global[k] += tab_local[k];

            }

            sem_post(sem);

        }



        // Affichage d'un échantillon des 10 premières valeurs du tableau local

        printf("[Processus Fils %d] Échantillon des 10 premières valeurs du tableau local :\n", i + 1);

        for (int k = 0; k < 10; k++) {

            printf("tab_local[%d] = %d\n", k, tab_local[k]);

        }



        printf("[Processus Fils %d] Fusion terminée.\n", i + 1);

        free(tab_local);

        munmap(tab_global, MAX_RAND * sizeof(int));

        exit(0);

    }

}



    for (int i = 0; i < MAX_FILS; i++) {

        wait(NULL);

    }

    

    printf("[Client] Échantillon des 10 premières valeurs du tableau global après fusion :\n");

    for (int k = 0; k < 10; k++) {

    printf("tab_global[%d] = %d\n", k, tab_global[k]);

    }



    printf("[Client] Tous les processus enfants ont terminé. Envoi des données...\n");

    envoyer_donnees(tab_global);

    munmap(tab_global, MAX_RAND * sizeof(int));

    printf("[Client] Fin du programme client.\n");

    return 0;

}
