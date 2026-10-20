#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_CANT_PALABRAS_X_LINEA 5

typedef char* string;

enum _retorno{
    OK, ERROR, NO_IMPLEMENTADA
};
typedef enum _retorno TipoRetorno;

typedef unsigned int Posicion;

typedef struct palabra {
    string info;
    struct palabra * sig;
    unsigned int cantidad;
} palabra;

typedef palabra * palabras;

typedef struct linea {
    struct linea * sig;
    palabras primera;
    unsigned int cantidad;
} linea;

typedef linea * lineas;

typedef struct cabezal {
    lineas primera;
    lineas ultima;
    unsigned int cantidad;
} cabezal;

static cabezal texto;

//FUNCIONES AUXILIARES:

//O(1)
lineas crearLinea() {
    lineas l = malloc(sizeof(linea));
    l->primera = NULL;
    l->sig = NULL;
    l->cantidad = 0;

    return l;
}

void liberarPalabras(palabras p) {
    if(p == NULL)
        return;
    liberarPalabras(p->sig);
    free(p);
}

void liberarLineas(lineas l) {
    if(l == NULL)
        return;
    liberarLineas(l->sig);
    liberarPalabras(l->primera);
    free(l);
}

void borrarLineaAux(Posicion p, lineas * l) {
    if(p == 1) {
        liberarPalabras((*l)->primera);
        lineas aux = *l;
        *l = (*l)->sig;
        if(aux == texto.ultima)
            texto.ultima = *l;
        free(aux);
        return;
    }

    if(p == 2) {
        liberarPalabras((*l)->sig->primera);
        lineas aux = (*l)->sig;
        (*l)->sig = aux->sig;
        if(aux == texto.ultima)
            texto.ultima = *l;
        free(aux);
        return;
    }

    borrarLineaAux(p-1, &((*l)->sig));
}


/*
    Funciones "Primarias". Muchas reciben el 'texto t'(la cabezera) como parametro y de ser necesario
    delegan a una función auxiliar la cual hace el "trabajo bruto", es decir, llamadas recursivas.
    De esta forma le damos a main() una "fachada bonita" de las funciones a usar.
*/
void Destruir() {
    liberarLineas(texto.primera);
}

void Inicializar() {
    Destruir();
    texto.primera = NULL;
    texto.ultima = NULL;
    texto.cantidad = 0;   
}



//O(1)
TipoRetorno InsertarLinea() {  
    lineas l = crearLinea();
    if(texto.cantidad == 0) { //texto vacío
        texto.primera = l;
        texto.ultima = l;
    } else {
        texto.ultima->sig = l;
        texto.ultima = l;
    }

    texto.cantidad++;
    
    return OK;
}

TipoRetorno BorrarLinea(Posicion p) {
    if(texto.cantidad < p || p == 0) 
        return ERROR;
    borrarLineaAux(p, &(texto.primera));

    texto.cantidad--;
    return OK;
}

TipoRetorno InsertarPalabra(Posicion linea, Posicion pos, string palabra) {

}



int main() {
    Inicializar();

    return 0;
}