#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ncurses.h>
#include <stdarg.h>
#undef OK   /* ncurses define OK y choca con tu enum */

/* ================== TAMAÑOS ================== */
/* La interfaz se adapta a la terminal (minimo MINW x MINH). */
#define MINW 100
#define MINH 34
#define HH 6                /* alto de la cabecera (logo + titulo) */
#define TR_N 30             /* cuadros de la transicion (30 = 1.5 s) */
#define MAXW 150
#define MAXH 36
#define N_OP 12
#define MH (N_OP + 4)       /* alto de la caja del menu */
#define P(n) COLOR_PAIR(n)

static int UW = MINW;       /* ancho total de la interfaz */
static int UH = MINH - 4;   /* alto total (sin el pie)    */
static int LW = 34;         /* ancho de la columna izquierda */

enum { C_NUBE = 1, C_LUNA, C_MAR, C_ESPUMA, C_CASCO, C_VELA,
       C_BANDERA, C_OK, C_ERR, C_TIT };

static const char *OPC[N_OP] = {
    "Insertar linea", "Borrar linea", "Insertar palabra", "Borrar palabra",
    "Imprimir texto", "Reiniciar texto", "Destruir texto",
    "Ingresar palabra al dicc.", "Imprimir diccionario",
    "Agregar texto al dicc.", "Creditos", "Salir"
};

/* ================== ESTADO ================== */
static long   tick = 0;
static double nave_x = 0;
static double nave_v = 0;
static int    nave_t = 0;
static int    nave_modo = 0;      /* 0 oculto, 1 entra, 2 navega, 3 sale */

static int    g_sel = 0;
static char   g_msg[160] = "";
static int    g_msg_tipo = 0;     /* 0 normal, 1 ok, 2 error */
static int    g_sea = 7;          /* filas de mar de la escena */
static int    g_fase = 0;         /* 0 portada, 1 transicion, 2 menu */
static int    g_trans = 0;        /* cuadro actual de la transicion */
static WINDOW *g_buf = NULL;      /* copia de la portada para la transicion */

static int         lp_modo = 0;   /* 0 menu, 1 campo, 2 confirmar, 3 visor */
static const char *lp_titulo = "";
static const char *lp_ayuda = "";
static char        lp_buf[32];
static int         lp_n = 0, lp_max = 0;
static int         lp_off = 0;
static int         lp_si = 1;     /* confirmar: 1 = Si marcado, 0 = No */
static int         g_cmd = 0;     /* 1 = escribiendo un comando con "/" */
static char        g_cbuf[4] = "";
static int         g_cn = 0;
static long        cr_t0 = 0;     /* creditos: tick en que arrancaron */

/* ================== SPRITES ================== */

/* barco de dos mastiles, armado por codigo (BH filas x BW columnas) */
#define BW 34
#define BH 11
#define NAVE_INI (-(BW + 2))
static char BARCO[BH][BW + 1];
static int  BARCO_COL[BH];

static void construirBarco(void) {
    const int ms[2] = { 8, 24 };
    int r, c, i, m;

    for (r = 0; r < BH; r++) {
        for (c = 0; c < BW; c++)
            BARCO[r][c] = ' ';
        BARCO[r][BW] = '\0';
    }
    for (i = 0; i < 2; i++) {
        m = ms[i];
        BARCO[0][m] = '|';
        BARCO[0][m + 1] = '>';
        BARCO[1][m] = '|';
        for (r = 2; r <= 6; r++) {
            int half = r - 1;
            for (c = m - half; c <= m + half; c++)
                BARCO[r][c] = '_';
            BARCO[r][m - half] = '/';
            BARCO[r][m + half] = '\\';
            BARCO[r][m] = '|';
        }
        BARCO[7][m] = '|';
    }
    for (c = 2; c <= BW - 3; c++)
        BARCO[8][c] = '_';
    BARCO[8][ms[0]] = '|';
    BARCO[8][ms[1]] = '|';

    BARCO[9][1] = '\\';
    BARCO[9][BW - 2] = '/';
    for (i = 0; i < 5; i++)
        BARCO[9][7 + 5 * i] = 'o';

    BARCO[10][2] = '\\';
    BARCO[10][BW - 3] = '/';
    for (c = 3; c <= BW - 4; c++)
        BARCO[10][c] = '_';

    for (r = 0; r < BH; r++)
        BARCO_COL[r] = (r == 0) ? C_BANDERA : (r <= 6 ? C_VELA : C_CASCO);
}

static const char *LUNA[7] = {
    "    _..._    ",
    "  .'     '.  ",
    " /         \\ ",
    " |         | ",
    " \\         / ",
    "  '._   _.'  ",
    "     '''     "
};

static const char *NUBE_A[4] = {
    "      .-~~~-.        ",
    "   .-(         )-.   ",
    "  (               )  ",
    "   `-._________.-'   "
};

static const char *NUBE_B[4] = {
    "     .--.        ",
    "  .-(    ).-.    ",
    " (           )   ",
    "  `-.______.-'   "
};

/* gaviotas: 2 cuadros de 2 filas */
static const char *GAV[2][2] = {
    { "\\_   _/", "  `v'  " },
    { "  _v_  ", " /   \\ " }
};
static const int GAV_Y[4] = { 1, 5, 3, 7 };

static const char *LEJOS[4] = {
    "    |\\    ",
    "    | \\   ",
    "  __|__\\__",
    "  \\_______/"
};

/* ================== UTILIDADES ================== */

/* seno sin libm (no hace falta -lm) */
static double seno(double x) {
    const double PI = 3.14159265358979, DOSPI = 6.28318530717959;
    double y;
    x = x - (double)(long)(x / DOSPI) * DOSPI;
    if (x > PI)  x -= DOSPI;
    if (x < -PI) x += DOSPI;
    y = 1.27323954 * x - 0.405284735 * x * (x < 0 ? -x : x);
    y = 0.225 * (y * (y < 0 ? -y : y) - y) + y;
    return y;
}

