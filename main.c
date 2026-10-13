#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_CANT_PALABRAS_X_LINEA 5

typedef char* string;

enum _retorno{
    OK, ERROR, NO_IMPLEMENTADA
};
typedef enum _retorno tipoRetorno;

typedef unsigned int posicion;

typedef struct palabra {
    string info;
    struct palabra * sig;
} palabra;

typedef palabra * palabras;

typedef struct linea {
    struct linea * sig;
    palabras primera;
} linea;

typedef linea * lineas;

struct cabezal {
    lineas primera;
    lineas ultima;
    unsigned int cantidad;
};

typedef struct cabezal * cabezal;

tipoRetorno insertarLinea(cabezal * c) {
    lineas nueva = malloc(sizeof(linea));
    nueva->primera = NULL;
    nueva->sig = NULL;

    if (*c == NULL) {
        *c = malloc(sizeof(struct cabezal));
        (*c)->primera = nueva;
        (*c)->ultima = nueva;
        (*c)->cantidad = 1;

        return OK;
    }

    (*c)->ultima->sig = nueva;
    (*c)->ultima = nueva;
    (*c)->cantidad++;

    return OK;
}


int main() {
    lineas l = NULL;
    cabezal c = NULL;

    insertarLinea(&c);
    return 0;
}