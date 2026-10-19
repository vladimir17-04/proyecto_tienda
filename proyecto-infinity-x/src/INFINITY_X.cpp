#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <conio.h>
#include <windows.h>
#include <string>
using namespace std;

// ============================================================
//  CONSTANTES
// ============================================================
const int    MAX_PERSONAS    = 200;
const int    NUM_PROD        = 20;
const int    NUM_CAT         = 5;
const double CAPITAL_INICIAL = 44096.00;
const double FACTOR_COSTO    = 0.80;   // costo de abastecimiento = 80% del precio de venta
const double DESC_TARJETA    = 0.10;   // descuento por pago con tarjeta

const char* ARCH_CLIENTES = "clientes.dat";
const char* ARCH_TRABAJ   = "trabajadores.dat";
const char* ARCH_STOCK    = "inventario.dat";
const char* ARCH_CAJA     = "caja.dat";

const char* NOMBRE_CAT[NUM_CAT] = {"Computadoras", "Accesorios", "Componentes", "Dispositivos móviles", "Pantallas"};
const int   CAT_INI[NUM_CAT]    = {0, 4, 9, 13, 17};   // primer indice de producto por categoria
const int   CAT_FIN[NUM_CAT]    = {3, 8, 12, 16, 19};  // ultimo indice de producto por categoria
const char* NOMBRE_CARGO[5]     = {"Administrador", "Vendedor", "Cajero", "Almacenero", "Técnico de computadoras"};
const int   STOCK_INICIAL[NUM_PROD] = {4, 4, 4, 7,   12, 12, 8, 10, 15,   10, 20, 15, 6,   12, 5, 15, 10,   10, 8, 6};

// ============================================================
//  VARIABLES GLOBALES
// ============================================================
HANDLE hOut, hIn;
bool   sesionSecreta = false;   // true solo si se ingreso con POO / 20
double saldo = CAPITAL_INICIAL;
int    stock[NUM_PROD];

// ============================================================
//  PANTALLA: salida y entrada de texto (UTF-8 <-> consola)
// ============================================================
void gotoxy(int x, int y){
    COORD c; c.X = (SHORT)x; c.Y = (SHORT)y;
    SetConsoleCursorPosition(hOut, c);
}

void CursorOn(bool visible, DWORD size){
    if(size == 0) size = 20;
    CONSOLE_CURSOR_INFO ci; ci.bVisible = visible; ci.dwSize = size;
    SetConsoleCursorInfo(hOut, &ci);
}

void CursorOff(){
    CONSOLE_CURSOR_INFO ci; ci.bVisible = FALSE; ci.dwSize = 1;
    SetConsoleCursorInfo(hOut, &ci);
}

void imprimirV(const char* fmt, va_list ap){
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    wchar_t w[1024];
    int n = MultiByteToWideChar(CP_UTF8, 0, buf, -1, w, 1024);
    if(n > 1){ DWORD esc; WriteConsoleW(hOut, w, n - 1, &esc, NULL); }
}

void P(const char* fmt, ...){
    va_list ap; va_start(ap, fmt); imprimirV(fmt, ap); va_end(ap);
}

void GP(int x, int y, const char* fmt, ...){   // gotoxy + printf
    gotoxy(x, y);
    va_list ap; va_start(ap, fmt); imprimirV(fmt, ap); va_end(ap);
}

// ---------- utilidades de texto UTF-8 ----------
int lenU8(const string& s){
    int n = 0;
    for(size_t i = 0; i < s.size(); i++) if((s[i] & 0xC0) != 0x80) n++;
    return n;
}

string cortar(const string& s, int w){
    if(lenU8(s) <= w) return s;
    int n = 0; size_t i = 0;
    for(; i < s.size(); i++){
        if((s[i] & 0xC0) != 0x80){ if(n == w) break; n++; }
    }
    return s.substr(0, i);
}

string padR(const string& s, int w){          // rellena a la derecha
    string r = cortar(s, w);
    int l = lenU8(r);
    if(l < w) r.append(w - l, ' ');
    return r;
}

string padL(const string& s, int w){          // rellena a la izquierda (numeros)
    string r = cortar(s, w);
    int l = lenU8(r);
    if(l < w) r.insert(0, string(w - l, ' '));
    return r;
}

string numStr(int v){ char b[24]; sprintf(b, "%d", v); return string(b); }

double redondear(double x){ return floor(x * 100.0 + 0.5) / 100.0; }

string dinero(double v){                       // S/ 2,500.00
    if(v < 0) v = 0;
    long long c = (long long)(v * 100.0 + 0.5);
    string ent = "";
    char tmp[32]; sprintf(tmp, "%lld", c / 100);
    string s(tmp);
    int cnt = 0;
    for(int i = (int)s.size() - 1; i >= 0; i--){
        ent.insert(0, 1, s[i]);
        if(++cnt % 3 == 0 && i > 0) ent.insert(0, 1, ',');
    }
    char b[64]; sprintf(b, "S/ %s.%02d", ent.c_str(), (int)(c % 100));
    return string(b);
}

// ---------- dibujo ----------
void dibujarCuadro(int x1, int y1, int x2, int y2){
    for(int i = x1 + 1; i < x2; i++){ GP(i, y1, "─"); GP(i, y2, "─"); }
    for(int i = y1 + 1; i < y2; i++){ GP(x1, i, "│"); GP(x2, i, "│"); }
    GP(x1, y1, "┌"); GP(x2, y1, "┐"); GP(x1, y2, "└"); GP(x2, y2, "┘");
}

void limpiaLinea(int y){ GP(2, y, "%s", string(76, ' ').c_str()); }

void limpia(){ for(int y = 5; y <= 23; y++) limpiaLinea(y); }

void pantalla(const string& titulo){           // limpia el area y centra el titulo en la fila 6
    limpia();
    int x = 2 + (76 - lenU8(titulo)) / 2;
    GP(x, 6, "%s", titulo.c_str());
}

void cabecera(){
    system("mode con: cols=80 lines=25");
    system("COLOR 70");
    CursorOff();
    dibujarCuadro(1, 1, 78, 24);   // cuadro grande
    dibujarCuadro(2, 2, 77, 4);    // cuadro del titulo
    GP(30, 3, "I N F I N I T Y  X");
}

void aviso(const char* t){ limpiaLinea(23); GP(4, 23, "%s", t); }

void esperar(const char* msg){
    limpiaLinea(23); GP(4, 23, "%s", msg); CursorOff();
    int c = getch(); if(c == 0 || c == 224) getch();
    limpiaLinea(23);
}

void pausa(const string& pre){ esperar((pre + " Presione una tecla para volver..").c_str()); }

bool confirmarSN(const char* preg){
    limpiaLinea(22);
    GP(4, 22, "%s Sí [S] / No [N]: ", preg);
    CursorOff();
    int c;
    do{
        c = getch();
        if(c == 0 || c == 224){ getch(); c = 0; }
        c = tolower(c);
    }while(c != 's' && c != 'n');
    limpiaLinea(22);
    return c == 's';
}

// ---------- entrada ----------
void leerLinea(char* dest, int max){           // lee una linea de la consola en UTF-8
    wchar_t w[512]; DWORD n = 0;
    CursorOn(1, 10);
    if(!ReadConsoleW(hIn, w, 500, &n, NULL)) n = 0;
    CursorOff();
    bool completa = false;
    while(n > 0 && (w[n-1] == L'\r' || w[n-1] == L'\n')){ completa = true; n--; }
    if(!completa) FlushConsoleInputBuffer(hIn);
    char buf[2048];
    int m = WideCharToMultiByte(CP_UTF8, 0, w, (int)n, buf, sizeof(buf) - 1, NULL, NULL);
    if(m < 0) m = 0;
    buf[m] = 0;
    // recortar espacios
    int ini = 0, fin = m;
    while(ini < fin && buf[ini] == ' ') ini++;
    while(fin > ini && buf[fin-1] == ' ') fin--;
    int len = fin - ini;
    if(len > max - 1){                          // truncar sin cortar un caracter UTF-8 a la mitad
        len = max - 1;
        while(len > 0 && (buf[ini + len] & 0xC0) == 0x80) len--;
    }
    memcpy(dest, buf + ini, len);
    dest[len] = 0;
}

