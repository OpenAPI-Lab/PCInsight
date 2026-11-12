# Procesos Windows: implementación inicial

Revision60: [decoder ETW y prueba de disco privada](WINDOWS_DISK_EVENTS.md).
Positivo controlado de un hilo propio, NO recolector general por PID ni nueva
metrica de Monitor. Disco/red de ProcessInfo siguen pendientes de ese proveedor.

Revision57: [E/S general por proceso](WINDOWS_PROCESS_IO.md) completa
ProcessInfo.windows_io con seis contadores acumulados y tres tasas, con
identidad/intervalo propio. No sustituye disco/red especificos pendientes.
Nuevo layout ProcessInfo0.x: reconstruir todos los consumidores. La captura
interna compartida puede continuar por usuarios/historial aun sin exponer
filas de procesos; las afirmaciones historicas de desactivacion se leen con
esa condicion. No hay E/S agregada nueva por usuario/app en esta revision.

Actualizado 2026-10-13. Enumeración nativa mediante Toolhelp, sin subprocess,
instalaciones adicionales, terminación de procesos ni cambios de privilegios.

## Datos disponibles

- PID, PID padre, nombre y cantidad de threads del snapshot Toolhelp.
- windows_session_id: ID Terminal Services del token primario, opcional y sin
  fallback por PID; se usa junto a cuenta/creación para sumas visibles por sesión.
  Ver WINDOWS_USER_RESOURCES.md; no es identidad persistente para acciones.
- Lectura de política EcoQoS disponible/solicitada, cuando Windows permite
  consultarla; ver WINDOWS_EFFICIENCY.md para la semántica y la acción.
- `windows_creation_time_100ns`: fecha nativa de creación si GetProcessTimes
  es accesible y el proceso no nació después del inicio del snapshot. Conservar
  junto al PID para `terminate_process_checked`; nunca redondear ni convertir
  a Unix antes de pasarlo a esa API. Sin dato no se ofrece cierre verificado.
- Ruta del ejecutable y arquitectura mediante APIs de Windows. Si
  `IsWow64Process2` no existe o no reconoce el tipo, la arquitectura queda vacía.
- Descripción desde el recurso VERSIONINFO `FileDescription`, convertida a UTF-8.
  Se recorren los idiomas/codificaciones declarados hasta encontrar una cadena
  válida; no se garantiza preferencia por el idioma del GUI. Ausencia, archivo
  inaccesible o recurso inválido dejan el campo vacío. Es metadato no verificado,
  no prueba de firma, editor o identidad legítima del programa.
- Usuario en formato dominio/nombre desde el token del proceso. La resolución
  de cuentas de dominio puede depender de conectividad y no tiene un plazo
  máximo propio en esta implementación.
- Memoria residente (working set), incluidas páginas compartidas. No equivale
  al working set privado ni se puede sumar entre procesos sin doble conteo.
- CPU: delta de tiempo kernel+usuario dividido entre tiempo monotónico y todos
  los procesadores lógicos. Primera muestra, contador regresivo, cambio de
  cantidad de CPUs o PID reutilizado no producen porcentaje. Un delta cero
  válido sí produce 0%. No es utilidad ponderada por frecuencia.

El estado `active` significa que el proceso no había terminado al consultar
sus tiempos; no certifica que sus threads estén ejecutándose en ese instante.
También se pueden obtener `exited` y `unknown`. No se detecta aún suspensión
ni falta de respuesta del GUI.

## Consistencia y errores

Cada delta exige el mismo PID y fecha de creación. Se descartan mediciones si
el proceso abierto nació después del inicio del snapshot, evitando mezclar
metadatos anteriores con un PID recién reutilizado. Los snapshots no son una
transacción atómica: los procesos pueden terminar durante la lectura.

Los procesos inaccesibles conservan el inventario básico, con métricas sin
valor y `permission_required` cuando Windows devuelve acceso denegado. Las
cadenas de metadatos no disponibles quedan vacías; la API pública actual no
incluye un estado por cada cadena. No se adivina SYSTEM por el PID.

Un fallo global de Toolhelp lanza `std::system_error`, no una lista vacía o
parcial presentada como exitosa. El consumidor de `Monitor` debe gestionar la
excepción; `Sampler` ya dispone de captura de errores. La enumeración termina
completa antes de abrir procesos individuales. Valida tamaño del registro,
terminación NUL del nombre y unicidad de PID; admite hasta 100000 registros.
Un registro inválido, exceso del límite o fallo intermedio descarta la lista
entera y reinicia cachés/deltas. El snapshot se cierra antes de leer métricas.

Pruebas Toolhelp inyectadas cubren error al crear/leer primero/leer siguiente,
snapshot vacío, handle nulo, registros malformados, nombres sin NUL, PID
duplicado, límite exacto/excedido, cierre de handles y recuperación posterior.
Estas pruebas no abren, modifican ni terminan procesos reales. Las consultas
de metadatos y métricas posteriores aún necesitan ampliar inyección de fallos.

`include_processes=false` evita la enumeración y borra sus deltas. Al volver
a activarlo hay que esperar otra muestra antes de obtener porcentajes. Se
eliminan de la caché los PIDs desaparecidos o inaccesibles.

Las descripciones se conservan por ruta mientras esa ruta siga presente entre
procesos accesibles. Esto evita releer el recurso en cada muestra, pero puede
retener metadatos anteriores si se reemplaza el archivo en disco. Recrear el
monitor o desactivar/reactivar procesos invalida la caché. Solo se aceptan
rutas absolutas, recursos de hasta 4 MiB y textos de hasta 32768 caracteres.
No se carga ni ejecuta el programa descrito.

## Pendiente

Estado detallado de threads/suspensión,
prioridad combinada del modo de eficiencia, E/S de disco y red atribuida por proceso. EcoQoS
está implementado como componente energético. No se usan contadores
genéricos de E/S como sustituto de disco: pueden incluir otros dispositivos.
Run new task está implementado mediante CreateProcessW, con pruebas inyectadas;
ver WINDOWS_LAUNCH.md para rutas, argumentos y límites. End task forzado está
implementado con pruebas inyectadas; ver WINDOWS_TERMINATE.md. La variante
checked compara PID+fecha; la antigua solo por PID mantiene su riesgo de
reutilización. Cierre cooperativo sigue pendiente.

## Verificación

Recurso VERSIONINFO compilado dentro del ejecutable de prueba: primer idioma
sin descripción y segundo con una descripción acentuada. Se comprueba selección
del segundo idioma, conversión UTF-8, estabilidad entre muestras y rechazo de
rutas vacías, relativas o inexistentes. Suite actual Windows 19/19 y Linux WSL 6/6; instalación
CMake y consumidor externo verificados con la dependencia nativa `version`.

La integración encuentra el propio proceso de prueba y comprueba identidad,
usuario, ejecutable, memoria, threads, CPU posterior y desactivación/reactivación.
Fixtures de deltas cubren PID reutilizado, topología cambiada, tiempo no positivo,
contadores regresivos, cero válido, >100% y CPU desconocida. No se modifica ni
finaliza ningún proceso real. Falta ampliar validación a procesos protegidos,
dominios, MSVC y otras arquitecturas.

Referencias: [enumeración de procesos](https://learn.microsoft.com/en-us/windows/win32/procthread/process-enumeration),
[working set](https://learn.microsoft.com/en-us/windows/win32/procthread/process-working-set)
y [funciones nativas de procesos](https://learn.microsoft.com/en-us/windows/win32/procthread/process-and-thread-functions).
