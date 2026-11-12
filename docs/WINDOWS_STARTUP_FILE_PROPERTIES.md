# Properties del archivo fuente de Startup Windows

`get_startup_properties(entry.id)` ahora admite `winfolder:user:...` y
`winfolder:common:...`, además de Run. Entrega datos del **archivo de Startup**,
no del ejecutable al que pudiera apuntar. Consulta explícita de solo lectura,
sin GUI del sistema, shell/COM, resolución de destinos, iconos, firma, CompanyName,
expansión de variables, lanzamiento, elevación ni cambio de habilitación.

## Identidad y campos

Selecciona el ID exacto dentro de un inventario recién obtenido de las carpetas
conocidas de Startup del caller. No decodifica un ID como permiso para abrir una
ruta arbitraria. Verifica scope, hex canónico y correspondencia exacta ID/source;
ID desconocido -> not_found. Otro contexto de carpetas o usuario puede invalidarlo.
El inventario se vuelve a consultar antes de terminar: entrada ausente/cambiada
-> stale_target. Una falla del inventario no se confunde con carpeta vacía.

`StartupProperties.source_file_size_bytes` devuelve tamaño lógico observado,
también si es0; no tamaño del destino ni espacio físico ocupado. El legacy
file_size_bytes continúa Linux-only. fields tiene grupo `Windows Startup file`:
path, scope, size_bytes, attributes_raw, creation_filetime_100ns y
last_write_filetime_100ns. Atributos son el bitmask Win32 en decimal. Fechas son
los FILETIME crudos reportados (intervalos100ns desde1601), no una promesa de
precisión/disponibilidad uniforme entre filesystems. No se devuelve last-access,
porque leer el archivo puede actualizarlo.

Entry mantiene nombre/ruta/ID actuales, command vacío, status unknown e impacto
not_measured. No promociona registro a autorun aprobado ni a acción autorizada.
La función no reusa metadata opcional de otro muestreo/handle.

## Metadatos guardados de accesos directos

En `.lnk` y `.url` de hasta1MiB lee bytes por el mismo handle utilizado para
consultar propiedades y llena `entry.windows_shortcut` con los parsers existentes:
[Shell Link](WINDOWS_SHORTCUT.md) e [InternetShortcut](WINDOWS_INTERNET_SHORTCUT.md).
La consulta Properties es explícita: no requiere los flags de SampleOptions de
Monitor. Un formato inválido/no soportado deja windows_shortcut sin valor y nota,
pero no elimina propiedades válidas del archivo. Archivos mayores de1MiB conservan
tamaño/atributos/fechas sin leer contenido; otros tipos nunca leen el contenido.
Metadatos de destino, argumentos y URLs son texto no confiable, nunca permisos de
ejecución/navegación ni prueba de un destino vigente. No se abre el destino.

## Consistencia, errores y límites

CreateFileW GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING y
FILE_FLAG_OPEN_REPARSE_POINT; exige archivo de disco y rechaza reparse point final,
directorio o device. Requiere permiso de lectura de datos incluso si el tipo no
necesita contenido. Compartición restrictiva puede fallar si ya hay un escritor.
Compara metadatos antes/después (atributos, tamaño, creación/escritura y par
volumen/file-index64). Luego reabre la ruta y compara otra vez, manteniendo el
primer handle abierto. Los handles se cierran por RAII en éxito/error.

Esto es detección de cambios, no snapshot atómico, identidad histórica/futura,
inmutabilidad universal ni token de control. File-index64 depende del filesystem
y no es identidad única garantizada en ReFS; no sirve de autorización. No detecta
todo ABA/cambio con metadatos iguales. El host puede sustituir el archivo tras la
consulta. Una carpeta conocida redirigida, unidad mapeada o reparse intermedio
puede producir IO de red: no hay promesa de almacenamiento exclusivamente local.

Permiso denegado -> permission_denied; archivo/ID ausente inicialmente -> not_found;
cambios detectados, lectura acortada o ruta no revalidable -> stale_target. Errores
nativos no clasificados -> system_error con código. Sintaxis/identidad inconsistente
-> invalid_argument; API incompleta/tipo no disco -> not_supported. Si falla IO del
contenido no se devuelve un resultado parcialmente actualizado. Formato de acceso
directo no soportado, en cambio, no es un fallo de IO.

Máximo20000 filas del inventario por consulta de selección (dos observaciones),
ID262160B, source131068B UTF-8 y32765 wchar_t, nombre32768B. El proveedor nativo de
inventario conserva sus propios límites de carpetas/texto. Contenido como máximo
1MiB, lecturas de hasta64KiB, máximo256 llamadas ReadFile. Metadatos de archivos
grandes pueden devolver tamaños64bit sin leer el contenido. Sin deadline duro de
API ni bloqueo de todos los cambios de filesystem. Consumidores API0.x deben
recompilar; ninguna dependencia runtime nueva.

## Verificación

2026-10-15: Windows72/72, Linux54/54 Release y consumidores instalados ejecutados
exit0. Windows: consulta pública de3 entradas reales de Startup; fixture `.url`
privado temporal, leído con proveedor de archivo nativo e inventario inyectado,
sin añadirlo a Startup ni abrir su URL. Se eliminó solo ese fixture al terminar.
Pruebas inyectadas de tamaño64bit/0-lecturas en otros tipos, parser inválido,
archivo grande, permisos/IO, reparse/directorio/device/tipo pipe, cambio/desaparición/
sustitución, inventario duplicado/erróneo, IDs y límite de llamadas de lectura,
con cierre de recursos. Linux verifica regresiones XDG, no estas APIs Win32.
MSVC, más equipos, filesystems y red requieren validación adicional.

Fuentes oficiales:
[GetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle),
[límites de identidad y fechas BY_HANDLE_FILE_INFORMATION](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/ns-fileapi-by_handle_file_information),
[CreateFileW y flags](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew).
