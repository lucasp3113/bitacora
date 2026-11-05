#include "procesador.h"
#include <iostream>
#include <string>

using std::string;
using std::cout;
using std::endl;

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

lineas crearLinea() {
    lineas l = new(linea);
    l->primera = NULL;
    l->sig = NULL;
    l->cantidad = 0;

    return l;
}

palabras crearPalabra(const string &s) {
    palabras p = new(palabra);
    p->info = s;
    p->sig = NULL;

    return p;
}

diccionario crearNodoDiccionario(const string &p) {
    diccionario d = new(diccionarioABB);
    d->info = p;
    d->izq = NULL;
    d->der = NULL;

    return d;
}

void liberarPalabras(palabras p) {
    if(p == NULL)
        return;
    liberarPalabras(p->sig);
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

bool tieneEspacios(const string &palabra, Posicion i = 0) {
    if(i >= palabra.size())
        return false;
    if(palabra[i] == ' ' || palabra[i] == '\t' || palabra[i] == '\n' || palabra[i] == '\r' || palabra[i] == '\v' || palabra[i] == '\f')
        return true;
    return tieneEspacios(palabra, i+1);
}

void insertarPalabraEnLinea(palabras * p, Posicion pos, const string &palabra) {
    if(pos == 1) {
        palabras nueva = crearPalabra(palabra);
        nueva->sig = *p;
        *p = nueva;

        return;
    }

    insertarPalabraEnLinea(&((*p)->sig), pos-1, palabra);
}

void borrarPalabraEnLinea(palabras * p, Posicion pos) {
    if(pos == 1) {
        palabras aux = *p;
        *p = (*p)->sig;
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

    palabras penultima = obtenerPenultimaPalabra(l->primera);
    palabras ultima = penultima->sig;
    penultima->sig = NULL;
    l->cantidad--;

    ultima->sig = l->sig->primera;
    l->sig->primera = ultima;
    l->sig->cantidad++;

    arreglarTextoDesde(l->sig);
}

void imprimirPalabras(palabras p){
    if(p == NULL)
        return;
    cout << " " << p->info;
    imprimirPalabras(p->sig);
}

void imprimirTextoAux(lineas l, Posicion n = 1) {
    if(l == NULL)
        return;
    cout << n << ":";
    imprimirPalabras(l->primera);
    cout << endl;
    imprimirTextoAux(l->sig, n+1);
}

void liberarDiccionario(diccionario d) {
    if(d == NULL)
        return;
    liberarDiccionario(d->izq);
    liberarDiccionario(d->der);
    delete(d);
}

TipoRetorno ingresarPalabraDiccionarioAux(const string &palabra, diccionario * d) {
    if(*d == NULL) {
        (*d) = crearNodoDiccionario(palabra);
        return OK;
    }
    if(palabra == (*d)->info)
        return ERROR;

    if(palabra > (*d)->info)
        return ingresarPalabraDiccionarioAux(palabra, &((*d)->der));
    return ingresarPalabraDiccionarioAux(palabra, &((*d)->izq));
}

void imprimirDiccionarioAux(diccionario d) {
    if(d == NULL)
        return;
    imprimirDiccionarioAux(d->izq);
    cout << d->info << endl;
    imprimirDiccionarioAux(d->der);
}

bool estaPalabraPerteneceAlDicc(const string &s, diccionario d) {
    if(d == NULL)
        return false;
    if(s == d->info)
        return true;

    if(s > d->info)
        return estaPalabraPerteneceAlDicc(s, d->der);
    return estaPalabraPerteneceAlDicc(s, d->izq);
}

void imprimirPalabrasIncorrectasAux(palabras p) {
    if(p == NULL)
        return;
    if(!estaPalabraPerteneceAlDicc(p->info, texto.dicc))
        cout << " " << p->info;
    imprimirPalabrasIncorrectasAux(p->sig);
}

void imprimirTextoIncorrectoAux(lineas l, Posicion n = 1) {
    if(l == NULL)
        return;
    cout << n << ":";
    imprimirPalabrasIncorrectasAux(l->primera);
    cout << endl;
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

//OPERACIONES (las 11 de procesador.h)

void Destruir() {
    liberarLineas(texto.primera);
    liberarDiccionario(texto.dicc);
    texto.primera = NULL;
    texto.ultima = NULL;
    texto.cantidad = 0;
    texto.dicc = NULL;
    inicializado = false;
}

void Inicializar() {
    Destruir();
    inicializado = true;
}

TipoRetorno InsertarLinea() {
    if(!inicializado) return ERROR;

    lineas l = crearLinea();
    if(texto.cantidad == 0) {
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

TipoRetorno InsertarPalabra(Posicion pLinea, Posicion pPalabra, std::string palabra) {
    if(!inicializado || pLinea < 1 || pLinea > texto.cantidad || pPalabra == 0 || pPalabra > MAX_CANT_PALABRAS_X_LINEA + 1 || palabra.empty() || tieneEspacios(palabra))
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
    if(!inicializado || pLinea == 0 || pLinea > texto.cantidad || pPalabra == 0 || pPalabra > MAX_CANT_PALABRAS_X_LINEA)
        return ERROR;
    lineas l = buscarLinea(texto.primera, pLinea);
    if(pPalabra > l->cantidad)
        return ERROR;
    borrarPalabraEnLinea(&(l->primera), pPalabra);
    l->cantidad--;

    return OK;
}

TipoRetorno ImprimirTexto() {
    if(!inicializado)
        return ERROR;
    if(texto.cantidad == 0) {
        cout << "Texto vacio" << endl;
        return OK;
    }
    imprimirTextoAux(texto.primera);
    return OK;
}

TipoRetorno IngresarPalabraDiccionario(std::string palabra) {
    if(!inicializado || palabra.empty() || tieneEspacios(palabra))
        return ERROR;
    return ingresarPalabraDiccionarioAux(palabra, &(texto.dicc));
}

TipoRetorno ImprimirDiccionario() {
    if(!inicializado)
        return ERROR;
    if(texto.dicc == NULL) {
        cout << "Diccionario vacio" << endl;
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
    if(texto.cantidad == 0) {
        cout << "Texto vacio" << endl;
        return OK;
    }
    imprimirTextoIncorrectoAux(texto.primera);
    return OK;
}

TipoRetorno AgregarIncorrectasAlDiccionario() {
    if(!inicializado)
        return ERROR;
    agregarIncorrectasAlDiccionarioAux(texto.primera);
    return OK;
}