# Motores y utilización GPU Windows

WindowsGpuEngines añade a Monitor los motores expuestos por el contador PDH
local GPU Engine(*) / Utilization Percentage, sin comandos, instalación ni carga
gráfica propia. Se enlaza el LUID de cada instancia al inventario DXGI, no al orden
de la lista ni al nombre de la tarjeta. Usa la misma frontera PDH inyectable de
memoria para probar llamadas nativas; queries/estado son independientes.

## Qué devuelve

GpuInfo.engines conserva TODOS los motores observados, sin limitar compute a2.
GpuEngine.id identifica phys:N/eng:M dentro de ese LUID; type conserva el tipo
reportado (3D, Copy, Compute 0, Video Codec Engine u otro), name es etiqueta legible
que incluye ID. Varios Copy o Compute no se fusionan solo por tener igual nombre.
Nodos físicos diferentes bajo un adaptador lógico permanecen separados.

Para cada motor se suman SOLO porcentajes PDH de procesos distintos de ese mismo
LUID/nodo/índice. La métrica es estimated: porcentaje de carga OBSERVADA por esas
instancias, no certificado de cobertura absoluta del scheduler. El porcentaje
general es el máximo entre esos motores, nunca suma ni promedio entre motores.
Si hay un motor observado inválido/desconocido, el general queda sin valor, pero
los otros motores válidos se conservan. Permission_required tiene precedencia.
Lista sin instancias válidas no equivale a GPU0%: general desconocido.

Esto no devuelve consumo GPU por proceso ni mantiene PIDs para acciones del GUI.
Los PIDs se usan solo internamente para evitar doble conteo por motor. Una app
breve que desaparece entre muestras puede no aparecer en el array final. Un driver
puede no exponer todos sus motores/contexts. Un porcentaje observado0 es legítimo
si existen filas válidas0; no demuestra ausencia global de actividad no observada.
No se promete coincidencia exacta con Task Manager ni porcentaje instantáneo.
El GUI debe mostrar las notas/confianza, no convertir estimated en available.

## Validación y lifecycle

Se aceptan nombres completos pid_N_luid_0xHHHHHHHH_0xLLLLLLLL_phys_N_eng_N_engtype_T
con enteros32 bits y tipo ASCII imprimible1..64 chars, incluido espacio/underscore.
Hex acepta mayúsculas/minúsculas. _Total se ignora. Duplicar PID/nodo/motor no se
suma dos veces; tipos contradictorios del mismo motor dejan tipo vacío y unknown.
Sufijos ambiguos #N, gramática nueva o nombres malformados no se adivinan: array
rechazado sin publicar un subconjunto como lista completa.

Solo CStatus VALID_DATA/NEW_DATA y doubles finitos no negativos se usan. Porcentaje
individual o suma fuera de0..100 se rechaza. Tolerancia de1e-6 puntos exclusivamente
para redondeo numérico: conserva suma SIN redondear durante acumulación, por lo que
muchos pequeños excesos no se pueden esconder haciendo clamp tras cada fila.
Se usa PDH_FMT_DOUBLE|NOSCALE|NOCAP100 para no ocultar un valor inválido con el cap
automático de100. Una suma inconsistente no se fuerza a100 como si fuera medida.

Requiere dos colecciones por query nueva. Primera muestra es unknown, no cero.
Open/add/collect/array fallidos cierran query; recuperación requiere preparación
y otra colección, sin porcentaje heredado. Item inválido afecta ese motor en la
muestra; un item válido posterior puede recuperarlo conforme al estado de PDH.
Cambio de LUIDs, gpus vacía o include_gpus=false cierra y borra toda la baseline;
al reactivar no se reaprovecha un intervalo de antes de la desactivación.

Arrays limitados8MiB/65536 items/256wchar por nombre/4096 motores observados. Verifica
tamaño/count, punteros dentro del bloque/alineación/terminación; crecimiento entre
las dos llamadas no dispara realloc infinito. No hay timer/thread ni deadline
garantizado ante driver/PDH lento. Memoria, motores e inventario no forman una
instantánea atómica. Caller serializa Monitor y el scheduler decide el intervalo.
GpuEngine0.x amplía layout: reconstruir paquetes/consumidores. Linux conserva sus
motores previos; los campos id/type nuevos permanecen vacíos donde no hay fuente.

## Verificación

Linux35/35 y Windows48/48 Release; paquetes/consumidores instalados reconstruidos y
ejecutados. Tests inyectados: agrupación de PIDs por motor, max no suma/promedio,
varios compute/copy/nodos, cambio de valores, cero real, epsilon/acumulación sin
clamp repetido, exceso100, NaN/Inf/negativo, permiso, duplicados/tipos contradictorios,
_Total/no instancias, nombres inválidos/límites, buffers malformados y recursos en
open/add/collect/format/nulls/recovery/desactivar/cambio de topología.

Prueba real en RX7900XT:8 motores observados, entre ellos3D, dos Copy, Compute0,
Timer0, Security1, VideoJPEG0 y VideoCodecEngine. En una muestra de200ms, máximo
observado0.255468% correspondió a3D. No se inició benchmark ni dispositivo D3D.
Los números son observaciones, no constantes esperadas. Otros drivers, linked/multigpu,
hotplug y cobertura de procesos muy breves no quedan certificados por esta prueba.
Pendiente GPU Windows: temperatura, ubicación/fecha driver y capacidades de runtime,
memoria libre con ámbito válido y mejorar cobertura de carga no observada.

Fuentes: [motores y máximo como resumen en Task Manager](https://devblogs.microsoft.com/directx/gpus-in-the-task-manager/),
[arrays PDH y estados/flags](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw).
