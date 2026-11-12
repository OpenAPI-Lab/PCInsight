# Consulta de identidad pidfd reutilizable y desarmada entre consultas

Continuacion revision89: [productor compartido INTERNO](LINUX_NETWORK_SHARED_PRODUCER.md)
ya validado con17actores privados y tres links de trafico; aun sin integracion
de joins/ventanas/tasas/updates/publico. La consulta de identidad de este documento
sigue en el proveedor existente; no se elimina ninguna comprobacion.

Revision88, 2026-10-22. Unidad interna F03 terminada y validada; F03 completa
sigue EN CURSO. Sin cambio de API/layout publico, GUI, persistencia, paquetes,
CLI de recoleccion ni configuracion permanente. Limites64/16/1024 no cambian.

## Recursos reutilizados, identidades siempre actuales

LinuxPidfdIdentityQuery conserva dos mapas ARRAY y tres programas cargados
(caller, thread y fexit). El programa fexit usa un mapa de seis slots: los cinco
resultados existentes y una compuerta caller_pid_tgid exacta en slot5. Con cero
o con otro hilo/proceso, sale ANTES de leer el contexto del objetivo. No hay
iterador de tareas ni enumeracion global. El link dura la sesion, NO esta armado
fuera de una consulta sincrona a pidfd_getfd(selected,-1,0).

Cada begin borra el mapa caller y ejecuta de nuevo las sondas caller/thread;
valida TGID/UID/inicio/TID, borra TODOS los resultados y arma solo al hilo actual.
Un cambio de hilo no requiere cargar ni adjuntar otra sonda. El wrapper nativo
duplica el pidfd, comprueba liveness, bloquea/restaura senales y hace la consulta
invalida sin duplicar un archivo del proceso objetivo. Se siguen exigiendo
mode ATTACH_REALCREDS y resultado de permiso0. Se comprueban errores del syscall,
calidad de la sonda y vida del proceso; no se conserva un permiso ni identidad
anterior como resultado de la nueva consulta.

Finish desarma ANTES de leer los resultados. El mismo hilo no puede ejecutar
otra llamada en paralelo a su consulta sincrona; otras hebras no coinciden con
la compuerta. Si el reset/armado/lectura falla, no hay resultado parcial. Si no
se puede desarmar, se retiran link/programa y se rechaza el resultado. Antes de
adjuntar un reemplazo se borra explicitamente cualquier compuerta residual.
Excepciones y salidas tempranas cancelan la transaccion. Stop/destruccion cierra
todos los recursos. Una copia fork se rechaza y cierra SUS referencias, sin
escribir los mapas compartidos del padre, incluso si hereda estado armado.

El join conserva DOS observaciones kernel independientes y todos sus brackets
procfs/pidfd/boot/contexto. El sampler sigue haciendo joins antes Y despues de
leer contadores. Los sources inyectados antiguos conservan join/join_layout;
solo el source nativo opta por join_query con recursos propios del sampler.
La llamada standalone previa conserva su ruta temporal original.

Serializar llamadas en la misma tabla de descriptores; no manipular FDs propios,
unshare ni usar cancelacion asincrona. No hay plazo duro garantizado para
syscalls. Recursos/links sin pinning, solo mientras vive la sesion autorizada.
No se inicia captura ni se elevan permisos automaticamente.

Se usa BPF_MAP_UPDATE_ELEM directo (callback interno opcional); los sources que
no necesitan esta optimizacion siguen sin exigir ese callback. La operacion de
actualizacion de mapas esta documentada por el [kernel Linux](https://docs.kernel.org/bpf/maps.html).

## Pruebas y resultados preservados

- Fixture linux-network-pidfd:40 transacciones con solo tres cargas y un adjunto,
  cambio de hilo sin nuevas cargas, identidad de salida fresca, resultado viejo
  borrado/sin trigger ausente, conteo ambiguo rechazado, fallos de inicio/API/
  actualizacion/run/lookup/adjunto/excepcion, fork y desarmado fallido con
  recuperacion. Interprete de bytecode: gate0/otro PID/otro TID/mapa ausente no
  lee contexto; modo/permiso/fallos instrumentados mantienen sus rechazos.
- Prototipo intermedio con adjunto/retirada POR CONSULTA: FALLA nativo17
  (`work/network-query-reuse-native17-linux.log`, start21,4s, sin terminar
  primera lectura antes de vida55s hijos). Se preserva, NO se cuenta aprobado.
  Hubo compilaciones simultaneas: no es benchmark ni comparacion causal aislada.
- Diseno final con compuerta: PASS nativo17
  (`work/network-query-gated-native17-linux.log`): preparado8ms, start1593ms,
  vacio1620ms, primer lote1647ms, segundo1676ms, stop2986ms desde inicio.
  Diferencias entre fases27/27/29ms INCLUYEN IPC/carga privada; no son una
  medicion aislada de syscall ni garantia de latencia universal. Se mantuvieron
  watchdogs55/60s; no hubo compilaciones simultaneas en este ensayo.
- Los17 hijos privados UDP64..80B pasaron cuatro tasas/intervalo nominal comun,
  bytes por actor, UID parcial, primera tasa ausente, trafico padre excluido,
  owner liberado mientras captura conserva sus referencias, todos los hijos
  salieron normalmente y censo FD final sin fugas. Un namespace real compartido,
  SO_NETNS_COOKIE como oracle independiente.64/1024 siguen siendo fixtures.
- PASS nativo Sampler observado con dos hijos, cambio main/worker/updater,
  AB->B->AB concurrente/historial/epochs, retained origin/sin gap, fallo de update
  cierra captura sin matar worker, restart no rearma, FD limpios.
  `work/network-query-gated-worker-linux.log`.
- PASS estricto nativo: cuatro tasas/UID/TCP4096B y UDP256B por direccion,
  exclusiones peek8, padre excluido, fork rechazado/padre intacto, restart,
  stale PID/inicio/exit y FD limpios. Prueba standalone adicional restaura mascara
  de senales. `work/network-query-gated-strict-linux.log`.
- Linux128/128 Release sin skips; seis pruebas focales20 veces (120 ejecuciones),
  consumidor externo instalado reconstruido y ejecutado exit0. MSVC6/6 focales;
  builds finales sin warnings C++ nuevos (D9025 de assertions esperado).
  Logs `work/network-query-gated-*`. No suite Windows completa; N03 consumidor
  revision75/0xC0000409 sigue abierto.

## Lo que NO queda terminado

No hay productor de trafico BPF compartido, admision/retiro automaticos del
inventario ni captura general de todos los procesos/apps. Sigue una captura por
actor seleccionado, registros limitados y tasas estimadas/sumas UID parciales.
No hay certificado de64 actores reales, multiples namespaces reales ni soporte
de todos los kernels/distro/privilegios. Siguiente unidad: productor multiactor
compartido y cobertura/capacidad explicitas, antes de admision automatica.
Pruebas siguen restringidas a actores privados autorizados.

Control oficial:57/75=76,0% cerrado;18/75=24,0% abierto. F03/H04/J04 no se cierran
con esta mejora. Los fallos de revision87 se conservan como evidencia historica.
