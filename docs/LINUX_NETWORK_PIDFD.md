# Identidad de un actor externo a partir de pidfd

Revision69 implementa `read_private_pidfd_network_identity(authorized,pidfd)`
en un componente interno Linux. Obtiene TGID/UID inicial y start_boottime del
lider sin pedir al objetivo que comunique esos valores. No hay API instalada,
activacion Monitor, recorrido global de tareas ni enumeracion de procesos.

## Consulta acotada

Sin opt-in o con descriptor negativo no hace consultas de captura. Duplica el
descriptor seleccionado con CLOEXEC para conservar el mismo objeto durante la
consulta y comprueba que no este listo/salido mediante poll. No abre otro PID
por numero como sustituto. Un handle que no sea pidfd no produce identidad.

El BTF valida el prototipo exacto
`security_ptrace_access_check(task_struct*, unsigned int) -> int` y offsets
task.tgid, group_leader, start_boottime, cred y cred.uid.val con signos/anchos,
wrappers, limites de contenedores y max1MiB. La capacidad es opcional: no se
inventa un offset o hook cuando falta. Un candidato anterior __pidfd_fget no
aparece en BTF del WSL; fue descartado, no hay fallback oculto a ese simbolo.

Descubre el TGID/TID **iniciales del hilo llamante** mediante programas locales
no adjuntados. Adjunta una sonda FEXIT temporal que filtra ese pid_tgid completo
antes de leer contexto y solo admite modo ATTACH_REALCREDS=0x12/retorno0.
No modifica la respuesta de seguridad, credenciales ni memoria del kernel.

Con la sonda activa, solo hace `pidfd_getfd(dup_pidfd,-1,0)`: provoca la
comprobacion de acceso a ese proceso, sin copiar ningun FD de su tabla. Exige
EBADF como resultado final, exactamente una observacion valida y cero errores.
EPERM/EACCES se propaga como permiso requerido. No se hace ptrace attach,
no se entrega senal ni se detiene el objetivo; si la comprobacion no llega al
hook o su resultado no es verificable, devuelve ausencia, no identidad adivinada.
La operacion puede tomar locks del kernel y pasar por LSM/auditoria del sistema;
no promete ausencia absoluta de coste o eventos de auditoria.

Para evitar reentrancia por handlers durante esa ventana, bloquea temporalmente
en el caller las senales que admite pthread_sigmask y restaura su mascara con
RAII. Esto puede retrasar handlers, incluido un watchdog basado en senal. No es
un plazo duro de ejecucion. No soporta cancelacion asincrona del hilo ni que el
caller haga operaciones reentrantes/manipule internamente sus descriptores.
No cambia la mascara del objetivo ni de los otros hilos. Las senales internas
reservadas por la libc y las no bloqueables conservan las reglas del sistema.

La sonda lee el task del chequeo, exige task==group_leader y copia TGID, UID de
cred.uid.val y start_boottime a un ARRAY5 privado con contadores exito/error.
El trampoline no sleepable protege la lectura con RCU en la via revisada. No
expone punteros, contenido de usuario o datos de red. Los retornos/contextos
ajenos y denegados no llegan a leer el task. El enlace se retira antes de leer
el mapa; la llamada disparadora es sincrona y esta filtrada al mismo hilo.
Mapas/programas/enlace/duplicado se cierran incluso con error. No hay pinning,
daemon, paquetes, comandos externos ni configuracion permanente.

## Garantia y limites

El resultado vincula una observacion de identidad al objeto pidfd consultado
en esta via comprobada. Se verifica liveness antes/despues, no vida futura.
No certifica UID estable para siempre, identidad atomica frente a cambios de
credenciales, estado de todos los threads ni correspondencia con un username.
Si el objetivo es un thread no lider, no se fabrica un token de proceso.
SELF puede omitir este hook por optimizacion del chequeo: debe usar el proveedor
SELF previo, no considerar un resultado ausente como PID0.

Depende de BTF, tracing/FEXIT, permiso BPF, pidfd_getfd, politica ptrace/LSM y
comportamiento del kernel compatible con la via revisada. Solo verificado
nativamente en este WSL x86_64. Un prototipo BTF compatible no certifica toda la
semantica de otro kernel; no prometer soporte universal. Revision70 agrega un
[join seleccionado procfs/boot/namespaces/start_ticks](LINUX_NETWORK_PIDFD_JOIN.md)
por extremos. F03 aun necesita revalidacion integrada en ventanas, buckets/cobertura e
integracion publica. Este cambio no convierte start_ticks a nanosegundos ni
obtiene el cookie de namespace de un socket externo automaticamente.

## Evidencia

Fixtures BTF: capacidad positiva y8negativos de prototipo/tipos/campos/linkage.
Interprete privado sin captura: filtro exacto TGID/TID antes de contexto,
modo/retorno, lider/nulos/lecturas fallidas, UID0/UINT32_MAX, mapa ausente,
offsets/support y validacion de resultado unico/no errores. No se ejecuta BPF
real desde ese CTest. Se conservan las suites anteriores.

Modo manual `--authorized-private-pidfd-session`, NO CTest: padre crea un hijo
privado de carga y conserva su pidfd. El hijo no ejecuta descubrimiento BPF ni
comunica token del kernel; solo el harness da instrucciones de trafico y datos
locales/cookie de socket. El padre obtiene y revalida dos veces la identidad
por el pidfd y luego usa start_selected. Por tanto prueba descubrimiento de la
identidad sin cooperacion del hijo en esa identidad, **no** un join universal
de aplicaciones ni discovery de cookies sin colaboracion.

WSL autorizado exit0: hijo PID local554/TGID kernel1640/UID inicial0,
TCP4096/4096B,UDP256/256B,8peeks; padre excluido, detach congelado, start/UID
incorrectos sticky, salida via pidfd, handles invalidos/exited rechazados.
Mascara del caller y conteo de FD iguales antes/despues de cada consulta; sesion
e hijo retirados. Sin senales al objetivo salvo limpieza del hijo privado si
falla el harness; no cambio de UID/namespace/configuracion.
Log `work/network-pidfd-native-linux.log`.

El primer build de un target nuevo se pidio antes de regenerar CMake y produjo
No rule; regenerado y conservado el log, sin relajar pruebas. Warning inicial
de copia de pair corregido. Primer candidato __pidfd_fget ausente documentado
en `work/network-pidfd-layout-native-linux.log`; alternativa BTF validada en
`work/network-pidfd-layout-security-native-linux.log`.

Regresion Release Linux120/120 sin skips,12pruebas de red repetidas20veces cada
una; consumidor instalado reconstruido y ejecutado exit0. Build final sin
warnings. Logs `work/network-pidfd-{final-build,suite,repeat,install,consumer}-linux.log`.
Sin builds/capturas Windows ni cambio de API/ABI/historial publico. H04/J04
no se cierran; el porcentaje oficial conserva F03 en curso.

Fuentes primarias revisadas Linux6.6:
[pidfd_getfd](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/pid.c),
[ptrace_may_access](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/ptrace.c),
[LSM](https://raw.githubusercontent.com/torvalds/linux/v6.6/security/security.c),
[flags](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/ptrace.h),
[RCU en trampoline](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/bpf/trampoline.c).
