# Prueba privada de capacidad eBPF (F03 en curso)

La sección siguiente conserva el modo original revisión28. Revisión29 añade
el modo clasificado con cuatro contadores TCP/UDP e IPv4/IPv6, flags excluidos
y pruebas sendmsg: [LINUX_NETWORK_CLASSIFIED.md](LINUX_NETWORK_CLASSIFIED.md).
El modo original sigue disponible y su regresión nativa fue positiva.

`src/linux_network_probe.hpp/.cpp` genera18instrucciones eBPF para una prueba
acotada. `tests/linux_network_probe_native.cpp` carga/adjunta y limpia sus
propios objetos mediante syscalls bpf, sin clang/libbpf/bpftool/utilidades.
No es el proveedor de producción: no se conecta al Monitor, al reductor de
contabilidad, a ProcessInfo/UserSession ni al historial. No hay API pública nueva.

## Autorización y ejecución

El usuario autorizó explícitamente el2026-10-18 sondas temporales en WSL,
filtradas a procesos privados y retiradas al terminar, con los privilegios
necesarios, sin configuración permanente ni instalar herramientas. La prueba
se ejecuta manualmente en un proceso root temporal. No asigna capacidades,
cambia sysctls, firewall, servicios, cgroups o configuración de arranque.

El ejecutable requiere `--authorized-private`; sin ese argumento devuelve77
sin cargar programa/mapa/enlace. No se registra como CTest: **la suite ordinaria
jamás lanza esta prueba privilegiada**. Ni construir ni usar Monitor adjunta
sondas. La autorización no se amplía a capturar procesos ajenos.

Antes de adjuntar, obtiene su propio TGID del kernel con un programa RAW_TRACEPOINT
no adjuntado, ejecutado por BPF_PROG_TEST_RUN sin contexto/datos ni flags de
CPU remota. Ese programa solo devuelve helper get_current_pid_tgid >>32 y
se cierra inmediatamente. No se supone que getpid() y ese helper usen el mismo
namespace: en la prueba observada PID visible508 era TGID kernel4492.
Si la identificación no puede verificarse, termina sin attach.

La sonda fexit filtra el TGID antes de leer el retorno del contexto o acceder
al mapa. Solo lee el retorno de esa operación privada; no lee el payload ni
los argumentos/socket de procesos ajenos. Cuenta retornos int32 positivos,
sign-extendidos, en dos contadores uint64 propios (envío/recepción). No altera
retornos ni escribe memoria del kernel excepto su propio mapa. No hay pinning.

BTF valida las firmas: __sys_sendto con6argumentos/slot de retorno48 y
sock_recvmsg con3argumentos/slot24. No se adivinan offsets de otra firma.
El programa usa BPF_PROG_TYPE_TRACING/BPF_TRACE_FEXIT y enlaces BPF_LINK_CREATE.
FDs de mapas/programas/enlaces tienen propietario RAII; el hijo privado cierra
sus copias heredadas antes de usar sockets. Se cierran enlaces antes de probar
detach y todos los objetos al salir, también en excepciones. No hay perf fds
ni fichero persistente que conserve una sonda.

## Qué se comprueba realmente

1. Mapa inicial0/0. TCP IPv4 loopback privado:1024bytes enviados y recibidos,
   contenidos comprobados y contadores verificados al terminar esa fase.
2. UDP IPv4 loopback privado:64bytes enviados/recibidos, incremento verificado.
   Total1088/1088. Son retornos de operación, no ACKs ni wire throughput.
3. Un hijo privado con otro TGID hace el mismo tráfico. Los contadores del
   TGID permitido no cambian: negativo explícito de atribución/filtro.
4. Cierra ambos enlaces, repite tráfico en el proceso permitido y comprueba
   que los contadores no cambian: negativo explícito de retirada.

El generador también tiene CTest portable con un intérprete mínimo de su
subconjunto de instrucciones: filtro antes de contexto/mapa, éxito, cero,
errores negativos/signo int32, máximo int32, lookup nulo, slots/jumps y
parámetros inválidos. No sustituye al verificador del kernel.

## Hallazgos y límites que no deben ocultarse

El primer intento de identificar el TGID usaba un programa SOCKET_FILTER no
adjuntado. El kernel rechazó helper14 como unknown func; no hubo attach en
ese intento. Se corrigió a RAW_TRACEPOINT no adjuntado, cuya ejecución local
por TEST_RUN permite obtener la identidad en el contexto del llamante.

El primer par fexit sock_sendmsg/sock_recvmsg sí cargó, pero dio0/1088 en la
prueba. No se consideró éxito: send/sendto elude sock_sendmsg en este camino,
como muestra el código primario de Linux6.6. Se probó resolver __sock_sendmsg,
pero no existe en el BTF observado (inline posible, no demostrado), y se rechazó
sin fallback ciego. Finalmente se validó __sys_sendto, con su propia firma y
slot de retorno, y el positivo1088/1088 pasó junto con filtro/detach.
Los logs iniciales de fallo/ausencia se conservan bajo work/network-probe-*.

Esto NO cubre sendmsg/sendmmsg/write/splice/io_uring/async ni todos los caminos
de recepción. La sonda no distingue familia/protocolo en el mapa: el fixture
elige TCP/UDP IPv4 y verifica fases, no declara clasificación general. No trata
MSG_PEEK/MSG_TRUNC, pérdidas, saturación/overflow, persistencia, PID reutilizado,
UID de operación, start_boottime o cookie del namespace de cada socket.
El proceso permitido permanece vivo durante toda la prueba, sin reutilización
de PID; no es un token seguro reutilizable para capturar terceros.

Por eso no alimenta automáticamente al reductor revision27: no satisface su
contrato completo de cuatro lanes/coherencia/identidad/coverage. Sigue pendiente
el productor real, su cobertura y privilegios tipados, filtros de producción,
unión con procfs/apps/users, ventanas/integración y más kernels/hardware.
F03 sigue E, J04/H04 abiertos; no se crea un ID resuelto para subir porcentaje.

Resultado final: Linux100/100 y Windows114/114 Release; consumidores instalados
ambos relinkados/ejecutados exit0. `work/network-probe-verified-native-linux.log`
registra el positivo con fases verificadas (PID293/kernelTGID7067), exclusión
del hijo y retirada, exit0. `network-probe-no-authorization-linux.log` registra
el rechazo77 sin flag, sin carga. Las suites normales no ejecutan sondas.

`network-probe-final-build-linux.log` y `network-probe-final-suite-linux.log`
confirman comprobación incremental final sin warnings y100/100. Un build
anterior tuvo avisos de clock skew de6-12ms en compiler_depend.make generados;
se conservan en `network-probe-verified-build-linux.log`, no se cambió el reloj
ni se ocultaron. Windows `network-probe-final-build-windows.log` sin warnings y
`network-probe-suite-windows.log`114/114.77significa ausencia/no positivo,
nunca éxito. Los riesgos históricos N03 no se declaran corregidos.

Referencias primarias: [tipos de programa/attach](https://docs.kernel.org/bpf/libbpf/program_types.html),
[BPF_PROG_RUN](https://docs.kernel.org/bpf/bpf_prog_run.html),
[implementación TEST_RUN raw_tp Linux6.6](https://raw.githubusercontent.com/torvalds/linux/v6.6/net/bpf/test_run.c),
[caminos socket Linux6.6](https://raw.githubusercontent.com/torvalds/linux/v6.6/net/socket.c),
[UAPI BPF](https://github.com/torvalds/linux/blob/v6.6/include/uapi/linux/bpf.h).