void leerPasw(char* d, int max){               // contraseña con asteriscos
    int i = 0;
    CursorOn(1, 10);
    for(;;){
        int c = getch();
        if(c == 13) break;
        if(c == 0 || c == 224){ getch(); continue; }
        if(c == 8){ if(i > 0){ i--; P("\b \b"); } }
        else if(c >= 32 && c < 127 && i < max - 1){ d[i++] = (char)c; P("*"); }
    }
    d[i] = 0;
    CursorOff();
}

bool soloDigitos(const char* s){
    if(!*s) return false;
    for(; *s; s++) if(!isdigit((unsigned char)*s)) return false;
    return true;
}

bool parseEntero(const char* s, int &v){
    int l = (int)strlen(s);
    if(l == 0 || l > 6 || !soloDigitos(s)) return false;
    v = atoi(s);
    return true;
}

bool parseDecimal(const char* s, double &v){
    int l = (int)strlen(s), puntos = 0, dig = 0;
    char t[32];
    if(l == 0 || l > 12) return false;
    for(int i = 0; i < l; i++){
        char ch = s[i];
        if(ch == ',') ch = '.';
        if(ch == '.'){ if(++puntos > 1) return false; }
        else if(isdigit((unsigned char)ch)) dig++;
        else return false;
        t[i] = ch;
    }
    t[l] = 0;
    if(!dig) return false;
    v = atof(t);
    return true;
}

bool validarCorreo(const char* s){
    const char* a = strchr(s, '@');
    if(!a || a == s || strchr(s, ' ')) return false;
    const char* p = strchr(a, '.');
    return p && p > a + 1 && p[1] != 0;
}

// Pide un entero en (x,y) con una etiqueta. Devuelve false si lo escrito no es entero valido.
bool pedirEntero(int x, int y, const char* etq, int &v){
    GP(x, y, "%s%s", etq, string(14, ' ').c_str());
    gotoxy(x + lenU8(etq), y);
    char b[32]; leerLinea(b, 32);
    return parseEntero(b, v);
}

bool pedirMonto(int x, int y, const char* etq, double &v){
    GP(x, y, "%s%s", etq, string(16, ' ').c_str());
    gotoxy(x + lenU8(etq), y);
    char b[32]; leerLinea(b, 32);
    return parseDecimal(b, v);
}

int pedirOpcion(int x, int y, int minOp, int maxOp, const char* etq = "Ingrese opción: "){
    for(;;){
        int v;
        if(pedirEntero(x, y, etq, v) && v >= minOp && v <= maxOp){ limpiaLinea(23); return v; }
        aviso("Error al ingresar valores. Presione una tecla para volver a ingresar..");
        CursorOff(); getch();
        limpiaLinea(23);
    }
}

int submenu(const char* titulo, const char* const* ops, int n, bool volverCero){
    pantalla(titulo);
    for(int i = 0; i < n; i++) GP(4, 8 + 2 * i, "%d. %s", i + 1, ops[i]);
    if(volverCero) GP(4, 8 + 2 * n, "0. Volver al menú principal");
    return pedirOpcion(4, 21, volverCero ? 0 : 1, n);
}

// ============================================================
//  PRODUCTOS (jerarquia de clases)
// ============================================================
void fila(int &y, const char* etq, const string& valor){
    GP(4, y, "%s", padR(etq, 22).c_str());
    GP(26, y, "%s", valor.c_str());
    y++;
}

#define BASE_P  int c, const char* n, const char* m, double p, const char* g, const char* t
#define BASE_A  c, n, m, p, g, t

class ProductoTecnologico{
public:
    int codigo; string nombre, marca, gama, tamano; double precio;
    ProductoTecnologico(BASE_P): codigo(c), nombre(n), marca(m), gama(g), tamano(t), precio(p){}
    virtual ~ProductoTecnologico(){}
    double costo() const { return redondear(precio * FACTOR_COSTO); }
    virtual void caracteristicas(int &y){
        fila(y, "Marca", marca); fila(y, "Gama", gama); fila(y, "Tamaño", tamano);
    }
};

class Computadora : public ProductoTecnologico{
protected: string procesador, almacenamiento, ram, uso;
public:
    Computadora(BASE_P, const char* pr, const char* al, const char* r, const char* u)
        : ProductoTecnologico(BASE_A), procesador(pr), almacenamiento(al), ram(r), uso(u){}
    virtual void caracteristicas(int &y){
        ProductoTecnologico::caracteristicas(y);
        fila(y, "Procesador", procesador); fila(y, "Almacenamiento", almacenamiento);
        fila(y, "Memoria RAM", ram); fila(y, "Uso", uso);
    }
};

class Laptop : public Computadora{
    string mousepad, camara, pantallaLap, teclado;
public:
    Laptop(BASE_P, const char* pr, const char* al, const char* r, const char* u,
           const char* mp, const char* cam, const char* pan, const char* tec)
        : Computadora(BASE_A, pr, al, r, u), mousepad(mp), camara(cam), pantallaLap(pan), teclado(tec){}
    virtual void caracteristicas(int &y){
        Computadora::caracteristicas(y);
        fila(y, "Mouse pad", mousepad); fila(y, "Cámara", camara);
        fila(y, "Pantalla", pantallaLap); fila(y, "Teclado", teclado);
    }
};

class PC : public Computadora{
    string torre;
public:
    PC(BASE_P, const char* pr, const char* al, const char* r, const char* u, const char* to)
        : Computadora(BASE_A, pr, al, r, u), torre(to){}
    virtual void caracteristicas(int &y){
        Computadora::caracteristicas(y);
        fila(y, "Torre", torre);
    }
};

class Pantalla : public ProductoTecnologico{
    string tipo, forma;
public:
    Pantalla(BASE_P, const char* ti, const char* fo): ProductoTecnologico(BASE_A), tipo(ti), forma(fo){}
    virtual void caracteristicas(int &y){
        ProductoTecnologico::caracteristicas(y);
        fila(y, "Tipo de pantalla", tipo); fila(y, "Forma", forma);
    }
};

class DispositivoEntrada : public ProductoTecnologico{   // base de Mouse y Teclado
protected: string rgb, conexion;
public:
    DispositivoEntrada(BASE_P, const char* r, const char* co): ProductoTecnologico(BASE_A), rgb(r), conexion(co){}
    virtual void caracteristicas(int &y){
        ProductoTecnologico::caracteristicas(y);
        fila(y, "RGB", rgb); fila(y, "Conexión", conexion);
    }
};
class Mouse   : public DispositivoEntrada{ public: Mouse(BASE_P, const char* r, const char* co): DispositivoEntrada(BASE_A, r, co){} };
class Teclado : public DispositivoEntrada{ public: Teclado(BASE_P, const char* r, const char* co): DispositivoEntrada(BASE_A, r, co){} };

class Audifonos : public ProductoTecnologico{
    string conexion;
public:
    Audifonos(BASE_P, const char* co): ProductoTecnologico(BASE_A), conexion(co){}
    virtual void caracteristicas(int &y){ ProductoTecnologico::caracteristicas(y); fila(y, "Conexión", conexion); }
};

class Reloj : public ProductoTecnologico{
    string tipo;
public:
    Reloj(BASE_P, const char* ti): ProductoTecnologico(BASE_A), tipo(ti){}
    virtual void caracteristicas(int &y){ ProductoTecnologico::caracteristicas(y); fila(y, "Tipo", tipo); }
};

