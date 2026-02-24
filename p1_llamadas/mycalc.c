#include <fcntl.h>    /* Para open y los flags O_RDONLY, O_CREAT, etc. */
#include <stdlib.h>   /* Para atoi */
#include <string.h>   /* Para strcmp y strlen */
#include <unistd.h>   /* Para write, read y close */
#include <sys/stat.h> /* Para umask */

/* Nombre del archivo de registro de operaciones */
const char *log_file = "mycalc.log";

/**
 * Función auxiliar: esc_num
 * Convierte un entero a caracteres ASCII y los escribe en el descriptor dado.
 * Es necesaria para cumplir con la prohibición de usar stdio.h.
 */
void esc_num(int n, int fd) {
    if (n == 0) { 
        write(fd, "0", 1); 
        return; 
    }
    /* El enunciado confirma que se pueden procesar números negativos */
    if (n < 0) { 
        write(fd, "-", 1); 
        n = -n; 
    }

    char buf[12]; 
    int i = 0;
    /* Extraemos dígitos de derecha a izquierda (unidades, decenas...) */
    while (n > 0) { 
        buf[i++] = (n % 10) + '0'; 
        n /= 10; 
    }
    /* El buffer está al revés, lo escribimos desde el último índice al primero */
    while (i--) {
        write(fd, &buf[i], 1);
    }
}

int main(int argc, char *argv[]) {

    /* --- VALIDACIÓN DE ARGUMENTOS --- */
    /* El programa requiere 3 argumentos para operar o 2 para el historial */
    if (argc < 3 || argc > 4) {
        char *u = "Uso: ./mycalc <n1> <op> <n2> o ./mycalc -b <linea>\n";
        write(2, u, strlen(u)); /* Descriptor 2: Salida de error */
        return -1;
    }

    /* --- CASO 1: OPERACIÓN MATEMÁTICA (./mycalc 5 + 3) --- */
    if (argc == 4) {
        int n1 = atoi(argv[1]);
        char op = argv[2][0];
        int n2 = atoi(argv[3]);
        int res;

        /* Soporte para operaciones de suma y resta según el enunciado */
        if (op == '+') res = n1 + n2;
        else if (op == '-') res = n1 - n2;
        else { 
            write(2, "Error: Operador no valido\n", 26); 
            return -1; 
        }

        /* 1. Mostrar resultado por pantalla (Descriptor 1) */
        write(1, "Resultado: ", 11);
        esc_num(res, 1);
        write(1, "\n", 1);

        /* 2. Guardar en el log (Gestión de máscara para asegurar permisos 0644) */
        mode_t m = umask(0);
        /* O_APPEND permite añadir contenido al final sin borrar lo anterior */
        int fd = open(log_file, O_RDWR | O_CREAT | O_APPEND, 0644);
        umask(m); /* Restauramos la máscara original del sistema */

        if (fd < 0) { 
            write(2, "Error al abrir el log\n", 22); 
            return -1; 
        }
        
        /* Escribimos la línea completa: "n1 op n2 = res\n" */
        write(fd, argv[1], strlen(argv[1])); write(fd, " ", 1);
        write(fd, argv[2], 1); write(fd, " ", 1);
        write(fd, argv[3], strlen(argv[3])); write(fd, " = ", 3);
        esc_num(res, fd);
        write(fd, "\n", 1);
        close(fd);
    } 

    /* --- CASO 2: CONSULTA DE HISTORIAL (./mycalc -b <n>) --- */
    else if (strcmp(argv[1], "-b") == 0) {
        int target = atoi(argv[2]);
        
        /* Validación: no existen líneas 0 o negativas */
        if (target <= 0) {
            write(2, "Error: La linea debe ser mayor que 0\n", 37);
            return -1;
        }

        int fd = open(log_file, O_RDONLY);
        if (fd < 0) {
            write(2, "Error: No existe el historial\n", 30);
            return -1;
        }

        char c; 
        int cur = 1, found = 0, n_bytes;

        /* Lectura byte a byte para localizar la línea (como en mywc.c) */
        while ((n_bytes = read(fd, &c, 1)) > 0) {
            /* Si coincide con el número de línea, imprimimos el carácter */
            if (cur == target) { 
                write(1, &c, 1); 
                found = 1; 
            }
            if (c == '\n') cur++; /* El salto de línea separa las operaciones */
            if (cur > target) break; /* Optimizamos: dejamos de leer tras pasar la línea */
        }
        
        /* Comprobamos errores físicos de lectura o si la línea no existía */
        if (n_bytes < 0) {
            write(2, "Error critico de lectura\n", 25);
        } else if (!found) {
            write(2, "Error: Linea no encontrada\n", 27);
        }
        
        close(fd);
    }

    return 0;
}
