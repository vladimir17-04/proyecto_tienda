# INFINITY X

Sistema de consola en C++ para administrar una tienda de tecnología: clientes, personal, inventario, abastecimiento, pedidos y caja. Proyecto del curso de Programación Orientada a Objetos.

## Dónde está el proyecto

Todo el código está dentro de la carpeta `proyecto-infinity-x` de este repositorio. Entra a esa carpeta para ver el programa, su propio README y el archivo `src/INFINITY_X.cpp`.

## Recomendaciones para correr el .cpp

Se recomienda correrlo en Code::Blocks.

1. Usa Windows. El programa usa `windows.h` y `conio.h`, así que en Linux o macOS no compila.
2. Instala Code::Blocks con el instalador que dice `mingw-setup` en el nombre. Ese ya trae el compilador, así que no hay que configurar nada más.
3. Descarga el archivo `INFINITY_X.cpp` desde la carpeta `proyecto-infinity-x/src` o clona el repositorio.
4. En Code::Blocks crea un proyecto: File, New, Project, Console application, lenguaje C++. Borra el `main.cpp` que viene por defecto y agrega `INFINITY_X.cpp` con Project, Add files.
5. Revisa la codificación en Settings, Editor, Other settings, y deja el encoding en UTF-8. Las tildes y los cuadros de la pantalla dependen de eso.
6. Presiona F9 (Build and run). Se abre la consola con el programa.
7. Para entrar, usa el usuario `Admin` y la clave `12345`.

Si algunos símbolos salen raros, cambia la fuente de la consola a Consolas o Lucida Console (clic derecho en la barra de la ventana, Propiedades, Fuente).

Los datos se guardan en archivos `.dat` que se crean junto al proyecto. Si cambias la carpeta de trabajo, el programa empezará con datos nuevos.

También se puede compilar desde la consola con MinGW, ubicado en la carpeta `proyecto-infinity-x`:

```
g++ -o infinity_x.exe src/INFINITY_X.cpp
```

## Autores

- Vladimir Josec Ticona
- Joel Sander Pele Mamani Llanos
- Paulo Andre Velarde Excelmes
