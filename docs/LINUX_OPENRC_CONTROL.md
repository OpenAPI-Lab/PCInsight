# K03: ejecutor OpenRC interno, opt-in

Actualización159: existe ya la [API pública instalada](LINUX_OPENRC_CONTROL_PUBLIC.md)
con selección fijada/consentimiento por request, revocación y ownership del runner.
El ejecutor ahora conserva esa selección en vez de reabrir otros archivos al
ejecutar. Se conserva el contrato158 salvo esa mejora; las referencias siguientes
a integración todavía pendiente son históricas158. Aceptación real sigue pendiente.

Revisión158, 2026-11-02. Implementación en `src/linux_openrc_control.*`;
no es todavía API instalada ni acción integrada en Monitor/SystemActions.
La excepción explícita148 permite scripts/runtime exclusivamente para control,
nunca para recopilación. No hay instalación, elevación ni servicios reales.

## Contrato

`dispatch_openrc_control` exige enabled y explicitly_requested. Ambos están
apagados por defecto; sin ellos retorna antes de consultar archivos. Solo admite
ServiceAction start/stop/restart, no opciones, comandos extra ni shell fallback.
Runtime ELF y script absoluto proceden de configuración confiable del mecanismo;
un nombre/índice del catálogo NO otorga autoridad. No se descubre un ejecutor por
PATH ni se certifica que cualquier ELF sea OpenRC por su nombre o cabecera.

Se fija cada directorio sin symlinks usando el guard de directorios existente,
sin conectar a D-Bus ni cargar OpenRC/librc para ejecutar. Ancestros root/owner,
directorio final del owner configurado; sticky rechazado por defecto y permitido
explícitamente solo en las pruebas privadas. Archivos regulares, mismo owner,
sin escritura grupo/otros ni bits especiales ni múltiples hardlinks; nombres
literales acotados. Runtime ejecutable <=16MiB, script no vacío <=1MiB. Se comparan
dev/inode/size/mode/uid/gid/nlink/mtime/ctime antes/después de abrir y antes de liberar
el hijo. El runtime se ejecuta por FD fijado, no se reabre por nombre.

