#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <string.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <sys/shm.h>

#include <sys/sem.h>

#include <time.h>

#include <sys/wait.h>



#define NOMBRE_DE_PROCESSUS 6

#define SERVER_IP "10.0.2.15"

#define PORT 12345

#define MEM_KEY 12345   // Clé de la mémoire partagée

#define SEM_KEY 12346   // Clé du sémaphore



// Structure pour la mémoire partagée

struct shared_data {

    int values[NOMBRE_DE_PROCESSUS];

};



// Fonction pour initialiser le sémaphore

void init_semaphore(int sem_id) {

    for (int i = 0; i < NOMBRE_DE_PROCESSUS; i++) {

        semctl(sem_id, i, SETVAL, 1); // Initialisation à 1 (sémaphore disponible)

    }

}



// Fonction pour manipuler un sémaphore

void P(int sem_id, int index) {

    struct sembuf sem_op = { index, -1, 0 }; // P (down) operation

    semop(sem_id, &sem_op, 1);

}



void V(int sem_id, int index) {

    struct sembuf sem_op = { index, 1, 0 }; // V (up) operation

    semop(sem_id, &sem_op, 1);

}



int main() {

    pid_t pid;

    int sock;

    struct sockaddr_in server_addr;



    // Création du socket

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

        perror("Échec de la création du socket");

        exit(1);

    }



    // Configuration de l'adresse du serveur

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {

        perror("Adresse IP invalide");

        exit(1);

    }



    // Connexion au serveur

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {

        perror("Échec de la connexion au serveur");

        exit(1);

    }



    // Création de la mémoire partagée

    int shmid;

    struct shared_data *shm_data;

    if ((shmid = shmget(MEM_KEY, sizeof(struct shared_data), IPC_CREAT | 0666)) < 0) {

        perror("Erreur de création de la mémoire partagée");

        exit(1);

    }



    // Attachement de la mémoire partagée

    if ((shm_data = (struct shared_data *)shmat(shmid, NULL, 0)) == (void *)-1) {

        perror("Erreur d'attachement de la mémoire partagée");

        exit(1);

    }



    // Création du sémaphore

    int semid;

    if ((semid = semget(SEM_KEY, NOMBRE_DE_PROCESSUS, IPC_CREAT | 0666)) < 0) {

        perror("Erreur de création du sémaphore");

        exit(1);

    }



    // Initialisation du sémaphore

    init_semaphore(semid);



    // Création des processus enfants

    for (int i = 0; i < NOMBRE_DE_PROCESSUS; i++) {

        pid = fork();

        if (pid < 0) {

            perror("Échec de la création du processus");

            exit(1);

        } else if (pid == 0) {

            // Processus enfant



            // Initialisation du générateur de nombres aléatoires avec une graine spécifique

            srand(time(NULL) + getpid());



            // Génération de la valeur aléatoire

            int valeur_aleatoire = rand() % 100;  // Valeur entre 0 et 99



            // Attente avant d'accéder à la mémoire partagée

            P(semid, i);  // Acquérir le sémaphore



            // Stockage de la valeur générée dans la mémoire partagée

            shm_data->values[i] = valeur_aleatoire;



            // Libération du sémaphore

            V(semid, i);  // Libérer le sémaphore



            // Envoi de la valeur au serveur

            char buffer[256];

            snprintf(buffer, sizeof(buffer), "%d", valeur_aleatoire);

            if (send(sock, buffer, strlen(buffer), 0) == -1) {

                perror("Échec de l'envoi de la valeur");

                exit(1);

            }



            printf("Processus enfant %d (PID: %d) a généré et envoyé la valeur : %d\n", i, getpid(), valeur_aleatoire);



            // Attente de 1 seconde avant le prochain envoi

            sleep(1);



            exit(0);

        }

    }



    // Attente de la fin des processus enfants

    for (int i = 0; i < NOMBRE_DE_PROCESSUS; i++) {

        wait(NULL);

    }



    // Affichage des valeurs générées par les processus enfants

    printf("Valeurs générées par les processus enfants (mémoire partagée) :\n");

    for (int i = 0; i < NOMBRE_DE_PROCESSUS; i++) {

        printf("Valeur générée par l'enfant %d : %d\n", i, shm_data->values[i]);

    }



    // Fermeture des sockets et nettoyage

    close(sock);

    shmdt(shm_data);

    shmctl(shmid, IPC_RMID, NULL);

    semctl(semid, 0, IPC_RMID);



    return 0;

}