class Webcam : public ProductoTecnologico{
    string hz, fps;
public:
    Webcam(BASE_P, const char* h, const char* f): ProductoTecnologico(BASE_A), hz(h), fps(f){}
    virtual void caracteristicas(int &y){
        ProductoTecnologico::caracteristicas(y);
        fila(y, "Frecuencia (Hz)", hz); fila(y, "FPS", fps);
    }
};

class Componente : public ProductoTecnologico{           // base de los componentes internos
protected: string compatibilidad;
public:
    Componente(BASE_P, const char* co): ProductoTecnologico(BASE_A), compatibilidad(co){}
    virtual void caracteristicas(int &y){ ProductoTecnologico::caracteristicas(y); fila(y, "Compatibilidad", compatibilidad); }
};

class Procesador : public Componente{
    string nucleos, frecuencia;
public:
    Procesador(BASE_P, const char* co, const char* nu, const char* fr): Componente(BASE_A, co), nucleos(nu), frecuencia(fr){}
    virtual void caracteristicas(int &y){
        Componente::caracteristicas(y);
        fila(y, "Núcleos / hilos", nucleos); fila(y, "Frecuencia", frecuencia);
    }
};

class MemoriaRAM : public Componente{
    string capacidad, tipo, velocidad;
public:
    MemoriaRAM(BASE_P, const char* co, const char* ca, const char* ti, const char* ve)
        : Componente(BASE_A, co), capacidad(ca), tipo(ti), velocidad(ve){}
    virtual void caracteristicas(int &y){
        Componente::caracteristicas(y);
        fila(y, "Capacidad", capacidad); fila(y, "Tipo", tipo); fila(y, "Velocidad", velocidad);
    }
};

class Ventilador : public Componente{
    string refrigeracion, velocidad;
public:
    Ventilador(BASE_P, const char* co, const char* re, const char* ve)
        : Componente(BASE_A, co), refrigeracion(re), velocidad(ve){}
    virtual void caracteristicas(int &y){
        Componente::caracteristicas(y);
        fila(y, "Refrigeración", refrigeracion); fila(y, "Velocidad", velocidad);
    }
};

class GPU : public Componente{
    string memoria, uso;
public:
    GPU(BASE_P, const char* co, const char* me, const char* u): Componente(BASE_A, co), memoria(me), uso(u){}
    virtual void caracteristicas(int &y){
        Componente::caracteristicas(y);
        fila(y, "Memoria de video", memoria); fila(y, "Uso", uso);
    }
};

class DispositivoMovil : public ProductoTecnologico{     // base de Celular y Tablet
protected: string sistema, almacenamiento, ram, bateria;
public:
    DispositivoMovil(BASE_P, const char* si, const char* al, const char* r, const char* ba)
        : ProductoTecnologico(BASE_A), sistema(si), almacenamiento(al), ram(r), bateria(ba){}
    virtual void caracteristicas(int &y){
        ProductoTecnologico::caracteristicas(y);
        fila(y, "Sistema operativo", sistema); fila(y, "Almacenamiento", almacenamiento);
        fila(y, "Memoria RAM", ram); fila(y, "Batería", bateria);
    }
};

class Celular : public DispositivoMovil{
    string camara;
public:
    Celular(BASE_P, const char* si, const char* al, const char* r, const char* ba, const char* cam)
        : DispositivoMovil(BASE_A, si, al, r, ba), camara(cam){}
    virtual void caracteristicas(int &y){ DispositivoMovil::caracteristicas(y); fila(y, "Cámara principal", camara); }
};

class Tablet : public DispositivoMovil{
    string conectividad;
public:
    Tablet(BASE_P, const char* si, const char* al, const char* r, const char* ba, const char* co)
        : DispositivoMovil(BASE_A, si, al, r, ba), conectividad(co){}
    virtual void caracteristicas(int &y){ DispositivoMovil::caracteristicas(y); fila(y, "Conectividad", conectividad); }
};

ProductoTecnologico* catalogo[NUM_PROD];

void crearCatalogo(){
    // ---- COMPUTADORAS ----
    catalogo[0]  = new Laptop(1, "Lenovo IdeaPad Slim 3", "Lenovo", 2500.00, "Media", "15.6 pulgadas",
                              "Intel Core i5", "SSD 512 GB", "8 GB DDR4", "Estudio y oficina",
                              "Touchpad multitáctil", "HD 720p", "15.6\" Full HD", "Español, con teclado numérico");
    catalogo[1]  = new Laptop(2, "HP Pavilion 15", "HP", 3200.00, "Media-alta", "15.6 pulgadas",
                              "Intel Core i5", "SSD 512 GB", "16 GB DDR4", "Productividad y multimedia",
                              "Touchpad multitáctil", "HD 720p", "15.6\" Full HD IPS", "Español, retroiluminado");
    catalogo[2]  = new PC(3, "PC Gamer Ryzen 7", "Ensamblada", 4500.00, "Alta", "Torre media",
                          "AMD Ryzen 7", "SSD NVMe 1 TB", "16 GB DDR4", "Videojuegos",
                          "ATX con panel de vidrio templado");
    catalogo[3]  = new PC(4, "PC Intel Core i5", "Ensamblada", 2800.00, "Media", "Mini torre",
                          "Intel Core i5", "SSD 512 GB", "8 GB DDR4", "Oficina y estudio",
                          "Micro ATX compacta");
    // ---- ACCESORIOS ----
    catalogo[4]  = new Mouse(5, "Mouse Logitech G203", "Logitech", 90.00, "Media", "Estándar", "Sí", "Cable USB");
    catalogo[5]  = new Audifonos(6, "Audífonos HyperX Cloud II", "HyperX", 350.00, "Alta", "Over-ear", "Cable USB / jack 3.5 mm");
    catalogo[6]  = new Reloj(7, "Reloj Samsung Galaxy Watch", "Samsung", 850.00, "Alta", "Caja de 44 mm", "Smartwatch");
    catalogo[7]  = new Webcam(8, "Webcam Logitech C920", "Logitech", 300.00, "Media", "Compacta", "60 Hz", "30 FPS (1080p)");
    catalogo[8]  = new Teclado(9, "Teclado Redragon Kumara", "Redragon", 180.00, "Media", "TKL (sin teclado numérico)", "Sí", "Cable USB");
    // ---- COMPONENTES ----
    catalogo[9]  = new Procesador(10, "Procesador Intel Core i5", "Intel", 850.00, "Media", "Socket LGA 1700",
                                  "Placas madre LGA 1700", "6 núcleos / 12 hilos", "Hasta 4.4 GHz");
    catalogo[10] = new MemoriaRAM(11, "Memoria RAM Kingston 16 GB", "Kingston", 180.00, "Media", "Módulo DIMM",
                                  "PC de escritorio", "16 GB", "DDR4", "3200 MHz");
    catalogo[11] = new Ventilador(12, "Ventilador Cooler Master", "Cooler Master", 120.00, "Básica", "120 mm",
                                  "Gabinetes y disipadores de 120 mm", "Por aire", "Hasta 1800 RPM");
    catalogo[12] = new GPU(13, "GPU NVIDIA RTX 4060", "NVIDIA", 1600.00, "Alta", "Doble ventilador",
                           "Ranura PCIe", "8 GB GDDR6", "Videojuegos y diseño");
    // ---- DISPOSITIVOS MOVILES ----
    catalogo[13] = new Celular(14, "Samsung Galaxy A55", "Samsung", 1400.00, "Media-alta", "6.6 pulgadas",
                               "Android", "128 GB", "8 GB", "5000 mAh", "50 MP");
    catalogo[14] = new Celular(15, "iPhone 15", "Apple", 3500.00, "Alta", "6.1 pulgadas",
                               "iOS", "128 GB", "6 GB", "3349 mAh", "48 MP");
    catalogo[15] = new Celular(16, "Xiaomi Redmi Note 13", "Xiaomi", 850.00, "Media", "6.67 pulgadas",
                               "Android", "128 GB", "8 GB", "5000 mAh", "108 MP");
    catalogo[16] = new Tablet(17, "Tablet Samsung Galaxy Tab A9", "Samsung", 750.00, "Media", "8.7 pulgadas",
                              "Android", "64 GB", "4 GB", "5100 mAh", "Wi-Fi");
    // ---- PANTALLAS ----
    catalogo[17] = new Pantalla(18, "Monitor LG 24 pulgadas", "LG", 650.00, "Media", "24 pulgadas", "IPS Full HD", "Plana");
    catalogo[18] = new Pantalla(19, "Monitor Samsung Curvo 27\"", "Samsung", 950.00, "Media-alta", "27 pulgadas", "VA Full HD", "Curva");
    catalogo[19] = new Pantalla(20, "Monitor ASUS TUF 165 Hz", "ASUS", 1200.00, "Alta", "27 pulgadas", "IPS 165 Hz", "Plana");
}

