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

#define MILLIARD 1000000000LL // definire la constante  1 milliard
#define MILLION 1000000LL     // definire la constante  1 million

#define MAX_RAND MILLIARD // Valeur maximale pour les nombres aléatoires

#define MAX_FILS 6 // Nombre de processus enfants

#define PORT 8080 // Port pour la connexion réseau

#define NOMBRE_DE_VALEURS (1 * MILLIARD) // Nombre total de valeurs à générer

#define NVPF (NOMBRE_DE_VALEURS / MAX_FILS) // Nombre total de valeurs générées par les processus fils

#define CHUNKSIZE MILLION // Taille du chunk pour la fusion des données

// Fonction pour envoyer les données au serveur

void envoyer_donnees(int *tab)
{

    int sock = 0;

    struct sockaddr_in serv_addr;

    // Création du socket

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {

        perror("Erreur de création du socket");

        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons(PORT);

    // Conversion de l'adresse IP en format binaire

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {

        perror("Adresse non valide");

        exit(EXIT_FAILURE);
    }

    // Connexion au serveur

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {

        perror("Erreur de connexion");

        exit(EXIT_FAILURE);
    }

    // Envoi des données au serveur

    send(sock, tab, NOMBRE_DE_VALEURS * sizeof(int), 0);

    printf("[Client] Données envoyées au serveur avec succès.\n");

    // Fermeture du socket

    close(sock);
}

int main()
{

    printf("[Client] Début du programme client...\n");

    // Allocation de mémoire partagée pour le tableau global

    int *tab_global = mmap(NULL, NOMBRE_DE_VALEURS * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (tab_global == MAP_FAILED)
    {

        // Gestion de l'erreur si l'allocation échoue

        perror("Erreur mmap");

        exit(EXIT_FAILURE);
    }

    // Initialisation du tableau global à zéro

    memset(tab_global, 0, NOMBRE_DE_VALEURS * sizeof(int));

    // Création d'un tableau de sémaphores pour chaque portion du tableau global

    sem_t *sems[MAX_FILS];

    for (int i = 0; i < MAX_FILS; i++)
    {

        char sem_name[20];

        sprintf(sem_name, "semaphore_%d", i); // Génération d'un nom unique pour chaque sémaphore

        sems[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1); // Création du sémaphore

        sem_unlink(sem_name); // Suppression du nom du sémaphore pour éviter les conflits futurs
    }

    printf("[Client] Création des processus enfants...\n");

    // Diviser l'indice de tableau entre les processus fils

    int chunk_size = NOMBRE_DE_VALEURS / MAX_FILS;

    for (int i = 0; i < MAX_FILS; i++)
    {

        if (fork() == 0)
        {

            printf("[Processus Fils %d] Génération des nombres aléatoires...\n", i + 1);

            // Initialisation de la graine pour rand()

            srand(time(NULL) + getpid());

            // Allocation de mémoire pour le tableau local

            int *tab_local = calloc(NVPF, sizeof(int));

            // Calculer la portion spécifique du tableau que ce processus va traiter

            int start_idx = i * chunk_size;

            // Génération des nombres aléatoires et incrémentation du tableau local

            for (int j = 0; j < NVPF; j++)
            {

                unsigned int random_value = rand() % MAX_RAND; // Utilisation de rand() pour générer un nombre aléatoire

                tab_local[j] = random_value;
            }

            // Gestion de la fusion des chunks

            for (int chunk = 0; chunk < (NVPF / CHUNKSIZE); chunk++)
            {

                // Attente du sémaphore du chunk

                sem_wait(sems[i]);

                // Fusion du chunk

                for (int k = chunk * CHUNKSIZE; k < (chunk + 1) * CHUNKSIZE && k < NVPF; k++)
                {

                    tab_global[start_idx + k] = tab_local[k];
                }

                // Libération du sémaphore du chunk

                sem_post(sems[i]);
            }

            printf("[Processus Fils %d] Fusion terminée.\n", i + 1);

            // Libération de la mémoire allouée pour le tableau local

            free(tab_local);

            // Terminer le processus fils

            exit(0);
        }
    }

    // Attendre que tous les processus enfants terminent

    for (int i = 0; i < MAX_FILS; i++)
    {

        wait(NULL);
    }

    printf("[Client] Tous les processus enfants ont terminé. Envoi du tableau global au serveur...\n");

    // Envoyer les données au serveur

    envoyer_donnees(tab_global);

    // Détacher et libérer la mémoire partagée

    munmap(tab_global, NOMBRE_DE_VALEURS * sizeof(int));

    printf("[Client] Fin du programme client.\n");

    return 0;
}
