# L05 — carga protegida de reglas y generaciones de recarga

Revision151, 2026-11-02. Unidad interna implementada y probada; no helper
instalado/activado, ejecutor ni despliegue privilegiado. L05 sigue E.

src/linux_privileged_policy.hpp/.cpp añade parser cerrado, lector Linux anclado
a descriptor y PrivilegedPolicyStore. No obtiene rutas/UID/FD del cliente, de
variables de entorno ni de comandos externos. El descriptor de directorio y su
UID de confianza proceden de un futuro bootstrap seguro; el UID predeterminado
es0. Las pruebas privadas pasan explícitamente su UID actual, sin cambiar dueño
de archivos ni elevar privilegios. Esto NO declara segura una raíz arbitraria:
se comprueba el ancla misma, no todos sus ancestros ni cómo la abrió el llamador.

## Formato cerrado

```text
PCINSIGHT-SERVICE-POLICY 1
private.service 7
blocked.service 0
```

Header exacto y LF final obligatorios. Cada línea: nombre literal de unidad,
un espacio y un dígito0..7. Bits start1/stop2/restart4;0 deniega. No comentarios,
CR, tabs, líneas vacías, NUL, expansiones, comodines, duplicados ni reglas
parcialmente aceptadas. Nombres validados con el contrato145. Header sin reglas
es válido pero deniega toda operación. Máximo256 reglas y96KiB en total.

## Lectura nativa

Ruta relativa de hasta4096 bytes/16 componentes, cada componente hasta255 bytes
y alfabeto ASCII cerrado (letras/dígitos/guion/punto/guion bajo), sin componentes
vacíos, `.` o `..`. No rutas absolutas ni dependencia del working directory.
Duplica el FD de entrada con CLOEXEC; no cierra ni cambia el offset del original.
Recorre directorios mediante openat/O_DIRECTORY/O_NOFOLLOW/CLOEXEC y verifica
propietario esperado y ausencia de escritura de grupo/otros o bits especiales
en ancla, directorios y archivo. Solo archivo regular con un único enlace duro.
fstatat sin seguir symlinks comprueba el tipo antes de abrir el archivo; compara
la identidad después de abrir. O_NONBLOCK evita la espera de FIFO; estos se
rechazan antes de leer. No escribe ni corrige permisos automáticamente.

Lee con límite96KiB; coteja tamaño leído, identidad/UID/GID/modo/enlaces/tamaño y
mtime/ctime antes/después, tanto en descriptor como en nombre. Revalida los
directorios fijados y sus enlaces al padre. Cualquier fallo descarta el conjunto,
sin reglas parciales ni fallback. Descriptores propios se cierran por RAII.
El estado de stat no es una transacción atómica: se presupone filesystem/ancla/
UID administrador fiables, sin escritor privilegiado adversario. Un propietario
privilegiado todavía puede mover nodos o escribir después del cotejo final.

## Recarga y receptor

PrivilegedPolicyStore arranca inactivo. Cada refresh invalida primero la política
anterior y avanza generación, incluso para igual contenido o una lectura fallida.
Publica solo snapshot propio completo; invalidate explícito también revoca.
No reutiliza la última política válida tras error. Los refresh se serializan;
snapshot/is_current/invalidate están protegidos por mutex. Invalidación durante
una carga impide publicar su resultado. Contador sin wrap: agotamiento mantiene
estado inactivo. Revoca el valor antes de asignar notas que podrían fallar por
memoria. Cada copia de snapshot conserva sus datos, pero NO vigencia ni permiso.

receive_native_privileged_policy_request toma snapshot del almacén, fija la
generación en el contexto y verifica vigencia antes/después de observaciones
de identidad y de CheckAuthorization. Una recarga/invalidez detectada descarta
la respuesta, aun si Authority devolvió autorización positiva. El almacén debe
vivir todo el handler. La variante150 que acepta vector directamente sigue
siendo un contrato de adaptador confiable interno, no entrada del cliente.

NO hay watcher: cambios de archivo sin refresh/invalidate no actualizan solos
el almacén. Tampoco hay garantía atómica entre la última comprobación y un uso
posterior del reporte: sigue siendo observación, no ticket/ejecutor. La futura
ejecución debe revalidar y coordinar configuración/identidad justo antes de actuar.

## Evidencia151

tests/linux_privileged_policy.cpp: parser portable, máscaras0..7, header vacío de
reglas, formato/NUL/duplicados/bounds256-257; Linux real con directorio privado,
lectura anidada, rutas/UID inválidos, symlinks intermedio/final, hardlink, FIFO,
modo inseguro de archivo/directorio/ancla, tamaño excesivo, error de contenido,
generaciones, no stale fallback, recuperación, desaparición y FD sin fuga.

tests/linux_privileged_receiver.cpp usa ahora política cargada, no reglas literales
inyectadas al receptor. El broker/libsystemd siguen REALES y privados; Authority
SIMULADA. Por recorrido21 solicitudes/8checks: conserva los casos150 y añade
invalidación desde el callback de Authority mientras está pendiente la consulta
(respuesta positiva descartada como desconocida), siguiente petición sin consulta,
recuperación, recarga con permisos inseguros sin fallback y recuperación final.
Sin polkitd/bus del host/activación/diálogos/servicios reales/elevación.

Linux179/179 +90 ejecuciones focales L05 (nueve tests10 veces,10 brokers privados),
MSVC20 parser/argumentos/fallback (sin Linux simulado), dos consumidores SDK
reinstalados/relinkados exit0. Builds Linux inicial/full/final limpios; logs
work/privileged-policy-*. Sin API pública/layout/formatos/CI remota nuevos.

## Pendiente

Actualización152: [ancla y conector propios](LINUX_PRIVILEGED_CONNECTION.md)
implementados: apertura absoluta validada y receptor unido a conexión propia,
generaciones/revocación ante cambios. Bootstrap de proceso/cargador y aceptación
siguen pendientes; los números151 de esta página son evidencia histórica.

Bootstrap protegido de proceso/ancla y entorno/cargador dinámico, configuración
fiable de endpoint/UID de broker y Authority, conexión/lifetime/recargas del host,
límites de transporte/FD/cola y deadline agregado, política/consentimiento polkit
reales, ejecutor cerrado y aceptación bajo autorización específica. No se detectan
descriptores/mappings de escritura preexistentes; bootstrap no debe adoptar archivos
creados por un propietario no fiable. O_NONBLOCK no impone deadline a archivos
regulares o filesystems remotos; no se garantiza cancelación remota. No defensa
contra root/kernel/filesystem comprometidos ni sustituto del aislamiento del host.

Siguiente: bootstrap/conexión y condiciones de despliegue sin instalar, elevar ni
activar nada. OpenRC148 solo control explícito intacto; SysV/K03/I07/UAC conservados.
Conteo oficial auditado: C60/E10/P3/B2,3X excluidas;60/75=80%,15/75=20% abierto.

Fuentes primarias para las primitivas usadas: [open/openat](https://man7.org/linux/man-pages/man2/open.2.html)
y [stat/fstatat](https://man7.org/linux/man-pages/man2/stat.2.html).
La gramática, límites, epochs y restricciones adicionales son decisiones PCInsight.
