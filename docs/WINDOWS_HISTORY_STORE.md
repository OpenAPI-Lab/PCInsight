# Persistencia explícita del historial observado Windows

`pcinsight/windows_history_store.hpp` expone `WindowsHistoryStore`: load, save,
merge, retain_since, prune y reset para WindowsAppHistoryEntry. Es portable:
Linux puede leer/escribir este informe, pero eso no activa un colector Win32.
Monitor/Sampler no guardan automáticamente ni escogen rutas. El consumidor
decide cuándo persistir y debe comprobar cada resultado antes de continuar.

Este almacén conserva valor opcional, Support y nota de CPU observado, red y
notificaciones. Ausente no se convierte en cero medido; las estimaciones siguen
siendo estimaciones. Persistir no añade colectores de red/notificaciones ni
reconstruye CPU anterior a la captura. Ver WINDOWS_APP_HISTORY.md.

## Uso y combinación

Para un archivo existente válido, cargar con load, comprobar success, combinar
sus entries con el valor del Metric snapshot.windows_app_history mediante merge,
y guardar el resultado con save. Para una ruta nueva, empezar con un vector
vacío explícitamente; no tratar cualquier error de load como archivo nuevo.
Si el Metric del snapshot no tiene valor, no sustituirlo por contadores cero.
El programa debe serializar toda la secuencia load/merge/save, incluyendo prune
y reset, entre hilos y procesos que compartan destino.

La clave acumulativa es (id, period_start en milisegundos). Snapshots repetidos
reemplazan esa fila, no suman de nuevo su CPU. Snapshots con period_end anterior
se ignoran. Uno más reciente o igual no puede reducir un contador conocido ni
perder su valor; merge lanza std::invalid_argument. Ausente a conocido sí está
permitido. Un reset de contadores requiere periodo nuevo; distintos colectores
no deben compartir id y comienzo de periodo en el mismo milisegundo. Periodos
distintos permanecen separados. Nombre/notas/soporte proceden de la fila aceptada.

retain_since conserva periodos completos cuyo final es igual o posterior al
cutoff, ambos normalizados a milisegundos. No prorratea consumo de periodos que
cruzan el límite. prune carga, filtra y guarda; no hay hilo de mantenimiento.
reset elimina exclusivamente la ruta elegida y reporta errores; ausente ya está
reseteado. Es una acción destructiva explícita, no recuperación de corrupción.

## Formato y límites

Formato propio v1, little-endian, distinto de HistoryStore legacy v1/v2:

- Magic de 8 bytes PCIWHST seguido de NUL; uint32 versión1 y uint32 filas.
- Por fila: id y nombre (cada cadena lleva longitud uint32), inicio/final uint64
  en milisegundos desde epoch, seguidos por CPU, red y notificaciones.
- Por Metric: byte Support (0 available, 1 permission_required, 2 unsupported,
  3 estimated), byte presencia0/1, uint64 de valor si presente, cadena de nota.
  CPU se expresa en nanosegundos no negativos que caben en int64; otros dos son
  contadores uint64. available/estimated requieren valor; los otros no lo admiten.
- Trailer uint64 FNV-1a sobre todos los bytes anteriores. Detecta corrupción
  accidental, no autentica datos ni impide manipulación deliberada.

Límites:64MiB por archivo codificado,100000filas,1MiB por cadena, identidad no
vacía y sin NUL, nombre sin NUL, tiempos no negativos y final no anterior al
inicio. Notas conservadas como bytes, sin validar UTF-8. Los tiempos se truncan
a milisegundos. load rechaza formatos/versiones desconocidos, truncamientos,
flags inválidos, intervalos/rangos inválidos y bytes sobrantes sin publicar filas
parciales. format_version informa1 solo tras lectura correcta. Estos presupuestos
no son límites de RAM total: hay copias de buffers, cadenas, mapas y filas.

No hay conversión automática desde/hacia HistoryStore. Elegir un archivo distinto:
el legacy no conserva disponibilidad por campo. Ambos lectores rechazan el otro
formato; el formato legacy existente y su migración v1/v2 no cambian.

## Escrituras y seguridad

save valida entradas y, si el destino existe, exige poder cargarlo como historial
Windows válido antes de reemplazarlo. Rechaza destinos corruptos/legacy/ajenos;
no los repara ni los sobrescribe deliberadamente. Esa comprobación es previa,
no compare-and-swap: otro escritor puede cambiar la ruta después. El caller
debe coordinar escritores y elegir un directorio de confianza; no es una defensa
contra un atacante con control del directorio ni un mecanismo de bloqueo.

Se reutiliza el reemplazo del HistoryStore: temporal hermano exclusivo, flush,
cierre y reemplazo del destino sin retirarlo primero ni usar backup compartido.
Los fallos antes del reemplazo conservan el destino y limpian el temporal propio.
Las garantías dependen del filesystem; no hay promesa universal ante corte de
energía, Linux no fsync del directorio y en Windows el temporal hereda ACL del
directorio, no conserva necesariamente metadatos del archivo sustituido.
No hay cifrado; rutas de ejecutables, nombres y notas pueden ser sensibles.
El consumidor elige permisos/ACL adecuados y maneja errores/excepciones de recursos.

## Verificación

2026-10-15: Release Windows78/78 y Ubuntu WSL Linux58/58, instalación y consumidor
externo reconstruidos/ejecutados exit0 en ambos. Nuevo test windows-history-store
con assertions activas en Release: roundtrip de notas/soportes/Unicode, ausente
frente a cero medido, merge repetido/stale/decreciente/pérdida de valor/periodos,
retención, cada truncamiento de un fixture, checksum, flags y tamaños alterados,
CPU overflow, tiempos invertidos/negativos, trailing bytes, formatos separados,
save rechazado conservando archivos corruptos/legacy/válidos ante entradas
inválidas, directorio destino, backup ajeno conservado, limpieza y reset repetido.
El consumidor instalado incluye el header y enlaza load/save/merge con entradas
vacías sin escribir datos del usuario. No se simula caída eléctrica ni se prueba
concurrencia como transacción; no hay afirmación de bloqueo multiwriter.
En Windows, ejecutar estas pruebas de filesystem con TMP/TEMP apuntando a work
y acceso permitido a sus temporales privados, como la suite completa. Una pasada
adicional sin ese entorno falló en el primer save; repetida con el entorno de la
suite pasó1/1. No se cambió la implementación para eludir permisos del sistema.