static int redondea(double v) {
    return (int)(v < 0 ? v - 0.5 : v + 0.5);
}

static void aviso(int tipo, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_msg, sizeof g_msg, fmt, ap);
    va_end(ap);
    g_msg_tipo = tipo;
}

/* pinta s en (y,x) ignorando espacios y recortando a [xlo, xhi) */
static void pinta(int y, int x, const char *s, int xlo, int xhi) {
    for (int i = 0; s[i]; i++) {
        int cx = x + i;
        if (s[i] != ' ' && cx >= xlo && cx < xhi)
            mvaddch(y, cx, (unsigned char)s[i]);
    }
}

static void caja(int y, int x, int h, int w, const char *titulo) {
    attrset(A_NORMAL);
    for (int i = 0; i < h; i++)
        mvprintw(y + i, x, "%*s", w, "");
    mvaddch(y, x, ACS_ULCORNER);
    mvhline(y, x + 1, ACS_HLINE, w - 2);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvvline(y + 1, x, ACS_VLINE, h - 2);
    mvvline(y + 1, x + w - 1, ACS_VLINE, h - 2);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    if (titulo) {
        attrset(P(C_TIT) | A_BOLD);
        mvprintw(y, x + 2, " %s ", titulo);
        attrset(A_NORMAL);
    }
}

/* escribe s en hasta maxl filas de ancho w, cortando por espacios */
static void escribirEnvuelto(int y, int x, int w, int maxl, const char *s) {
    int n = (int)strlen(s), i = 0, l = 0;
    while (i < n && l < maxl) {
        int len = n - i;
        if (len > w) {
            int k;
            len = w;
            k = len;
            while (k > 0 && s[i + k] != ' ')
                k--;
            if (k > 0)
                len = k;
        }
        mvprintw(y + l, x, "%.*s", len, s + i);
        i += len;
        while (i < n && s[i] == ' ')
            i++;
        l++;
    }
}

/* calcula el tamaño de la interfaz segun la terminal */
static void calcularTamano(void) {
    UW = COLS > MAXW ? MAXW : COLS;
    UH = (LINES - 4 - HH) > MAXH ? MAXH : (LINES - 4 - HH);
    LW = UW * 3 / 10;
    if (LW < 34) LW = 34;
    if (LW > 44) LW = 44;
}

/* ================== VISOR DE TEXTO ================== */

/* arma las filas del texto con ancho w. Si dibuja, pinta las visibles.
   Devuelve el total de filas. */
static int filasTexto(int off, int alto, int dibuja, int y, int x, int w) {
    int fila = 0, nl = 1, cur, pl, vis;
    char pref[16];
    lineas l;
    palabras p;

    for (l = texto.primera; l != NULL; l = l->sig, nl++) {
        snprintf(pref, sizeof pref, "%d: ", nl);
        pl = (int)strlen(pref);
        cur = pl;
        vis = dibuja && fila >= off && fila < off + alto;
        if (vis) {
            attrset(P(C_TIT) | A_BOLD);
            mvprintw(y + fila - off, x, "%s", pref);
            attrset(A_NORMAL);
        }
        for (p = l->primera; p != NULL; p = p->sig) {
            int len = (int)strlen(p->info);
            if (cur > pl && cur + len > w) {
                fila++;
                cur = pl;
            }
            vis = dibuja && fila >= off && fila < off + alto;
            if (vis) {
                int mal = !estaPalabraPerteneceAlDicc(p, texto.dicc);
                if (mal)
                    attrset(P(C_ERR) | A_BOLD);
                mvprintw(y + fila - off, x + cur, "%.*s", w - cur, p->info);
                attrset(A_NORMAL);
            }
            cur += len + 1;
        }
        fila++;
    }
    return fila;
}

static int dc_fila, dc_off, dc_alto, dc_y, dc_x, dc_w, dc_dib;

static void recorrerDicc(diccionario d) {
    if (d == NULL)
        return;
    recorrerDicc(d->izq);
    if (dc_dib && dc_fila >= dc_off && dc_fila < dc_off + dc_alto)
        mvprintw(dc_y + dc_fila - dc_off, dc_x, "%.*s", dc_w, d->info);
    dc_fila++;
    recorrerDicc(d->der);
}

/* una palabra por fila, en orden alfabetico. Devuelve el total de filas. */
static int filasDicc(int off, int alto, int dibuja, int y, int x, int w) {
    dc_fila = 0; dc_off = off; dc_alto = alto;
    dc_y = y; dc_x = x; dc_w = w; dc_dib = dibuja;
    recorrerDicc(texto.dicc);
    return dc_fila;
}

static int maxOffDicc(void) {
    int total = filasDicc(0, 0, 0, 0, 0, LW - 4);
    int m = total - (UH - 2);
    return m > 0 ? m : 0;
}

static int maxOffTexto(void) {
    int total = filasTexto(0, 0, 0, 0, 0, LW - 4);
    int m = total - (UH - 2);
    return m > 0 ? m : 0;
}

/* ================== ESCENA ================== */

static int bobBarco(void) {
    return seno(tick * 0.11) > 0.25 ? 1 : 0;
}

/* una fila de olas; solo dibuja donde la ola supera 'umbral' */
static void filaOlas(int y, int x, int w, int fila, double umbral, int lunaC) {
    for (int c = 0; c < w; c++) {
        double v = seno(c * 0.33 - tick * 0.16 + fila * 1.7)
                 + 0.55 * seno(c * 0.11 + tick * 0.05 + fila * 2.3);
        char ch;
        int atr;
        if (v <= umbral)
            continue;
        if (v > 1.15)       { ch = '~'; atr = P(C_ESPUMA) | A_BOLD; }
        else if (v > 0.35)  { ch = '~'; atr = P(C_MAR); }
        else if (v > -0.45) { ch = '-'; atr = P(C_MAR) | A_DIM; }
        else
            continue;
        if (ch == '~' && atr == P(C_MAR) && fila * 3 >= g_sea * 2)
            atr |= A_BOLD;                       /* mar de adelante, mas vivo */
        if (lunaC >= 0 && abs(c - lunaC) <= 3)
            atr = P(C_LUNA) | A_BOLD;
        attrset(atr);
        mvaddch(y, x + c, ch);
    }
}

