/*
 * BITACORA - demo de interfaz en terminal con ncurses
 *
 * Compilar:  gcc menu.c -o menu -lncursesw
 * Controles: flechas / W S / J K  mover      Enter o click  elegir
 *            1-6 atajo directo    T  cambiar tema
 *            P   pausar lluvia    Q  salir
 */
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ---------- temas ---------- */
typedef struct { const char *nombre; short color; } Tema;
static const Tema TEMAS[] = {
    {"matrix", COLOR_GREEN},  {"ocean", COLOR_CYAN},
    {"neon",   COLOR_MAGENTA}, {"amber", COLOR_YELLOW},
    {"fuego",  COLOR_RED},
};
#define N_TEMAS ((int)(sizeof TEMAS / sizeof TEMAS[0]))

enum { P_CABEZA = 1, P_RASTRO, P_BORDE, P_SEL, P_TEXTO, P_SUAVE, P_BARRA };

static void aplicar_tema(int t) {
    short c = TEMAS[t].color;
    init_pair(P_CABEZA, COLOR_WHITE, -1);
    init_pair(P_RASTRO, c, -1);
    init_pair(P_BORDE,  c, -1);
    init_pair(P_SEL,    COLOR_BLACK, c);
    init_pair(P_TEXTO,  COLOR_WHITE, -1);
    init_pair(P_SUAVE,  c, -1);
    init_pair(P_BARRA,  COLOR_BLACK, c);
}

/* ---------- lluvia ---------- */
typedef struct { float y, vel; int largo; } Gota;
static Gota *gotas = NULL;
static char *celdas = NULL;
static int filas = 0, cols = 0;

static char car_azar(void) {
    static const char s[] = "abcdefghijklmnopqrstuvwxyz0123456789@#$%&*+=<>?/";
    return s[rand() % (int)(sizeof s - 1)];
}

static void gota_reset(Gota *g) {
    g->y = -(float)(rand() % (filas * 2 + 1));
    g->vel = 0.25f + (rand() % 100) / 100.0f * 0.9f;
    g->largo = 5 + rand() % (filas / 2 + 1);
}

static void lluvia_init(void) {
    getmaxyx(stdscr, filas, cols);
    free(gotas);
    free(celdas);
    gotas  = malloc((size_t)cols * sizeof *gotas);
    celdas = calloc((size_t)filas * cols, 1);
    if (!gotas || !celdas) { endwin(); fprintf(stderr, "sin memoria\n"); exit(1); }
    for (int c = 0; c < cols; c++) gota_reset(&gotas[c]);
}

static void lluvia_paso(int avanzar) {
    for (int c = 0; c < cols; c++) {
        Gota *g = &gotas[c];
        if (avanzar) g->y += g->vel;
        int cab = (int)g->y;
        if (cab - g->largo > filas) { gota_reset(g); continue; }
        if (avanzar && cab >= 0 && cab < filas) celdas[cab * cols + c] = car_azar();

        for (int k = 0; k < g->largo; k++) {
            int y = cab - k;
            if (y < 0 || y >= filas) continue;
            char *p = &celdas[y * cols + c];
            if (!*p) *p = car_azar();
            if (avanzar && rand() % 60 == 0) *p = car_azar();

            chtype a;
            if (k == 0)                  a = COLOR_PAIR(P_CABEZA) | A_BOLD;
            else if (k < 3)              a = COLOR_PAIR(P_RASTRO) | A_BOLD;
            else if (k < g->largo * 2/3) a = COLOR_PAIR(P_RASTRO);
            else                         a = COLOR_PAIR(P_RASTRO) | A_DIM;
            if (y == filas - 1 && c == cols - 1) continue;   /* evita error en esquina */
            mvaddch(y, c, (chtype)*p | a);
        }
    }
}

