# Grabador opcional de PCInsight para un host temprano

Actualizacion117: [captura de hijos propiedad del grabador](STARTUP_RECORDER_BIRTH.md)
opt-in por creador seleccionado; claves/ventanas separadas, binding explicito,
sin sumas ni relojes inventados. save rechaza capturas birth hasta un formato
capaz de preservarlas; no perdida silenciosa. Recompilar SDK/consumidores.

Actualizacion108: CPU final compatible integrada en la via opt-in existente;
ver STARTUP_SCHEDULER_CPU.md. El nombre historico del flag exit_disk se conserva,
pero permite CPU adicional solo con baseline scheduler compatible; fallback
procfs sigue lower bound. Sample agrega tipo de contador: recompilar consumidores.
Formato v1/v2 no cambia; notes distinguen fuente CPU y no se reanuda desde archivo.

Actualizacion107: save guarda observaciones Y diagnosticos historicos en v2;
ver STARTUP_DIAGNOSTICS.md. V1 sigue legible con metadata ausente. Nuevos campos
de archive/finished_at/counter exigen recompilar consumidores; no ABI antigua.

Actualizacion106: recuperacion opcional de disco de objetivos preseleccionados
integrada en esta API, OFF por defecto; [STARTUP_DISK_TAIL.md](STARTUP_DISK_TAIL.md).
CPU final no se mezcla con procfs. El CLI conserva el modo sin BPF; la opcion
privilegiada solo se expone mediante StartupRecorderOptions en la biblioteca.
Options/Report agregan campos: recompilar consumidores; no se promete ABI binaria
de layouts anteriores. El formato de StartupImpactStore no cambia.

Revision103, 2026-10-24. El usuario autorizo desarrollar la segunda alternativa
de STARTUP_EARLY_DECISION.md mediante "prosigue" a la propuesta concreta, SIN
instalarla ni activarla todavia. Se implementa el componente, NO se cierra I07.

## Biblioteca y ejecutable propio

`pcinsight/startup_recorder.hpp` ofrece `LinuxStartupRecorder`, un controlador
reutilizable de una captura. El host opcional `tools/startup_recorder.cpp` usa
la MISMA API, no otra aplicacion de terceros ni una herramienta ejecutada por
la biblioteca. El GUI tambien puede alojar la API con su scheduler.

El SDK contiene el controlador. El ejecutable es opt-in de compilacion mediante
`PCINSIGHT_BUILD_STARTUP_RECORDER=ON` (OFF por defecto), solo Linux. No tiene
regla de install, servicio, .desktop ni tarea de login. En esta revision se
compila y ejecuta unicamente en pruebas privadas, sin instalar/activar el host.
Instalar SDK en work/install-linux para el consumidor de pruebas NO instala
el ejecutable ni modifica el login real. Activar inicio real requiere otra
autorizacion. El modo predeterminado no requiere root/BPF/daemon de terceros
ni nuevos paquetes; la recuperacion opcional106 si requiere privilegios/kernel
compatibles, sin elevarlos ni instalar dependencias automaticamente.

```cpp
#include <pcinsight/startup_recorder.hpp>

pcinsight::LinuxStartupRecorder recorder;
pcinsight::StartupRecorderOptions options;
options.capture_id = "sesion-elegida-por-el-host";
options.accept_estimated_gio_associations = true; // consentimiento consciente
auto result = recorder.start(options);
// Si result es valido, el scheduler del host consulta report().next_scan y
// llama poll(). Al cerrarse, save(path) es una operacion EXPLICITA separada.
// No hay thread/timer oculto ni escrituras por el mero paso del tiempo.
```

Construir o llamar start sin aceptar los hints no inicia el backend. Hay una
captura por instancia; start repetido se rechaza, no resetea ni suma otra fase.
Move transfiere la captura; la instancia movida no puede iniciar/guardar.
El objeto conserva un mutex; los lectores nativos y guardado se serializan.
No ejecutar callbacks reentrantes en el seam privado de tests.

