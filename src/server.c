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

#define MILLIARD 1000000000LL // definire la constante  1 milliard
#define MILLION 1000000LL     // definire la constante  1 million

#define PORT 8080                               // Port pour la connexion réseau
#define MAX_RAND MILLIARD                       // Valeur maximale pour les nombres aléatoires
#define MAX_FILS 6                              // Nombre de processus enfants du serveur
#define NOMBRE_DE_VALEURS (2 * MILLIARD)        // Nombre total de valeurs à générer
#define NVPF (NOMBRE_DE_VALEURS / 2 / MAX_FILS) // Nombre total de valeurs générées par les processus fils
#define CHUNKSIZE (50 * MILLION)                // Taille de chaque chunk

// Fonction pour calculer le coefficient de variation
void coef_variation(int *tab)
{
    long double somme = 0.0;
    double moyenne = 0.0;
    float variance = 0.0, ecart_type = 0.0, coef_variation = 0.0;

    // Calcul de la somme des valeurs
    for (int i = 0; i < NOMBRE_DE_VALEURS; i++)
    {
        somme += tab[i];
    }
    // Calcul de la moyenne
    moyenne = somme / NOMBRE_DE_VALEURS;

    // Calcul de la variance
    for (int i = 0; i < NOMBRE_DE_VALEURS; i++)
    {
        variance += (tab[i] - moyenne) * (tab[i] - moyenne);
    }
    variance /= NOMBRE_DE_VALEURS;
    ecart_type = sqrt(variance);
    coef_variation = ecart_type / moyenne;

    // Affichage du coefficient de variation
    printf("[Serveur] Coefficient de variation final : %f\n", coef_variation);
    if (coef_variation < 0.05)
    {
        printf("[Serveur] La distribution est équilibrée.\n");
    }
    else
    {
        printf("[Serveur] La distribution n'est pas équilibrée.\n");
    }
}

int main()
{
    printf("[Serveur] Démarrage du serveur...\n");

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Création du tableau global partagé avec mmap
    int *tab_global = mmap(NULL, NOMBRE_DE_VALEURS * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (tab_global == MAP_FAILED)
    {
        perror("Erreur mmap");
        exit(EXIT_FAILURE);
    }
    // Initialisation du tableau global à zéro
    memset(tab_global, 0, NOMBRE_DE_VALEURS * sizeof(int));

    // Création des sémaphores pour chaque chunk
    sem_t *sem_chunks[NVPF / CHUNKSIZE];
    for (int i = 0; i < NVPF / CHUNKSIZE; i++)
    {
        char sem_name[20];
        snprintf(sem_name, sizeof(sem_name), "semaphore_chunk_%d", i);
        sem_chunks[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
        sem_unlink(sem_name); // Unlink après création pour éviter les conflits
    }

    printf("[Serveur] Création des processus enfants pour générer des données...\n");

    // Création des processus enfants du serveur pour générer des données
    for (int i = 0; i < MAX_FILS; i++)
    {
        if (fork() == 0)
        {
            printf("[Processus Serveur %d] Génération des nombres aléatoires...\n", i + 1);

            // Initialisation de la graine pour rand() avec srand
            srand(time(NULL) + getpid()); // Utilisation de time(NULL) et getpid() pour garantir une graine unique par processus

            // Allocation de mémoire pour le tableau local
            int *tab_local = calloc(NVPF, sizeof(int));

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
                sem_wait(sem_chunks[chunk]);

                // Fusion du chunk
                for (int k = chunk * CHUNKSIZE; k < (chunk + 1) * CHUNKSIZE && k < NVPF; k++)
                {
                    tab_global[k + (i * NVPF)] = tab_local[k];
                }

                // Libération du sémaphore du chunk
                sem_post(sem_chunks[chunk]);
            }

            printf("[Processus Serveur %d] Fusion terminée.\n", i + 1);

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

    printf("[Serveur] Tous les processus enfants ont terminé la génération des données.\n");

    // Création du socket pour la communication avec le client
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Erreur de création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Liaison du socket à l'adresse et au port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Erreur de liaison (bind)");
        exit(EXIT_FAILURE);
    }
    // Mise en écoute du socket
    if (listen(server_fd, 3) < 0)
    {
        perror("Erreur d'écoute");
        exit(EXIT_FAILURE);
    }

    printf("[Serveur] En attente de connexion de client...\n");

    while (1)
    {
        // Acceptation de la connexion du client
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Erreur d'acceptation");
            exit(EXIT_FAILURE);
        }

        printf("[Serveur] Connexion acceptée. Réception des données...\n");

        // Allocation de mémoire pour recevoir les données du client
        int *tab_recu = malloc((NOMBRE_DE_VALEURS / 2) * sizeof(int));
        recv(new_socket, tab_recu, (NOMBRE_DE_VALEURS / 2) * sizeof(int), 0);
        printf("[Serveur] Données reçues du client.\n");

        // Fusion des données du client avec le tableau global
        sem_wait(sem_chunks[0]); // Attente du sémaphore du premier chunk
        for (int i = 0; i < (NOMBRE_DE_VALEURS / 2); i++)
        {
            tab_global[i + (NOMBRE_DE_VALEURS / 2)] = tab_recu[i];
        }
        sem_post(sem_chunks[0]); // Libération du sémaphore du premier chunk

        printf("[Serveur] Fusion terminée.\n");

        // Libération de la mémoire allouée pour les données reçues
        free(tab_recu);
        close(new_socket);

        // Calcul et affichage du coefficient de variation
        coef_variation(tab_global);
    }

    // Fermeture du socket du serveur
    close(server_fd);

    // Détacher et libérer la mémoire partagée
    munmap(tab_global, NOMBRE_DE_VALEURS * sizeof(int));

    return 0;
}
