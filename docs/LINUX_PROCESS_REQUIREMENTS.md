# N03: Processes, Details y acciones de proceso Linux

Revisión161, 2026-11-06. Requisitos R13 y acciones asociadas de Processes/Details;
IDs existentes D01/D02/D03/D04/F03/L01/L02/L03/L04, entrega N03. No nuevas
subtareas ni certificación de otros paneles. El GUI muestra dos vistas de la
misma fila `SystemSnapshot.processes[]`; no necesita otra API de «Details».

## Captura y selección de filas

Conservar un `Monitor` y pedir `SampleOptions.include_processes=true`. Dos
observaciones continuas producen tasas cuando sus fuentes son válidas. Primer
intervalo, PID omitido, lectura inaccesible, reset o identidad distinta dejan
tasas desconocidas. Una lista es el inventario visible y aceptado de procfs,
no todos los procesos físicos de todas las máquinas/namespaces.

No indexar la selección del GUI por posición, nombre o PID aislado. Conservar
PID y `linux_start_ticks` sin convertir a segundos/fecha, dentro del mismo
host/boot/PID namespace. Exec puede conservar ambos: no es generación de exe,
identidad criptográfica ni una referencia pidfd retenida desde la muestra.
Un dato observado puede dejar de ser actual antes de dibujarlo o pedir una acción.
No existe transacción entre CPU/RSS/status/NSS/ELF/disco y todas las filas.

`metric_is_known` más support/note decide disponibilidad. Cero/false conocidos
no son ausencia. Preferir Metrics de linux_details a strings legacy vacíos; no
rellenar username=root, descripción=name, arquitectura=x64 o status=running.
Los campos raw pid/parent_pid/thread_count proceden del stat inicial aceptado,
sin disponibilidad individual; no son identidad durable del padre ni contadores
de toda una app/árbol. El ejemplo los etiqueta como scalars de la fila.

## Matriz de campos — prefijo `ProcessInfo`

