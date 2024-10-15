#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#define MAX 256

int contar_archivos(const char *ruta);
void intToStr(int num, char *str);


int main(int argc, char *argv[]) {
    pid_t pid;
    FILE *archivo;
    int pfd[2];
    int pfd2[2];
    char mensaje[MAX]="Hola hijo, realiza el respaldo de archivos";
    char mensajehijo[MAX]="cuantos archivaldos?";
    char bufferHijo[MAX];
    char bufferPadre[MAX];

    char ruta_lista[MAX];

    // Verificar que se pasen las rutas de origen y destino
    if (argc != 3) {
        fprintf(stderr, "Uso: %s /ruta/origen /ruta/destino\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Verificacion del pipe
    if (pipe(pfd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(pfd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
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
        
        close(pfd[1]);// Cerrar el extremo de escritura del pipe
        close(pfd2[0]);// cerrar lectura del pipe2
        // Leer el mensaje del padre
        printf("\n\nHIJO(pid=%d): esperando mensaje de mi padre...", getpid());
        read(pfd[0], bufferHijo, MAX);
        //close(pfd[0]);
        printf("\nHIJO(pid=%d): Instrucción de mi padre: %s\n", getpid(), bufferHijo);  
        fflush(stdout);

        write(pfd2[1],mensajehijo,MAX);
        //close(pfd2[1]);

        read(pfd[0],bufferHijo,MAX);

        int numArch;
        int resta=1;
        numArch=atoi(bufferHijo);

        printf("=======RESPALDANDO %d ARCHIVOS=======\n",numArch);
        fflush(stdout);

        for (int i = 0; i < numArch; i++)
        {
         read(pfd[0],bufferHijo,MAX);
                printf("\n\tHijo(pid=%d),respaldando el archivo %s   pendientes:%d/%d\n",getpid(), bufferHijo,(numArch-resta), numArch );
                fflush(stdout);
                resta++;
        }

        write(pfd2[1],"Adios Padre, termine el respaldo...",36);
        
        // Ejecutar el comando "cp -r /ruta/origen /ruta/destino"
        execlp("cp", "cp", "-r", origen, destino, NULL);
        

        // Si execlp falla
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {  // Proceso padre

        // Cerrar el extremo de lectura del pipe
        close(pfd[0]);
        close(pfd2[1]);//cerramos escritura del pipe2

        printf("\nPADRE(pid=%d): generando LISTA DE ARCHIVOS A RESPALDAR", getpid());
        printf("\nPADRE(pid=%d): borrando respaldo viejo...", getpid());

        // Revisando si el directorio de destino existe
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
                    printf("\nremoved '%s'\n", ent->d_name);
                    total_archivos++;
                }
            }
            printf("\nremoved directory '%s'\n\n",origen);
            closedir(dir);
        } else {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        int numero;

        numero=total_archivos;

        // Escribir el número total de archivos y directorios en lista.txt
        fprintf(archivo, "Total de archivos y directorios: %d\n", total_archivos);
        fclose(archivo);


        // Enviar el mensaje al hijo para que comience el respaldo
        write(pfd[1], mensaje,MAX );
        
        
        read(pfd2[0],bufferPadre,MAX);
        

        printf("\nPADRE(pid=%d)->mensaje de mi hijo:%s\n",getpid(),bufferPadre);
        fflush(stdout);
        
        
        
        intToStr(total_archivos,bufferPadre);
        write(pfd[1],bufferPadre,MAX);
        


        if ((dir = opendir(origen)) != NULL) {
            // Recorrer los archivos y directorios en el origen
            while ((ent = readdir(dir)) != NULL) {
                // Ignorar "." y ".."
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    printf("\n(PADRE-->%s)\n",ent->d_name);
                    write(pfd[1],ent->d_name,sizeof(ent->d_name));
                     
                    
                }
            }
            closedir(dir);
        } else {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        read(pfd2[0],bufferPadre,MAX);
        printf("\nPADRE(pid=%d), Mensaje del hijo:<---%s",getpid(),bufferPadre);
        printf("\nPADRE(pid=%d), recibe el TOTAL de %d archivos respaldados con exito",getpid(),numero);

        printf("\nPADRE(pid=%d), comprobando respaldo:",getpid());
        printf("\n======================================================================\n");
        archivo=fopen(ruta_lista,"r");
        if (archivo == NULL) {
        perror("Error al abrir el archivo");
        return 1;
        }
        char linea[256];

        while (fgets(linea, sizeof(linea), archivo)) {
        printf("%s", linea);
        }

        fclose(archivo);
        printf("\n======================================================================\n");
        // Esperar la finalización del hijo
       
        
        wait(NULL);
    }
    printf("\nTermino el proceso padre...\n");
    execl("/bin/rm", "rm", "-r", origen, NULL);

    return 0;
}

void intToStr(int num, char *str) {
    sprintf(str, "%d", num);
}

