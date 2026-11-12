# Selecciones Linux mayores de 64 procesos

Revision97, 2026-10-24. El limite estructural publico pasa a 1024 procesos
(`LinuxProcessNetworkSampler::max_processes`). El registro por proceso conserva
16 namespaces observados, hasta 16384 buckets. No es soporte ilimitado ni una
promesa de frecuencia de refresco. Recompilar biblioteca y consumidores porque
cambia una constante publica constexpr; no cambia el layout de los informes.

## Lotes y continuidad

En modo observado, cada productor RAW sigue aceptando como maximo 64 actores.
La seleccion completa se reparte en lotes disjuntos de hasta64. Con N procesos,
el inicio usa ceil(N/64) lotes, tres programas/tres links/dos mapas por lote y
una consulta de identidad compartida con compuerta. El backend publico conserva
N pidfds propios: N+6+8*ceil(N/64) FDs persistentes inicialmente. La admision
visible conserva ademas N pidfds del owner (2N+6+8*ceil(N/64)). No se modifica
RLIMIT_NOFILE, memlock, capacidades ni configuracion para hacer que quepan.

Una actualizacion retira rutas antiguas ANTES de agregar reemplazos; los actores
retenidos conservan productores/contadores sin reinicio. Nuevos/readmitidos usan
mapas nuevos, nunca slots retirados ni contadores del intervalo excluido. Las
altas se dividen tambien en lotes de hasta64. Tras churn puede haber G lotes
fragmentados:1<=G<=N, no siempre ceil(N/64). Coste persistente N+6+8G (o 2N+6+8G
con admision); NO tres sondas constantes independientemente de las actualizaciones.
La nueva seleccion renueva epoch y resetea todas las tasas, no los contadores
retenidos. Un refresh de admision sin cambio de conjunto conserva epoch/tasas.
Cada programa sigue ejecutando su filtro por lote al ocurrir una operacion de
red; el coste crece con G. El historial en memoria de Sampler tambien crece con
numero de buckets y muestras retenidas; dimensionarlo en el consumidor. No se
promete que16384 buckets completos a1Hz sean apropiados para cualquier maquina.

Se conserva coherencia por lote, barrido nominal completo <=1s, NO snapshot
atomico entre lotes. La admision visible mantiene presupuesto cooperativo de5s,
no un plazo duro ni una ampliacion automatica para conjuntos grandes. Un fallo
al admitir un lote posterior cierra TODOS los lotes y pidfds, no publica un prefijo.
Una lectura incoherente/denegada/agotada no inventa ceros ni tasas. Los rechazos
de forma/capacidad en seleccion manual preservan la captura previa; un refresh
visible autorizado fallido cierra TODO conforme al contrato existente.

Estricto/declarado conservan el productor por fila (mucho mayor coste de FDs/
sondas), aunque el limite estructural compartido sea1024; no se les atribuye el
coste ni la validacion nativa del modo observado. Declarado cuenta filas
PID/start/cookie, no procesos unicos. Ningun modo utiliza CLI externas.

## Admision y consumidores

El owner, puente estricto de inventario, planificador strict_all/verified_only,
proyeccion publica y Monitor/Sampler usan la misma capacidad. Mas1024 elegibles
rechaza TODO, no first1024/truncamiento/rotacion. El inventario puede seguir
describiendo hasta100000 entradas; las exclusiones explicitas no consumen slots
de captura, pero siguen visibles con PID/estado/motivo. No se inicia captura
automaticamente por subir el limite, por construir objetos ni por activar flags.

## Verificacion

Fixtures acotados:65/129/1024 actores en lotes de64, hasta16384 buckets, cuatro
tasas por bucket/sumas UID, retiro/readmision/origen/gap/reset epoch y rechazo
de1025 sin truncar. Fallo/excepcion al crear un lote posterior en inicio/update
debe cerrar lo ya abierto. Owner/inventario/controlador/politica/proyeccion y
Monitor/worker verifican el nuevo limite con checks activos en Release.

El harness nativo admite el flag explicito --authorized-private-capacity65:
solo hijos privados elegidos manualmente, nada de /proc global ni procesos
ajenos; guardias60s/55s y limpieza de hijos/FDs. El flag no es autorizacion del
usuario por si mismo. Incluye la admision nativa del mismo conjunto privado
inyectando SOLO el enumerador: comprueba presupuesto vigente, censo owner+
captura, metadata/red pareadas y refresh igual que no reinicia tasas/origen.
Resultados finales y logs work/network-scale-* en el TXT.
1024 procesos reales y namespaces heterogeneos no se certifican con fixtures.

Resultado revision97: Linux133/133 sin skips, siete fixtures10veces (70), MSVC
siete fixtures10veces (70). Consumidor instalado recompilado/enlazado/ejecutado
exit0 con asserts1024procesos/16384buckets. Su primer build detecto un assert
viejo1024buckets que se actualizo al nuevo contrato sin relajar la comprobacion;
log inicial conservado. Builds finales sin nuevos warnings C++/D9025 esperado.

Nativo65 privado PUBLICO+admision restringida PASS: censo87FDs captura y152FDs
con owner, dos lotes; contadores/rates UDP por actor y suma UID, epoch/metadata
pareados y refresh igual conserva baseline, padre excluido, FDs limpios/todos
hijos normales. Guardias60s/55s y presupuestos1s/5s SIN ampliarlos. Fases del
ensayo final:PUBLICO iniciado1242ms/stop2679ms, admision4206ms/stop5678ms desde
inicio del harness; no garantia de latencia. Regresion nativa17 de ventanas/
updates/TCP-UDP/IPv4-IPv6/gap/fork tambien positiva. Logs native65-final y
native17-regression. No prueba full /proc ni autorizacion ampliada inferida.
F03 sigue abierta por validacion/cobertura de vista real; permiso temporal WSL
solicitado al usuario y no recibido en esta revision. TXT manda sobre ClickUp.
Espejo97 confirmado: comentario90140272937319; titulo/estados historicos intactos.