static void dibujarBarco(int sy, int sx, int IW, int HZ) {
    int top = sy + HZ - 7 + bobBarco();
    int bx = sx + (int)nave_x;
    double roll = 3.0 * seno(tick * 0.11 + 1.0);
    for (int i = 0; i < BH; i++) {
        int off = (i < 8) ? redondea(roll * (8 - i) / 8.0) : 0;
        attrset(P(BARCO_COL[i]) | ((i >= 1 && i <= 6) ? A_BOLD : A_NORMAL));
        pinta(top + i, bx + off, BARCO[i], sx, sx + IW);
    }
}

static void dibujarEspuma(int sy, int sx, int IW, int HZ) {
    int top = sy + HZ - 7 + bobBarco();
    int bx = sx + (int)nave_x;
    int moviendo = (nave_modo == 1 || nave_modo == 3);

    attrset(P(C_ESPUMA) | A_BOLD);
    pinta(top + 10, bx + BW - 2, ((tick / 3) % 2) ? "~~~" : "~~ ", sx, sx + IW);
    pinta(top + 9,  bx + BW - 1, ((tick / 3) % 2) ? " ~" : "~ ", sx, sx + IW);

    if (moviendo) {
        for (int k = 0; k < 16; k++) {
            if (((k + tick / 2) % 3) == 0) {
                attrset(k > 7 ? (P(C_MAR) | A_DIM) : (P(C_ESPUMA) | A_BOLD));
                pinta(top + 10, bx + 1 - k, "~", sx, sx + IW);
                if (k % 2 == 0)
                    pinta(top + 11, bx + 3 - k, "~", sx, sx + IW);
            }
        }
    }
}

static void dibujarEscena(int y0, int x0) {
    const int IW = UW - LW - 2;                  /* ancho interior */
    const int IH = UH - 2;                       /* alto interior  */
    const int SEA = (IH / 3 < 7) ? 7 : IH / 3;  /* filas de mar   */
    const int HZ = IH - SEA;                     /* fila del horizonte */
    const int lunaC = IW - 17 + 6;
    const int cl = ((HZ - 8) / 3 < 3) ? 3 : (HZ - 8) / 3;
    int sy = y0 + 1, sx = x0 + 1, i, k, r;

    g_sea = SEA;
    caja(y0, x0, UH, UW - LW, NULL);

    /* luna */
    attrset(P(C_LUNA) | A_BOLD);
    for (r = 0; r < 7; r++)
        pinta(sy + 1 + r, sx + IW - 17, LUNA[r], sx, sx + IW);

    /* nubes */
    for (k = 0; k < 3; k++) {
        const char **nb = (k % 2 == 0) ? NUBE_A : NUBE_B;
        int span = IW + 30;
        int p = (int)((tick / (8 + k * 4) + k * (span / 3)) % span);
        int cx = IW + 4 - p;
        int cy = 1 + k * cl;
        attrset(P(C_NUBE) | (k == 0 ? A_BOLD : A_DIM));
        for (r = 0; r < 4; r++)
            pinta(sy + cy + r, sx + cx, nb[r], sx, sx + IW);
    }

    /* gaviotas */
    attrset(P(C_NUBE) | A_BOLD);
    for (k = 0; k < 2; k++) {
        int gx = (int)((tick / 3 + k * 23) % (IW + 6)) - 3;
        int gy = 2 + k * 2 + (int)(1.2 * seno(tick * 0.07 + k * 2));
        pinta(sy + gy, sx + gx, ((tick / 5 + k) % 2) ? "\\v/" : "-v-", sx, sx + IW);
    }

    /* barquito lejano cuando no hay barco principal */
    if (nave_modo == 0) {
        int dx = (int)((tick / 5) % (IW + 20)) - 10;
        attrset(P(C_VELA) | A_DIM);
        for (r = 0; r < 4; r++)
            pinta(sy + HZ - 4 + r, sx + dx, LEJOS[r], sx, sx + IW);
    }

    /* olas de atras */
    for (i = 0; i < 3; i++)
        filaOlas(sy + HZ + i, sx, IW, i, -9.0, lunaC);

    /* barco */
    if (nave_modo != 0)
        dibujarBarco(sy, sx, IW, HZ);

    /* olas de adelante (tapan un poco el casco) */
    for (i = 3; i < SEA; i++)
        filaOlas(sy + HZ + i, sx, IW, i,
                 (i <= 4 && nave_modo != 0) ? 0.45 : -9.0, lunaC);

    if (nave_modo != 0)
        dibujarEspuma(sy, sx, IW, HZ);

    attrset(A_NORMAL);
}

/* ================== DIBUJO GENERAL ================== */

static void dibujarCabecera(int y0, int x0) {
    const char *ol[3] = { "~~~", "~~ ", " ~~" };
    /* conjunto = logo (7) + espacio (4) + texto (19) = 30 columnas */
    const int gx = x0 + UW / 2 - 15;   /* inicio del logo */
    const int tx = gx + 11;            /* inicio del texto */

    caja(y0, x0, HH, UW, NULL);

    attrset(P(C_TIT) | A_BOLD);
    mvprintw(y0 + 1, gx, "  |\\");
    mvprintw(y0 + 2, gx, "  | \\");
    mvprintw(y0 + 3, gx, " _|__\\_");
    mvprintw(y0 + 4, gx, " \\____/");

    attrset(A_BOLD);
    mvprintw(y0 + 2, tx + 2, "B I T A C O R A");
    attrset(A_DIM);
    mvprintw(y0 + 3, tx, "viento en popa  ");
    attrset(P(C_MAR));
    printw("%s", ol[(tick / 4) % 3]);
    attrset(A_NORMAL);
}

