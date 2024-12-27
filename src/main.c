#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h" // Inclusion du fichier d'en-tête pour le client
#include "server.h" // Inclusion du fichier d'en-tête pour le serveur

int main() {
    printf("Début du projet Random Balance Test.\n");

    // Exécuter la logique côté serveur
    printf("Exécution du serveur...\n");
    executer_serveur();

    // Exécuter la logique côté client
    printf("Exécution du client...\n");
    executer_client();

    printf("Fin du projet Random Balance Test.\n");
    return 0;
}