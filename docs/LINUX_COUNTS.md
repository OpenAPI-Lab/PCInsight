# Contadores de procesos, hilos y descriptores Linux

El panel CPU enumera los directorios PID numéricos visibles en `/proc`.
No utiliza `processes` de `/proc/stat`: ese contador registra procesos e hilos
creados desde el arranque, no la población actual.

- `processes`: directorios PID observados, incluidos procesos no ejecutables
  en ese instante (por ejemplo, sleeping o zombie). No es `procs_running`.
- `threads`: suma de los campos `Threads` válidos de `/proc/PID/status`.
  Valores ausentes, negativos, cero, duplicados, truncados o con overflow no
  se convierten en un cero válido. Las lecturas completas vacías son cero.
- `handles`: suma de entradas FD numéricas de `/proc/PID/fd` de procesos
  legibles. Siempre `estimated`: no es un conteo de objetos Windows ni de
  archivos únicos. Las tablas compartidas entre procesos pueden duplicarse.

El alcance es la vista procfs del llamador: namespaces y `hidepid` pueden
ocultar procesos. `available` significa lectura completa de esa vista, no
garantiza acceso al host completo. La enumeración no es una transacción:
procesos, hilos y FD pueden cambiar mientras se toma la muestra. La desaparición
o un error de lectura produce un subtotal `estimated` si hay lecturas válidas;
sin ninguna lectura válida se devuelve ausencia (`permission_required` si se
observó denegación, `unsupported` para otros errores). El desbordamiento descarta
el total. Los errores globales de enumeración se propagan a los tres contadores.

El lector no omite silenciosamente permisos con `skip_permission_denied`.
Se usan iteradores con `error_code`, un máximo de un millón de entradas por
enumeración y archivos status de menos de 64 KiB. Alcanzar un límite se trata
como lectura incompleta. No se ejecutan programas externos ni se elevan permisos.
Desactivar `include_processes` elimina los detalles de procesos, pero mantiene
estos contadores del panel CPU.

Verificación 2026-10-13: fixtures portables de PID actual frente a acumulado,
entradas no PID, directorio vacío/ausente, FD cero/ausentes, campos inválidos,
duplicados, límites y overflow. Pruebas Linux de permisos como usuario UID 1000
y muestreo nativo Ubuntu WSL. Suites Release: Linux 7/7, Windows 20/20.
Los fixtures Windows no prueban el backend Linux; solo el lector portable.

Referencia: [documentación procfs del kernel](https://www.kernel.org/doc/html/latest/filesystems/proc.html).