static void dibujarMenuUI(void) {
    int i, ox, oy;
    const char *pie;

    ox = (COLS - UW) / 2;
    oy = (LINES - (HH + UH + 4)) / 2;

    dibujarCabecera(oy, ox);
    oy += HH;
    dibujarEscena(oy, ox + LW);

    if (lp_modo == 5) {
        /* creditos: los nombres bajan por la caja */
        const char *cr[11] = { "DESARROLLADO POR", "", "Lucas Pereyra", "",
                               "Cristofer Ramos", "", "",
                               "Quien se concentra solo",
                               "en la cima de la montana,",
                               "se pierde la belleza",
                               "del camino" };
        const int n = 11;
        int r, y, top;
        caja(oy, ox, UH, LW, "Creditos");
        top = oy + UH - 1 - (int)(((tick - cr_t0) / 4) % (UH - 2 + n));
        for (r = 0; r < n; r++) {
            y = top + r;
            if (y < oy + 1 || y > oy + UH - 2 || cr[r][0] == '\0')
                continue;
            attrset(r == 0 ? A_DIM : (r >= 7 ? (P(C_ESPUMA) | A_BOLD) : (P(C_TIT) | A_BOLD)));
            mvprintw(y, ox + (LW - (int)strlen(cr[r])) / 2, "%s", cr[r]);
            if (r == 2 || r == 4) {
                int len = (int)strlen(cr[r]);
                int hx = (int)((tick / 2) % (len + 6));
                if (hx < len) {
                    attrset(P(C_ESPUMA) | A_BOLD);
                    mvaddch(y, ox + (LW - len) / 2 + hx, (unsigned char)cr[r][hx]);
                }
            }
        }
        attrset(A_NORMAL);
    } else if (lp_modo == 3 || lp_modo == 4) {
        /* visor del texto */
        int alto = UH - 2, total, mx;
        caja(oy, ox, UH, LW, lp_modo == 3 ? "Texto" : "Diccionario");
        total = (lp_modo == 3) ? filasTexto(0, 0, 0, 0, 0, LW - 4)
                               : filasDicc(0, 0, 0, 0, 0, LW - 4);
        mx = total > alto ? total - alto : 0;
        if (lp_off > mx) lp_off = mx;
        if (lp_off < 0)  lp_off = 0;
        if (total == 0) {
            attrset(A_DIM);
            mvprintw(oy + 1, ox + 2, lp_modo == 3 ? "(texto vacio)" : "(diccionario vacio)");
        } else if (lp_modo == 3) {
            filasTexto(lp_off, alto, 1, oy + 1, ox + 2, LW - 4);
        } else {
            filasDicc(lp_off, alto, 1, oy + 1, ox + 2, LW - 4);
        }
        attrset(A_NORMAL);
        if (lp_off > 0)
            mvaddch(oy + 1, ox + LW - 1, ACS_UARROW);
        if (lp_off < mx)
            mvaddch(oy + UH - 2, ox + LW - 1, ACS_DARROW);
    } else {
        if (lp_modo == 0) {
            caja(oy, ox, MH, LW, "Menu");
            for (i = 0; i < N_OP; i++) {
                int y = oy + 2 + i;
                if (i == g_sel) {
                    attrset(A_REVERSE | A_BOLD);
                    mvprintw(y, ox + 1, " > %2d  %-*s", i + 1, LW - 9, OPC[i]);
                } else {
                    attrset(A_NORMAL);
                    mvprintw(y, ox + 1, "   %2d  %-*s", i + 1, LW - 9, OPC[i]);
                }
                attrset(A_NORMAL);
            }
        } else if (lp_modo == 1) {
            caja(oy, ox, MH, LW, lp_titulo);
            escribirEnvuelto(oy + 2, ox + 2, LW - 4, 3, lp_ayuda);
            attrset(P(C_TIT) | A_BOLD);
            mvprintw(oy + 8, ox + 2, "> ");
            attrset(A_BOLD);
            mvprintw(oy + 8, ox + 4, "%s", lp_buf);
            if ((tick / 5) % 2 == 0 && lp_n < lp_max)
                addch('_');
            attrset(A_DIM);
            mvhline(oy + 9, ox + 2, ACS_HLINE, LW - 4);
            mvprintw(oy + 10, ox + 2, "%d/%d", lp_n, lp_max);
            attrset(A_NORMAL);
        } else {
            caja(oy, ox, MH, LW, lp_titulo);
            escribirEnvuelto(oy + 2, ox + 2, LW - 4, 4, lp_ayuda);
            attrset(lp_si ? (A_REVERSE | A_BOLD) : A_NORMAL);
            mvprintw(oy + 9, ox + 2, " [S] Si ");
            attrset(!lp_si ? (A_REVERSE | A_BOLD) : A_NORMAL);
            mvprintw(oy + 9, ox + 13, " [N] No ");
            attrset(A_NORMAL);
        }

        /* salida */
        caja(oy + MH, ox, UH - MH, LW, "Salida");
        if (g_msg_tipo == 1)      attrset(P(C_OK) | A_BOLD);
        else if (g_msg_tipo == 2) attrset(P(C_ERR) | A_BOLD);
        else                      attrset(A_NORMAL);
        escribirEnvuelto(oy + MH + 1, ox + 2, LW - 4, UH - MH - 2, g_msg);
        attrset(A_NORMAL);
    }

    switch (lp_modo) {
    case 1:  pie = " Enter: aceptar | Esc: cancelar | Backspace: borrar"; break;
    case 2:  pie = " S: si | N: no"; break;
    case 5:  pie = " Q, Esc o Enter: volver"; break;
    case 3:
    case 4:  pie = " flechas / PgUp / PgDn: desplazar | Q, Esc o Enter: volver"; break;
    default: pie = " flechas o w/s: mover | Enter: elegir | /: comando (numero) | Q: salir"; break;
    }
    attrset(A_DIM);
    if (g_cmd)
        mvprintw(oy + UH + 2, ox + LW + 2, "Enter: ejecutar | Esc: cancelar | 1-%d", N_OP);
    else
        mvprintw(oy + UH + 2, ox, "%s", pie);
    attrset(A_NORMAL);
    if (g_cmd) {
        caja(oy + UH + 1, ox, 3, LW, "Comando");
        attrset(P(C_TIT) | A_BOLD);
        mvprintw(oy + UH + 2, ox + 2, "/ ");
        attrset(A_BOLD);
        printw("%s", g_cbuf);
        if ((tick / 5) % 2 == 0 && g_cn < 2)
            addch('_');
        attrset(A_NORMAL);
    }
}

