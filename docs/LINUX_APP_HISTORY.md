# Historial observado de CPU Linux (H01)

Cierre funcional2026-10-16. `SystemSnapshot::linux_app_history` es un
`Metric<vector<LinuxAppHistoryEntry>>` separado del historial legacy. Cada fila
expone `id`, `name`, `observed_cpu_time`, `network_bytes`, `notifications` y
`period_start/period_end`. CPU es una duracion acumulada en nanosegundos, NO un
porcentaje ni una grafica de60s. La ventana del Sampler se cerro despues en A03;
ver SAMPLER_WINDOW.md. No es la retencion del historial acumulado por app.

## Uso y ciclo de vida

```cpp
pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
options.include_app_history = true;
options.include_processes = false; // oculta filas, no la captura interna
auto baseline = monitor.sample(options);
// Esperar el intervalo elegido por el consumidor y volver a muestrear.
auto snapshot = monitor.sample(options);
if (snapshot.linux_app_history.value) {
    for (const auto& app : *snapshot.linux_app_history.value) {
        if (app.observed_cpu_time.value) {
            auto ns = app.observed_cpu_time.value->count();
            // Usar ns y conservar support/note; no sustituir ausencias por0.
        }
    }
}
```

El llamador serializa llamadas al mismo Monitor; no se ofrece acceso concurrente
sin sincronizacion externa. Solo se captura mientras `include_app_history=true`.
Una muestra con la opcion desactivada borra acumuladores, fracciones y baselines.
Reactivarla empieza otro periodo. Construir otro Monitor tambien empieza de cero.
No hay hilo oculto ni escrituras automaticas. Ocultar la lista de procesos no
desactiva historial; activar solo procesos/usuarios NO acumula historial.

Primera observacion de una ruta: fila con CPU ausente, no tiempo0 inventado.
Dos observaciones consecutivas validas con delta0 si publican0 estimated.
Una lista vacia valida significa que no hubo rutas elegibles, no fallo del informe.
Las filas anteriores permanecen al salir una app. Repetir una observacion no suma
el tiempo dos veces. CPU ya confirmado se conserva ante un hueco de captura.

## Identidad, alcance y disponibilidad

Identidad de agrupacion: ruta absoluta exacta observada de `/proc/PID/exe`, entre
usuarios visibles. No se agrupa por comm, paquete o marca comercial, ni se
canonicaliza una supuesta identidad global entre namespaces. `name` es el nombre
kernel de la primera observacion (o la ruta si falta), no descripcion ni publisher.
Dos programas distintos instalados sucesivamente en la misma ruta comparten fila;
el cambio de archivo rompe el intervalo de ese PID, no crea identidad de paquete.

Cada intervalo exige PID/start_ticks iguales, utime y stime no decrecientes,
sumas comprobadas, reloj steady propio creciente, ruta y stamp completo del
ejecutable iguales. Stat y ejecutable reutilizan los lectores verificados de D03
y el helper de deltas D04. Se compara metadata de archivo, no una generacion exec
universal. Un exec del mismo archivo o cambios ABA entre extremos pueden pasar
inadvertidos. No hay snapshot atomico de todos los procesos/campos.

PID reutilizado, exec detectado, reinicio individual, ausencia/denegacion de stat
o exe, desaparicion o reloj no creciente: no se atribuye ese intervalo. La proxima
observacion elegible sirve como referencia. Procesos que nacen y mueren entre
muestras, tiempo antes de la primera muestra y procesos invisibles no se
reconstruyen. `Support::estimated` declara ese subconjunto observado, no el total
historico del sistema ni recursos de un usuario concreto.

Se suman utime+stime; guest ya esta incluido en utime. No se agregan otra vez
guest ni cutime/cstime de hijos. Duracion no normalizada por numero de cores,
cuota cgroup, afinidad o frecuencia. El contexto CPU valido aporta evidencia de
boot y esquema; no se exige delta global positivo ni se usa como denominador.

`network_bytes` y `notifications` quedan ausentes en ESTE informe CPU con nota
explicita; falta de soporte no es cero medido. H04 se cerro en revision100 como
informe separado linux_network_app_history (LINUX_NETWORK_APP_HISTORY.md), y H05
como observador separado de notificaciones (LINUX_NOTIFICATIONS.md). No se unen
periodos por coincidir rutas/hints. El informe Linux no esta
disponible en Windows, que conserva su propio `windows_app_history`.