## Estado, tiempo y cobertura

Estados: idle -> recording -> closed o failed. ID1..256 bytes sin NUL,
duracion10ms..1h, intervalo de scan10ms..60s. Se validan antes de convertir
duraciones a ticks nativos. Desde106, despues de verificar el contexto, la fase
temporizada empieza ANTES del primer scan/poll/setup de sondas: trabajo lento
no compra tiempo adicional. start puede retornar closed si consumio el plazo.
No hay cancelacion dura del I/O. Las apps tardias conservan ventanas parciales,
no low/medium certificados. Esto reemplaza el calentamiento fuera de fase103.

Un tick no debido no lee procesos. Un tick debido hace un scan y poll; programa
el siguiente desde el reloj actual, sin acumulacion de scans atrasados. Al/pasado
el deadline admite un ultimo poll, cierra y no vuelve a leer. Un scan/lector
lento puede sobrepasar el plazo; no hay cancelacion de I/O ni deadline duro.
`close()` cierra inmediatamente sin lectura final ni guardado implicito; las
observaciones incompletas siguen unknown o high lower bound segun el observer.

El contexto nativo propio se verifica antes de scans/final/save: boot/PIDns
del sampler mas identidad actual propia procfs/PID/start/UID/namespaces y euid.
Fork/cambio de caller/euid/contexto se rechaza, no continua atribucion en otro
dominio. `close()` hace solo guardia barata PID/euid, sin I/O; save revalida
contexto fresco. Los chequeos de extremos no son una traza de cambios ABA.

El controlador descubre candidatos del UID efectivo mediante la fuente existente
STARTUP_CANDIDATES.md, filtra identidades ya registradas y registra automaticamente
solo candidatos estimated sin una afirmacion numerica de origen automatico.
Una identidad no se registra dos veces ni cambia de app por otro hint posterior.
Errores globales de lote no aplican los candidatos de ese lote; scans parciales
sin error pueden registrar su subconjunto declarado y aumentan partial_scans.
No oculta un error de lectura con un cero. Contexto distinto/excepcion/reloj
invertido/limites/overflow fallan cerrado y conservan observaciones confirmadas.
No hay rollback atomico de registros ya aceptados si una excepcion ocurre a
mitad del procesamiento: se cierran y se exponen como evidencia parcial de una
fase failed, que el metodo save del recorder rehusa guardar como exito.

Limites heredados:4096 observaciones/8MiB en observer,4096 PIDs visitados y8MiB
de entorno por scan del descubridor,100000 filas de snapshot. Se comprueban
tambien limites de reportes y sumas uint64 de diagnosticos, sin wrap/eviccion.

`report()` devuelve scans/partial_scans/examined/ignored/unavailable/registered/
rejected, poll_attempted/updated/failed, exit_disk_attempted/attached/skipped/
unavailable/recovered, fase, contexto, tiempos y observaciones. Los contadores
exit_disk y los demas contadores se guardan desde107 mediante diagnostics
opcional del archivo v2. exit_disk_released_without_final cuenta sondas cerradas
sin registro final, incluso procesos vivos al cierre; no prueba un fallo.
finished_at es tiempo steady local solo en memoria; se guarda elapsed relativo,
no epoca steady ni next_scan/deadline absolutos.
`automatic_startup_origin` sigue AUSENTE: GIO hints pueden venir de launch manual
o falsificarse. El controlador no los convierte en prueba de causa de startup.
No suma automaticamente procesos/descendientes ni aplica un overlay oculto.

## Guardado y CLI opcional

```text
pcinsight-startup-recorder --accept-estimated-gio FILE CAPTURE_ID [DURATION_MS]
pcinsight-startup-recorder --help
```