/* ================== PORTADA ================== */

/* letras gigantes de B I T A C O R A (5 filas c/u, mismo ancho por letra) */
static const char *TIT_G[8][5] = {
    { " ____  ", "| __ ) ", "|  _ \\ ", "| |_) |", "|____/ " },
    { " ___ ", "|_ _|", " | | ", " | | ", "|___|" },
    { " _____ ", "|_   _|", "  | |  ", "  | |  ", "  |_|  " },
    { "    _    ", "   / \\   ", "  / _ \\  ", " / ___ \\ ", "/_/   \\_\\" },
    { "  ____ ", " / ___|", "| |    ", "| |___ ", " \\____|" },
    { "  ___  ", " / _ \\ ", "| | | |", "| |_| |", " \\___/ " },
    { " ____  ", "|  _ \\ ", "| |_) |", "|  _ < ", "|_| \\_\\" },
    { "    _    ", "   / \\   ", "  / _ \\  ", " / ___ \\ ", "/_/   \\_\\" }
};

static void dibujarPortada(int y0, int x0, int H) {
    const int IW = UW - 2;
    const int IH = H - 2;
    const int top = y0 + 1 + (IH > 26 ? (IH - 26) / 2 : 0);
    const int cx = x0 + UW / 2;
    const char *ol[3] = { "~~~", "~~ ", " ~~" };
    int i, r, g, x, bx, bt, hl;
    double roll;

    caja(y0, x0, H, UW, NULL);

    /* barco grande, cabeceando */
    bt = top + 1 + bobBarco();
    bx = cx - BW / 2;
    roll = 2.0 * seno(tick * 0.11 + 1.0);
    for (i = 0; i < BH; i++) {
        int off = (i < 8) ? redondea(roll * (8 - i) / 8.0) : 0;
        attrset(P(BARCO_COL[i]) | ((i >= 1 && i <= 6) ? A_BOLD : A_NORMAL));
        pinta(bt + i, bx + off, BARCO[i], x0 + 1, x0 + UW - 1);
    }

    /* mar bajo el casco */
    for (i = 0; i < 3; i++)
        filaOlas(top + 12 + i, x0 + 1, IW, i, -9.0, -1);

    /* titulo gigante: un brillo recorre las letras */
    x = cx - 29;
    hl = (int)((tick / 3) % 16);
    for (g = 0; g < 8; g++) {
        attrset(P(g == hl ? C_ESPUMA : C_TIT) | A_BOLD);
        for (r = 0; r < 5; r++)
            pinta(top + 16 + r, x, TIT_G[g][r], x0 + 1, x0 + UW - 1);
        x += (int)strlen(TIT_G[g][0]);
    }

    attrset(A_NORMAL);
    mvprintw(top + 22, cx - 19, "p r o c e s a d o r   d e   t e x t o s");
    attrset(A_DIM);
    mvprintw(top + 23, cx - 9, "viento en popa  ");
    attrset(P(C_MAR));
    printw("%s", ol[(tick / 4) % 3]);

    /* aviso titilando */
    attrset(((tick / 8) % 2) ? A_DIM : A_BOLD);
    mvprintw(top + 25, cx - 19, "Presiona cualquier tecla para comenzar");
    attrset(A_NORMAL);
}

/* ================== DIBUJO GENERAL ================== */

static void dibujarTodo(void) {
    int H, ox, oy, y, fb;
    double p;

    erase();
    if (COLS < MINW || LINES < MINH) {
        mvprintw(0, 0, "Agranda la terminal (minimo %dx%d)", MINW, MINH);
        return;
    }
    calcularTamano();
    H = HH + UH;
    ox = (COLS - UW) / 2;
    oy = (LINES - (H + 4)) / 2;

    if (g_fase == 0) {
        dibujarPortada(oy, ox, H);
        return;
    }
    if (g_fase == 2) {
        dibujarMenuUI();
        return;
    }

    /* transicion: una ola de espuma barre la portada y deja ver el menu */
    dibujarPortada(oy, ox, H);
    if (g_buf != NULL && (getmaxy(g_buf) != LINES || getmaxx(g_buf) != COLS)) {
        delwin(g_buf);
        g_buf = NULL;
    }
    if (g_buf == NULL)
        g_buf = newwin(LINES, COLS, 0, 0);
    if (g_buf == NULL) {
        g_fase = 2;
        return;
    }
    overwrite(stdscr, g_buf);          /* guarda la portada */
    erase();
    dibujarMenuUI();                   /* el menu queda debajo */

    p = g_trans / (double)TR_N;
    p = p * p * (3.0 - 2.0 * p);       /* arranca y frena suave */
    fb = (int)(p * (COLS + 16)) - 8;
    for (y = 0; y < LINES; y++) {
        int f = fb + redondea(2.5 * seno(y * 0.7 - g_trans * 0.6));
        int c0 = f < 0 ? 0 : f;
        if (f < COLS)
            copywin(g_buf, stdscr, y, c0, y, c0, y, COLS - 1, FALSE);
        attrset(P(C_ESPUMA) | A_BOLD);
        if (f >= 0 && f < COLS)
            mvaddch(y, f, '~');
        attrset(P(C_MAR));
        if (f + 1 >= 0 && f + 1 < COLS)
            mvaddch(y, f + 1, '~');
        attrset(P(C_MAR) | A_DIM);
        if (f - 1 >= 0 && f - 1 < COLS)
            mvaddch(y, f - 1, '-');
    }
    attrset(A_NORMAL);
}