| Pedido | API / unidad | Alcance y evidencia |
| --- | --- | --- |
| Name | linux_details.name / texto | Name observado, normalizado/escapado para visualizar; no producto comercial ni nombre único. [Detalles](LINUX_PROCESS_DETAILS.md), [test](../tests/linux_process_details.cpp). |
| PID | pid / PID visible en el namespace del caller | Del stat validado, positivo en el colector nativo; no usar como identidad durable ni convertir a un identificador de grupo para señalizar. [Parser](../tests/linux_process_stat.cpp). |
| Parent y threads adicionales | parent_pid / PID; thread_count / conteo | Scalars observados de stat; threads de software, no logical processors CPU. No afirmar que el padre siga existiendo o que la app no tenga otros procesos. Mismo parser. |
| Status/running | linux_details.status / estado kernel | R/S/D/Z/T/etc. conservan semántica Linux: sleeping no significa proceso terminado; zombie no implica aplicación útil ejecutándose. No mapear todo a running/not-running ni predecir estado futuro. Misma evidencia detalles. |
| Usuario/sistema | linux_details.real_uid, username / UID/texto | UID REAL, no efectivo; UID0 válido. NSS getpwuid_r puede no resolver o bloquear/consultar servicios de identidad. No equiparar «system» Windows a todo proceso root, ni inventar root al fallar. Misma evidencia detalles. |
| CPU usada | cpu_percent / % capacidad CPU visible | `100*(delta utime+delta stime)/CLK_TCK/intervalo_propio/CPU_visibles`. Estimated, no % de un core, cuota cgroup, potencia ni frecuencia ponderada. No sumar tiempos de hijos. [CPU](LINUX_PROCESS_CPU.md), [test](../tests/linux_process_cpu.cpp). |
| Memoria usada | memory_bytes / B | VmRSS estimated: no memoria privada/única/PSS, virtual o swap. Shared pages pueden repetirse; no sumar como RAM física única. kB de status se convierte por1024. [RSS](LINUX_PROCESS_MEMORY.md), [test](../tests/linux_process_memory.cpp). |
| Disk | disk_bytes_per_second / B/s | Delta separado read_bytes+write_bytes de proc/io, reloj propio e identidad comprobados. Estimated, no rchar/wchar, porcentaje, throughput físico por disco ni escrituras completadas. [E/S](LINUX_PROCESS_IO.md), [test](../tests/linux_process_io.cpp). |
| Atribución UID de disco | linux_details.disk_rate_real_uid / UID | UID estable observado alrededor de los contadores y entre extremos; puede faltar aunque tasa por proceso sea conocida. No demuestra credenciales constantes ni sesión causal. [Usuario/E/S](LINUX_USER_IO.md), [test](../tests/linux_user_io.cpp). |
| Network legacy | network_bytes_per_second / B/s | En colector Linux queda unsupported; NO se rellena con datos parciales de captura seleccionada. Informe separado descrito abajo. [Integración](LINUX_PROCESS_NETWORK_MONITOR.md), [test](../tests/linux_process_network_monitor.cpp). |
| Arquitectura | linux_details.architecture / etiqueta ELF | Clase/endian/e_machine del ejecutable fijado; reconoce combinaciones compatibles. No deducir x64 de64bits, ni arquitectura del CPU/host/Windows. Scripts/wrappers no identificables quedan desconocidos. [Descripción/ELF](LINUX_PROCESS_DESCRIPTION.md), [test](../tests/linux_process_details.cpp). |
| Description | linux_details.description, description_source / texto | Comment no localizado XDG asociado con exe por evidencia, estimated, no texto firmado/producto comercial. Ambigüedad, wrapper/argumentos no soportados, fuente cambiada o comentario ausente => desconocido. include_linux_process_descriptions=false desactiva el escaneo. [Contrato](LINUX_PROCESS_DESCRIPTION.md), [test](../tests/linux_process_metadata.cpp). |
| Token de vida | linux_start_ticks / ticks desde boot | Campo22 stat; 0 puede ser lectura válida, pero checked termination lo rechaza. No FILETIME/fecha ni token global. [Terminación](LINUX_TERMINATE.md), [test](../tests/linux_terminate.cpp). |
| Efficiency disponible | efficiency_mode_available / bool | Linux publica false conocido para la ruta generic/checked. No significa que todo preset avanzado/privilegio de OS sea imposible. [Decisión](LINUX_EFFICIENCY_DECISION.md), [test](../tests/linux_efficiency.cpp). |
| Efficiency requested | efficiency_mode_requested / bool | Campo Windows; no convertir ausencia Linux a false ni usarlo como comprobación de la variante avanzada. Mismo contrato de eficiencia. |

CPU normaliza contra todas las CPUs visibles: un proceso con un hilo ocupado
en32CPUs ronda3,125%, no100%. No se promete que sumar filas sea igual a CPU global:
ventanas y cobertura distintas. La nota histórica de [LINUX_PROCESSES](LINUX_PROCESSES.md)
que habla de dividir por ticks globales fue sustituida por D04, no usar como fórmula actual.

## Network seleccionado: datos parciales, no un total inventado

API `snapshot.linux_process_network` (`Metric<LinuxProcessNetworkSnapshot>`),
definida en [header público](../include/pcinsight/linux_process_network.hpp).
La bandera include_linux_process_network no inicia ni autoriza captura. Hace
falta setup explícito con consentimiento, kernel/permisos adecuados y selección
validada. Este ejemplo NO lo hace, ni siquiera con --sample.

