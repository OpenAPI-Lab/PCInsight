# Coleccion multiactor seleccionada

Revision88: [consulta con compuerta](LINUX_NETWORK_QUERY_SESSION.md) PASA nativo17
completo/tasas/limpieza. Sin productor compartido ni64actores reales certificados.

Revision87 (historica): [capacidad estructural64 y coste real](LINUX_NETWORK_CAPACITY64.md).
El limite16/256 de procesos/filas citado debajo es historico; el limite16 cookies
por actor NO cambia. Nativo17 falla por coste/vida acotada; NO captura escalable
certificada. Se conserva la evidencia original de cada revision.

Revision72 introduce `LinuxMultiNetworkSampler`, interno/no instalado. Agrupa
de1a16 [samplers seleccionados](LINUX_NETWORK_SELECTED_WINDOW.md), cada uno con
su propio pidfd retenido, captura, identidad revalidada, reloj y baseline.
Sin enumerar procesos, descubrir cookies, publicar en Monitor/historial o tocar
Windows/API/ABI publica. F03 sigue en curso; H04/J04 no se cierran.

## Inicio y recursos

start exige opt-in explicito y una seleccion fija pidfd/PID/start_ticks/cookie.
Lista vacia, mas16, argumentos invalidos o PID local duplicado se rechazan
antes de duplicar/capturar. Se permite UN cookie declarado por proceso, no dos
capturas del mismo actor para diferentes cookies que luego se sumen. Tras los
joins se comprueba unicidad TGID inicial/start_boottime, sin incluir UID/cookie
en la clave de deduplicacion. Dos procesos distintos pueden compartir UID/cookie.

Todos deben observar el mismo boot/contexto del colector al iniciar. Una falla
o excepcion revierte todos los samplers ya iniciados, incluido el actual. Cada
uno posee un duplicado, por lo que se pueden cerrar los descriptores de entrada
tras start. La seleccion tiene una epoca propia del serial compartido con SELF
y selected; stop/restart genera otra, sin retener referencias de tasas.
No cambia membresia en caliente: stop y nuevo start para otra seleccion.

Se heredan contratos de llamadas serializadas, misma tabla FD y contextos
prestados con vida suficiente. Fork del colector se rechaza antes de leer filas
y cierra sus copias heredadas; no cierra las referencias del padre. Destructor
y stop cierran todas las capturas y pidfds propios, sin pinning/configuracion.

## Lectura, huecos y disponibilidad

sample recorre en orden de seleccion; cada fila realiza los joins completos
antes/despues y checks del sampler individual. Revalida forma, boot/observador
comunes y unicidad kernel/local. Solo devuelve la coleccion si todas las filas
son validas. No devuelve un grupo parcialmente exitoso como si estuviera completo.

Si falla cualquier fila, incluso la segunda despues de avanzar el reductor de
la primera, se descartan TODOS los baselines. Excepcion tiene el mismo rollback.
Tambien se rechaza un cambio de contexto entre colecciones exitosas, y se
descartan las referencias. La siguiente coleccion estable empieza sin tasas
para todas las filas; luego pueden volver. No mezcla intervalos sobre huecos.
Una condicion sticky en captura o salida del objetivo no se oculta con ceros;
puede requerir stop/reinicio/cambio de seleccion, como el proveedor individual.

`PrivateMultiNetworkWindow` devuelve epoca y vector de ventanas individuales,
con PID/UID locales y kernel separados. NO es un snapshot simultaneo: cada
actor tiene sus propios timestamps/intervalo/tasas estimadas. No hay timestamp
global de observacion ni suma UID entre actores con ventanas diferentes.
Las sumas UID dentro de cada fila siguen siendo parciales al actor/cookie.
No se convierte la lista seleccionada en cobertura global, total del proceso,
trafico wire o identidad comercial. sample no hace agregacion temporal comun.
Revision73 agrega un metodo separado [sample_common](LINUX_NETWORK_COMMON.md)
con barrido acotado y sumas UID parciales estimadas; sample conserva sus
intervalos independientes. Alternar modos reinicia todas las referencias.

Se heredan los limites de BTF/privilegios/ptrace/mascara caller, cambios ABA/
extremos no atomicos/vida futura/no plazo duro/no garantia de rendimiento. El
coste crece por actor (cuatro descubrimientos pidfd por muestra individual).
El limite16 es de recursos/seleccion, no prueba de rendimiento con16actores.

## Pruebas y siguiente unidad

Fixtures con2actores, mismo UID/cookie pero distintos TGID/localPID, intervalos
deliberadamente2s/3s y cuatro oraculos independientes. Rechazo preflight de
duplicado/lista vacia/>16/cookie invalido/sin opt-in, cinco fallos de inicio
(permiso/excepcion/alias kernel/observador/boot) con limpieza. Cuatro fallos de
segundo actor (join/excepcion/captura/contexto) despues del primero, recuperacion
sin tasas para AMBOS; reinicio/epoca/fork. Los CTests no cargan BPF.

Modo manual `--authorized-private-multi-window`, NO CTest: crea dos hijos propios
antes de cargar sondas, estos solo reportan cookie, no token kernel. Padre lee
start_ticks; A genera una unidad TCP4096/4096B UDP256/256B y B dos unidades.
Se comprueban cuatro lanes por actor con intervalos propios, A sin trafico de B
y viceversa, padre excluido/idle0, entrada FD cerrada, duplicado y fork rechazados,
salida de B rechaza grupo entero y limpieza sin fugaFD. Watchdogs harness60s/
50s no son plazos duros del proveedor. Sin cambiar UID/namespaces/configuracion.
Prueba autorizada solo en hijos privados, no captura del sistema entero.

Siguiente F03: ventana temporal comun acotada/agregacion UID, cobertura y buckets
antes de integracion publica. El control oficial SEGUIMIENTO.txt revision72 y
logs `work/network-multi-*` conservan resultados, limites y porcentaje.

Resultado nativo WSL exit0: todas esas comprobaciones pasaron; log
`work/network-multi-native-linux.log`. Linux124/124 Release sin skips,
16tests red repetidos20veces cada uno, consumidor instalado reconstruido y
ejecutado exit0; build final sin warnings. Warning inicial de indentacion de
for corregido y log inicial preservado, sin relajar tests. Logs final-build,
suite,repeat,install,consumer del mismo prefijo. Solo positivo real en este WSL
x86_64, no CI/hardware/distros universales ni rendimiento validado de16actores.
