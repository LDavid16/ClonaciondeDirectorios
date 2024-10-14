#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

int contar_archivos(const char *ruta);

int main(int argc, char *argv[]) {
    pid_t pid;
    FILE *archivo;
    int pfd[2];
    char ruta_lista[256];

    // Verificar que se pasen las rutas de origen y destino
    if (argc != 3) {
        fprintf(stderr, "Uso: %s /ruta/origen /ruta/destino\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Verificacion del pipe

    if (pipe(pfd)==-1)
    {
        perror("pipe");
        exit(1);
    }

    

    // Rutas de origen y destino
    char *origen = argv[1];
    char *destino = argv[2];

    
    // Crear el proceso hijo
    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Proceso hijo
        // Ejecutar el comando "cp -r /ruta/origen /ruta/destino"
        printf("\n");
        execlp("cp", "cp", "-r",origen, destino, NULL);


        // Si execlp falla
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {  // Proceso padre

        printf("\nPADRE(pid=%d): generando LISTA DE ARCHIVOS A RESPALDAR",getpid());
        printf("\nPADRE(pid=%d): borrando respaldo viejo...",getpid());

        //revisando si el directorio existe
        struct stat st;
        
        if (stat(destino, &st) == -1) {
        // Si el directorio de destino no existe, lo crea
        printf("\nEl directorio de destino no existe. Creando...\n");
        if (mkdir(destino, 0700) == -1) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
        printf("Directorio de destino creado: %s\n", destino);
        } else {
            printf("El directorio de destino ya existe: %s\n", destino);
        }
            wait(NULL);
            printf("Copia completa.\n");

        // Crear el archivo lista.txt en el directorio de destino
        snprintf(ruta_lista, sizeof(ruta_lista), "%s/Respaldo.txt", destino);
        archivo = fopen(ruta_lista, "w");
        if (archivo == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Escribir los nombres de archivos y directorios del directorio de origen en lista.txt
        DIR *dir;
        struct dirent *ent;
        int total_archivos = 0;

        if ((dir = opendir(origen)) != NULL) {
            // Recorrer los archivos y directorios en el origen
            while ((ent = readdir(dir)) != NULL) {
                // Ignorar "." y ".."
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    fprintf(archivo, "%s\n", ent->d_name);
                    printf("\nremoved '%s' \n",ent->d_name);
                    total_archivos++;
                }
            }
            closedir(dir);
        } else {
            perror("opendir");

            
        
            exit(EXIT_FAILURE);
        }

        // Escribir el número total de archivos y directorios en lista.txt
        fprintf(archivo, "Total de archivos y directorios: %d\n", total_archivos);
        fclose(archivo);
    }

    return 0;
}