void liberarCatalogo(){ for(int i = 0; i < NUM_PROD; i++) delete catalogo[i]; }

// ============================================================
//  PERSONAS: Persona -> Cliente / Trabajador
// ============================================================
int buscarDNI(int tipo, const char* dni);   // tipo 0 = cliente, 1 = trabajador

int filaCampo(int k){ return 8 + 2 * (k - 1); }
void limpiaCampo(int y){ GP(26, y, "%s", string(49, ' ').c_str()); }

const char* ETQ_PERSONA[6] = {"", "Nombres y apellidos:", "DNI:", "Edad:", "Número de celular:", "Correo electrónico:"};

class Persona{
public:
    char nombre[50]; char dni[15]; int edad; char celular[15]; char correo[50];

    bool pedirComun(int k, int tipo, int idxPropio){      // campos 1..5, comunes a clientes y trabajadores
        char b[64]; int v; int y = filaCampo(k);
        for(;;){
            limpiaCampo(y); gotoxy(26, y);
            switch(k){
            case 1:
                leerLinea(b, 50);
                if(!b[0]){ aviso("Este campo no puede quedar vacío."); continue; }
                strcpy(nombre, b); break;
            case 2:
                leerLinea(b, 15);
                if(!soloDigitos(b)){ aviso("DNI no válido. Ingrese nuevamente."); continue; }
                { int j = buscarDNI(tipo, b);
                  if(j >= 0 && j != idxPropio){ aviso("DNI ya registrado. Ingrese nuevamente."); continue; } }
                strcpy(dni, b); break;
            case 3:
                leerLinea(b, 16);
                if(!parseEntero(b, v) || v < 1 || v > 120){ aviso("Edad no válida. Ingrese nuevamente."); continue; }
                edad = v; break;
            case 4:
                leerLinea(b, 15);
                if(!soloDigitos(b)){ aviso("Número de celular no válido. Ingrese nuevamente."); continue; }
                strcpy(celular, b); break;
            case 5:
                leerLinea(b, 50);
                if(!b[0]){ aviso("Este campo no puede quedar vacío."); continue; }
                strcpy(correo, b); break;
            default: return false;
            }
            limpiaLinea(23);
            return true;
        }
    }
    string valorComun(int k) const{
        switch(k){
        case 1: return nombre;
        case 2: return dni;
        case 3: return numStr(edad);
        case 4: return celular;
        case 5: return correo;
        }
        return "";
    }
};

class Cliente : public Persona{
public:
    char direccion[70];
    static const int NCAMPOS = 6;

    const char* etq(int k) const { return k <= 5 ? ETQ_PERSONA[k] : "Dirección:"; }
    string valor(int k) const { return k <= 5 ? valorComun(k) : string(direccion); }
    void etiqueta(int k){ GP(4, filaCampo(k), "%s", padR(etq(k), 22).c_str()); }
    void pedirCampo(int k, int idxPropio){
        if(k <= 5){ pedirComun(k, 0, idxPropio); return; }
        for(;;){
            limpiaCampo(filaCampo(k)); gotoxy(26, filaCampo(k));
            leerLinea(direccion, 70);
            if(direccion[0]){ limpiaLinea(23); return; }
            aviso("Este campo no puede quedar vacío.");
        }
    }
    void mostrarFicha(){
        for(int k = 1; k <= NCAMPOS; k++){ etiqueta(k); GP(26, filaCampo(k), "%s", valor(k).c_str()); }
    }
    void listar(int y){
        GP(4, y, "%s", padR(nombre, 27).c_str());
        GP(32, y, "%s", padR(dni, 9).c_str());
        GP(42, y, "%s", padR(numStr(edad), 5).c_str());
        GP(48, y, "%s", celular);
        GP(6, y + 1, "%s", padR(correo, 31).c_str());
        GP(38, y + 1, "%s", padR(direccion, 37).c_str());
    }
    // operaciones del submenu (definidas mas abajo)
    void registrar(); void consultar(); void actualizar(); void eliminar(); void mostrarTodos();
};

class Trabajador : public Persona{
public:
    int cargo;           // 1..5
    double sueldo;
    static const int NCAMPOS = 7;

    const char* etq(int k) const { return k <= 5 ? ETQ_PERSONA[k] : (k == 6 ? "Cargo:" : "Sueldo:"); }
    string valor(int k) const{
        if(k <= 5) return valorComun(k);
        if(k == 6) return NOMBRE_CARGO[cargo - 1];
        return dinero(sueldo);
    }
    void etiqueta(int k){ GP(4, filaCampo(k), "%s", padR(etq(k), 22).c_str()); }
    void pedirCampo(int k, int idxPropio){
        if(k <= 5){ pedirComun(k, 1, idxPropio); return; }
        int y = filaCampo(k); int v; double d; char b[32];
        if(k == 6){
            for(int i = 0; i < 5; i++) GP(46, 9 + i, "%d. %s", i + 1, NOMBRE_CARGO[i]);
            for(;;){
                limpiaCampo(y); gotoxy(26, y);
                leerLinea(b, 8);
                if(parseEntero(b, v) && v >= 1 && v <= 5){ cargo = v; break; }
                aviso("Cargo no válido. Elija una opción de la lista.");
            }
            for(int i = 0; i < 5; i++) GP(46, 9 + i, "%s", string(30, ' ').c_str());
        }else{
            for(;;){
                limpiaCampo(y); gotoxy(26, y);
                leerLinea(b, 16);
                if(parseDecimal(b, d) && d > 0 && d <= 999999.99){ sueldo = redondear(d); break; }
                aviso("Sueldo no válido. Ingrese nuevamente.");
            }
        }
        limpiaLinea(23);
    }
    void mostrarFicha(){
        for(int k = 1; k <= NCAMPOS; k++){ etiqueta(k); GP(26, filaCampo(k), "%s", valor(k).c_str()); }
    }
    void listar(int y){
        GP(4, y, "%s", padR(nombre, 27).c_str());
        GP(32, y, "%s", padR(dni, 9).c_str());
        GP(42, y, "%s", padR(numStr(edad), 5).c_str());
        GP(48, y, "%s", celular);
        GP(6, y + 1, "%s", padR(correo, 31).c_str());
        GP(38, y + 1, "%s", padR(NOMBRE_CARGO[cargo - 1], 21).c_str());
        GP(60, y + 1, "%s", dinero(sueldo).c_str());
    }
    void registrar(); void consultar(); void actualizar(); void eliminar(); void mostrarTodos();
};

Cliente    listaCli[MAX_PERSONAS];   int nCli  = 0;
Trabajador listaTrab[MAX_PERSONAS];  int nTrab = 0;

