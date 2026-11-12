# I07: sensor interno de salida de un proceso seleccionado

Revision105 agrega un proveedor SEPARADO de contadores de liberacion directa:
[STARTUP_FINAL_RESOURCES.md](STARTUP_FINAL_RESOURCES.md). El contrato de este
evento de ciclo de vida no cambia ni se convierte en un contador final.

Revision104, 2026-10-24. Componente INTERNO en linux_startup_exit.hpp/.cpp,
no API publica/Monitor ni activacion automatica del recorder. Captura un evento
de ciclo de vida, NO contadores finales CPU/disco ni causa de autostart. I07
sigue abierta; no se convierte este evento en StartupProcessSample ni impacto.

## Punto de observacion y alcance

La [fuente Linux6.6 do_exit](https://github.com/torvalds/linux/blob/v6.6/kernel/exit.c)
llama taskstats_exit(tsk,group_dead) despues de establecer exit_code, antes de
otras fases de limpieza. group_dead indica el ultimo hilo vivo; entrar en esa
funcion no significa haber terminado toda la limpieza ni estar ya recolectado.
La [documentacion taskstats](https://www.kernel.org/doc/html/latest/accounting/taskstats.html)
distingue estadisticas por hilo y grupo; no se presupone que sus contadores
sean intercambiables ni se usan ceros de campos ajenos como mediciones.

Se engancha FENTRY de `taskstats_exit(task_struct*,int)->void`, verificado contra
BTF real. El filtro TGID inicial se ejecuta ANTES de acceder a argumentos,
credenciales, punteros o mapas. Para el objetivo seleccionado solo acepta
group_dead=1; una salida de lider con otro hilo vivo NO publica el evento.
Verifica task* igual a current, UID real inicial y start_boottime del lider.
El hook es una funcion global del kernel, pero no se registran datos de otros
procesos; no se subscribe a difusion netlink taskstats ni registra listeners CPU.

Datos fijos: marcador group_dead, TGID/UID iniciales, inicio del lider y timestamp
CLOCK_MONOTONIC del punto de entrada. No nombres, argumentos, entorno, contenido
de red/archivos, CPU, E/S, codigo de salida ni asociaciones a .desktop.
Se trata del reloj monotono del kernel: no convertir automaticamente a los
timepoints de un consumidor con offsets de time namespace ni a hora wall.
Una sola seleccion/instancia, evento one-shot, sin buffer variable/eviccion.

## Identidad y ciclo de vida

`LinuxStartupSelectedExitCapture::start(true,pidfd,pid,start_ticks)` requiere
consentimiento explicito y objetivo todavia vivo. Duplica el pidfd, obtiene
asociacion autenticada kernel/procfs mediante el corredor existente de permisos
pidfd (dos observaciones kernel y contexto antes/despues), adjunta y revalida
la misma identidad. No infiere TGID inicial por PID local ni convierte ticks
en identidad kernel. Cambio/salida durante setup rechaza y cierra todo; no
promete recuperar la carrera de un proceso que murio antes de armar captura.

El mapa persiste el evento aunque el padre real recolecte el objetivo y procfs
ya no tenga su PID. `sample` no vuelve a abrir el PID desaparecido ni consulta
waitid de un proceso ajeno. Usa el pidfd fijado para publicar `pidfd_exited`
POR SEPARADO del evento. Un evento puede observarse antes de que pidfd confirme
salida; no se presenta el timestamp del evento como instante exacto del final.

Antes/despues de leer verifica boot/observador/namespace propios, caller/euid.
Fork rechaza lectura y cierra SOLO las referencias heredadas sin escribir mapas
del padre; la captura original sigue funcionando. Cambios de dominio no se
traducen automaticamente. Llamadas serializadas por caller; misma tabla FD,
sin unshare/cierre/reutilizacion externa de descriptores propios.

## Publicacion, ausencia y fallos

ARRAY7 de u64: estado0/1, fallo sticky, group_dead, TGID, UID, start_ns, timestamp.
Un escritor (ultimo hilo) escribe payload y publica estado al final mediante
intercambio atomico. Fallo/identidad distinta/duplicado activa OR sticky, nunca
wrap de contador. El lector revisa estado antes/despues y valida los7slots.
Si la publicacion cruza la lectura, ausencia temporal/reintento sin resetear
mapa; no se entrega payload parcial. Fallos reales cierran todos los recursos.
Estado0 es observacion valida sin evento, NO prueba de que el proceso siga vivo.
Pidfd listo sin evento es error/ausencia, no evento o contadores inventados.

`stop`/destructor cierran link/programa/mapa/pidfd; no pinning, archivos, servicios,
configuracion de kernel persistente ni instalacion. Repetir sample del evento
devuelve el MISMO timestamp, sin suma ni nueva observacion inventada.

## Requisitos y reutilizacion

Proveedor opcional Linux64-bit little-endian con BTF, taskstats_exit attachable,
trampolines FENTRY, helpers/atomics requeridos y privilegios BPF/ptrace/LSM.
Ausencia/denegacion no ejecuta herramientas ni instala paquetes. Emite
permission_required cuando el permiso es identificable; otros fallos quedan
unsupported con motivo. No compatibilidad universal de distro/kernel.

Se reutilizan esquema/joins/lectura de mapas nativos de F03, NO trafico de red.
Hoy el bootstrap de identidad aun exige el esquema BTF de red existente; es
una dependencia interna declarada, no se afirma independencia universal de
esos hooks. El resolver del hook de salida es independiente de los tipos socket.

read_linux_kernel_btf_blob extrae el lector acotado64MiB con la misma validacion
de archivo/identidad/truncamientos; read_linux_network_btf conserva su contrato.
No cambio de layouts publicos/vtable/almacen ni nuevas funciones Windows.

## Evidencia

`tests/linux_startup_exit.cpp`: BTF minimo/prototipo exacto/ambiguedad/todos los
truncamientos, decodificador/state/UID0/identidad/fallo/cero-no-evento, limites
de bytecode y filtro antes de contexto/mapas, rechazo sin autorizacion. Release
mantiene asserts. No solo fixtures como prueba de funcionamiento kernel.

`pcinsight-linux-startup-exit-native --authorized-private-exit` es MANUAL, no
CTest. Usa autorizacion previa de sondas temporales filtradas a procesos
privados, sin extenderla a captura general. Crea padre/objetivo nieto propios:
waitid del observador devuelve ECHILD, setup con pidfd/joins nativos, salida de
sentinela no seleccionado ignorada, copia fork rechazada sin afectar al padre.
El padre real hace waitpid y confirma reaping ANTES de sample; procfs ausente,
evento e identidad retenidos y pidfd de salida confirmado, replay estable.

Se probaron DOS casos reales WSL: un hilo y multihilo con lider zombie mientras
el worker sigue vivo. El primero no crea eventos ajenos; en el segundo no hay
evento ni pidfd_exited al salir solo el lider, y si aparecen al salir el worker
final, aun despues de reaping. Sin proceso objetivo siendo hijo directo del
observador, sin baseline/final procfs posterior a la salida. No CPU/disco
reconstruidos ni soporte de procesos nunca seleccionados. Sondas retiradas,
FD census restaurado e hijos normales. Watchdogs25s/20s; ejecucion real<5s.

Logs work/startup-exit-native.log y startup-exit-native-threaded.log preservan
los positivos; fixture inicial tenia indices de mutacion incorrectos, corregidos
sin relajar el esquema, log startup-exit-fixture.log retenido. Warning inicial
de write de limpieza ignorado corregido conservando su resultado; logs intactos.

Pendiente I07: contadores finales CPU/disco verificables con semantica de hilos/
grupo, cobertura de vidas cortas NO preseleccionadas/descendientes, vinculacion
con evidencia fiable de autostart y recorder/inventario, diagnosticos durables.
No activar host en login ni ampliar captura a apps reales por este componente.
