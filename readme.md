# Audio Light Sync

Captura el audio de salida del sistema, lo analiza y envía colores por UDP a un
ESP32. La interfaz de consola y la lógica de procesamiento están completamente
separadas.

## Uso de la TUI

Al iniciar, la captura comienza en segundo plano y aparece un panel interactivo.
Todos los cambios se aplican mientras el audio continúa ejecutándose:

- `↑` / `↓`: seleccionar un control.
- `←` / `→`: usar los botones de disminuir/aumentar.
- `Enter`: editar IP/puerto o activar el botón seleccionado.
- `P`: pausar o reanudar el envío.
- `Q` o `Esc`: salir.

El panel muestra en vivo el nivel RMS, detección de beat, color RGB enviado,
formato del dispositivo y número de frames procesados.

Los ajustes se guardan automaticamente. En Windows se almacenan en
`%APPDATA%\AudioLightSync\settings.ini`; en Linux, en
`$XDG_CONFIG_HOME/audio-light-sync/settings.ini` o `~/.config/audio-light-sync/settings.ini`.

## Arquitectura

```text
include/als/                 Interfaces públicas y modelos
src/
  application/              Coordinación del flujo de la aplicación
  audio/                    Buffer, análisis RMS/FFT y procesamiento
  effects/                  Catálogo, color y generación de efectos
  network/                  Transporte UDP
  platform/                 Captura WASAPI y PipeWire
  ui/                       Dashboard TUI y control de terminal
  main.cpp                  Punto de entrada
```

La dependencia fluye desde `main` y `application` hacia interfaces pequeñas.
La UI solo produce un `Config`; no conoce sockets, FFT ni APIs de audio. El
procesador solo consume muestras y un `IColorSender`; no conoce la consola ni
la implementación UDP.

## Compilar en Windows

Desde la terminal de w64devkit:

```sh
cd OneDrive/Documentos/projects/audio_definitive
make windows
```

O directamente:

```sh
x86_64-w64-mingw32-g++ -std=c++17 -O3 -Wall -Wextra -Iinclude -I. \
  -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE \
  src/*.cpp src/*/*.cpp -o audio_light_sync.exe -L. \
  -static-libgcc -static-libstdc++ \
  -lfftw3f -lws2_32 -lole32 -loleaut32 -lwinmm
```

`libfftw3f-3.dll` debe permanecer junto al ejecutable.

Para ejecutar las pruebas unitarias de conversión de muestras, colores,
catálogo y efectos:

```sh
make test-windows
```

## Compilar en Linux

Requiere los paquetes de desarrollo de PipeWire y FFTW:

```sh
make linux
```