/* ---------- dibujo de cajas ---------- */
static void caja(int y, int x, int h, int w, const char *titulo) {
    chtype b = COLOR_PAIR(P_BORDE);
    attrset(COLOR_PAIR(P_TEXTO));
    for (int i = 0; i < h; i++) mvprintw(y + i, x, "%*s", w, "");   /* relleno: tapa la lluvia */

    mvaddch(y, x, ACS_ULCORNER | b);
    mvhline(y, x + 1, ACS_HLINE | b, w - 2);
    mvaddch(y, x + w - 1, ACS_URCORNER | b);
    mvvline(y + 1, x, ACS_VLINE | b, h - 2);
    mvvline(y + 1, x + w - 1, ACS_VLINE | b, h - 2);
    mvaddch(y + h - 1, x, ACS_LLCORNER | b);
    mvhline(y + h - 1, x + 1, ACS_HLINE | b, w - 2);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER | b);

    if (titulo) {
        attrset(COLOR_PAIR(P_BORDE) | A_BOLD);
        mvprintw(y, x + 2, " %s ", titulo);
    }
    attrset(A_NORMAL);
}

/* ---------- menu ---------- */
static const char *OPCIONES[] = {
    "Insertar linea", "Insertar palabra", "Imprimir texto",
    "Diccionario",    "Borrar todo",      "Salir",
};
#define N_OP ((int)(sizeof OPCIONES / sizeof OPCIONES[0]))

#define ANCHO 48
#define MIN_FILAS 22
#define MIN_COLS  56

static int menu_y, menu_x;     /* se recalculan en cada frame (para el mouse) */

static void dibujar_ui(int sel, int tema, int pausa, const char *msg) {
    int alto_total = 6 + 1 + (N_OP + 2) + 1 + 3;
    int y0 = (filas - 1 - alto_total) / 2; if (y0 < 0) y0 = 0;
    int x0 = (cols - ANCHO) / 2;

    /* --- cabecera con barquito --- */
    caja(y0, x0, 6, ANCHO, NULL);
    attrset(COLOR_PAIR(P_BORDE) | A_BOLD);
    mvprintw(y0 + 1, x0 + 4,  "  |\\");
    mvprintw(y0 + 2, x0 + 4,  "  | \\");
    mvprintw(y0 + 3, x0 + 4,  " _|__\\_");
    mvprintw(y0 + 4, x0 + 4,  " \\____/");
    attrset(COLOR_PAIR(P_TEXTO) | A_BOLD);
    mvprintw(y0 + 1, x0 + 17, "B I T A C O R A");
    attrset(COLOR_PAIR(P_TEXTO));
    mvprintw(y0 + 2, x0 + 17, "procesador de textos");
    attrset(COLOR_PAIR(P_SUAVE) | A_DIM);
    mvprintw(y0 + 4, x0 + 17, "viento en popa  ~~~");
    attrset(A_NORMAL);

    /* --- menu --- */
    menu_y = y0 + 7;
    menu_x = x0;
    caja(menu_y, menu_x, N_OP + 2, ANCHO, "Menu");
    for (int i = 0; i < N_OP; i++) {
        int y = menu_y + 1 + i;
        if (i == sel) {
            attrset(COLOR_PAIR(P_SEL) | A_BOLD);
            mvprintw(y, menu_x + 1, " > %-*s %d ", ANCHO - 8, OPCIONES[i], i + 1);
        } else {
            attrset(COLOR_PAIR(P_TEXTO));
            mvprintw(y, menu_x + 1, "   %-*s ", ANCHO - 8, OPCIONES[i]);
            attrset(COLOR_PAIR(P_SUAVE) | A_DIM);
            mvprintw(y, menu_x + ANCHO - 4, "%d", i + 1);
        }
        attrset(A_NORMAL);
    }

    /* --- salida --- */
    int sy = menu_y + N_OP + 3;
    caja(sy, x0, 3, ANCHO, "Salida");
    attrset(COLOR_PAIR(P_TEXTO));
    mvprintw(sy + 1, x0 + 2, "%-*.*s", ANCHO - 4, ANCHO - 4, msg);
    attrset(A_NORMAL);

    /* --- reloj (solo si hay lugar) --- */
    if (cols >= ANCHO + 2 * 22) {
        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        char hora[16], fecha[32];
        strftime(hora, sizeof hora, "%H:%M:%S", tm);
        strftime(fecha, sizeof fecha, "%d/%m/%Y", tm);
        int cx = cols - 22, cy = 1;
        caja(cy, cx, 5, 21, "Reloj");
        attrset(COLOR_PAIR(P_TEXTO) | A_BOLD);
        mvprintw(cy + 2, cx + 6, "%s", hora);
        attrset(COLOR_PAIR(P_SUAVE));
        mvprintw(cy + 3, cx + 6, "%s", fecha);
        attrset(A_NORMAL);
    }

    /* --- barra inferior --- */
    attrset(COLOR_PAIR(P_BARRA));
    mvprintw(filas - 1, 0, "%-*.*s", cols, cols,
             " flechas/WS mover | Enter/click elegir | 1-6 atajo | T tema | P pausa | Q salir");
    char info[48];
    snprintf(info, sizeof info, " tema:%s%s ", TEMAS[tema].nombre, pausa ? " (pausa)" : "");
    mvprintw(filas - 1, cols - (int)strlen(info), "%s", info);
    attrset(A_NORMAL);
}

