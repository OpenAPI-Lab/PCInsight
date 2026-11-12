# Ventana nominal comun y sumas parciales UID

Revision73 agrega `LinuxMultiNetworkSampler::sample_common()` al componente
[multiactor seleccionado](LINUX_NETWORK_MULTI.md). Sigue siendo interno, no
instalado/publicado en Monitor, ProcessInfo, UserSession o historial. Sin nueva
dependencia/GUI/Windows/API/ABI publica. F03 E; H04/J04 permanecen abiertas.

## Orden de muestreo

Primero hace joins pidfd/procfs frescos de TODOS los actores y valida identidades
fijas/contexto/boot/observador. Despues toma CLOCK_MONOTONIC, lee cada mapa de
contadores con los checks de calidad existentes y vuelve a tomar el reloj.
El barrido completo debe durar como maximo1s, tener inicio positivo/ordenado y
no superponerse al barrido anterior. Los joins costosos quedan FUERA de ese
bracket, no se trasladan los timestamps de una muestra independiente anterior.

Por ultimo repite joins frescos de todos, exige contextos iguales a los previos
al barrido y mismo caller, y SOLO entonces entrega todas las filas al reductor.
Productores/boot/dominios de cada captura deben coincidir. Forma de seleccion
fija1..16 y dedup inicial se conservan; cobertura nueva selected_actor_set tambien
rechaza cero/>16 filas, TGID/start invalidos o un mismo TGID/start con otro
UID/cookie. No se suma dos veces el mismo proceso.

Ante cualquier ausencia/fallo/excepcion/contexto cambiado descarta TODOS los
baselines, el bracket previo y las referencias individuales. La recuperacion
estable inicial no tiene tasas. Alternar sample (independiente) y sample_common
tambien descarta referencias; no hay tasas que crucen modos. stop/reinicio/fork
mantienen la propiedad/limpieza de pidfds y capturas previas.

## Que significa comun

Todas las filas usan el final de este barrido como timestamp NOMINAL comun;
el intervalo es la diferencia entre finales consecutivos. Cada tasa es
floor(delta_bytes*1e9/intervalo_ns), entera/comprobada y Support::estimated.
Es una estimacion alineada, NO adquisicion simultanea/atomica de todos los mapas.
Cada fila se leyo en algun momento dentro del bracket, cuyo ancho queda expuesto.

`PrivateCommonNetworkWindow` conserva accounting completo, bracket actual y
previous_read_bracket opcional. Con ambos se ve el desfase de lectura: el tiempo
entre adquisiciones de una fila esta acotado por inicio_actual-fin_previo y
fin_actual-inicio_previo, no necesariamente por el intervalo nominal exacto.
El limite1s es del barrido, no plazo duro del proveedor ni de joins/syscalls.
No se promete frecuencia de muestreo/rendimiento, especialmente con16actores.

associations contiene clave NetworkActor, contexto procfs y calidad en orden de
seleccion. accounting.actors usa orden del reductor; se asocia POR CLAVE, nunca
por posicion de vector. UID inicial kernel de la operacion permanece separado
del UID procfs/namespace del observador y de un nombre de usuario/sesion.

El reductor agrupa por UID inicial y suma las tasas de sus buckets seleccionados
ya alineadas (suma de floor por actor, no floor de bytes agregados). known/missing
por lane y overflow siguen visibles. Sin buckets con tasa medida, la suma queda
ausente, no cero. Desbordamiento conserva ausencia/flag, no wrap/saturacion. Una
fila sin baseline por reset de contador/intervalo>60s puede tener tasa ausente;
las otras tasas no pasan a representar el UID completo. Usuarios distintos se
mantienen separados. La suma UID siempre es parcial y estimated.

## Cobertura y evidencia

enumeration_complete solo afirma la lista EXPLICITA de actores/cookies, no
todos los procesos/usuarios/sockets del equipo. Cookie aun declarado, un cookie
por proceso, sin discovery/membresia dinamica/global. Se heredan BTF/FEXIT/
privilegios/ptrace/mascara temporal caller, misma tabla FD/serializacion y
limites de extremos no atomicos/ABA/vida futura/no plazo duro. No nuevo ptrace
attach/senal/parada objetivo ni configuracion permanente/paquetes/CLI de captura.

Fixtures ampliados en linux_network_multi y linux_network_accounting: un UID
con2actores,2UID distintos, mismo intervalo nominal,4oraculos actor/UID, brackets,
cambio de modo/fork,12fallos de muestra con reset/recuperacion sin tasa sobre
hueco (incluido cambio tras barrido), limite1s/intervalo>60s y overflow UID.
Reductor rechaza alias TGID/start con UID/cookie diferentes y cardinalidad invalida.
Ningun CTest carga BPF real. El total de CTests no se infla por ampliar casos.

Manual `--authorized-private-common-window`, NO CTest:2hijos privados generan
A4096/4096/256/256B y B8192/8192/512/512B, claves/contextos revalidados, cuatro
oraculos de actor y cuatro sumas UID con intervalo nominal comun, brackets actual/
previo conservados, idle/padre excluido, entrada FD cerrada, duplicado/fork/salida
rechazados, limpieza sin fugaFD. Watchdogs harness60s padre/50s hijos no son
plazos duros BPF. No cambia UID/namespaces/configuracion; permiso previo solo
para procesos privados, no captura global. `work/network-common-native-linux.log`.

Linux124/124 Release sin skips,16tests red repetidos20veces cada uno, consumidor
instalado reconstruido/ejecutado exit0 y build final sin warnings. Regresiones
nativas selected e independent tambien exit0 con el binario final; selected
PID388/kernel4939 UID0, intervalo1.143744392s, oraculos/reinicio/fork/limpieza.
Se ampliaron CTests existentes, no nuevo denominador ni credito por test.
Resultados finales y regresiones nativas anteriores: control oficial revision73
y logs `work/network-common-*`. Solo positivo real WSL x86_64, no validacion
universal de hardware/distros/CI. Siguiente F03: contrato de cobertura y proveedor
publico opt-in seleccionado sin presentar estas sumas como trafico total.