El [runtime OpenRC revisado](https://raw.githubusercontent.com/OpenRC/openrc/master/src/openrc-run/openrc-run.c)
acepta el script en argv1, resuelve su ruta y usa su basename/configuración;
por eso se conserva la ruta original en lugar de sustituirla por un número de FD.
Los tres argumentos son runtime, script y un único verbo literal. Esta ruta
directa evita ejecutar rc-service como herramienta auxiliar de descubrimiento.
Compatibilidad con versiones/distribuciones reales sigue pendiente; no se promete
que toda implementación de ese nombre acepte este contrato.

El hijo permanece gated hasta que el padre obtiene pidfd y revalida destinos.
No hay ejecución si falla esa etapa. Nuestro cuerpo posterior a fork solo hace
preparación async-signal-safe, sin asignadores ni callbacks propios. Se usa fork
de libc: los pthread_atfork del host pueden ejecutarse y deben ser confiables,
no alterar los destinos/FD/credenciales ni interferir con el protocolo. No se
ofrece aislamiento frente a callbacks/handlers/reapers hostiles del propio host.
NO_NEW_PRIVS, cwd `/`, máscara de señales vacía y disposiciones predeterminadas
para TERM/INT/HUP/PIPE/CHLD; no se cambia la configuración de señales del padre.
Entorno nuevo: solo PATH fijo, LANG=C y LC_ALL=C. Sin HOME/RC_*/LD_* heredados.
Se cierran los FD heredados, incluidos los altos; stdio a `/dev/null` comprobado.
Los dos descriptores reservados de exec/error son CLOEXEC. Caller set-ID y
SIGCHLD ignorado/SA_NOCLDWAIT se rechazan. No se crea un grupo de procesos objetivo.

## Resultado, espera y propiedad

El canal CLOEXEC informa errores previos a exec; su EOF NO certifica ejecución
porque el hijo también podría morir. La respuesta exitosa dice únicamente que
se liberó el runner sin error de exec comunicado. runtime_may_have_executed es
conservador: tras liberar no se reintenta, cancela ni revierte automáticamente.
La espera de ACK tiene presupuesto cooperativo2s; syscalls/fork/FS/exec/limpieza
no tienen hard deadline ni garantías contra kernel/FS bloqueados.

Todo hijo devuelto debe conservarse hasta consumir su salida con wait, incluso
ante resultado incierto. wait(0..30000ms) consulta pidfd y recolecta por
[waitid P_PIDFD (ABI3)](https://raw.githubusercontent.com/torvalds/linux/v6.12/include/uapi/linux/wait.h),
sin reap por PID desnudo. Retiene exit_code o signal históricos, nunca estado
actual del servicio, readiness o éxito de hooks/dependencias. Timeout no cancela;
terminate_runner requiere petición explícita y señala solo el pidfd del runner,
no árbol/grupo/dependencias. Terminar el runner NO equivale a stop del servicio.
El destructor cierra el FD, NO mata ni crea un reaper oculto: abandonar un hijo
sin esperar puede dejar un zombie. El host debe respetar este contrato.

## Límites y aceptación pendiente

No es un sandbox ni un helper root seguro/desplegado. Conserva las credenciales,
namespace/cgroup/recursos del proceso; NO_NEW_PRIVS impide ganancia por exec pero
no limita lo que un caller ya privilegiado puede hacer. Deben serializarse
forks, cambios de credenciales/namespaces/cwd/environment/FD y reapers externos;
el handle rechaza uso desde otro proceso/UID y serializa sus propios métodos.

Los propietarios confiables deben serializar cambios de scripts/config/runtime.
FD fijado no congela contenido; OpenRC reabre el script por ruta. Revalidaciones
no son snapshot atómico, protección contra root/owner hostil ni garantía ABA.
Hooks, shell/OpenRC instalados, configuración y dependencias forman parte de la
base confiable y pueden afectar más de un servicio. No se prueban aquí.
Faltan integración pública con selección/consentimiento y aceptación aislada con
OpenRC real. Runit/s6/SysV conservan sus pendientes; SysV no recibe esta excepción.
L05 despliegue real queda aplazado; no se añade otro transporte por esta unidad.

## Pruebas privadas

`tests/linux_openrc_control.cpp` copia SU PROPIO ELF en un directorio mkdtemp
protegido. Ese modelo verifica argv/script legible, entorno, cwd, NNP y ausencia
de FD, y devuelve códigos3/7/11 por verbo; NO es OpenRC, no ejecuta scripts de
servicios ni instala herramientas. Se prueban default-off, consentimiento,
verbo inválido, sticky sin permiso, owner/mode/symlink/hardlink/FIFO/NUL/ruta
inválida, runtime no ELF/malformado, timeout, TERM/KILL explícitos del hijo
privado, contexto tras fork, reaper externo inyectado y cierre de FD/limpieza.
La suite mantiene asserts activos en Release, alarma20s y timeout CTest30s.

Verificación final158: Linux187/187, cuatro tests OpenRC repetidos diez veces
(40 ejecuciones), MSVC diez recorridos del rechazo portable. SDK instalado y
dos consumidores recompilados/relinkados exit0. Guard de FD y limpieza privada
correctos. Logs `work/openrc-control-*-delivery*`; builds finales sin warnings.
El primer build tuvo dos errores del test y dos warnings del código, corregidos;
los logs iniciales se conservaron. El warning MSVC de parámetro sin uso se
corrigió con maybe_unused y se recompiló/reprobó. Un build intermedio detectó clock skew del
montaje, sin tocar reloj/mtime; la entrega se recompiló y verificó limpia.
El último ajuste portable usa logs `work/openrc-control-*-portable-clean.log`
y `work/openrc-control-msvc-*-delivery-clean.log`.
Esto NO es CI remota ni aceptación OpenRC real. K03 conserva estado E y el
conteo oficial sigue C60/E10/P3/B2+3X excluidas: 60/75=80%, 20% abierto.