/* ---------- acciones ---------- */
static int activar(int op, char *msg, size_t n) {
    if (op == N_OP - 1) return 1;                 /* Salir */
    /* AQUI van las llamadas reales de tu trabajo, por ejemplo:
     *   case 0: r = insertarLinea(&texto); break;
     *   case 2: imprimirTexto(texto);      break;
     */
    snprintf(msg, n, "%s  ->  OK", OPCIONES[op]);
    return 0;
}

int main(void) {
    setlocale(LC_ALL, "");
    srand((unsigned)time(NULL));

    initscr();
    cbreak(); noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    set_escdelay(25);
    timeout(40);                                    /* ~25 fps: getch no bloquea */
    mousemask(BUTTON1_PRESSED | BUTTON1_CLICKED, NULL);
    mouseinterval(0);

    if (has_colors()) { start_color(); use_default_colors(); }
    int tema = 0;
    aplicar_tema(tema);
    lluvia_init();

    int sel = 0, pausa = 0, salir = 0;
    char msg[96] = "Listo. Elegi una opcion.";

    while (!salir) {
        erase();
        if (filas < MIN_FILAS || cols < MIN_COLS) {
            attrset(COLOR_PAIR(P_TEXTO) | A_BOLD);
            mvprintw(filas / 2, 1, "Agranda la terminal (min %dx%d)", MIN_COLS, MIN_FILAS);
            attrset(A_NORMAL);
        } else {
            lluvia_paso(!pausa);
            dibujar_ui(sel, tema, pausa, msg);
        }
        refresh();

        int t = getch();
        switch (t) {
        case ERR: break;
        case KEY_UP:   case 'w': case 'k': sel = (sel + N_OP - 1) % N_OP; break;
        case KEY_DOWN: case 's': case 'j': sel = (sel + 1) % N_OP;        break;
        case 't': case 'T': tema = (tema + 1) % N_TEMAS; aplicar_tema(tema); break;
        case 'p': case 'P': pausa = !pausa; break;
        case 'q': case 'Q': salir = 1; break;
        case '\n': case KEY_ENTER: salir = activar(sel, msg, sizeof msg); break;
        case KEY_RESIZE: lluvia_init(); break;
        case KEY_MOUSE: {
            MEVENT ev;
            if (getmouse(&ev) == OK) {
                int i = ev.y - (menu_y + 1);
                if (i >= 0 && i < N_OP && ev.x > menu_x && ev.x < menu_x + ANCHO - 1) {
                    sel = i;
                    salir = activar(sel, msg, sizeof msg);
                }
            }
            break;
        }
        default:
            if (t >= '1' && t < '1' + N_OP) {
                sel = t - '1';
                salir = activar(sel, msg, sizeof msg);
            }
        }
    }

    endwin();                                       /* restaura la terminal */
    free(gotas);
    free(celdas);
    return 0;
}