| Dato | API / unidad | Regla del consumidor |
| --- | --- | --- |
| Disponibilidad/alcance | Metric externo, coverage | Comprobar padre antes de cualquier hijo. Estricto, buckets declarados y observados tienen alcance diferente; todos son operaciones TCP/UDP instrumentadas, NO tráfico total/interfaz/wire. |
| Identidad de dominio | observer_boot_token, observer, producer_initial_tgid, producer_start_boottime_ns | Comparar el dominio de observación, no UID/nombre/PID sueltos. Tokens no son credenciales/autorización. |
| Época y sweep | selection_epoch; counter_sweep_started_ns/finished_ns / CLOCK_MONOTONIC ns | Cambios/reset/readmisión rompen continuidad; no interpolar entre epochs ni mezclar clocks con boot ticks o CLOCK_BOOTTIME ns. No snapshot atómico entre lotes. |
| Identidad de cada bucket | processes[].process, processes[].kernel | Conservar procfs/namespace/inode/PID/start y TGID inicial/UID/boottime/cookie según proveedor. Un proceso puede tener varias filas/cookies. No join por PID+nombre ni suma por posición. |
| Bytes acumulados observados | payload_bytes_since_start / array4 de B payload | Orden TCPsend/TCPreceive/UDPsend/UDPreceive. Desde origen de captura, no toda vida del proceso ni historial anterior al setup. |
| Tasas por lane | payload_bytes_per_second[4] / B/s payload | Metric por lane, estimated. First/recovered/warmup ausente no es0. Un acumulado0 conocido no obliga tasa0 conocida. |
| Intervalo | nominal_interval_ns / ns | Intervalo nominal compartido, no simultaneidad perfecta de todos los contadores. |
| Actores sin buckets | observed_processes / filas | Modo observado puede tener actor admitido sin actividad positiva observada. No crear cookie ficticia ni bucket/tasa0. Lista vacía tampoco significa captura deshabilitada. |

El ejemplo imprime identidades/scopes y cuatro lanes en sección separada; NO
une ni suma sus filas en ProcessInfo.network_bytes_per_second. Las pruebas
incluyen mismo PID con start_ticks distintos, padre desconocido que contiene
datos, acumulados0 sin tasas y actor sin bucket. No prueban captura real BPF.
Para captura autorizada existente consultar [modo observado](LINUX_PROCESS_NETWORK_OBSERVED.md),
[selección](LINUX_NETWORK_SELECTION_OWNER.md), [admisión](LINUX_NETWORK_ADMISSION_MONITOR.md)
y [aceptación](LINUX_NETWORK_ACCEPTANCE.md). Su evidencia anterior se conserva,
no se reetiqueta como captura nueva de esta unidad. Máximos1024actores y16cookies
por actor son límites estructurales, no garantía de throughput/cadencia universal.

## Acciones del GUI: separadas del muestreo

El [header actions.hpp](../include/pcinsight/actions.hpp) ofrece SystemActions;
el ejemplo solo comprueba firmas al compilar, jamás crea/invoca el backend.
Los botones de Processes y Details comparten estas acciones, no nuevas copias
por panel. Run new task de Performance/App history/Startup/Users también es
la misma operación de lanzamiento, no un proveedor de métricas distinto.

