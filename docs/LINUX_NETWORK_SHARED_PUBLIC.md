# Backend compartido en la API publica Linux

Revision97: [capacidad1024](LINUX_NETWORK_SCALE.md), lotes RAW de hasta64,
16384buckets estructurales; nativo65 privado en dos lotes. Los limites y costes
de un solo lote que figuran debajo corresponden al checkpoint historico93.

Continuacion revision94: [admision estricta standalone](LINUX_NETWORK_ADMISSION.md)
conecta inventario-owner-captura; conjunto igual conserva tasas/epoch. Rechaza
unknown/>64sinfiltrar; solo2privados nativos, NO captura completa autorizada/
validada ni admision automatica de Monitor/Sampler. Pendientes93 debajo historicos.

Revision93, 2026-10-22. El modo publico
`LinuxProcessNetworkSelectionMode::observed_namespace_buckets` usa ahora
`LinuxSharedSelectedNetworkSampler` y sus lotes compartidos de admision.
No se agrega otro modo, firma, layout publico, dependencia ni flag de build.
La construccion de cualquiera de los backends no abre pidfds ni carga sondas.

## Rutas conectadas

La seleccion de backend ocurre dentro del Pimpl de LinuxProcessNetworkSampler:

- Modo observado: start/sample/update al productor compartido autenticado.
- Modo estricto predeterminado: proveedor por fila anterior, sin cambios.
- Modo de cookies declaradas: proveedor por fila anterior, sin cambios.

Stop cierra ambas ramas incondicionalmente; solo la elegida tiene recursos
activos. Move transfiere el Pimpl y su modo; el objeto movido no captura.
Las ventanas se publican por la MISMA proyeccion validada, sin saltarse checks
de identidad, dominios, tasas, calidad, metadata de actores vacios o sumas UID.
Un rechazo de la proyeccion detiene la captura elegida.

Monitor delega en este sampler y publica en SystemSnapshot.linux_process_network.
Sampler conserva su serializacion existente entre worker/update/latest/history;
no se introduce otro hilo de captura. El modo se elige en setup parado y no
se cambia mientras captura. SelectionOwner prepara PID/start/pidfds de entrada
sin capturar, y puede liberar sus copias despues del start/update exitoso.

El include flag solo incluye el informe; NO autoriza eBPF. Opt-out/stop/salida
del worker cierran captura; reiniciar el worker o volver a habilitar el flag no
la rearma. Sigue haciendo falta setup explicito con consentimiento.

## Semantica y coste

Se conserva la cobertura publica de payload TCP/UDP instrumentado por actor y
namespace del SOCKET. Cookie de entrada0,1..64actores y16cookies por actor,
1024buckets maximos. No es inventario de todos los sockets ni trafico de enlace,
sumas globales de usuario o atribucion causal de una app por socket propietario.
TCP send retorno positivo no equivale a entrega/ACK; operaciones no instrumentadas
no se convierten en cero. Metadata de actor sin actividad no fabrica bucket.

Ventanas nominales estimated, coherencia por lote y barrido total<=1s; no corte
atomico entre lotes. Calidad perdida en cualquier lote rechaza TODO el informe,
sin publicar parcial. Joins por pidfd actuales de TODOS antes/despues del barrido
y de cada update; sin permisos cacheados ni conversion PID/UID entre dominios.

Retenidos conservan sus pidfds, captura y origen; retirados dejan de admitirse.
Altas/readmisiones tienen lotes NUEVOS; nunca se reciclan slots que una sonda
en curso podria seguir escribiendo. Reincorporados no incluyen bytes del gap.
Todo update exitoso, aun reordenar, genera epoch nuevo/reset de TODAS las tasas.
Descripciones completas: LINUX_NETWORK_SHARED_UPDATES.md y
LINUX_NETWORK_SHARED_RETENTION.md.

Nativo, despues de cada operacion: N pidfds +6 FDs query +8G trafico, G lotes no
vacios,1<=G<=N<=64. Inicial N+14, no un set de sondas por actor. Churn puede
fragmentar en hasta64 lotes, no se prometen tres sondas constantes. Propuestas
mantienen transitoriamente hasta64 copias nuevas de pidfd junto a las anteriores;
consultas/BTF pueden tener FDs efimeros adicionales. Se cierran lotes vacios.

El contrato de rechazo NO se homogeneiza silenciosamente en esta revision:
el sampler standalone preserva captura ante forma/consentimiento invalidos;
Monitor conserva su wrapper anterior que cierra captura tras un update autorizado
fallido, incluso forma invalida; Sampler hereda ese comportamiento, pero el worker
continua. Denegar consentimiento no cambia ninguna ruta. Fallos runtime de update
cierran toda la captura. El muestreo ausente descarta tasas y no publica un cero.

## Verificacion

Logs `work/network-shared-public-*`; el control oficial registra los resultados
finales, nunca se deduce activacion publica solo de una proyeccion interna.
Pruebas manuales nativas con autorizacion temporal existente, SOLO hijos
privados/loopback, guardias de tiempo, sin configuracion permanente ni utilidades
externas de recoleccion. No instalacion ni captura global.

Capacidad17 valida el proveedor PUBLICO observado con censo N+14, cuatro lanes,
intervalo nominal y suma UID. Los escenarios de sampler/Monitor/worker/owner
validan altas-retencion-gap-readmision/epochs/FDs/cierre de inputs/stop-restart,
denegacion/fork/procesos muertos y que el flag no rearma. Un solo netns real;
64actores/1024buckets siguen siendo fixtures, no certificacion fisica universal.

Final93: Linux Release130/130 sin skips y3focales20veces(60ejecuciones), consumidor
instalado recompilado/relinkado exit0; MSVC3/3 focales y60ejecuciones repetidas.
Sin nuevos warnings C++/D9025 esperado. Dos builds iniciales lanzados por error
del asistente en el MISMO directorio colisionaron en ranlib (archivo truncado/
ausente); logs preservados. Reconstruccion posterior serializada por directorio
y todos los resultados finales positivos. No es causa nueva de codigo ni
resolucion del fallo consumidor Windows0xC0000409 revision75/N03.
Espejo93 confirmado: comentario90140272305719; el TXT conserva autoridad.

F03/H04/J04 siguen abiertas: esta activacion no da admision automatica de todos
los procesos visibles ni incorpora red a historia por app o totales de sesiones.
El consumidor elige el conjunto explicito; la politica de admision aun pendiente
debe respetar capacidad/privilegios e identidades sin truncamiento silencioso.