int buscarDNI(int tipo, const char* dni){
    if(tipo == 0){ for(int i = 0; i < nCli;  i++) if(strcmp(listaCli[i].dni,  dni) == 0) return i; }
    else         { for(int i = 0; i < nTrab; i++) if(strcmp(listaTrab[i].dni, dni) == 0) return i; }
    return -1;
}

// ============================================================
//  ARCHIVOS
// ============================================================
template<class T> int cargarLista(const char* arch, T* lista, int max){
    FILE* f = fopen(arch, "rb");
    if(!f) return 0;
    int n = 0;
    while(n < max && fread(&lista[n], sizeof(T), 1, f) == 1) n++;
    fclose(f);
    return n;
}

template<class T> bool guardarLista(const char* arch, T* lista, int n){
    FILE* f = fopen(arch, "wb");
    if(!f) return false;
    if(n > 0) fwrite(lista, sizeof(T), n, f);
    fclose(f);
    return true;
}

bool guardarStock(){
    FILE* f = fopen(ARCH_STOCK, "wb");
    if(!f) return false;
    fwrite(stock, sizeof(int), NUM_PROD, f);
    fclose(f);
    return true;
}

bool guardarCaja(){
    FILE* f = fopen(ARCH_CAJA, "wb");
    if(!f) return false;
    fwrite(&saldo, sizeof(double), 1, f);
    fclose(f);
    return true;
}

bool existeArchivo(const char* arch){
    FILE* f = fopen(arch, "rb");
    if(!f) return false;
    fclose(f);
    return true;
}

void sembrarCliente(const char* nom, const char* dni, int edad, const char* cel, const char* correo, const char* dir){
    Cliente c; memset(&c, 0, sizeof(c));
    strcpy(c.nombre, nom); strcpy(c.dni, dni); c.edad = edad;
    strcpy(c.celular, cel); strcpy(c.correo, correo); strcpy(c.direccion, dir);
    listaCli[nCli++] = c;
}

void sembrarTrabajador(const char* nom, const char* dni, int edad, const char* cel, const char* correo, int cargo, double sueldo){
    Trabajador t; memset(&t, 0, sizeof(t));
    strcpy(t.nombre, nom); strcpy(t.dni, dni); t.edad = edad;
    strcpy(t.celular, cel); strcpy(t.correo, correo); t.cargo = cargo; t.sueldo = sueldo;
    listaTrab[nTrab++] = t;
}

void cargarDatos(){
    // Si el archivo no existe (primera ejecucion) se crea con los registros precargados.
    if(existeArchivo(ARCH_CLIENTES)) nCli = cargarLista(ARCH_CLIENTES, listaCli, MAX_PERSONAS);
    else{
        sembrarCliente("Rosa Elena Quispe Condori", "45213876", 28, "951203874", "rosaquispe28@gmail.com", "Av. El Sol 245, Puno");
        sembrarCliente("Carlos Alberto Huanca Apaza", "70384521", 35, "962417589", "carloshuanca35@gmail.com", "Jr. Lima 512, Puno");
        guardarLista(ARCH_CLIENTES, listaCli, nCli);
    }
    if(existeArchivo(ARCH_TRABAJ)) nTrab = cargarLista(ARCH_TRABAJ, listaTrab, MAX_PERSONAS);
    else{
        sembrarTrabajador("Joel Sander Pele Mamani Llanos", "72981678", 17, "951217748", "joelmamani056@gmail.com", 1, 4500.00);
        sembrarTrabajador("Vladimir Josec Ticona", "71518681", 22, "948995745", "vladimirticona22@gmail.com", 1, 4500.00);
        sembrarTrabajador("Paulo Andre Velarde Excelmes", "60770467", 17, "970367349", "paulovelarde17@gmail.com", 3, 850.00);
        guardarLista(ARCH_TRABAJ, listaTrab, nTrab);
    }

    bool ok = false;
    FILE* f = fopen(ARCH_STOCK, "rb");
    if(f){
        ok = (fread(stock, sizeof(int), NUM_PROD, f) == (size_t)NUM_PROD);
        fclose(f);
        for(int i = 0; i < NUM_PROD && ok; i++) if(stock[i] < 0) ok = false;
    }
    if(!ok){ for(int i = 0; i < NUM_PROD; i++) stock[i] = STOCK_INICIAL[i]; guardarStock(); }

    ok = false;
    f = fopen(ARCH_CAJA, "rb");
    if(f){
        ok = (fread(&saldo, sizeof(double), 1, f) == 1 && saldo >= 0 && saldo < 1e12);
        fclose(f);
    }
    if(!ok){ saldo = CAPITAL_INICIAL; guardarCaja(); }
}

// ============================================================
//  OPERACIONES GENERICAS (clientes y trabajadores)
// ============================================================
template<class T> void tRegistrar(T* lista, int &n, const char* arch, const char* titulo,
                                  const char* preg, const char* ok, const char* no){
    pantalla(titulo);
    if(n >= MAX_PERSONAS){ pausa("Se alcanzó el límite de registros."); return; }
    T nuevo; memset(&nuevo, 0, sizeof(T));
    for(int k = 1; k <= T::NCAMPOS; k++){ nuevo.etiqueta(k); nuevo.pedirCampo(k, -1); }
    if(confirmarSN(preg)){
        lista[n] = nuevo; n++;
        if(guardarLista(arch, lista, n)) pausa(ok); else pausa("Error al guardar el archivo.");
    }else pausa(no);
}

int pedirYBuscarDNI(int tipo, const char* titulo, const char* etq){
    char b[32];
    pantalla(titulo);
    GP(4, 8, "%s", etq);
    gotoxy(4 + lenU8(etq), 8);
    leerLinea(b, 15);
    return buscarDNI(tipo, b);
}

template<class T> void tConsultar(T* lista, int tipo, const char* titulo, const char* etq, const char* noEnc){
    int idx = pedirYBuscarDNI(tipo, titulo, etq);
    if(idx < 0){ pausa(noEnc); return; }
    pantalla(titulo);
    lista[idx].mostrarFicha();
    esperar("Presione una tecla para volver..");
}

template<class T> void tActualizar(T* lista, int tipo, const char* arch, const char* titulo,
                                   const char* etq, const char* noEnc, const char* ok){
    int idx = pedirYBuscarDNI(tipo, titulo, etq);
    if(idx < 0){ pausa(noEnc); return; }
    T t = lista[idx];
    for(;;){
        pantalla(titulo);
        t.mostrarFicha();
        int k = pedirOpcion(4, 22, 0, T::NCAMPOS, "Campo a modificar (0 = terminar): ");
        if(k == 0) break;
        limpiaLinea(22);
        t.etiqueta(k);
        t.pedirCampo(k, idx);
    }
    if(confirmarSN("¿Guardar los cambios?")){
        lista[idx] = t;
        if(guardarLista(arch, lista, (tipo == 0 ? nCli : nTrab))) pausa(ok); else pausa("Error al guardar el archivo.");
    }else pausa("Cambios descartados.");
}

template<class T> void tEliminar(T* lista, int &n, int tipo, const char* arch, const char* titulo,
                                 const char* etq, const char* noEnc, const char* preg, const char* ok){
    int idx = pedirYBuscarDNI(tipo, titulo, etq);
    if(idx < 0){ pausa(noEnc); return; }
    pantalla(titulo);
    lista[idx].mostrarFicha();
    if(confirmarSN(preg)){
        for(int i = idx; i < n - 1; i++) lista[i] = lista[i + 1];
        n--;
        if(guardarLista(arch, lista, n)) pausa(ok); else pausa("Error al guardar el archivo.");
    }else pausa("Operación cancelada.");
}