/* ================== ANIMACION ================== */

static void pasoAnim(void) {
    const int IW = UW - LW - 2;
    int centro = (IW - BW) / 2;
    tick++;
    if (g_fase == 1 && ++g_trans >= TR_N) {
        g_fase = 2;
        if (g_buf != NULL) {
            delwin(g_buf);
            g_buf = NULL;
        }
        flushinp();
    }
    switch (nave_modo) {
    case 1: {
        double dist = centro - nave_x;
        if (dist < 0.6) {
            nave_x = centro;
            nave_modo = 2;
            nave_t = 0;
        } else {
            double v = dist * 0.1 + 0.3;
            if (v > 2.2) v = 2.2;
            nave_x += v;
        }
        break;
    }
    case 2: {
        double amp;
        nave_t++;
        amp = nave_t < 40 ? 6.0 * nave_t / 40.0 : 6.0;
        nave_x = centro + amp * seno(tick * 0.03);
        break;
    }
    case 3:
        nave_v += 0.1;
        if (nave_v > 2.4) nave_v = 2.4;
        nave_x += nave_v;
        if (nave_x > IW) {
            nave_modo = 0;
            nave_x = NAVE_INI;
        }
        break;
    }
}

/* dibuja un cuadro, espera una tecla (o timeout) y avanza la animacion */
static int esperar(void) {
    int t;
    dibujarTodo();
    refresh();
    t = getch();
    pasoAnim();
    return t;
}

static void zarpar(void) {
    /* no espera: el barco entra mientras ya se usa el menu/form */
    if (nave_modo == 0 || nave_modo == 3) {
        nave_x = NAVE_INI;
        nave_v = 0;
        nave_modo = 1;
    }
}

static void atracar(void) {
    /* no espera: el barco se va solo mientras el menu ya responde */
    if (nave_modo == 1 || nave_modo == 2) {
        nave_modo = 3;
        nave_v = 0.3;
    }
}

/* ================== ENTRADA DE DATOS ================== */

/* devuelve 1 si acepto, 0 si cancelo con Esc */
static int leerCampo(const char *titulo, const char *ayuda, int soloNum,
                     int maxlen, char *salida) {
    int t;
    lp_modo = 1;
    lp_titulo = titulo;
    lp_ayuda = ayuda;
    lp_n = 0;
    lp_max = maxlen;
    lp_buf[0] = '\0';

    for (;;) {
        t = esperar();
        if (t == '\n' || t == '\r' || t == KEY_ENTER) {
            if (lp_n > 0)
                break;
        } else if (t == 27) {
            lp_modo = 0;
            return 0;
        } else if ((t == KEY_BACKSPACE || t == 127 || t == '\b') && lp_n > 0) {
            lp_buf[--lp_n] = '\0';
        } else if (lp_n < lp_max &&
                   (soloNum ? (t >= '0' && t <= '9') : (t > 32 && t < 127))) {
            lp_buf[lp_n++] = (char)t;
            lp_buf[lp_n] = '\0';
        }
    }
    strcpy(salida, lp_buf);
    lp_modo = 0;
    return 1;
}

static int confirmar(const char *titulo, const char *pregunta) {
    int t;
    lp_modo = 2;
    lp_titulo = titulo;
    lp_ayuda = pregunta;
    lp_si = 1;
    for (;;) {
        t = esperar();
        if (t == KEY_LEFT || t == KEY_RIGHT || t == KEY_UP || t == KEY_DOWN ||
            t == 'a' || t == 'd' || t == 'h' || t == 'l' || t == '\t')
            lp_si = !lp_si;
        else if (t == '\n' || t == '\r' || t == KEY_ENTER) {
            lp_modo = 0;
            return lp_si;
        }
        else if (t == 's' || t == 'S' || t == 'y' || t == 'Y') {
            lp_modo = 0;
            return 1;
        }
        else if (t == 'n' || t == 'N' || t == 27) {
            lp_modo = 0;
            return 0;
        }
    }
}

static lineas lineaEn(unsigned n) {
    lineas l = texto.primera;
    while (l != NULL && n > 1) {
        l = l->sig;
        n--;
    }
    return l;
}

static void cancelado(void) {
    aviso(0, "Cancelado.");
    atracar();
}

/* ================== OPERACIONES ================== */

static void opInsertarLinea(void) {
    if (!inicializado) {
        aviso(2, "El texto esta destruido. Usa Reiniciar texto.");
        return;
    }
    zarpar();
    if (InsertarLinea() == OK)
        aviso(1, "Linea %u agregada al final.", texto.cantidad);
    else
        aviso(2, "No se pudo agregar la linea.");
    atracar();
}

static void opBorrarLinea(void) {
    char s[32], ayuda[96];
    unsigned nl;
    if (!inicializado) {
        aviso(2, "El texto esta destruido. Usa Reiniciar texto.");
        return;
    }
    if (texto.cantidad == 0) {
        aviso(2, "No hay lineas para borrar.");
        return;
    }
    zarpar();
    snprintf(ayuda, sizeof ayuda, "Numero de linea a borrar (1 a %u)", texto.cantidad);
    if (!leerCampo("Borrar linea", ayuda, 1, 5, s)) { cancelado(); return; }
    nl = (unsigned)atoi(s);
    if (nl < 1 || nl > texto.cantidad) {
        aviso(2, "La linea %u no existe (hay %u).", nl, texto.cantidad);
        atracar();
        return;
    }
    if (BorrarLinea(nl) == OK)
        aviso(1, "Linea %u borrada.", nl);
    else
        aviso(2, "No se pudo borrar la linea.");
    atracar();
}