| Botón | API | Confirmación, efecto y límites |
| --- | --- | --- |
| Run new task | launch_process(LaunchRequest) | Solo intención explícita. Executable/arguments/cwd son bytes sin NUL, argumentos literales; no concatenar texto como shell ni ejecutar el command de metadata automáticamente. PATH/cwd/env/FD heredados siguen contrato. No sandbox ni readiness. Scripts con shebang/intérprete explícito pueden ejecutarse por esa acción, **nunca para recopilar**. [Lanzamiento](LINUX_LAUNCH.md), [test](../tests/linux_launch.cpp). |
| End task preferido | terminate_process_checked(pid, start_ticks, force) | Confirmación explícita de pérdida de trabajo; muestra reciente mismo dominio y token conocido NO0. Rechaza0/1/self/rango, fija pidfd, revalida identidad y señala ese FD. force=false SIGTERM, true SIGKILL; no auto-escalar ni matar árbol/grupo. Éxito confirma salida en poll<=5s, no readiness/código de salida; timeout puede haber enviado señal. [Contrato](LINUX_TERMINATE.md), [test](../tests/linux_terminate.cpp). |
| End task PID-only | terminate_process(pid, force) | Usa pidfd desde apertura pero no protege intervalo muestra-apertura. No fallback automático desde checked si falla token/identidad/permisos. Mismo contrato/test. |
| Efficiency normal | set_efficiency_mode / set_efficiency_mode_checked | En Linux not_supported sin mutación. No prometer EcoQoS, checked seguro o ahorro físico garantizado. [Decisión](LINUX_EFFICIENCY_DECISION.md). |
| Efficiency avanzado opcional | apply_linux_efficiency_unchecked / restore_linux_efficiency_unchecked, linux_efficiency.hpp | Solo aceptación explícita del riesgo PID/TID numérico, OFF por defecto. Preset de threads capturados, nice/I/O, no cubre nuevos threads/descendientes. Puede dejar efectos parciales: conservar receipt y pendientes incluso al fallar; restauración explícita, nunca defaults fabricados ni rollback garantizado. [Contrato](LINUX_EFFICIENCY.md), [test](../tests/linux_efficiency.cpp). No se ejecuta en este ejemplo. |
| Auditoría opcional | AuditedActions | Registra llamadas que atraviesan el decorador, no autentica al usuario ni asegura persistencia inviolable. requested sin completed puede ser resultado desconocido. No audita automáticamente variantes standalone avanzadas. [Contrato](AUDIT.md), [test](../tests/audit.cpp). |

launch_process puede devolver process_id incluso ante error de canal incierto:
el padre debe recolectar ese hijo con waitpid/supervisor propio, sin reaper oculto.
No confundirlo con terminate_process_checked, que NO recolecta hijos ajenos; su
padre conserva esa responsabilidad. Ninguna acción solicita sudo/pkexec por sí
sola. L05 helper/consentimiento privilegiado real sigue aplazado y no se certifica
por el booleano de UI. Las rutas avanzadas no se habilitan porque un usuario
pida solamente sample, propiedades o actualizar la tabla.

## Ejemplo y validación

[linux_processes.cpp](../examples/linux_processes.cpp) usa exclusivamente SDK
público y un helper de formato de ejemplos compartido con Performance. Ayuda
por defecto; --self-test usa datos sintéticos sin SO ni asserts desactivables;
--sample enumera los procesos visibles pero imprime solo SELF. Esto NO optimiza
la recopilación a un solo PID. Descripciones XDG apagadas en ese smoke para
evitar el scan; CPU/RSS/estado/ELF/NSS siguen sus fuentes/permisos. NSS o syscalls
pueden bloquear: el alarm20s y CTest30s son límites del harness, no SLA de la API.

Salida terminal con campo/unidad/support/valor/note; controles ASCII escapados,
no formato JSON ni serializador UTF-8. Índices son locales a esa salida, no claves
del GUI. No imprime argumentos/cwd ni lanza/termina/nicea procesos. El
[consumer externo](../tests/process_consumer/CMakeLists.txt) compila el mismo
programa usando el SDK instalado, sin includes privados ni obligación de crear
ahora el CMake del GUI. La fuente compartida de formato también revalida los
tests Performance160. Sin ABI/layout ni comportamiento de la biblioteca nuevos.

Esta unidad no cierra App history/Startup/Users/Services ni sus acciones; su
trazabilidad sigue aparte. Tampoco cierra N03: auditoría/entrega global, riesgos
históricos no reproducidos/corregidos y aceptación física siguen pendientes.
SysV sin autorización explícita, L05/I07/Windows y G07/N02/B08 se conservan.

Verificación final: Linux Release 192/192, cuatro pruebas de ejemplos repetidas
diez veces, MSVC 20 recorridos portables y consumidor del SDK instalado con
self-test y muestra SELF exit0. 39 enlaces locales comprobados. Logs
`work/process-contract-*`. No es CI remota ni aceptación física adicional.