template<class T> void tMostrarTodos(T* lista, int n, const char* titulo, const string& cab1, const string& cab2){
    const int POR_PAG = 5;
    if(n == 0){ pantalla(titulo); pausa("No hay registros."); return; }
    int pag = 0, paginas = (n + POR_PAG - 1) / POR_PAG;
    for(;;){
        pantalla(titulo);
        GP(4, 8, "%s", cab1.c_str());
        GP(6, 9, "%s", cab2.c_str());
        GP(4, 10, "%s", string(72, '-').c_str());
        for(int i = 0; i < POR_PAG; i++){
            int j = pag * POR_PAG + i;
            if(j >= n) break;
            lista[j].listar(11 + 2 * i);
        }
        GP(4, 22, "[N] Siguiente  [A] Anterior  [otra tecla] Volver     Pág. %d/%d  (%d registros)", pag + 1, paginas, n);
        CursorOff();
        int c = getch();
        if(c == 0 || c == 224){ c = getch(); c = (c == 77) ? 'n' : (c == 75 ? 'a' : 0); }
        c = tolower(c);
        if(c == 'n' && pag < paginas - 1) pag++;
        else if(c == 'a' && pag > 0) pag--;
        else if(c != 'n' && c != 'a') return;
    }
}

// ---------- Cliente ----------
void Cliente::registrar(){
    tRegistrar(listaCli, nCli, ARCH_CLIENTES, "REGISTRO DE CLIENTES",
               "¿Registrar nuevo cliente?", "Cliente registrado.", "Cliente NO registrado.");
}
void Cliente::consultar(){
    tConsultar(listaCli, 0, "CONSULTAR CLIENTE", "Ingrese el DNI del cliente: ", "Cliente no encontrado.");
}
void Cliente::actualizar(){
    tActualizar(listaCli, 0, ARCH_CLIENTES, "ACTUALIZAR DATOS DEL CLIENTE", "Ingrese el DNI del cliente: ",
                "Cliente no encontrado.", "Datos del cliente actualizados.");
}
void Cliente::eliminar(){
    tEliminar(listaCli, nCli, 0, ARCH_CLIENTES, "ELIMINAR CLIENTE", "Ingrese el DNI del cliente: ",
              "Cliente no encontrado.", "¿Eliminar este cliente?", "Cliente eliminado.");
}
void Cliente::mostrarTodos(){
    tMostrarTodos(listaCli, nCli, "CLIENTES REGISTRADOS",
                  padR("Nombres y apellidos", 28) + padR("DNI", 10) + padR("Edad", 6) + "Celular",
                  padR("Correo electrónico", 32) + "Dirección");
}

// ---------- Trabajador ----------
void Trabajador::registrar(){
    tRegistrar(listaTrab, nTrab, ARCH_TRABAJ, "REGISTRO DE TRABAJADORES",
               "¿Registrar nuevo trabajador?", "Trabajador registrado.", "Trabajador NO registrado.");
}
void Trabajador::consultar(){
    tConsultar(listaTrab, 1, "CONSULTAR TRABAJADOR", "Ingrese el DNI del trabajador: ", "Trabajador no encontrado.");
}
void Trabajador::actualizar(){
    tActualizar(listaTrab, 1, ARCH_TRABAJ, "ACTUALIZAR DATOS DEL TRABAJADOR", "Ingrese el DNI del trabajador: ",
                "Trabajador no encontrado.", "Datos del trabajador actualizados.");
}
void Trabajador::eliminar(){
    tEliminar(listaTrab, nTrab, 1, ARCH_TRABAJ, "ELIMINAR TRABAJADOR", "Ingrese el DNI del trabajador: ",
              "Trabajador no encontrado.", "¿Eliminar este trabajador?", "Trabajador eliminado.");
}
void Trabajador::mostrarTodos(){
    tMostrarTodos(listaTrab, nTrab, "TRABAJADORES REGISTRADOS",
                  padR("Nombres y apellidos", 28) + padR("DNI", 10) + padR("Edad", 6) + "Celular",
                  padR("Correo electrónico", 32) + padR("Cargo", 22) + "Sueldo");
}

// ---------- submenus ----------
void menuClientes(){
    static const char* OPS[5] = {"Registrar cliente", "Consultar cliente", "Actualizar datos del cliente",
                                 "Eliminar cliente", "Mostrar todos los clientes"};
    Cliente cl;
    for(;;){
        switch(submenu("CLIENTES", OPS, 5, true)){
        case 1: cl.registrar();   break;
        case 2: cl.consultar();   break;
        case 3: cl.actualizar();  break;
        case 4: cl.eliminar();    break;
        case 5: cl.mostrarTodos();break;
        case 0: return;
        }
    }
}

void menuPersonal(){
    static const char* OPS[5] = {"Registrar trabajador", "Consultar trabajador", "Actualizar datos",
                                 "Eliminar trabajador", "Mostrar todos los trabajadores"};
    Trabajador tr;
    for(;;){
        switch(submenu("PERSONAL ADMINISTRATIVO", OPS, 5, true)){
        case 1: tr.registrar();   break;
        case 2: tr.consultar();   break;
        case 3: tr.actualizar();  break;
        case 4: tr.eliminar();    break;
        case 5: tr.mostrarTodos();break;
        case 0: return;
        }
    }
}

// ============================================================
//  CATALOGO (compartido por inventario, abastecimiento y pedidos)
// ============================================================
void listarCategoria(int cat, bool conCosto){
    GP(4, 8, "Cód.");
    GP(9, 8, "Producto");
    GP(40, 8, "%s", padL("P. venta", 11).c_str());
    if(conCosto){
        GP(53, 8, "%s", padL("Costo", 11).c_str());
        GP(66, 8, "Stock");
    }else GP(54, 8, "Stock");
    GP(4, 9, "%s", string(72, '-').c_str());
    for(int i = CAT_INI[cat], y = 10; i <= CAT_FIN[cat]; i++, y++){
        ProductoTecnologico* p = catalogo[i];
        char cod[8]; sprintf(cod, "%03d", p->codigo);
        string st = stock[i] > 0 ? numStr(stock[i]) : string("Agotado");
        GP(4, y, "%s", cod);
        GP(9, y, "%s", padR(p->nombre, 30).c_str());
        GP(40, y, "%s", padL(dinero(p->precio), 11).c_str());
        if(conCosto){
            GP(53, y, "%s", padL(dinero(p->costo()), 11).c_str());
            GP(66, y, "%s", st.c_str());
        }else GP(54, y, "%s", st.c_str());
    }
}

// Muestra categorias y productos. Devuelve el indice del producto elegido, o -1 si el usuario vuelve atras.
int elegirProducto(const string& titulo, bool conCosto){
    for(;;){
        pantalla(titulo);
        for(int i = 0; i < NUM_CAT; i++) GP(4, 8 + 2 * i, "%d. %s", i + 1, NOMBRE_CAT[i]);
        GP(4, 18, "0. Volver");
        int c = pedirOpcion(4, 21, 0, NUM_CAT);
        if(c == 0) return -1;
        c--;
        for(;;){
            pantalla(titulo + " - " + NOMBRE_CAT[c]);
            listarCategoria(c, conCosto);
            int cod;
            if(pedirEntero(4, 21, "Código del producto (0 = volver): ", cod)){
                if(cod == 0) break;
                if(cod - 1 >= CAT_INI[c] && cod - 1 <= CAT_FIN[c]){ limpiaLinea(23); return cod - 1; }
            }
            aviso("Código no válido. Elija un producto de la lista.");
        }
    }
}

void mostrarDetalle(int idx){
    ProductoTecnologico* p = catalogo[idx];
    pantalla(p->nombre);
    int y = 8; char cod[8]; sprintf(cod, "%03d", p->codigo);
    fila(y, "Código", cod);
    fila(y, "Precio de venta", dinero(p->precio));
    fila(y, "Disponibilidad", stock[idx] > 0 ? numStr(stock[idx]) + " unidades" : string("Agotado"));
    y++;
    p->caracteristicas(y);
    esperar("Presione una tecla para volver..");
}

