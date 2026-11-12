# Actualizar red seleccionada con el Sampler activo

Revision77 agrega Sampler.update_linux_process_network_selection. Actualiza la
captura EXISTENTE sin detener el worker ni borrar las graficas de CPU/memoria.
Hereda lista completa1..16, un cookie declarado por proceso, identidad/permisos/
cobertura parcial y politica fail-closed de [Monitor](LINUX_PROCESS_NETWORK_UPDATE.md).
No descubrimiento automatico.

Revision79 permite elegir [modo buckets al hacer setup](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.md)
con tercer argumento. Update conserva modo de sesion; para cambiarlo detener y
repetir setup. Dos argumentos siguen estrictos, incluso tras detener buckets.

```cpp
pcinsight::SampleOptions opciones;
opciones.include_linux_process_network = true;
pcinsight::Sampler fondo(std::chrono::seconds(1), 0, opciones);
auto inicio = fondo.start_linux_process_network(consentimiento, seleccion);
if (!pcinsight::metric_is_known(inicio)) return;
fondo.start();
auto cambio = fondo.update_linux_process_network_selection(consentimiento,
                                                           nueva_seleccion);
// Esperar muestras con selection_epoch == cambio.value si el cambio tuvo exito.
// La primera de esa epoch no tiene tasas; no conectar series entre epochs.
fondo.stop();
```

Funcion sincronica: no retiene span ni pidfds prestados despues de volver. Para
filas retenidas usa el pidfd propio; adiciones requieren pidfd valido durante la
llamada. Calls y worker deben compartir tabla de descriptores (threads normales
lo hacen): no unshare ni cerrar/reemplazar FDs propios de la biblioteca. No usar
Sampler/jthread heredado tras fork, ni mover/destruir concurrentemente con sus
metodos. start/stop/update se serializan; history/latest/running pueden leerse
concurrentemente.

## Orden y disponibilidad

Bloqueo lifecycle -> Monitor -> historial. Captura/muestra y publicacion terminan
bajo el mismo bloqueo Monitor: update no vuelve antes del commit de la muestra
anterior. Worker no toma lifecycle ni lo necesita para finalizar; stop no espera
al worker sosteniendo Monitor.

Update pendiente se anuncia ANTES de esperar Monitor: worker cede en su proxima
frontera y no inicia muestras sucesivas mientras update espera. Reanudacion se
publica/notifica manteniendo Monitor, sin perder notificacion entre comprobar
predicado y esperar. Esto evita readquisicion repetida delante del update, pero
NO da plazo duro: BPF/procfs en curso puede tardar. Timestamp de muestra empieza
DESPUES de obtener Monitor; la espera no se presenta como tiempo de coleccion.
No fuerza muestra inmediata ni cambia cadencia: siguen intervalo configurado y
coste real del recolector.

False consent retorna permission_required sin tocar captura. Sin flag, worker
detenido, objeto movido o captura inactiva se rechaza: update nunca inicia ni
rearma. Exito nueva epoch/resetea TODOS baselines; retiene origen de contadores
de filas mantenidas y no recupera bytes anteriores de nuevas filas. Fallo
autorizado de Monitor, incluso lista invalida, cierra captura pero NO detiene
worker de otros grupos. Informes siguientes quedan ausentes con motivo; recuperar
exige stop/setup explicito/start. start del worker solo no reautoriza;
stop/fin/error cierran captura.

## Historial no es estado de captura

No se borra ni reescribe historial al actualizar. latest puede seguir siendo la
ultima muestra de otra epoch hasta la siguiente, como despues de stop conserva
datos historicos. Al recibir cambio.value, esperar informe conocido con esa
selection_epoch. Tras fallo, mostrar error devuelto sin interpretar latest
anterior como captura activa. timed_history conserva epochs y huecos: GUI debe
separar series, no restar contadores ni conectar tasas entre ellas. Otras metricas
y ventanas60s/capacidad/expiracion no cambian. No nueva persistencia de apps ni
cambio de formato HistoryStore.

## Verificacion y limites

Mismos CTests sampler-lifecycle/sampler-window/linux-process-network-monitor:
start/stop concurrentes con40updates/lecturas, false consent, movido, detenido,
deshabilitado, worker sin captura no rearma, continuidad de otras muestras.
Checks require/throw o asserts activos en Release, esperas acotadas.

Manual NO CTest --authorized-private-worker-update: dos hijos privados ya
autorizados, AB->B->AB con worker activo, updater en otro thread e historial
concurrente, primeras muestras sin tasas, origen B conservado, trafico de A
durante ausencia no recuperado, cuatro oraculos tasas actor/UID parcial.
Update invalido cierra captura/worker continua; update detenido/restart solos no
rearman; stop deja FD originales/ambos hijos terminan normalmente. Watchdogs
parent60s/child50s, no limite duro syscall BPF.

Primera prueba nativa fallo al retirar una fila. Revision de sincronizacion
elimino posible inanicion por readquisicion del mutex con worker ocupado;
despues paso la prueba nativa. Se ajusto ademas clear/notify para conservar mutex
durante reanudacion. Logs iniciales preservados, sin ocultar fallo. Evidencia
final work/network-worker-final-* y consumidor/instalacion del mismo prefijo.
Sin herramienta instalada ni captura ajena/global.

API aditiva, sin layouts publicos nuevos. Enlazar biblioteca actual para metodo
nuevo; recompilar todos si se viene de layouts previos a75. F03 abierta por
politica/descubrimiento automatico, buckets namespace/cobertura general. H04/J04
mantienen integraciones pendientes; sin totales legacy parciales ni Windows nuevo.

Resultado final: Linux126/126 Release sin skips,3tests focalizados20repeticiones
cada uno, consumidor instalado reconstruido/enlazado/ejecutado exit0 y build sin
warnings. MSVC3/3 focalizadas con biblioteca actual/sin warnings C++ nuevos;
no suite completa/consumidor Windows nuevos, fallo consumidor75 sigue N03. Control
oficial revision77:57/75=76,0% cerrado,18/75=24,0% abierto; sin credito parcial.
