# SysV: catálogo nativo de configuración

Revisión138 añade [lector de comentarios LSB](LINUX_SYSV_LSB.md), como API
separada. No habilita lectura de contenido automática en este catálogo.

Revisión137, K03. API pública `pcinsight/linux_sysv.hpp`:

```cpp
auto catalog = pcinsight::query_linux_sysv_catalog("/etc/init.d", "/etc");
// En otro layout, seleccionar explícitamente /etc/rc.d como segundo root.
// Comprobar value/support, complete/diagnostics y disponibilidad de cada campo.
```

Un init.d y un parent de rc[0-9S].d explícitos, no discovery del manager actual,
paquetes, todos los repositorios o namespaces. Se observan once niveles fijos
0..9/S; raíz de runlevels válida con directorio rcN.d ausente -> presencia false
conocida. Link rcN.d roto/inaccesible o nodo no-directorio -> parcial/UNKNOWN,
nunca ausencia fingida. Root faltante no se crea: resultado parcial con campos
desconocidos, no censo global vacío.

## Contenido y significado

- `scripts` contiene candidatos regulares/symlinks de init.d, omitiendo ocultos
  y tipos directos no-file. Se conservan nombres `.sh`: no se impone aquí el
  filtro del catálogo OpenRC. No se leen cuerpos, headers o shebangs. Un destino
  no regular/roto se conserva como candidato desconocido y marca parcial.
- `executable_mode` describe cualquier bit x, NO permiso efectivo, validez de
  intérprete, script SysV confirmado, paquete instalado o capacidad de ejecutar.
- `runlevels` tiene once filas con presencia de directorio y entradas visibles.
  Las entradas de nombre no convencional se conservan, no se ejecutan ni se
  presentan como activas. Se admiten aliases normales de directorios.
- `convention` decodifica exclusivamente S/K + dos dígitos + sufijo no vacío:
  start/stop, NN0..99 y sufijo literal. Rechaza NUL/slash y más de255 bytes.
  Lowercase o nombres desconocidos -> UNKNOWN. Un tercer dígito pertenece al
  sufijo, no se inventa prioridad de tres cifras. No promete orden efectivo.
- `symlink` y `regular_target` son tipos observados, no capacidad/estado. Targets
  se inspeccionan con O_PATH sin abrir FIFO/device para I/O. Archivos regulares
  directos en rcN.d también se describen; no se presupone que todo sea symlink.
- `script_index` solo asocia un target regular con un candidato único de igual
  dispositivo/inodo en ESTE catálogo completo. No usa sufijo/basename como
  identidad. Hardlinks ambiguos, destino externo o parcial -> índice UNKNOWN.

Los índices/nombres/paths no son tokens para ejecutar ni identidades persistentes.
S/K son configuración de nombre, NO enabled/disabled global, runlevel actual,
PID/running/readiness o prueba de init activo. Duplicados por aliases quedan
visibles; no se suman como procesos distintos. Se ignoran dependencias y política
de dispatch/distribución: no equivale a init, update-rc.d, chkconfig o systemd.
No se abre `/proc` de PIDs inferidos, se lee pidfile o se envía señal alguna.

## Consistencia y límites

Roots absolutos/hasta32768 bytes/sin NUL. max_entries1..4096 (default1024) cuenta
**scripts más entradas de todos los niveles**, no las once filas. Dos pasadas
con descripciones de directorio nuevas (sin offsets compartidos de dup), listas
ordenadas y comparación de metadata de targets/directorios. Hasta16384 visitas
incluyendo dot/ocultos, 8MiB nombres y64 diagnósticos por pasada más finales.
Subset truncado no promete primeros N alfabéticos y nunca se anuncia completo.

openat2 NO_MAGICLINKS para roots/targets, O_PATH/fstat, metadata mtime/ctime/size,
selección de runlevel y roots revalidada. Contexto boot/procfs/PID/inicio/UID/
namespaces/eUID antes/después; cambio de contexto descarta. Errores/cambios/bounds
-> complete=false/estimated, campos derivados e índices UNKNOWN; convención
pura de nombres conservados puede seguir conocida. Sin snapshot atómico,
garantía contra todo ABA ni deadline duro de I/O. Host serializa fork/setns/
credenciales/FD contexto. Requiere openat2/procfs; sin fallback inseguro.

La consulta NO ejecuta scripts, status/start/stop/restart, shell/CLI/lib externa,
update-rc.d/install_initd, no crea/modifica enlaces o directorios, no habilita
apps ni evalúa contenido. No añade worker/timer/caché o elevación.

## Evidencia y pendientes

`tests/linux_sysv.cpp`: decoder/bounds/NUL/slash/tercer dígito; fixture privada
raíces ausentes/vacías sin creación, once niveles, `.sh`/modo x true-false,
alias de rc3.d, orden, S/K/sufijo diferente con match de inodo, ambigüedad por
hardlinks, target externo, link roto, FIFO sin bloqueo, magic link rechazado,
runlevel roto distinguido de ausente, partial/bounds y FD sin fuga. Scripts son
texto privado que no se ejecuta; no se arranca init/servicio real. MSVC solo
decoder/argumentos/fallback; SDK enlaza header/símbolos nuevos. No se afirman
fixtures deterministas de cada carrera ni aceptación en init SysV real.

K03 sigue abierta por vinculación de metadatos, control/vida/integración/aceptación
real y otros pendientes. Revisión138 añade lector LSB separado; revisión139 añade
[catálogo descrito opt-in](LINUX_SYSV_DESCRIBED_CATALOG.md) ligado al candidato
fijado, sin unión insegura con snapshots anteriores por nombre. No se infiere autorización para ejecutar scripts SysV de
los «prosigue» ni de una excepción OpenRC132 que sigue sin concederse. I07 intacto.

Fuentes primarias revisadas:
[update-rc.d](https://manpages.debian.org/testing/init-system-helpers/update-rc.d.8.en.html)
para convenciones de nombres/runlevels y
[LSB Init Script Comments](https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/initscrcomconv.html)
para distinguir metadatos declarados de política efectiva. API propia, no réplica
de los gestores o evaluación LSB implementada por este catálogo.