void menuInventario(){
    static const char* OPS[1] = {"Mostrar productos"};
    for(;;){
        if(submenu("INVENTARIO", OPS, 1, true) == 0) return;
        for(;;){
            int idx = elegirProducto("INVENTARIO", false);
            if(idx < 0) break;
            mostrarDetalle(idx);
        }
    }
}

// ============================================================
//  CARRITOS (abastecimiento y pedidos)
// ============================================================
double precioUnit(int i, bool costo){ return costo ? catalogo[i]->costo() : catalogo[i]->precio; }

double totalCarrito(const int* cant, bool costo){
    double t = 0;
    for(int i = 0; i < NUM_PROD; i++) if(cant[i] > 0) t += precioUnit(i, costo) * cant[i];
    return redondear(t);
}

int unidadesCarrito(const int* cant){
    int u = 0; for(int i = 0; i < NUM_PROD; i++) u += cant[i];
    return u;
}

// Tabla del carrito (filas 8..19). Devuelve el total.
double mostrarResumen(const int* cant, bool costo){
    GP(4, 8, "Producto");
    GP(35, 8, "%s", padL("Cant.", 5).c_str());
    GP(41, 8, "%s", padL(costo ? "Costo unit." : "P. unit.", 12).c_str());
    GP(55, 8, "%s", padL("Subtotal", 13).c_str());
    GP(4, 9, "%s", string(72, '-').c_str());
    int y = 10, mostrados = 0, total = 0;
    for(int i = 0; i < NUM_PROD; i++){
        if(cant[i] <= 0) continue;
        total++;
        if(mostrados >= 9) continue;
        GP(4, y, "%s", padR(catalogo[i]->nombre, 30).c_str());
        GP(35, y, "%s", padL(numStr(cant[i]), 5).c_str());
        GP(41, y, "%s", padL(dinero(precioUnit(i, costo)), 12).c_str());
        GP(55, y, "%s", padL(dinero(redondear(precioUnit(i, costo) * cant[i])), 13).c_str());
        y++; mostrados++;
    }
    if(total > mostrados) GP(4, 19, "... y %d producto(s) más", total - mostrados);
    return totalCarrito(cant, costo);
}

// ============================================================
//  3. ABASTECIMIENTO DE PRODUCTOS
// ============================================================
void solicitarProductos(){
    char dni[32];
    pantalla("ABASTECIMIENTO - SOLICITAR PRODUCTOS");
    GP(4, 8, "DNI del trabajador: ");
    gotoxy(24, 8); leerLinea(dni, 15);
    int t = buscarDNI(1, dni);
    if(t < 0){ pausa("Trabajador no registrado. No se puede solicitar productos."); return; }

    pantalla("ABASTECIMIENTO - SOLICITAR PRODUCTOS");
    GP(4, 8, "Trabajador: %s", listaTrab[t].nombre);
    GP(4, 10, "Cargo:      %s", NOMBRE_CARGO[listaTrab[t].cargo - 1]);
    esperar("Presione una tecla para continuar..");

    int sol[NUM_PROD]; memset(sol, 0, sizeof(sol));
    for(;;){
        int idx = elegirProducto("ABASTECIMIENTO", true);
        if(idx < 0) break;
        pantalla("ABASTECIMIENTO - SOLICITAR PRODUCTOS");
        int y = 8;
        fila(y, "Producto", catalogo[idx]->nombre);
        fila(y, "Precio de venta", dinero(catalogo[idx]->precio));
        fila(y, "Costo de abastecimiento", dinero(catalogo[idx]->costo()));
        fila(y, "Stock actual", numStr(stock[idx]));
        if(sol[idx] > 0) fila(y, "Ya solicitado", numStr(sol[idx]));
        int q;
        for(;;){
            if(pedirEntero(4, 16, "Cantidad a solicitar: ", q) && q > 0){ limpiaLinea(23); break; }
            aviso("Cantidad no válida. Ingrese un número mayor que cero.");
        }
        sol[idx] += q;
        GP(4, 18, "Total solicitado de este producto: %d  (subtotal %s)", sol[idx],
           dinero(redondear(catalogo[idx]->costo() * sol[idx])).c_str());
        if(!confirmarSN("¿Desea seguir solicitando?")) break;
    }
    if(unidadesCarrito(sol) == 0){ pantalla("ABASTECIMIENTO"); pausa("No se solicitaron productos."); return; }

    pantalla("ABASTECIMIENTO - RESUMEN");
    double total = mostrarResumen(sol, true);
    GP(4, 20, "Costo total:                  %s", dinero(total).c_str());
    GP(4, 21, "Saldo del establecimiento:    %s", dinero(saldo).c_str());
    if(total > saldo){ pausa("Fondos insuficientes. No se realizó el abastecimiento."); return; }
    if(!confirmarSN("¿Confirmar abastecimiento?")){ pausa("Abastecimiento cancelado."); return; }

    double saldoAnterior = saldo;
    saldo = redondear(saldo - total);
    for(int i = 0; i < NUM_PROD; i++) stock[i] += sol[i];
    bool ok = guardarStock() & guardarCaja();

    pantalla("ABASTECIMIENTO CONFIRMADO");
    GP(4, 9,  "Unidades ingresadas al inventario: %d", unidadesCarrito(sol));
    GP(4, 11, "Costo total:     %s", dinero(total).c_str());
    GP(4, 12, "Saldo anterior:  %s", dinero(saldoAnterior).c_str());
    GP(4, 13, "Saldo actual:    %s", dinero(saldo).c_str());
    GP(4, 15, "El stock se actualizó de inmediato.");
    pausa(ok ? "Cambios guardados." : "ATENCIÓN: no se pudieron guardar los archivos.");
}

void menuAbastecimiento(){
    static const char* OPS[2] = {"Solicitar productos", "Volver"};
    for(;;){
        if(submenu("ABASTECIMIENTO DE PRODUCTOS", OPS, 2, false) == 2) return;
        solicitarProductos();
    }
}

// ============================================================
//  5. PEDIDOS (VENTAS A CLIENTES)
// ============================================================
void mostrarRecibo(const char* dni, const char* nombre, double total, double desc, double pagado,
                   bool tarjeta, double entregado, double vuelto, int unidades){
    pantalla("RECIBO - INFINITY X");
    GP(4, 8,  "DNI del cliente:     %s", dni);
    GP(4, 9,  "Cliente:             %s", nombre);
    GP(4, 10, "Unidades compradas:  %d", unidades);
    GP(4, 12, "Valor total de la compra:   %s", dinero(total).c_str());
    if(tarjeta) GP(4, 13, "Descuento (10%%):            %s", dinero(desc).c_str());
    else        GP(4, 13, "Descuento:                  %s", dinero(0).c_str());
    GP(4, 14, "Total pagado:               %s", dinero(pagado).c_str());
    GP(4, 15, "Método de pago:             %s", tarjeta ? "Tarjeta" : "Efectivo");
    if(!tarjeta){
        GP(4, 16, "Monto entregado:            %s", dinero(entregado).c_str());
        GP(4, 17, "Vuelto:                     %s", dinero(vuelto).c_str());
    }
    GP(4, 19, "Compra realizada con éxito. ¡Gracias por elegir INFINITY X!");
    esperar("Presione una tecla para volver...");
}

