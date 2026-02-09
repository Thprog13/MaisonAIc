#include <stdio.h>
#include <string.h>

int main() {
    char username[20];
    char password[20];

    // Identifiants corrects (exemple)
    char correctUser[] = "admin";
    char correctPass[] = "1234";

    printf("=== Page de connexion ===\n");

    printf("Nom d'utilisateur : ");
    scanf("%19s", username);

    printf("Mot de passe : ");
    scanf("%19s", password);

    if (strcmp(username, correctUser) == 0 && strcmp(password, correctPass) == 0) {
        printf("\nConnexion reussie ✅\n");
    } else {
        printf("\nNom d'utilisateur ou mot de passe incorrect ❌\n");
    }

    return 0;
}