static void opInsertarPalabra(void) {
    char pal[32], sl[32], sp[32], ayuda[96];
    unsigned nl, np, maxp;
    int desborda;
    lineas l;

    if (!inicializado) {
        aviso(2, "El texto esta destruido. Usa Reiniciar texto.");
        return;
    }
    if (texto.cantidad == 0) {
        aviso(2, "No hay lineas. Inserta una linea primero.");
        return;
    }
    zarpar();
    if (!leerCampo("Palabra", "Escribe la palabra (sin espacios)", 0, 20, pal)) {
        cancelado();
        return;
    }
    snprintf(ayuda, sizeof ayuda, "Numero de linea (1 a %u)", texto.cantidad);
    if (!leerCampo("Linea", ayuda, 1, 5, sl)) { cancelado(); return; }
    nl = (unsigned)atoi(sl);
    if (nl < 1 || nl > texto.cantidad) {
        aviso(2, "La linea %u no existe (hay %u).", nl, texto.cantidad);
        atracar();
        return;
    }
    l = lineaEn(nl);
    maxp = l->cantidad + 1;
    if (maxp > MAX_CANT_PALABRAS_X_LINEA)
        maxp = MAX_CANT_PALABRAS_X_LINEA;
    snprintf(ayuda, sizeof ayuda, "Posicion en la linea %u (1 a %u)", nl, maxp);
    if (!leerCampo("Posicion", ayuda, 1, 5, sp)) { cancelado(); return; }
    np = (unsigned)atoi(sp);
    if (np < 1 || np > maxp) {
        aviso(2, "Posicion invalida: %u (rango 1 a %u).", np, maxp);
        atracar();
        return;
    }
    desborda = (l->cantidad >= MAX_CANT_PALABRAS_X_LINEA);
    {
        char *copia = (char *)malloc(strlen(pal) + 1);
        if (copia != NULL) {
            strcpy(copia, pal);
            if (InsertarPalabra(nl, np, copia) == OK) {
                aviso(1, "'%s' insertada en linea %u, pos %u.%s", pal, nl, np,
                      desborda ? " Se reacomodo el texto." : "");
            } else {
                free(copia);
                aviso(2, "No se pudo insertar la palabra.");
            }
        } else {
            aviso(2, "Sin memoria.");
        }
    }
    atracar();
}

static void opBorrarPalabra(void) {
    char sl[32], sp[32], ayuda[96];
    unsigned nl, np;
    lineas l;

    if (!inicializado) {
        aviso(2, "El texto esta destruido. Usa Reiniciar texto.");
        return;
    }
    if (texto.cantidad == 0) {
        aviso(2, "No hay lineas.");
        return;
    }
    zarpar();
    snprintf(ayuda, sizeof ayuda, "Numero de linea (1 a %u)", texto.cantidad);
    if (!leerCampo("Linea", ayuda, 1, 5, sl)) { cancelado(); return; }
    nl = (unsigned)atoi(sl);
    if (nl < 1 || nl > texto.cantidad) {
        aviso(2, "La linea %u no existe (hay %u).", nl, texto.cantidad);
        atracar();
        return;
    }
    l = lineaEn(nl);
    if (l->cantidad == 0) {
        aviso(2, "La linea %u esta vacia.", nl);
        atracar();
        return;
    }
    snprintf(ayuda, sizeof ayuda, "Posicion de la palabra (1 a %u)", l->cantidad);
    if (!leerCampo("Posicion", ayuda, 1, 5, sp)) { cancelado(); return; }
    np = (unsigned)atoi(sp);
    if (np < 1 || np > l->cantidad) {
        aviso(2, "Posicion invalida: %u (rango 1 a %u).", np, l->cantidad);
        atracar();
        return;
    }
    if (BorrarPalabra(nl, np) == OK)
        aviso(1, "Palabra %u de la linea %u borrada.", np, nl);
    else
        aviso(2, "No se pudo borrar la palabra.");
    atracar();
}

static void opImprimir(void) {
    int t, mx;
    zarpar();
    lp_modo = 3;
    lp_off = 0;
    for (;;) {
        t = esperar();
        if (t == 'q' || t == 'Q' || t == 27 || t == '\n' || t == KEY_ENTER)
            break;
        mx = maxOffTexto();
        switch (t) {
        case KEY_UP:   case 'k': case 'w': if (lp_off > 0)  lp_off--; break;
        case KEY_DOWN: case 'j': case 's': if (lp_off < mx) lp_off++; break;
        case KEY_PPAGE: lp_off -= 10; if (lp_off < 0)  lp_off = 0;  break;
        case KEY_NPAGE: lp_off += 10; if (lp_off > mx) lp_off = mx; break;
        case KEY_HOME:  lp_off = 0;  break;
        case KEY_END:   lp_off = mx; break;
        }
    }
    lp_modo = 0;
    aviso(0, "Texto: %u linea(s).", texto.cantidad);
    atracar();
}

static void opReiniciar(void) {
    zarpar();
    if (confirmar("Reiniciar", "Se borra TODO el texto. Continuar?")) {
        Inicializar();
        aviso(1, "Texto reiniciado: vacio y listo para usar.");
    } else {
        aviso(0, "Cancelado.");
    }
    atracar();
}

static void opDestruir(void) {
    if (!inicializado) {
        aviso(2, "El texto ya esta destruido.");
        return;
    }
    zarpar();
    if (confirmar("Destruir", "Se libera todo el texto de memoria. Continuar?")) {
        Destruir();
        aviso(1, "Texto destruido. Usa Reiniciar para volver a empezar.");
    } else {
        aviso(0, "Cancelado.");
    }
    atracar();
}

static void opIngresarDicc(void) {
    char pal[32];
    char *copia;
    if (!inicializado) {
        aviso(2, "El texto esta destruido. Usa Reiniciar texto.");
        return;
    }
    zarpar();
    if (!leerCampo("Diccionario", "Palabra a agregar al diccionario (sin espacios)",
                   0, 20, pal)) {
        cancelado();
        return;
    }
    copia = (char *)malloc(strlen(pal) + 1);
    if (copia == NULL) {
        aviso(2, "Sin memoria.");
        atracar();
        return;
    }
    strcpy(copia, pal);
    if (IngresarPalabraDiccionario(copia) == OK) {
        aviso(1, "'%s' agregada al diccionario.", pal);
    } else {
        free(copia);
        aviso(2, "'%s' ya estaba en el diccionario.", pal);
    }
    atracar();
}

