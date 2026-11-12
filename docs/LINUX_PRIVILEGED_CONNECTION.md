# L05 — ancla absoluta y conexión nativa propia

Revision152, 2026-11-02. Unidad interna implementada y probada. NO certifica
bootstrap seguro del proceso/cargador ni instala o activa un helper. L05 sigue E.

src/linux_privileged_connection.hpp/.cpp añade PrivilegedDirectory y
PrivilegedBusConnection con propiedad RAII y un receptor unido a esa conexión.
No se invoca ningún programa externo, no se eleva UID ni se toma una dirección
D-Bus del entorno, un bus por defecto o una conexión heredada del consumidor.

## Ancla de configuración

open_privileged_directory abre desde `/`, componente a componente, con
O_DIRECTORY/O_NOFOLLOW/O_CLOEXEC. Ruta absoluta lexical, hasta4096 bytes y32
componentes ASCII cerrados; no `.`/`..`, componentes vacíos, slash final, espacios,
symlinks o expansiones. Conserva cada descriptor y coteja dispositivo/inodo,
UID/GID/modo y vínculo al padre. Antecesores solo root o UID confiable configurado;
directorio final del UID configurado, sin escritura grupo/otros ni bits especiales.

Por defecto rechaza antecesores escribibles incluso con sticky bit. Las pruebas
en /tmp activan explícitamente allow_sticky_ancestors: permite sticky en un
antecesor de dueño confiable, nunca en el directorio final protegido. NO activar
esa excepción implícitamente en despliegue. No se cambia ningún permiso del host.
El lector151 puede usar el FD prestado del ancla; no debe cerrarlo el consumidor.

Fija PID/EUID y los inodos de namespaces mount/user del HILO actual mediante
/proc/thread-self/ns; revalida al usarla. No usa /proc/self/ns, que podría observar
el líder en vez del hilo. Procfs/namespaces ausentes => rechazo, no suposición de
contexto. No se ejecutó unshare/setns ni se validaron transiciones reales de
namespace en esta unidad. No es una transacción contra root/filesystem adversarios.

## Conexión explícita

Endpoint cerrado: ancla, nombre de socket simple, UID esperado del nodo, UID de
SO_PEERCRED esperado y timeout no nulo <=5s. Los dos UID son distintos conceptos:
el dueño del nodo no tiene que ser el UID capturado por el kernel para el peer.
No se descubren ni se aceptan desde el payload. Socket regular de namespace
filesystem, S_ISSOCK, dueño y único enlace verificados; sin sockets abstractos,
symlinks o sintaxis de inyección de dirección. Path Unix completo hasta107 bytes.
Los permisos conectables del socket pueden ser amplios (como666); la protección
de la ruta viene de los directorios y la del interlocutor de las credenciales.

Crea sd_bus NUEVO con dirección exacta unix:path, cliente, anonymous0,
negotiate_fds0, interactive_authorization0, exit_on_disconnect0, timeout explícito.
Completa Hello, exige ready/nombre único, socket stream Unix, peer PID válido y
UID configurado/SO_PEERCRED, pathname real y descriptor CLOEXEC. Revalida ruta y
proceso antes de publicar. Sin búsqueda/autolanzamiento de broker/Authority,
solicitud de nombre de helper, registro, bucle permanente o reconexión automática.

Cada conexión exitosa obtiene generación monotónica por proceso, sin wrap. Cierre
explícito/destructor: close+unref sin flush; fallo de creación también limpia.
No se publica handle si cambian proceso/EUID/namespaces/ancla/nodo o readiness:
la conexión queda revocada de forma persistente, aunque luego restauren permisos.
Se requiere un objeto nuevo para reconectar. identity_config deriva endpoint,
peer UID y generación de la conexión; generación de política proviene del store151.

receive_owned_privileged_request exige que el mensaje pertenezca exactamente a
esa conexión, delega al receptor gestionado151 y vuelve a cotejar conexión/ancla/
generación antes de devolver la observación. Cambio detectado => UNKNOWN/stale,
aun si Authority respondió positivo. No ticket ni ejecutor: sigue faltando la
revalidación/serialización inmediatamente anterior a una futura acción.

