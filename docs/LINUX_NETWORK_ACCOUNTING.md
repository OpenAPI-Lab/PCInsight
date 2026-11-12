# Contabilidad interna de actividad de sockets (F03 en curso)

Actualizacion revision66: [sampler SELF](LINUX_NETWORK_WINDOW.md) conecta la
captura privada y este reductor para un actor/cookie declarados. El scope
privado distingue dominios namespace_inode/kernel_initial y cobertura SELF
de una fila frente a buckets declarados. No certifica enumeracion global ni
join procfs; el contrato/requisitos generales siguientes permanecen vigentes.

`src/linux_network_accounting.hpp/.cpp` implementa la reducción de contadores
acumulados por actor a tasas TCP/UDP y sumas por UID. Es código portable sin
syscalls, cargador, captura, hilos ni almacenamiento. No hay API pública nueva,
no está conectado al Monitor y no rellena red en ProcessInfo/UserSession.
**Los fixtures no demuestran captura TCP/UDP en el kernel.**

## Contrato de entrada para el futuro productor

Cuatro contadores uint64 por bucket: TCP envío/recepción y UDP envío/recepción.
Su significado previsto son bytes de payload de operaciones de socket exitosas
observadas por el productor, no bytes ACKed TCP, retransmisiones, cabeceras,
tráfico de interfaz ni propiedad exclusiva de un socket. Definir y validar los
casos MSG_PEEK/MSG_TRUNC, operaciones no cubiertas, async/io_uring y las firmas
reales de hooks es responsabilidad pendiente del productor. Este módulo no
decide que una llamada equivale a una transferencia física.

Identidad de bucket: TGID, start_boottime del líder, UID real **al ejecutar la
operación** y cookie del namespace de red del socket. No inferir actor por FD,
inode, socket UID ni holder. Cambios de credenciales producen buckets distintos;
los contadores de un UID anterior no se reasignan al dueño actual del proceso.
La salida puede tener varias filas para el mismo TGID. No es un listado de
sesiones actuales ni se puede multiplicar por sesiones del mismo usuario.

El ámbito incluye boot, epoch único de instancia del productor y dominios
explícitos de PID/UID. Un futuro productor debe resolver esos dominios, probar
su correspondencia con procfs/sesiones y cambiar epoch si reinicia/recrea mapas.
No convertir start_boottime en start_ticks mediante este reductor. Ninguno de
estos tokens se valida contra el kernel aquí: son precondiciones de entrada.

Una entrada Support::available certifica el contrato del productor: enumeración
completa, punto temporal monotónico no cero y filas coherentes, con un contador
monotónico de actualizaciones perdidas. El productor debe comprobar la coherencia
de lecturas concurrentes/contadores de pérdida antes de emitir esa certificación;
un recorrido de mapa sin garantías no basta. La marca por fila coherent=false
o saturated=true permite conservar cobertura de un bucket sin dar cifras.
Los cuatro lanes deben estar cubiertos por los hooks declarados del productor;
si no soporta UDP o alguna dirección, no puede certificar esa entrada como
available ni rellenar el lane ausente con0. Enumeración/coherencia por defecto
son false: construir una estructura vacía no certifica una lectura.
Estimated como soporte del snapshot completo no satisface el contrato y se
rechaza. Revision66 solo certifica el scope SELF de una fila instrumentada,
no el contrato de un productor general multiactor.

## Ventanas, pérdidas y agregación

- La primera muestra solo expone contadores; necesita dos extremos para tasas.
- Mismo ámbito y bucket, tiempo creciente y máximo60s por intervalo. Es límite
  entre muestras, **no** buffer de gráfica ni historial de60s.
- Cualquier contador que retrocede invalida las cuatro tasas del bucket en ese
  intervalo. Saturación/incoherencia impide usar ambos extremos afectados.
- Una pérdida nueva invalida todas las tasas del intervalo que la cruza. El
  siguiente intervalo sin pérdidas nuevas puede medirse; los totales acumulados
  siguen estimated como límite inferior si hubo pérdidas desde el inicio.
- Un bucket desaparecido no deja baseline; su reaparición es primera muestra.
- Ausencia, denegación, duplicados, ámbito/tiempo inválido, reloj/pérdidas que
  retroceden, enumeración parcial o más4096filas descartan todos los baselines.
- El cambio de ámbito o intervalo demasiado largo comienza nuevos baselines.

Las tasas se calculan como floor(delta_bytes * 1e9 / intervalo_ns), sin floats
ni multiplicación uint64 que pueda desbordar. División binaria portable conserva
exactitud incluso en UINT64_MAX. Un resultado fuera de rango queda ausente,
no saturado silenciosamente. Un delta cero válido sí produce tasa cero.

Sumas UID: solo buckets con tasa conocida, siempre estimated/parciales, con
known/missing/overflow por cada uno de los cuatro contadores. No se suman
ausencias como0. Overflow de la suma invalida esa métrica; conocidas/ausentes
se conservan. Sin ningún bucket medido no se fabrica un UID con tasa0. La
salida de una enumeración completa vacía es una lista vacía, no tráfico global0.
No se suman ni mezclan estos payloads con TCP_INFO ACKed/received o interfaces.

El reductor no es thread-safe. Máximo4096filas por entrada; estado acotado a
la muestra anterior, sin caché global. Orden determinista por clave y UID.
No es validación de un blob/mapa BPF ni límite de tiempo duro del productor.

## Verificación y pendiente

Fixtures: cuatro lanes TCP/UDP, intervalo2s, cero conocido, identidad/UID/boot/
epoch/namespaces, pérdidas/retroceso, saturación, incoherencia, desaparición,
enumeración vacía/parcial, duplicados, ausencia/permiso/support contradictorio,
límites, overflow de tasa/suma y fronteras uint64. Oráculo portable determinista
de10000casos aritméticos donde la multiplicación de referencia no desborda.
Resultados Release finales: Linux99/99, Windows113/113, ambos consumidores
instalados relinkados/ejecutados exit0. Builds finales sin warnings. Primer
Windows en sandbox98/113 con15fallos (acceso a registro/servicios/temporales,
PDH); repetido fuera del aislamiento113/113 sin cambiar código de esos tests.
El log fallido `work/network-accounting-suite-windows.log` se conserva, junto
a `network-accounting-verified-suite-windows.log` y la suite final
`network-accounting-final-suite-windows.log`. Linux inicial/final99/99 en
`network-accounting-suite-linux.log` y `network-accounting-verified-suite-linux.log`.
Logs build/install/consumer con el mismo prefijo. Riesgos históricos N03 no se
declaran corregidos por estas suites verdes.

F03 sigue E. Faltan cargador/bytecode/attach seguro, productor contable real,
filtros y alcance verificables, compatibilidad/privilegios, cobertura y pruebas
TCP/UDP positivas, integración/ventanas públicas y unión de identidades con
usuarios/apps. J04/H04 no se cierran. Esta unidad no carga sondas ni cambia
privilegios. El2026-10-18 el usuario autorizó explícitamente la futura prueba
temporal en WSL, filtrada a procesos privados y retirada al terminar, con los
privilegios necesarios, sin configuración permanente ni instalar herramientas. Este
módulo no agrega IDs al inventario para inflar el porcentaje.