Ejecucion explicita de un host en foreground; no se daemoniza, instala, eleva
privilegios, abre GUI ni ejecuta Exec de .desktop. El CLI anuncia readiness solo
despues del setup, indicando si la fase sigue activa o ya cerro durante ese
trabajo; muestrea mientras siga activa y guarda explicitamente al cerrarse.
No instala manejo especial de senales; una terminacion abrupta puede perder la
captura en memoria. Ninguna escritura periodica escondida ni checkpoint resume.

Archivo/directorio privados elegidos por caller, directorio ya existente.
`save` exige fase closed sin fallo, contexto fresco y un destino existente
cargable; rechaza corrupcion/formato ajeno y destinos no validos. Usa
StartupImpactStore (STARTUP_STORE.md), reemplazo de captura, NO merge.
Un fallo de durabilidad puede tener replaced=true: no reintentar como si nada
se hubiera publicado. Preflight/load no es CAS; caller serializa escritores.

IMPORTANTE: el formato existente guarda SOLO las observaciones historicas
cerradas, contexto/ID/recorded_at; NO conserva los diagnosticos del report del
recorder ni certifica cobertura de descubrimiento. El CLI lo avisa en salida.
El GUI no debe inferir del archivo un censo completo ni un estado reanudable.
Captura vacia es "ningun candidato aceptable observado", NO impacto global0.
Errores de CLI:2 argumentos/consentimiento,3 proveedor/start no disponible,
4 destino/guardado (puede haber replaced),5 fallo durante captura/ejecucion.

## Pruebas y pendientes

Fixture determinista con seam privado POR OBJETO: denegacion sin I/O, limites,
horarios/no trabajo antes de due, dedup, llegadas tardias, final/overshoot,
partial/failed batch, context change/denegacion/excepciones, gaps, cierre sin
lectura, reloj invertido/overflow y move. No global hooks ni proveedor publico
de datos fabricados. MSVC prueba este controlador con fuentes simuladas y
denegacion nativa sin construir Monitor Windows; no certifica soporte Windows.

Prueba Linux real del ejecutable opcional: XDG/archivo/child C++ privados;
host anuncia readiness ANTES de crear el candidato, luego descubre su identidad,
mide CPU real, cierra ventana tardia parcial y hace save/load de archivo0600.
Comprueba .desktop intacto y Exec nunca ejecutado, requests invalidos/sin
consentimiento no reemplazan el archivo, destino corrupto conservado, captura
vacia honesta y ambos hijos recolectados. No login/servicios/apps reales tocados.
La fuente nativa enumera procesos visibles y lee hints solo del mismo euid;
no es una prueba de captura exclusivamente de PIDs privados ni de otros UID.

I07 NO cerrada: faltan causa fiable de autostart por proveedor, ciclo de vida
de procesos ajenos breves/descendientes, cobertura temprana end-to-end y politica
durable de diagnosticos. Este host permite ejecutar la biblioteca antes del GUI,
pero no puede medir antes de SU arranque ni recuperar procesos ya desaparecidos.
No activar login real ni ampliar captura global por el permiso de desarrollo.
La proxima unidad debe trabajar esa evidencia/ciclo de vida, no instalar este
binario ni volver a contar el ejemplo previo como captura completa.

Revision104 termina un sensor INTERNO de evento de ultimo hilo para un objetivo
preseleccionado, retenido incluso tras reaping por su padre; ver
STARTUP_SELECTED_EXIT.md. Ese evento sigue separado. Revision106 conecta SOLO
el disco del proveedor de liberacion105 al recorder opt-in; no CPU final ni
cobertura total de procesos breves. Ver STARTUP_DISK_TAIL.md.

Resultado103: Linux139/139 sin skips+30repeticiones focales, MSVC10controlador
simulado/denegacion sin backend, consumidor instalado relinkado exit0; build
final sin warnings. El ejecutable NO esta en el prefijo SDK instalado.
Fixture inicial esperaba enum not_measured para una metrica unknown; corregida
expectativa a ausencia (sin cambiar el clasificador), log inicial preservado.
Logs work/startup-recorder-*, riesgos historicos N03 no declarados corregidos.
