# SysV: descripciones LSB sin ejecutar scripts

Revisión138/K03. API pública en `pcinsight/linux_sysv.hpp`:

```cpp
auto info = pcinsight::read_linux_sysv_lsb_metadata("/etc/init.d/ejemplo");
if (info.value) {
    // short_description y description tienen disponibilidad independiente.
    // Son comentarios declarados, no publisher/estado/proceso autenticados.
}
```

`parse_linux_sysv_lsb_metadata(text, input_complete)` es puro. El lector nativo
selecciona un archivo regular explícito, lee dos veces un prefijo acotado y
verifica bytes/metadata/contexto antes de parsearlo. No shell/CLI/exec, expansión
de variables, comandos status/start/stop, subprocess, instalación o cambios al
archivo. Leer el prefijo puede incluir bytes de cuerpo; **no se evalúan** ni se
extraen descripciones de código ejecutable. El catálogo137 continúa siendo
solo metadata filesystem, sin lecturas de contenido automáticas.

## Subconjunto de comentarios soportado

Se busca un único bloque `### BEGIN INIT INFO`/`### END INIT INFO` en el preámbulo
de comentarios inicial. Delimitadores columna cero, espacios/tabs finales y
CRLF permitidos. Antes/después puede haber líneas vacías o comentarios, incluidos
comentarios indentados; el primer texto no-comentario termina el preámbulo. No
se descubren headers en heredocs, strings o código después de ese punto. Una
cabecera situada tras un comando como set/source no se adivina como metadata.

Dentro del bloque, comentarios columna cero; keywords con un espacio después
de #. Solo `Short-Description` y `Description` generan campos. Se ignoran otros
keywords sintácticamente reconocibles: NO se evalúan Provides/Required/Default-
Start/Stop ni extensiones/política de arranque. Continuaciones de Description
empiezan con # más tab o al menos dos espacios; se unen con un espacio. No se
acepta continuación textual huérfana o de otra keyword. Comentarios vacíos de
continuación no borran Description. Espacios/tabs de valores se normalizan, no
comillas/variables/backticks/$(...) u otros caracteres de texto visibles.

Duplicados de campos descriptivos, bloques duplicados/anidados en el preámbulo,
end huérfano, bloque incompleto, línea no-comentario dentro, texto inválido o
bounds -> UNKNOWN, sin elegir arbitrariamente un valor. En valores descriptivos:
UTF-8 scalar estricto sin C0/DEL/C1, sin reparar bytes corruptos; valores de
keywords ignoradas no se publican. Campos ausentes o vacíos quedan
UNKNOWN individualmente. Un bloque válido sin campos descriptivos puede dar
info available con ambos campos UNKNOWN. No fallback a nombre/variable shell.

Límites: input/prefijo64KiB, preámbulo1024 líneas, línea4096 bytes, valor de cada
línea descriptiva1024 bytes después de quitar sangría/extremos; Description
unida4096 bytes incluyendo separadores. Exceso no se trunca en silencio. El
lector permite archivos mayores que64KiB si hay un bloque válido y el inicio
de código se ve dentro del prefijo. Prefijo truncado que aún es solo comentarios
no se asume EOF, aunque se haya visto un end: podría haber otro bloque fuera.
Esto es un subconjunto conservador, no validador LSB completo ni shell parser.

## Identidad, I/O y alcance

Path absoluto/sin NUL/hasta32768 bytes, openat2 NO_MAGICLINKS/O_PATH, aliases
ordinarios permitidos. fstat confirma regular antes de reabrir SOLO el FD propio
por /proc/self/fd para lectura NONBLOCK/CLOEXEC. FIFO/device/directorio se rechaza
sin abrirlo para I/O. pread desde offset0 hasta min(tamaño,64KiB), dos veces;
EOF temprano/errores/bytes distintos o metadata modo/tamaño/dev/inode/mtime/ctime
cambiada -> UNKNOWN. Se revalida path original y contexto boot/procfs/PID/inicio/
UID/namespaces/eUID. Permiso denegado se distingue; prueba de permisos solo bajo
usuario non-root. No caché/worker/timer, garantías de snapshot atómico/ABA o
deadline duro de I/O. Host serializa fork/setns/credenciales/FD contexto.

Es observación fresca del path dado, NO asociación garantizada con un índice de
catálogo anterior. No se fusiona por basename con snapshots viejos. Comentarios
pueden ser falsos/obsoletos: available significa texto declarado leído/validado,
no publisher, daemon vivo, descripción de un binario en memoria o enablement.
No se agregan PID, servicio actual, grupo Windows, defaults efectivos o control.

## Evidencia y pendientes

`tests/linux_sysv_lsb.cpp`: formato/CRLF/continuaciones/ausente-vacío, literal
shell/Unicode, UTF-8/C0/C1 inválidos, duplicados/anidados/heredoc/body no metadata,
input truncado, límites64KiB/1024líneas/4096línea/1024valor y descripción exacta4096
frente a exceso. Primer fixture falló por contar sangría en valor1024; corregido
y logs iniciales conservados, sin ocultarlo como fallo externo.

Nativo privado: archivo >64KiB con sentinel de no ejecución, alias, ausente sin
crear, FIFO/directorio/magiclink, header truncado, permisos si non-root y FD sin
fuga. No se ejecuta script/init/servicio real. MSVC prueba parser/fallback;
SDK enlaza nuevos símbolos. No se afirma aceptación SysV real ni fixtures para
cada carrera. No cambia HISTORY/formatos, Monitor o modelos previos.

K03 sigue abierta por proyección/integración, control/vida y aceptación real.
Revisión139 añade [catálogo descrito](LINUX_SYSV_DESCRIBED_CATALOG.md) opt-in,
ligado al candidato fijado dentro de la misma consulta. No cambia esta API fresca
ni reinterpreta defaults como habilitado. OpenRCscripts132
y controlscript SysV siguen sin excepción; I07 observador intacto.

Referencia primaria:
[LSB Comment Conventions](https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/initscrcomconv.html).
No se copia el ejemplo de script ni se ejecutan herramientas de instalación LSB.
