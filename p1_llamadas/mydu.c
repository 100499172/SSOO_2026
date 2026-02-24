#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>   // para opendir, readdir, etc
#include <limits.h>   // para PATH_MAX

const char *binary_file = "mydu.bin";

// estructura que guardaremos en el .bin de golpe
typedef struct {
    char path[PATH_MAX];
    int size;
} DuRecord;

// funcion recursiva para meterse en las carpetas y sumar tamaños
int calcular_tamano(const char *ruta) {
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    int total = 0;
    char ruta_act[PATH_MAX];

    // primero sacamos el tamano del propio directorio en el que estamos
    if (lstat(ruta, &st) == 0) {
        // st_blocks cuenta bloques de 512 bytes. Dividimos entre 2 para sacar los KB
        total += st.st_blocks / 2;
    }

    dir = opendir(ruta);
    if (dir == NULL) {
        return total; // si no hay permisos o falla, devolvemos lo que tengamos
    }

    // leemos todo lo que hay dentro de la carpeta
    while ((ent = readdir(dir)) != NULL) {
        // hay que saltarse . y .. para no entrar en bucle infinito
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        // montamos la ruta completa del nuevo archivo/carpeta
        snprintf(ruta_act, sizeof(ruta_act), "%s/%s", ruta, ent->d_name);

        if (lstat(ruta_act, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // si es otra carpeta, nos llamamos a nosotros mismos
                total += calcular_tamano(ruta_act);
            } else {
                // si es un archivo normal, sumamos su tamaño en KB
                total += st.st_blocks / 2;
            }
        }
    }
    
    closedir(dir);
    return total;
}

int main(int argc, char *argv[]) {
    // control basico de argumentos
    if (argc > 2) {
        printf("Error: demasiados argumentos\n");
        return -1;
    }

    // --- MODO HISTORIAL (-b) ---
    if (argc == 2 && strcmp(argv[1], "-b") == 0) {
        int fd = open(binary_file, O_RDONLY);
        if (fd < 0) {
            printf("Error: No existe el historial\n");
            return -1;
        }

        DuRecord reg;
        // leemos cajas (structs) enteras hasta que se acabe el archivo
        while (read(fd, &reg, sizeof(DuRecord)) > 0) {
            printf("%d\t%s\n", reg.size, reg.path);
        }
        
        close(fd);
        return 0;
    }

    // --- MODO NORMAL (calcular tamano) ---
    char target[PATH_MAX];
    
    // si no le pasamos nada, usamos el directorio actual (.)
    if (argc == 1) {
        strcpy(target, ".");
    } else {
        strcpy(target, argv[1]);
    }

    struct stat st;
    if (lstat(target, &st) < 0) {
        printf("Error: el archivo o directorio no existe\n");
        return -1;
    }

    // comprobamos que sea una carpeta de verdad y no un archivo suelto
    if (!S_ISDIR(st.st_mode)) {
        printf("Error: %s no es un directorio\n", target);
        return -1;
    }

    // llamamos a la funcion que hace el trabajo duro
    int tamano = calcular_tamano(target);
    printf("%d\t%s\n", tamano, target);

    // guardamos el resultado en mydu.bin (sin machacar lo anterior)
    int fd = open(binary_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        printf("Error al abrir el historial para guardar\n");
        return -1;
    }

    // rellenamos el struct y lo tiramos al archivo binario
    DuRecord reg;
    strcpy(reg.path, target);
    reg.size = tamano;
    
    write(fd, &reg, sizeof(DuRecord));
    close(fd);

    return 0;
}
