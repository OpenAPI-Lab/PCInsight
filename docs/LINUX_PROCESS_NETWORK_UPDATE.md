# Cambiar la seleccion explicita de red Linux

Revision76 permite anadir, retirar y reordenar procesos sin recrear las capturas
que mantienen PID local, start_ticks y cookie de namespace del socket. Es una
lista completa de reemplazo, no una lista de diferencias ni descubrimiento
automatico. Continuan el limite1..16 y un cookie declarado por proceso.

```cpp
// red ya tiene una captura activa. Cada llamada necesita consentimiento nuevo.
auto cambio = red.update_selection(consentimiento, nueva_seleccion);
// Equivalente en Monitor, serializado con sample/start/stop:
auto otro = monitor.update_linux_process_network_selection(consentimiento,
                                                           nueva_seleccion);
```

Son metodos ADITIVOS de LinuxProcessNetworkSampler y Monitor. Esta revision no
cambia layouts publicos; sigue siendo necesario recompilar todos los consumidores
que aun usen layouts anteriores a revision75. Para usar los metodos nuevos se
necesita enlazar la biblioteca actual. No requiere CMake en el proyecto consumidor.

## Identidad, recursos y fallos

La seleccion debe contener1..16 PID distintos, pidfd no negativo, start_ticks y
cookie no nulos. En filas retenidas se usa el pidfd PROPIO ya duplicado, no se
consulta el numero de FD del caller que pudo cerrar despues del inicio. Para
adiciones/reemplazos el pidfd prestado tiene que seguir valido durante la llamada;
se duplica y valida con las mismas comprobaciones kernel/procfs que start.
Cambiar start_ticks o cookie implica cerrar la captura anterior e iniciar otra.
Una fila retirada no se revalida: permite quitar un proceso ya salido/inaccesible.
Las filas que quedan se revalidan, sin inventar continuidad de identidad.

False consent no modifica recursos. En el proveedor individual una lista mal
formada se rechaza antes de modificar captura. Tras comenzar la reconciliacion,
cualquier denegacion, identidad/contexto incoherente, error o excepcion cierra
TODAS las capturas: no vuelve silenciosamente a una seleccion anterior. Monitor
aplica una politica aun mas estricta: CUALQUIER fallo de un update autorizado,
incluso lista mal formada/proveedor ausente, detiene su captura y conserva la
nota de fallo en el informe. No usar update para iniciar una sesion detenida.
Para seleccion vacia usar stop, no un informe vacio presentado como disponible.

Se cierran retiradas antes de crear reemplazos. La operacion NO es un intercambio
atomico ni garantiza captura sin huecos. Las capturas retenidas siguen contando
mientras se reconcilia; las nuevas solo cuentan desde su propio inicio. No se
retiene el span ni un pidfd prestado. No hay thread nuevo, elevacion automatica,
CLI, paquete, servicio o captura de otros procesos.

## Ventanas y origen de los contadores

Cada exito crea otra selection_epoch y descarta TODOS los baselines/brackets,
tambien al reordenar o repetir una lista identica. La primera muestra siguiente
tiene contadores, pero NO tasas ni intervalo; la segunda puede volver a tener
tasas. No unir diferencias entre epochs ni periodos sin datos.

Los contadores payload_bytes_since_start de actores retenidos conservan el
origen de SU captura, no se ponen a cero al cambiar selection_epoch. Los actores
nuevos empiezan con sus propias capturas, sin trafico retroactivo del hueco.
La suma por UID sigue siendo parcial/estimada del conjunto seleccionado actual,
no trafico total de una cuenta. No llena campos legacy de proceso/app/sesion.

Monitor es de uso serializado. Revision77 agrega
[actualizacion segura del worker](LINUX_PROCESS_NETWORK_WORKER.md) con el mismo
nombre update_linux_process_network_selection en Sampler; start no reconfigura.
El contrato detenido/setup explicito/restart sin reautorizar sigue igual. Faltan
descubrimiento/politica dinamica, buckets namespace/cobertura general antes de
cerrar F03; H04/J04 abiertas.

## Evidencia

Mismos CTests linux-network-multi y linux-process-network-monitor, asserts en
Release: AB->B->AB, retencion sin recrear FDs, reorden/identico, cambio de cookie,
consentimiento/lista invalida sin callbacks, fallos de adicion/revalidacion/
duplicado kernel/contexto/fork/excepcion, reinicio obligatorio tras fallo y
retiro de fila inaccesible. Reducer ordena por clave; las asociaciones conservan
orden de seleccion. Un primer assert que suponia el orden del reducer fue
corregido, sin cambiar ese contrato; log targeted inicial preservado.

Manual NO CTest --authorized-private-update-window en WSL: solo dos hijos
privados ya autorizados, sin token kernel reportado por ellos. Monitor usa la API
publica: AB->B->AB/reorden, FDs retirados cerrados, contador retenido conservado,
trafico del hueco excluido para A, cuatro oraculos actor/UID de payload y tasas,
epochs/baselines nuevos, denegacion sin cambio, fork update rechazado, retirar B
ya salido permite recuperar A, limpieza sin fugaFD. Intervalo inicial medido
2.215693931s, no garantia de cadencia. Watchdogs parent60s/child50s, no plazo duro
para operaciones BPF. work/network-update-native-linux.log y logs de build/tests
del mismo prefijo. No prueba de otros kernels/distros/hardware ni captura global.

Linux126/126 Release sin skips,2tests modificados repetidos20veces cada uno;
build completo sin warnings, instalacion y consumidor externo reconstruido/
enlazado/ejecutado exit0. No nuevos CTests ni cierres parciales:57/75=76,0%,
18abiertas=24,0%. El fallo nativo del consumidor general MSVC de revision75 sigue
sin causa demostrada/N03 abierto; no se afirma compatibilidad Windows total.
Compatibilidad comun MSVC76: biblioteca y2pruebas focalizadas compiladas,2/2
pasan, sin warnings C++ nuevos (D9025 esperado /UNDEBUG conserva asserts).
No se repite ahora suite Windows completa ni consumidor general Windows.