Usos de conexión, getters, handler, bucle y cierre deben serializarse. NO cerrar
con un handler/operación pendiente ni conservar el raw pointer más allá de su
propietario. Objetos y handles no se reutilizan tras fork/exec; un hijo de fork
debe ir a exec/_exit sin destruir buses heredados. Las guardas de PID no convierten
un bus compartido tras fork en seguro. No promesa de defensa contra código C++
privilegiado malicioso dentro del mismo proceso.

## Pruebas152

tests/linux_privileged_connection.cpp verifica rutas/propietario/modo/FD_CLOEXEC,
rechazo sticky por defecto, permiso sticky explícito solo para antecesores en
pruebas, rechazo symlink, reemplazo de directorio detectado, nodo no-socket,
inyección de dirección/timeout inválido, pérdida y recuperación de ancla y FD
sin fuga. MSVC solo argumentos/fallback, no simulación de syscalls Linux.

tests/linux_privileged_receiver.cpp usa ahora el conector propio para el receptor
REAL, no su constructor sd-bus de prueba. Broker/libsystemd privados REALES,
Authority SIMULADA. Verifica segunda conexión con generación mayor, cierre con
handle/generación inválidos, UID de nodo y peer erróneos rechazados, fds0/interactive0/
exitdisconnect0. Mantiene21 casos151 y añade cambio de permisos del directorio
durante respuesta positiva: reporte UNKNOWN/stale y conexión revocada aun después
de restaurar permisos.22solicitudes/9checks por recorrido. El padre retira broker
y archivos privados; no bus del host/polkitd/servicios reales/privilegios nuevos.

Linux180/180 +100 ejecuciones focales L05 (10tests repetidos10,10brokers), MSVC20
argumentos/fallback, dos consumidores SDK reinstalados/relinkados exit0. FD y
cleanup privados correctos. Dos avisos iniciales de indentación corregidos,
preservados en work/privileged-connection-build.log; compilación verificada y final
limpias. Resto work/privileged-connection-*. Sin API pública/layout/formatos nuevos.

## Alcance restante

Actualización153: [ejecución cerrada](LINUX_PRIVILEGED_EXECUTION.md) interna OFF
por defecto, nuevo método ControlSystemService separado de auth-only, pruebas con
Manager simulado. Números152 anteriores históricos. No aceptación ni despliegue
reales; incertidumbre tras envío y falta de atomicidad se documentan explícitamente.

Proceso/ejecutable, entorno y cargador/dependencias deben ser confiables ANTES
de usar este código. dlopen(libsystemd.so.0) no es un cargador endurecido ni sanea
LD_PRELOAD/LD_LIBRARY_PATH ya procesados. Los UID/rutas siguen siendo configuración
del mecanismo, no un instalador seguro ni política polkit real. No hay watcher,
deadline agregado de handshake/consulta/IO, límites globales de cola/tamaño o
cancelación remota garantizada. No aceptación root/helper/polkit real/consentimiento
humano, ejecutor, instalación, activación, elevación ni CI remota en esta unidad.

Siguiente: límites y flujo de ejecución cerrado/despliegue endurecido antes de
aceptación real; no instalar/activar/elevar sin autorización específica. OpenRC148
solo control explícito intacto; SysV/K03/I07/UAC conservados.
Conteo oficial C60/E10/P3/B2,3X excluidas:60/75=80%,15/75=20% abiertas.

Fuentes primarias: [sd-bus ABI](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h),
[sockets Unix](https://man7.org/linux/man-pages/man7/unix.7.html),
[sticky/unlink](https://man7.org/linux/man-pages/man2/unlink.2.html),
[namespace identity](https://man7.org/linux/man-pages/man7/namespaces.7.html),
[proc/thread-self](https://man7.org/linux/man-pages/man5/proc_thread-self.5.html).
Restricciones, generaciones y revocación son contratos PCInsight.
