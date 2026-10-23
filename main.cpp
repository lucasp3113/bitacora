#include <cstdio>
#include <cstdlib>
#include <cstring>

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

typedef struct diccionarioABB {
    string info;
    struct diccionarioABB * der;
    struct diccionarioABB * izq;
} diccionarioABB;

typedef diccionarioABB *  diccionario;

typedef struct cabezal {
    linea * primera;
    linea * ultima;
    diccionario dicc;
    unsigned int cantidad;
} cabezal;

static cabezal texto;
static bool inicializado = false;

//FUNCIONES AUXILIARES:

//O(1)
lineas crearLinea() {
    lineas l = new(linea);
    l->primera = NULL;
    l->sig = NULL;
    l->cantidad = 0;

    return l;
}

palabras crearPalabra() {
    palabras p = new(palabra);
    p->info = NULL;
    p->sig = NULL;

    return p;
}

diccionario crearNodoDiccionario() {
    diccionario d = new(diccionarioABB);
    d->info = NULL;
    d->izq = NULL;
    d->der = NULL;

    return d;
} 

void liberarPalabras(palabras p) {
    if(p == NULL)
        return;
    liberarPalabras(p->sig);
    free(p->info);
    delete(p);
}

void liberarLineas(lineas l) {
    if(l == NULL)
        return;
    liberarLineas(l->sig);
    liberarPalabras(l->primera);
    delete(l);
}

void borrarLineaAux(Posicion p, lineas * l) {
    if(p == 1) {
        liberarPalabras((*l)->primera);
        lineas aux = *l;
        *l = (*l)->sig;
        if(aux == texto.ultima)
            texto.ultima = *l;
        delete(aux);
        return;
    }

    if(p == 2) {
        liberarPalabras((*l)->sig->primera);
        lineas aux = (*l)->sig;
        (*l)->sig = aux->sig;
        if(aux == texto.ultima)
            texto.ultima = *l;
        delete(aux);
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

    insertarPalabraEnLinea(&((*p)->sig), pos-1, palabra);
}

void borrarPalabraEnLinea(palabras * p, Posicion pos) {
    if(pos == 1) {
        palabras aux = *p;
        *p = (*p)->sig;
        free(aux->info);
        delete(aux);
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
    printf("%d: ", n);
    imprimirPalabras(l->primera);
    printf("\n");
    imprimirTextoAux(l->sig, n+1);
}

void liberarDiccionario(diccionario d) {
    if(d == NULL) 
        return;
    liberarDiccionario(d->izq);
    liberarDiccionario(d->der);
    delete(d);
}

TipoRetorno ingresarPalabraDiccionarioAux(string palabra, diccionario * d) {
    if(*d == NULL) {
        (*d) = crearNodoDiccionario();
        (*d)->info = palabra;
        return OK;
    }
    int a = strcmp(palabra, (*d)->info);
    if(a == 0)
        return ERROR;
    return (a > 0) ? ingresarPalabraDiccionarioAux(palabra, &((*d)->der)) : ingresarPalabraDiccionarioAux(palabra, &((*d)->izq));
}

void imprimirDiccionarioAux(diccionario d) {
    if(d == NULL)
        return;
    imprimirDiccionarioAux(d->izq);
    printf("%s\n", d->info);
    imprimirDiccionarioAux(d->der);
}

bool estaPalabraPerteneceAlDicc(palabras p, diccionario d) {
    if(d == NULL)
        return false;
    int a = strcmp(p->info, d->info);
    if(a == 0)  
        return true;
    return (a > 0) ? estaPalabraPerteneceAlDicc(p, d->der) : estaPalabraPerteneceAlDicc(p, d->izq);
}

void imprimirPalabrasIncorrectasAux(palabras p) {
    if(p == NULL)
        return;
    if(!estaPalabraPerteneceAlDicc(p, texto.dicc))
        printf("%s ", p->info);
    imprimirPalabrasIncorrectasAux(p->sig);
}

void imprimirTextoIncorrectoAux(lineas l, int n) {
    if(l == NULL)
        return;
    printf("%d:", n);
    imprimirPalabrasIncorrectasAux(l->primera);
    printf("\n");
    imprimirTextoIncorrectoAux(l->sig, n+1);
}

void agregarTodasLasPalabrasIncorrectasAlDiccionario(palabras p) {
    if(p == NULL)
        return;
    ingresarPalabraDiccionarioAux(p->info, &(texto.dicc));
    agregarTodasLasPalabrasIncorrectasAlDiccionario(p->sig);
}

void agregarIncorrectasAlDiccionarioAux(lineas l) {
    if(l == NULL)
        return;
    agregarTodasLasPalabrasIncorrectasAlDiccionario(l->primera);
    agregarIncorrectasAlDiccionarioAux(l->sig);
}

/*
    Funciones "Primarias". Muchas delegan a una función auxiliar la cual hace el "trabajo bruto",
    es decir, llamadas recursivas. De esta forma le damos a main() una "fachada bonita"
    de las funciones a usar.
*/

void Destruir() {
    liberarLineas(texto.primera);
    texto.primera = NULL;
    texto.ultima = NULL;
    texto.cantidad = 0;
    liberarDiccionario(texto.dicc);
    texto.dicc = NULL;
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

TipoRetorno IngresarPalabraDiccionario(string palabra) {
    if(!inicializado || palabra == NULL || palabra[0] == '\0' || tieneEspacios(palabra))
        return ERROR;
    return ingresarPalabraDiccionarioAux(palabra, &(texto.dicc));
}

TipoRetorno ImprimirDiccionario() {
    if(!inicializado)
        return ERROR;
    if(texto.dicc == NULL) {
        printf("Diccionario vacio");
        return OK;
    }
    imprimirDiccionarioAux(texto.dicc);
    return OK;
}

TipoRetorno ImprimirTextoIncorrecto() {
    if(!inicializado)
        return ERROR;
    if(texto.dicc == NULL)
        return ImprimirTexto();
    if(texto.cantidad == 0)
        printf("Texto vacio");
    imprimirTextoIncorrectoAux(texto.primera, 1);
    return OK;
}

TipoRetorno AgregarIncorrectasAlDiccionario() {
    if(!inicializado)
        return ERROR;
    agregarIncorrectasAlDiccionarioAux(texto.primera);
    return OK;
}

#include "ui.cpp"

int main(void) {
    Inicializar();
    iniciarUI();
    Destruir();
    return 0;
}