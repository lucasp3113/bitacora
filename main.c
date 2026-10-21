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
} palabra;

typedef palabra * palabras;

typedef struct linea {
    struct linea * sig;
    palabra * primera;
    unsigned int cantidad;
} linea;

typedef linea * lineas;

typedef struct cabezal {
    linea * primera;
    linea * ultima;
    unsigned int cantidad;
} cabezal;

static cabezal texto;
static bool inicializado = false;

//FUNCIONES AUXILIARES:

//O(1)
lineas crearLinea() {
    lineas l = malloc(sizeof(linea));
    l->primera = NULL;
    l->sig = NULL;
    l->cantidad = 0;

    return l;
}

palabras crearPalabra() {
    palabras p = malloc(sizeof(palabra));
    p->info = NULL;
    p->sig = NULL;

    return p;
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

bool tieneEspacios(string palabra) {
    if(palabra[0] == '\0')
        return false;
    if(palabra[0] == ' ' || palabra[0] == '\t' || palabra[0] == '\n' || palabra[0] == '\r' || palabra[0] == '\v' || palabra[0] == '\f')
        return true;
    return tieneEspacios(palabra + 1);
}

void insertarPalabraEnLinea(palabras * p, Posicion pos, string palabra) {
    if(pos == 1) {
        palabras aux = *p;
        (*p) = crearPalabra();
        (*p)->info = palabra;
        (*p)->sig = aux;

        return;
    }

    if(pos == 2) {
        palabras aux = (*p)->sig;
        (*p)->sig = crearPalabra();
        (*p)->sig->info = palabra;
        (*p)->sig->sig = aux;
        

        return;
    }

    insertarPalabraEnLinea(&((*p)->sig), pos-1, palabra);
}

void borrarPalabraEnLinea(palabras * p, Posicion pos) {
    if(pos == 1) {
        palabras aux = *p;
        *p = (*p)->sig;
        free(aux);
        return;
    }
    if(pos == 2) {
        palabras aux = (*p)->sig;
        (*p)->sig = (*p)->sig->sig;
        free(aux);
        return;
    }

    borrarPalabraEnLinea(&((*p)->sig), pos-1);
}

lineas buscarLinea(lineas l, Posicion p) {
    if(p == 1)
        return l;
    return buscarLinea(l->sig, p-1);
}

palabras obtenerPenultimaPalabra(palabras p) {
    if(p == NULL || p->sig == NULL)
        return NULL;
    if(p->sig->sig == NULL)
        return p;
    return obtenerPenultimaPalabra(p->sig);
}

void arreglarTextoDesde(lineas l) {
    if(l->cantidad <= MAX_CANT_PALABRAS_X_LINEA)
        return;
    if(l->sig == NULL) {
        l->sig = crearLinea();
        texto.ultima = l->sig;
        texto.cantidad++;
    }
    palabras aux = l->sig->primera;
    palabras penultima = obtenerPenultimaPalabra(l->primera);
    l->sig->primera = penultima->sig; //ultima
    l->sig->primera->sig = aux;
    penultima->sig = NULL;
    l->cantidad--;
    l->sig->cantidad++;

    arreglarTextoDesde(l->sig);
}

void imprimirPalabras(palabras p){
    if(p == NULL)
        return;
    printf("%s ", p->info);
    imprimirPalabras(p->sig);
}

void imprimirTextoAux(lineas l, int n) {
    if(l == NULL)
        return;
    printf("%d: ");
    imprimirPalabras(l->primera);
    printf("\n");
    imprimirTextoAux(l->sig, n+1);
}

/*
    Funciones "Primarias". Muchas reciben el 'texto t'(la cabezera) como parametro y de ser necesario
    delegan a una función auxiliar la cual hace el "trabajo bruto", es decir, llamadas recursivas.
    De esta forma le damos a main() una "fachada bonita" de las funciones a usar.
*/

void Destruir() {
    liberarLineas(texto.primera);
    texto.primera = NULL;
    texto.ultima = NULL;
    texto.cantidad = 0;
    inicializado=false;
}

void Inicializar() {
    Destruir();
    texto.primera = NULL;
    texto.ultima = NULL;
    texto.cantidad = 0;   
    inicializado=true;
}

//O(1)
TipoRetorno InsertarLinea() {
    if(!inicializado) return ERROR;

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
    if(!inicializado) return ERROR;

    if(texto.cantidad < p || p == 0) 
        return ERROR;
    borrarLineaAux(p, &(texto.primera));

    texto.cantidad--;
    return OK;
}

TipoRetorno InsertarPalabra(Posicion pLinea, Posicion pPalabra, string palabra) {
    if(!inicializado || pLinea < 1 || pLinea > texto.cantidad || pPalabra < 1 || pPalabra > MAX_CANT_PALABRAS_X_LINEA || palabra == NULL || palabra[0] == '\0' || tieneEspacios(palabra))
        return ERROR;
    lineas l = buscarLinea(texto.primera, pLinea);
    if(pPalabra > l->cantidad + 1)
        return ERROR;
    insertarPalabraEnLinea(&(l->primera), pPalabra, palabra);
    l->cantidad++;
    if(l->cantidad > MAX_CANT_PALABRAS_X_LINEA) {
        arreglarTextoDesde(l);
    }

    return OK;
}

TipoRetorno BorrarPalabra(Posicion pLinea, Posicion pPalabra) {
    if(!inicializado || pLinea < 1 || pLinea > texto.cantidad || pPalabra < 1 || pPalabra > MAX_CANT_PALABRAS_X_LINEA)
        return ERROR;
    lineas l = buscarLinea(texto.primera, pLinea);
    if(pPalabra > l->cantidad)
        return ERROR;
    borrarPalabraEnLinea(&(l->primera), pPalabra);
    l->cantidad--;
    
    return OK;
}

TipoRetorno ImprimirTexto() {
    if(texto.cantidad == 0)
        printf("Texto vacio");
    imprimirTextoAux(texto.primera, 1);
    
    return OK;
}


int main() {
   Inicializar();

// 6 líneas
for (int i = 0; i < 6; i++)
    InsertarLinea();

// Línea 1: llena (5 palabras)
InsertarPalabra(1, 1, "El");
InsertarPalabra(1, 2, "veloz");
InsertarPalabra(1, 3, "murcielago");
InsertarPalabra(1, 4, "hindu");
InsertarPalabra(1, 5, "comia");

// Línea 2: llena, con palabras raras
InsertarPalabra(2, 1, "3.14159");
InsertarPalabra(2, 2, "@#$%^&*()");
InsertarPalabra(2, 3, "%s%d%n");        // si imprimís con printf(s) a secas, esto te rompe
InsertarPalabra(2, 4, "---");
InsertarPalabra(2, 5, "x");

// Línea 3: vacía a propósito

// Línea 4: dos palabras
InsertarPalabra(4, 1, "a");
InsertarPalabra(4, 2, "b");

// Línea 5: una palabra larguísima
InsertarPalabra(5, 1, "supercalifragilisticoespialidoso_y_ademas_un_poco_mas_largo_todavia");

// Línea 6: vacía a propósito

printf("\n--- paso 1: estado inicial ---\n");
ImprimirTexto();

// Desborde en cascada: línea 1 -> línea 2 -> línea 3
InsertarPalabra(1, 1, "HOLA");
printf("\n--- paso 2: desborde en cascada ---\n");
ImprimirTexto();

// Otro desborde, ahora en el medio de la línea 2
InsertarPalabra(2, 3, "INSERTADA");
printf("\n--- paso 3: insertar en el medio de la línea 2 ---\n");
ImprimirTexto();

// Llenar la línea 6 y desbordar la última línea (crea una línea nueva)
InsertarPalabra(6, 1, "u");
InsertarPalabra(6, 2, "v");
InsertarPalabra(6, 3, "w");
InsertarPalabra(6, 4, "y");
InsertarPalabra(6, 5, "z");
InsertarPalabra(6, 1, "OVERFLOW");
printf("\n--- paso 4: desborde de la última línea ---\n");
ImprimirTexto();

// Borrados
BorrarPalabra(1, 1);
BorrarLinea(3);
BorrarLinea(6);
printf("\n--- paso 5: tras BorrarPalabra(1,1), BorrarLinea(3), BorrarLinea(6) ---\n");
ImprimirTexto();

// Casos de error (esperás 1 en cada uno, porque ERROR vale 1)
printf("\n--- paso 6: errores (todos deberían dar 1) ---\n");
printf("pLinea 0:         %d\n", InsertarPalabra(0, 1, "x"));
printf("con espacio:      %d\n", InsertarPalabra(1, 1, "con espacio"));
printf("linea inexistente:%d\n", InsertarPalabra(99, 1, "x"));
printf("palabra vacia:    %d\n", InsertarPalabra(1, 1, ""));
printf("borrar pos 0:     %d\n", BorrarPalabra(1, 0));
printf("borrar linea 99:  %d\n", BorrarLinea(99));

Destruir();

    return 0;
}