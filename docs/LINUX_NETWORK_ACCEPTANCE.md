# F03: validacion real y criterio de aceptacion

Revision98, 2026-10-24. Criterio conservado del control oficial: proveedor
opcional de contabilidad nativa, soporte/privilegios declarados, atribucion y
ventanas probadas, sin ejecutar utilidades externas. No confundir su cierre
con integracion del historial por app H04, agregacion de sesiones J04 o una
garantia de trafico global/universal.

## Vista real y fallo corregido

El usuario confirmo continuar tras la pregunta explicita de autorizacion para
una prueba temporal de hasta30s sobre la vista /proc de WSL. Solo PID/UID y
contadores TCP/UDP, sin contenido/argumentos/entorno, sin instalar ni cambiar
configuracion. Esta confirmacion NO autoriza captura futura permanente.

Kernel ensayado:6.6.114.1-microsoft-standard-WSL2. El nuevo ejecutable manual
linux-network-visible-native NO es un CTest; requiere el flag separado
--authorized-visible-wsl-30s, que NO sustituye autorizacion humana.

Primer ensayo: strict y verified_only rechazaron EINVAL de identidad; segundo
ensayo diagnostico confirmo PID seleccionado igual al PID del recolector.
La biblioteca habia asumido que la consulta de un pidfd propio pasaria por
security_ptrace_access_check como las consultas a hijos. El retorno temprano
para el mismo grupo de threads en el [codigo upstream Linux6.6](https://github.com/torvalds/linux/blob/v6.6/kernel/ptrace.c)
es consistente con la ausencia de evento observada en WSL; no se afirma que
todo el codigo de ese tag sea identico al kernel vendor instalado.

Correccion: tras verificar pidfd/procfs/contexto antes de consultar, la ruta
SELF solo se elige si TODA la identidad local del proceso coincide con la del
observador nativo (/proc/self): PID, UID, inicio, procfs/dispositivo/inodo,
inodo de proceso y ambos namespaces. No basta un PID igual. Usa dos lecturas
kernel actuales del propio caller mediante BPF_TEST_RUN y conserva validacion
posterior/liveness/igualdad de contextos e identidad kernel. No conversion de
PID/UID local a inicial, identidad cacheada ni fallback para procesos externos.
La ruta externa sigue exigiendo sus dos consultas pidfd y permisos vigentes.
Los fallos incluyen ahora el PID seleccionado/caller para diagnosticar, sin
publicar linea de comandos o contenido de trafico.

Un ensayo intermedio admitio38/38 pero detecto un pidfd ya no vivo antes del
sample. Se preserva ese fallo: la seleccion es un censo dinamico, no una foto
atomica ni garantia de vida. No se oculto al actor ni se publicaron ceros.

Ensayo final, terminadas las compilaciones concurrentes: inventario REAL sin
inyeccion,30visibles/30verificados/0excluidos, strict_all. Monitor produjo tres
snapshots con metadata/red/epoch y cardinalidad pareadas. El UDP del recolector
dio delta97bytes en envio y recepcion y tasa entera97e9/intervalo nominal.
Stop restauro el censo de FDs, sample posterior no rearma, sin cambios permanentes.
Duracion final1326ms; primeros diagnosticos498ms y490ms, ensayo intermedio
comando completo<3s. Total de estos ensayos por debajo de10s, dentro de30s;
guardia final reducida a20s para reservar margen acumulado. No quedan sondas.

## Evidencia del criterio F03

| Criterio | Evidencia | Limite conservado |
|---|---|---|
| Fuente sin aplicaciones auxiliares | procfs/pidfd/syscalls BPF/BTF propios | Kernel/configuracion/permisos compatibles requeridos |
| Atribucion por actor vivo | joins por ambos extremos, dominios local/kernel tipados, source SELF o externo atestada | Operador de socket, no supuesto propietario exclusivo ni app inferida |
| Contadores/tasas/ventanas | TCP/UDP IPv4/IPv6 nativo17, UDP nativo65, real30/Monitor y fixtures | Payload de rutas instrumentadas; no wire/ACK/async total |
| Escala y ciclo de vida |1024procesos/16384buckets fixtures,65privados reales/2lotes, updates/gap/fork/worker | Limites/coste G/1s de barrido/5s de admision explicitos |
| Descubrimiento y cobertura | Inventario real30/30, politicas estricto/parcial, exclusiones tipadas | Vista visible/dinamica, no censo del host ni captura retroactiva |
| API consumible | Standalone/owner/Monitor/Sampler, consumidor instalado | H04 y J04 tienen integraciones propias aun pendientes |

La prueba real positiva satisface la validacion de fuente/admisibilidad que
quedaba en F03; no se exige certificacion de todo hardware/distro para declarar
implementado este proveedor opcional. N02 conserva la matriz multiplataforma y
N03 su auditoria/riesgos propios. Fallos/ausencias de campos no soportados NO
se convierten en soporte por cerrar F03. La politica ante churn sigue fallando
cerrado: el consumidor necesita un nuevo refresh explicitamente autorizado,
no hay rearmado/filtrado silencioso. Esa limitacion permanece visible en la API.

Logs work/network-visible-*. SEGUIMIENTO.txt registra la decision final,
regresion y espejo secundario. El nuevo harness no imprime nombres, argumentos,
payload ni listado de PIDs de terceros; solo conteos/notas de fallo permitidas.

Decision final98: F03 E->C. Linux133/133 sin skips+30focales, MSVC10join,
consumidor instalado exit0. Regresion nativa17 privada TCP/UDP IPv4/IPv6,
updates/gap/fork/limpieza PASS. Diez discordancias de identidad y contexto SELF
invalido rechazadas por fixtures. Sin nuevos layouts/API publicas. Avisos
temporales de reloj en archivos generados WSL preservados, recheck build exit0
sin avisos; primer target sin regla corregido regenerando CMake/log preservado.
Control oficial C58/E10/P5/B2,58/75=77,3%,17/75=22,7%. Siguiente H04: leer
HISTORY.md. Espejo98 confirmado: comentario90140272937903; TXT manda.
