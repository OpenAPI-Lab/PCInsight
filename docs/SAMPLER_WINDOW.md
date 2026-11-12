# Sampler: ventana monotónica de 60 segundos (A03)

`Sampler::history()` y `timed_history()` devuelven muestras cuyo inicio de
recolección está en el intervalo inclusivo `[steady_now-60s, steady_now]`.
La antigüedad se comprueba al añadir una muestra y al consultar el historial,
incluso con el muestreador detenido. No depende del reloj de calendario ni de
contar60 elementos. El límite temporal se aplica por igual a Linux y Windows.

## API y uso para gráficas

```cpp
#include <pcinsight/sampler.hpp>
using namespace std::chrono_literals;
pcinsight::Sampler sampler(100ms); // capacidad automática: hasta601 puntos
sampler.start();
// En el tick del GUI, después de que existan muestras:
auto points = sampler.timed_history();
auto now = std::chrono::steady_clock::now();
for (const auto& point : points) {
    auto age_seconds = std::chrono::duration<double>(now-point.started_at).count();
    const auto& cpu = point.snapshot.cpu.utilization_percent;
    // Dibujar en x=-age_seconds; si cpu.value falta, dejar hueco, no dibujar0.
    // No interpolar sin una política explícita del consumidor.
}
sampler.stop();
```

`SamplerSample` contiene `started_at`, `finished_at` (steady_clock) y el
`SystemSnapshot` original. `latest_timed()` ofrece el último resultado con ese
envoltorio; `latest()` conserva la API de snapshot. Ambas pueden devolver un
resultado de más de60s: significan *última recolección exitosa*, no dato fresco.
El historial puede estar vacío mientras latest sigue presente. Se retiene como
máximo una muestra caducada para latest, sin duplicar todos sus datos.

Los puntos se ordenan por inicio monotónico; se exige inicio<=fin y que la
colección siguiente no empiece antes del fin anterior. Esos timestamps son un
envoltorio de la colección, NO un instante atómico común a todas las métricas.
Cada proveedor conserva su propia ventana de tasas/porcentajes. `captured_at`
sigue siendo la hora de calendario proporcionada por Monitor; sus saltos no
cambian retención ni orden. No persistir valores steady entre procesos, boots o
dominios de reloj diferentes. La semántica de suspensión depende del reloj de
la plataforma; no se promete tiempo civil transcurrido durante suspensión.

`now` se toma dentro del bloqueo de consulta, antes de copiar resultados. Si el
GUI tarda en recibirlos/dibujarlos, su edad sigue creciendo; debe recortar de
nuevo al renderizar si exige un borde exacto en ese instante. Llamadas separadas
a history/timed_history/latest no son una transacción única ni la misma ventana.

## Intervalo, coste y capacidad

El constructor mantiene tres argumentos: `(interval, history_capacity, options)`.
El valor por defecto de `history_capacity` cambia de60 a0;0 ahora significa
automático: `floor(60000ms/interval_ms)+1`. A1s admite61 puntos (extremos0y60),
a100ms601, a1ms60001. No se reserva ese espacio por adelantado. Un intervalo
positivo que quepa en steady_clock es obligatorio; entradas no representables,
cero o negativas producen invalid_argument antes de arrancar el hilo.

Una capacidad explícita positiva sigue siendo un límite adicional: con2 se
conservan como máximo las2 muestras más recientes dentro de60s. Puede recortar
la cobertura temporal y NO garantiza llenar60s. La capacidad automática basta
para una ejecución sin reinicios con la separación mínima configurada. No se
rellenan segundos sin datos ni se crean muestras sintéticas.

El worker toma el inicio una sola vez y espera hasta inicio+intervalo, con suma
acotada para no desbordar el reloj. No hay recolectores solapados ni recuperación
en ráfaga de ticks perdidos. Si la colección cuesta más que el intervalo, la
siguiente comienza al terminar la anterior. Por tanto no se promete frecuencia
exacta ni latencia máxima; una colección que tarda más de60s puede ser latest
pero quedar fuera del historial desde su publicación. Esto evita mostrarla como
un punto fresco tomado al final cuando empezó hace más de un minuto.

Las copias de snapshots completos y la recolección cuestan CPU/memoria; activar
muchos inventarios a1ms puede ser caro. Reducir grupos con SampleOptions o poner
capacidad explícita si el consumidor necesita un techo menor. No hay límite de
bytes global ni garantía de tiempo real. El dibujo de gráficas sigue en el GUI.

## Ciclo de vida, errores y compatibilidad

`start()` estando activo es no-op. Un arranque nuevo, también tras fallo, limpia
el historial gráfico y el último error; no mezcla puntos de distintas ejecuciones.
No reconstruye Monitor: sus contadores/historial de apps conservan su ciclo de
vida propio. Los valores de la primera muestra tras una pausa pueden cubrir
intervalos del backend diferentes de la ventana de puntos del GUI.

`stop()` solicita parada y despierta la espera; no interrumpe una recolección en
curso. Puede publicarse su último resultado antes de terminar. Una llamada de
kernel/driver bloqueada puede retrasar stop/destrucción; no hay cancelación forzada
de esas APIs. Errores del worker siguen en `last_error()`, detienen esa ejecución
y no escapan del hilo; resultados exitosos anteriores no se convierten en ceros.
Un fallo al crear el hilo se propaga al llamador. Movimiento exige que el llamador
no use simultáneamente el objeto que mueve; start/stop/consultas usan sus bloqueos.

Cambio de comportamiento0.x: capacidad0 ahora válida/automática, default61 a1s,
ventana por tiempo y limpieza del historial en cada nuevo arranque. Nuevos métodos
y tipo SamplerSample. No cambia el layout de SystemSnapshot en A03; recompilar
biblioteca y consumidores para aplicar nuevos defaults y usar símbolos nuevos.
Un objeto antiguo que pase explícitamente60 seguirá limitado a60 puntos. No se
promete ABI estable ni se eliminan los límites descritos para las métricas.

## Pruebas y evidencia

`tests/sampler_window.cpp` usa el mismo buffer interno que el worker con tiempos
sintéticos:601 puntos/100ms en0..60s inclusivos; borde+1ns, costes variables,
muestreo disperso, colección de61s, expiración al leer estando detenido, latest
caducado, saltos de calendario en ambos sentidos, límite explícito, limpieza,
retrocesos/solapamiento rechazados y aritmética en extremos de steady_clock.
El reloj sintético permite verificar más de60s sin ralentizar pruebas ni cambiar
el reloj del sistema. No es una prueba real de suspensión ni de latencia de60s.

Integración nativa en Linux WSL y Windows: worker con intervalo10ms, al menos3
muestras con envoltorios ordenados, consultas mientras trabaja, opciones de grupos,
parada y reinicio con nuevos puntos. `sampler_lifecycle` conserva movimiento con
worker dormido1h, despertar y start/stop concurrentes. Aserciones activas Release;
tests acotados por CTest. No acciones sobre procesos/servicios/sesiones ajenos.

Regresión de cierre2026-10-16: Linux79/79, Windows99/99 Release; paquetes
reinstalados y consumidores externos recompilados/ejecutados exit0. No cierra CI,
hardware pendiente ni el fallo aislado de CPU global registrado anteriormente N03.