void realizarCompra(){
    char dni[32];
    pantalla("PEDIDOS - COMPRAR");
    GP(4, 8, "DNI del cliente: ");
    gotoxy(21, 8); leerLinea(dni, 15);
    int c = buscarDNI(0, dni);
    if(c < 0){ pausa("Cliente no registrado en el sistema."); return; }

    pantalla("PEDIDOS - COMPRAR");
    GP(4, 8, "Cliente: %s", listaCli[c].nombre);
    esperar("Presione una tecla para continuar..");

    int cant[NUM_PROD]; memset(cant, 0, sizeof(cant));
    for(;;){
        int idx = elegirProducto("PEDIDOS", false);
        if(idx < 0) break;
        pantalla("PEDIDOS - COMPRAR");
        int disp = stock[idx] - cant[idx];       // disponible considerando lo ya añadido al pedido
        int y = 8;
        fila(y, "Producto", catalogo[idx]->nombre);
        fila(y, "Precio", dinero(catalogo[idx]->precio));
        fila(y, "Stock disponible", stock[idx] > 0 ? numStr(stock[idx]) : string("Agotado"));
        if(stock[idx] <= 0){ pausa("Producto agotado. No se puede vender."); continue; }
        if(disp <= 0){ pausa("Stock insuficiente. Ya añadió todas las unidades disponibles."); continue; }
        int q;
        for(;;){
            if(pedirEntero(4, 16, "Cantidad: ", q) && q > 0){ limpiaLinea(23); break; }
            aviso("Cantidad no válida. Ingrese un número mayor que cero.");
        }
        if(q > disp){
            char msg[96]; sprintf(msg, "Stock insuficiente. Unidades disponibles: %d.", disp);
            pausa(msg);
            continue;
        }
        cant[idx] += q;
        GP(4, 18, "Subtotal de este producto: %s", dinero(redondear(catalogo[idx]->precio * cant[idx])).c_str());
        if(!confirmarSN("¿Desea seguir comprando?")) break;
    }
    if(unidadesCarrito(cant) == 0){ pantalla("PEDIDOS"); pausa("No se añadieron productos. Compra cancelada."); return; }

    // ---------- pago ----------
    double total = totalCarrito(cant, false);
    for(;;){
        pantalla("PEDIDOS - PAGO");
        mostrarResumen(cant, false);
        GP(4, 20, "TOTAL A PAGAR: %s", dinero(total).c_str());
        GP(40, 20, "1. Efectivo   2. Tarjeta   0. Cancelar");
        int op = pedirOpcion(4, 22, 0, 2, "Método de pago: ");
        if(op == 0){ pausa("Compra cancelada. No se modificó el inventario ni el dinero."); return; }

        double desc = 0, aPagar = total, entregado = 0, vuelto = 0;
        bool tarjeta = (op == 2);
        pantalla("PEDIDOS - PAGO");
        GP(4, 8, "Valor total de la compra: %s", dinero(total).c_str());
        double monto;
        if(!tarjeta){
            for(;;){
                if(pedirMonto(4, 10, "Dinero entregado por el cliente: S/ ", monto)){ limpiaLinea(23); break; }
                aviso("Monto no válido. Ingrese nuevamente.");
            }
            if(monto + 0.0001 < total){ pausa("Dinero insuficiente. La compra no se concluyó."); continue; }
            entregado = redondear(monto);
            vuelto = redondear(entregado - total);
        }else{
            desc = redondear(total * DESC_TARJETA);
            aPagar = redondear(total - desc);
            GP(4, 9,  "Descuento por tarjeta (10%%): %s", dinero(desc).c_str());
            GP(4, 10, "Total con descuento:         %s", dinero(aPagar).c_str());
            for(;;){
                if(pedirMonto(4, 12, "Monto disponible en la tarjeta: S/ ", monto)){ limpiaLinea(23); break; }
                aviso("Monto no válido. Ingrese nuevamente.");
            }
            if(monto + 0.0001 < aPagar){ pausa("Dinero insuficiente. La compra no se concluyó."); continue; }
        }

        // ---------- pago exitoso: actualizar stock y dinero en la misma operacion ----------
        for(int i = 0; i < NUM_PROD; i++) stock[i] -= cant[i];
        saldo = redondear(saldo + aPagar);
        bool ok = guardarStock() & guardarCaja();
        mostrarRecibo(listaCli[c].dni, listaCli[c].nombre, total, desc, aPagar, tarjeta, entregado, vuelto,
                      unidadesCarrito(cant));
        if(!ok){ pantalla("ATENCIÓN"); pausa("No se pudieron guardar los archivos de datos."); }
        return;
    }
}

void menuPedidos(){
    static const char* OPS[2] = {"Comprar", "Volver"};
    for(;;){
        if(submenu("PEDIDOS", OPS, 2, false) == 2) return;
        realizarCompra();
    }
}

// ============================================================
//  6. DINERO  /  7. EASTER EGG
// ============================================================
void mostrarDinero(){
    pantalla("DINERO DEL ESTABLECIMIENTO");
    GP(30, 9, "I N F I N I T Y  X");
    GP(4, 12, "Saldo disponible: %s", dinero(saldo).c_str());
    esperar("Presione una tecla para volver...");
}

void easterEgg(){
    pantalla("INFINITY X - SECRET FOUND!");
    GP(4, 9,  "EASTER EGG ENCONTRADO");
    GP(4, 12, "¡Felicidades! Descubriste el acceso secreto de");
    GP(4, 13, "Programación Orientada a Objetos.");
    esperar("Presiona una tecla para volver...");
}

// ============================================================
//  LOGIN Y MENU PRINCIPAL
// ============================================================
void login(){
    for(;;){
        char user[32], pass[32];
        pantalla("LOGIN");
        dibujarCuadro(18, 9, 60, 15);
        GP(23, 11, "Usuario: ");
        GP(23, 13, "Clave:   ");
        gotoxy(32, 11); leerLinea(user, 20);
        gotoxy(32, 13); leerPasw(pass, 20);

        for(int i = 0; i < 3; i++){ GP(33, 19, "Verificando"); GP(44 + i, 19, "."); Sleep(400); }

        bool admin  = (strcmp(user, "Admin") == 0 && strcmp(pass, "12345") == 0);
        bool secret = (strcmp(user, "POO") == 0   && strcmp(pass, "20") == 0);
        if(admin || secret){
            sesionSecreta = secret;
            limpiaLinea(19);
            for(int i = 0; i < 3; i++){ GP(30, 19, "Accediendo al sistema"); GP(52 + i, 19, "."); Sleep(400); }
            return;
        }
        limpiaLinea(19);
        GP(30, 19, "Datos incorrectos");
        esperar("Presione una tecla para volver a ingresar..");
    }
}

void menuPrincipal(){
    for(;;){
        pantalla("MENÚ PRINCIPAL");
        GP(4, 8,  "1. Clientes");
        GP(4, 9,  "2. Personal administrativo");
        GP(4, 10, "3. Abastecimiento de productos");
        GP(4, 11, "4. Inventario");
        GP(4, 12, "5. Pedidos");
        GP(4, 13, "6. Dinero del establecimiento");
        if(sesionSecreta) GP(4, 14, "7. Easter Egg encontrado");
        GP(4, 16, "0. Salir");
        switch(pedirOpcion(4, 19, 0, sesionSecreta ? 7 : 6)){
        case 1: menuClientes();       break;
        case 2: menuPersonal();       break;
        case 3: menuAbastecimiento(); break;
        case 4: menuInventario();     break;
        case 5: menuPedidos();        break;
        case 6: mostrarDinero();      break;
        case 7: easterEgg();          break;
        case 0:
            pantalla("HASTA PRONTO");
            GP(4, 10, "Gracias por usar INFINITY X.");
            esperar("Presione una tecla para salir...");
            return;
        }
    }
}

// ============================================================
//  MAIN
// ============================================================
int main(){
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn  = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleTitleA("INFINITY X");
    crearCatalogo();
    cargarDatos();
    cabecera();
    login();
    menuPrincipal();
    liberarCatalogo();
    gotoxy(0, 25);
    return 0;
}