## Conversion exacta, errores y limites

Por ruta se conserva un resto fraccionario: cada intervalo agrega
`floor((delta_ticks * 1e9 + resto_anterior) / CLK_TCK)` nanosegundos y conserva
el modulo CLK_TCK. Asi tres ticks individuales con3Hz suman exactamente1s;
no se divide antes de multiplicar ni se pierden fracciones entre procesos de
una misma ruta. Conversion portable sin uint128: parte entera comprobada y
nueve digitos decimales con aritmetica modular acotada. Se comprueban conversion
y suma contra el maximo de `chrono::nanoseconds::rep`, sin wrap ni saturacion.

CLK_TCK debe ser positivo y constante; cambio de boot (incluida presencia de su
token) o unidades exige reiniciar la captura explicitamente. Contexto ausente o
invalido, reloj wall no positivo/retrocediendo, PID0/duplicado y entradas excesivas
rechazan el informe con nota. Limites:100000 observaciones, nombre16384B,
ruta65536B, texto entrante16MiB,4096 rutas retenidas, texto retenido4MiB.
Son limites de memoria/entrada, no un deadline global de procfs.

Actualizacion interna transaccional: error de validacion/overflow devuelve
informe ausente unsupported, conserva filas y restos confirmados y rompe las
baselines. No devuelve resultados parciales. Una llamada posterior valida puede
volver a exponer los acumulados; el intervalo fallido no se recupera. Errores
inesperados se propagan tras romper continuidad. No hay transaccion atomica con
todos los otros grupos de Monitor: una excepcion en otro proveedor no convierte
automaticamente todas sus capturas en una sola operacion transaccional.

`period_start` es el timestamp de captura de la primera muestra aceptada del
Monitor; `period_end` por fila se actualiza al crearla o aceptar un intervalo.
Una app ausente conserva su ultimo extremo. Son tiempos wall de captura, no
limites exactos de ejecucion CPU ni garantia de duracion continua observada.

## Persistencia y compatibilidad

`snapshot.app_history` legacy queda vacio en Linux. HistoryStore acepta solo
AppHistoryEntry legacy y NO serializa este informe ni sus banderas de soporte.
No convertir red/notificaciones desconocidas a0 para guardarlas alli. H02 sigue
cerrado para su alcance legacy, no para un nuevo formato Linux. El consumidor
que necesite persistir este informe elige su archivo y conserva disponibilidad/notas.
Desde revision99 LinuxHistoryStore conserva estas
filas con disponibilidad/notas sin convertir ausencias a0; ver LINUX_HISTORY_STORE.md.
Desde revision100 H04 integra F03 en un informe/periodo independiente: guardar
CPU y red en archivos distintos, no fusionarlos por ruta. A01 se cerro despues
con la vista comun AppHistorySample, alias legacy explicito e importacion
conservadora (METRIC_AVAILABILITY.md). A05/API1.0 y auditoria final N03 siguen
abiertos; la vista no crea un formato nuevo de almacenamiento Linux.

Nuevo layout publico0.x: LinuxAppHistoryEntry y campo al final de SystemSnapshot.
Recompilar libreria Y TODOS los consumidores con cabeceras instaladas nuevas;
no mezclar objetos antiguos con la libreria nueva. No se promete ABI estable.

## Evidencia

`tests/linux_app_history.cpp`: fracciones3Hz/7Hz/LONG_MAX, varias instancias,
replay, PID/start/archivo/ruta, permisos/ausencias, retrocesos individuales y de
reloj, huecos, reset, contexto/unidades/boot, duplicados/bounds,4096 rutas y
overflow de conversion/acumulacion sin commit parcial. Aserciones activas Release.
Integracion nativa WSL: carga solo del proceso propio, primer registro observado
PID2123 CPU130000000ns; procesos ocultos en resultado, CPU positiva, red y
notificaciones ausentes; desactivar/reactivar reinicia. Monitor lee procesos
visibles, pero la prueba no modifica ni genera carga en procesos ajenos.

Regresion final: Linux78/78 y Windows98/98 Release; paquetes reinstalados,
consumidores externos recompilados y ejecutados exit0. No valida otros kernels,
distros/hardware ni corrige la ausencia CPU global aislada previa registrada N03.
Sin utilidades externas, nuevas dependencias de runtime ni cambios de permisos.
