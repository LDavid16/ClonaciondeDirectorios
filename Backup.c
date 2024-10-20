#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#define MAX 256

void intToStr(int num, char *str);

int main(int argc, char *argv[])
{
    __pid_t pid;
    FILE *archivo;
    char comando[256];
    int pfd[2];
    int pfd2[2];
    char mensaje[MAX] = "Hola hijo, realiza el respaldo de archivos";
    char mensajehijo[MAX] = "cuantos archivaldos?";
    char bufferHijo[MAX];
    char bufferPadre[MAX];

    char ruta_lista[200];

    // Verificar que se pasen las rutas de origen y destino
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s /ruta/origen /ruta/destino\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Verificacion del pipe
    if (pipe(pfd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(pfd2) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Rutas de origen y destino
    char *origen = argv[1];
    char *destino = argv[2];

    // Crear el proceso hijo
    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    { // Proceso hijo

        close(pfd[1]);  // Cerrar el extremo de escritura del pipe
        close(pfd2[0]); // cerrar lectura del pipe2
        
        printf("\n\nHIJO(pid=%d): esperando mensaje de mi padre...", getpid());
        read(pfd[0], bufferHijo, MAX);
        printf("\nHIJO(pid=%d): Instrucción de mi padre: %s\n", getpid(), bufferHijo);
        fflush(stdout);

        write(pfd2[1], mensajehijo, MAX);
    
        read(pfd[0], bufferHijo, MAX);

        int numArch;
        int resta = 1;
        numArch = atoi(bufferHijo);

        printf("=======RESPALDANDO %d ARCHIVOS=======\n", numArch);
        fflush(stdout);

        for (int i = 0; i < numArch; i++)
        {
            read(pfd[0], bufferHijo, MAX);
            printf("\n\tHijo(pid=%d),respaldando el archivo %s   pendientes:%d/%d\n", getpid(), bufferHijo, (numArch - resta), numArch);
            fflush(stdout);
            resta++;
            
        }

        write(pfd2[1], "Adios Padre, termine el respaldo...", 36);
        snprintf(comando, sizeof(comando), "mv %s %s", origen, destino);
        system(comando);

    
    }
    else
    { // Proceso padre

        // Cerrar el extremo de lectura del pipe
        close(pfd[0]);
        close(pfd2[1]); // cerramos escritura del pipe2

        printf("\nPADRE(pid=%d): generando LISTA DE ARCHIVOS A RESPALDAR", getpid());
        printf("\nPADRE(pid=%d): borrando respaldo viejo...\n", getpid());

     
        snprintf(comando, sizeof(comando), "test -d %s", destino);
        if (system(comando) != 0)
        {
           
            printf("\nEl directorio de destino no existe. Creando...\n");
            snprintf(comando, sizeof(comando), "mkdir -p %s", destino);
            if (system(comando) == -1)
            {
                perror("mkdir");
                exit(EXIT_FAILURE);
            }
            printf("Directorio de destino creado: %s\n", destino);
        }
        else
        {
            printf("El directorio de destino ya existe: %s\n", destino);
        }
        
        snprintf(ruta_lista, sizeof(ruta_lista), "%s/Respaldo.txt", destino);
        archivo = fopen(ruta_lista, "w");

        char mander[MAX];
        snprintf(mander, sizeof(mander), "ls -l %s > %s", origen,ruta_lista);
        system(mander);
        if (archivo == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Escribir los nombres de archivos y directorios del directorio de origen en lista.txt
        DIR *dir;
        struct dirent *ent;        
        int total_archivos = 0;

        if ((dir = opendir(origen)) != NULL)
        {
            // Recorrer los archivos y directorios en el origen
            while ((ent = readdir(dir)) != NULL)
            {
                // Ignorar "." y ".."
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
                {
                    printf("\nremoved '%s'\n", ent->d_name);
                    total_archivos++;
                }
            }
            printf("\nremoved directory '%s'\n\n", origen);
            closedir(dir);
        }
        else
        {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        

        int numero;

        numero = total_archivos;

        fprintf(archivo, "%d ARCHIVOS RESPALDO \n", total_archivos);
        fclose(archivo);

        // Enviar el mensaje al hijo para que comience el respaldo
        write(pfd[1], mensaje, MAX);

        read(pfd2[0], bufferPadre, MAX);

        printf("\nPADRE(pid=%d)->mensaje de mi hijo:%s\n", getpid(), bufferPadre);
        fflush(stdout);

        intToStr(total_archivos, bufferPadre);
        write(pfd[1], bufferPadre, MAX);

        if ((dir = opendir(origen)) != NULL)
        {
            while ((ent = readdir(dir)) != NULL)
            {
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
                {
                    printf("\n(PADRE-->%s)\n", ent->d_name);
                    write(pfd[1], ent->d_name, sizeof(ent->d_name));
                }
            }
            closedir(dir);
        }
        else
        {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        read(pfd2[0], bufferPadre, MAX);
        printf("\nPADRE(pid=%d), Mensaje del hijo:<---%s", getpid(), bufferPadre);
        printf("\nPADRE(pid=%d), recibe el TOTAL de %d archivos respaldados con exito", getpid(), numero);

        printf("\nPADRE(pid=%d), comprobando respaldo:", getpid());
        printf("\n======================================================================\n");
        archivo = fopen(ruta_lista, "r");
        if (archivo == NULL)
        {
            perror("Error al abrir el archivo");
            return 1;
        }
        char linea[256];

        while (fgets(linea, sizeof(linea), archivo))
        {
            printf("%s", linea);
        }

        fclose(archivo);
        printf("\n======================================================================\n");
        printf("\nTermino el proceso padre...\n");
        wait(NULL);
    }

    return 0;
}

void intToStr(int num, char *str)
{
    sprintf(str, "%d", num);
}