static void opImprimirDicc(void) {
    int t, mx;
    if (!inicializado) {
        aviso(2, "El texto esta destruido. Usa Reiniciar texto.");
        return;
    }
    zarpar();
    lp_modo = 4;
    lp_off = 0;
    for (;;) {
        t = esperar();
        if (t == 'q' || t == 'Q' || t == 27 || t == '\n' || t == KEY_ENTER)
            break;
        mx = maxOffDicc();
        switch (t) {
        case KEY_UP:   case 'k': case 'w': if (lp_off > 0)  lp_off--; break;
        case KEY_DOWN: case 'j': case 's': if (lp_off < mx) lp_off++; break;
        case KEY_PPAGE: lp_off -= 10; if (lp_off < 0)  lp_off = 0;  break;
        case KEY_NPAGE: lp_off += 10; if (lp_off > mx) lp_off = mx; break;
        case KEY_HOME:  lp_off = 0;  break;
        case KEY_END:   lp_off = mx; break;
        }
    }
    lp_modo = 0;
    aviso(0, "Diccionario mostrado.");
    atracar();
}

static void opAgregarTextoDicc(void) {
    if (!inicializado) {
        aviso(2, "El texto esta destruido. Usa Reiniciar texto.");
        return;
    }
    if (texto.cantidad == 0) {
        aviso(2, "No hay texto para agregar.");
        return;
    }
    zarpar();
    if (AgregarIncorrectasAlDiccionario() == OK)
        aviso(1, "Palabras del texto agregadas al diccionario.");
    else
        aviso(2, "No se pudo agregar al diccionario.");
    atracar();
}

/* lee un numero en el pie. Devuelve 1..N_OP, o 0 si cancelo o era invalido */
static int leerComando(void) {
    int t, n;
    g_cmd = 1;
    g_cn = 0;
    g_cbuf[0] = '\0';
    for (;;) {
        t = esperar();
        if (t == '\n' || t == '\r' || t == KEY_ENTER) {
            if (g_cn > 0)
                break;
        } else if (t == 27) {
            g_cmd = 0;
            return 0;
        } else if (t == KEY_BACKSPACE || t == 127 || t == '\b') {
            if (g_cn > 0) {
                g_cbuf[--g_cn] = '\0';
            } else {
                g_cmd = 0;          /* backspace con el campo vacio cancela */
                return 0;
            }
        } else if (g_cn < 2 && t >= '0' && t <= '9') {
            g_cbuf[g_cn++] = (char)t;
            g_cbuf[g_cn] = '\0';
        }
    }
    n = atoi(g_cbuf);
    g_cmd = 0;
    if (n < 1 || n > N_OP) {
        aviso(2, "Comando invalido: %d (1 a %d).", n, N_OP);
        return 0;
    }
    return n;
}

static void opCreditos(void) {
    int t;
    zarpar();
    lp_modo = 5;
    cr_t0 = tick;
    for (;;) {
        t = esperar();
        if (t == 'q' || t == 'Q' || t == 27 || t == '\n' || t == KEY_ENTER)
            break;
    }
    lp_modo = 0;
    aviso(0, "Gracias por navegar con nosotros.");
    atracar();
}

/* devuelve 1 si hay que salir */
static int activar(int op) {
    switch (op) {
    case 0: opInsertarLinea();   break;
    case 1: opBorrarLinea();     break;
    case 2: opInsertarPalabra(); break;
    case 3: opBorrarPalabra();   break;
    case 4: opImprimir();        break;
    case 5: opReiniciar();       break;
    case 6: opDestruir();        break;
    case 7: opIngresarDicc();    break;
    case 8: opImprimirDicc();    break;
    case 9: opAgregarTextoDicc(); break;
    case 10: opCreditos();        break;
    default:
        if (confirmar("Salir", "Seguro que queres salir de Bitacora?"))
            return 1;
        aviso(0, "Seguimos a bordo.");
    }
    return 0;
}

/* ================== PUNTO DE ENTRADA ================== */

static void iniciarColores(void) {
    if (!has_colors())
        return;
    start_color();
    use_default_colors();
    init_pair(C_NUBE,    COLOR_WHITE,  -1);
    init_pair(C_LUNA,    COLOR_YELLOW, -1);
    init_pair(C_MAR,     COLOR_CYAN,   -1);
    init_pair(C_ESPUMA,  COLOR_WHITE,  -1);
    init_pair(C_CASCO,   COLOR_YELLOW, -1);
    init_pair(C_VELA,    COLOR_WHITE,  -1);
    init_pair(C_BANDERA, COLOR_RED,    -1);
    init_pair(C_OK,      COLOR_GREEN,  -1);
    init_pair(C_ERR,     COLOR_RED,    -1);
    init_pair(C_TIT,     COLOR_CYAN,   -1);
}

void iniciarUI(void) {
    int salir = 0, t;

    construirBarco();
    nave_x = NAVE_INI;

    setenv("ESCDELAY", "25", 1);
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(50);
    iniciarColores();
    aviso(0, "Bienvenido a bordo. Elige una opcion.");

    while (!salir) {
        t = esperar();
        if (g_fase == 0) {
            if (t != ERR && t != KEY_RESIZE) {
                g_fase = 1;
                g_trans = 0;
            }
            continue;
        }
        if (g_fase == 1)
            continue;
        switch (t) {
        case KEY_UP:   case 'w': case 'k': g_sel = (g_sel + N_OP - 1) % N_OP; break;
        case KEY_DOWN: case 's': case 'j': g_sel = (g_sel + 1) % N_OP;        break;
        case 'q': case 'Q': salir = activar(11); break;
        case '/': {
            int n = leerComando();
            if (n > 0) {
                g_sel = n - 1;
                salir = activar(g_sel);
            }
            break;
        }
        case '\n': case KEY_ENTER: salir = activar(g_sel); break;
        }
    }

    endwin();
}