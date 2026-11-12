# Inicio Windows: carpetas Startup conocidas

Monitor ahora combina registros Run con archivos de las carpetas Startup del
usuario caller y del sistema cuando include_startup_entries=true. Las rutas
proceden de FOLDERID_Startup / FOLDERID_CommonStartup, no de nombres ingleses
hardcodeados ni variables de entorno. No crea carpetas (KF_FLAG_DONT_VERIFY,
sin KF_FLAG_CREATE), modifica archivos ni ejecuta aplicaciones.

## Datos y límites de interpretación

Una fila por archivo directo no directorio/device/reparse-point, excluyendo
desktop.ini sin distinguir mayúsculas. Se conservan archivos ocultos regulares;
ser oculto no se interpreta como un flag enable/disable. No recorre subcarpetas
ni abre contenido/destinos de archivos o accesos directos. WINDOWS_SHORTCUT.md
añade lectura opt-in separada del contenido .lnk compatible (nunca del destino);
el inventario básico sigue sin abrirlo. La raíz la resuelve Windows y puede
estar redirigida o atravesar un junction del sistema;
el filtro reparse-point corresponde a las entradas, no garantiza ausencia de
redirecciones en componentes de la ruta raíz. Una ubicación UNC puede requerir
IO de red al enumerarse; no se crea un timeout para esa operación nativa.

name es el nombre de archivo completo, no el nombre comercial de la aplicación.
source es su ruta completa UTF-8. id=winfolder:user:HEX o winfolder:common:HEX,
codificando la ruta UTF-8 completa en hexadecimal. Nombres iguales en carpetas
distintas se conservan. Si ambas carpetas están redirigidas al mismo destino,
pueden aparecer dos registros por alcance; no se afirma que sean apps distintas.

command queda vacío: no se presenta un .lnk/.url/script como un ejecutable ni
se inventan argumentos. No usa ShellExecute, Resolve, carga Shell extensions,
COM de accesos directos ni asociación de extensiones. publisher vacío,
status=unknown, impact=not_measured, métricas observadas ausentes y
impact_scope=windows-startup-folder-registration-only. Los campos XDG no se usan.
El GUI debe indicar que son archivos observados y el destino no está resuelto.
No usar id/source directamente como petición autorizada de lanzamiento/borrado.

No determina aprobación efectiva/StartupApproved, validez o ejecución del archivo.
Revision43 agrega [control explicito del archivo](WINDOWS_STARTUP_FILE_CONTROL.md)
para el usuario actual: retirar/restaurar por rename sin reemplazo, con respaldo
en subcarpeta. No cambia el estado unknown del inventario ni implementa la
aprobacion efectiva del shell; no hay listado de respaldos integrado aun.
Revision44 completa list_windows_startup_file_backups como consulta explicita
separada para descubrir respaldos/conflictos y restaurar tras reiniciar el host.
No se mezcla automaticamente con Monitor ni se presenta como aprobacion Windows.
No añade tareas programadas, paquetes StartupTask, RunOnce ni otras cuentas.
Resolución de accesos directos, publisher, impacto, enable/disable/reset/properties
siguen pendientes en sentido efectivo del shell; WINDOWS_SHORTCUT.md aporta
solo metadatos guardados de un subconjunto .lnk; WINDOWS_INTERNET_SHORTCUT.md
añade destino guardado .url sin navegación. Resolución/apertura efectiva
y las demás fuentes siguen pendientes. No es inventario universal de autoruns.

## Fallos y acotación

Carpeta ausente o sin coincidencias se omite. Fallo al obtener la ruta conocida,
permiso denegado o enumeración interrumpida lanza system_error, sin lista parcial.
Errores de Known Folders conservan HRESULT; Find conserva código Win32. Fallar
uno de los dos proveedores Run/carpetas hace fallar la muestra combinada.
Lista vacía no prueba ausencia de apps de inicio en el equipo. No se eleva ni
consulta otro token de usuario, y no hay reintentos ocultos.

Ruta raíz absoluta y literal validada, incluidos prefijos extendidos Win32;
se rechazan rutas de dispositivos, relativas, comodines y UTF-16 inválido.
Revision42 corrige un fallo reproducido: `std::filesystem::is_absolute`
aceptaba indebidamente una ruta con sintaxis de stream (`C:\Fixture:stream`).
La comprobacion aislada del predicado anterior confirma ese caso; GLOBALROOT,
pipes y UNC incompleto ya eran rechazados por el runtime actual, no se afirma
una vulnerabilidad previa de esos casos. Ahora se exige una unidad absoluta (`C:\...`) o servidor/share
UNC no vacios, con sus variantes extendidas; el prefijo por si solo no autoriza
otros namespaces. Se rechazan `GLOBALROOT`, pipes, namespaces Volume GUID,
UNC sin share, rutas relativas a unidad y colon de stream. Volume GUID queda
fuera del proveedor por politica conservadora, no se afirma que no sea un
filesystem. Se conserva la ruta original para Win32 y se aceptan `UNC`/`unc`
extendidos. No se abren ni enumeran rutas rechazadas. Esto valida sintaxis,
no evita junctions/redirecciones de la raiz ni autentica el servidor UNC.
Máximo32765 unidades UTF-16 de raíz, nombre terminado dentro de cFileName,
10000 entradas por carpeta (incluidas ignoradas) y16MiB de texto de salida.
Nombres duplicados exactos, separadores/colon en nombre, ruta ausente, API
incompleta o límites excedidos fallan sin truncar. Sets de nombres quedan
acotados por número y tamaño nativo de nombres. Sin snapshot atómico ni garantía
frente a renombrados/cambios concurrentes; no certifica identidad persistente.

CoTaskMemFree libera rutas incluso si SHGetKnownFolderPath falla tras asignar;
FindClose libera búsquedas también en excepciones. El proveedor no inicializa
ni cambia el apartment COM del caller. Shell32/UUID/Ole32 son bibliotecas del sistema enlazadas al
compilar, no paquetes que deba descargar el usuario. La salida combinada con
Run está acotada por ambos proveedores: hasta50000 registros y32MiB de texto
en AMD64, sin deduplicación semántica entre registro y archivos.

## Verificación

Pruebas inyectadas: dos carpetas, Unicode, mismo nombre en distintos alcances,
ocultos, exclusión de metadatos/directorios/reparse, rutas relativas/dispositivo/
comodines/excesivas/extendidas/Unicode inválido, nombre sin terminador/duplicado,
permisos/fallos tardíos, carpetas ausentes, API incompleta, límites de entradas y
payload, limpieza incluso cuando falla la segunda carpeta o una asignación.
La regresión de ruta extendida falló primero y se corrigió normalizando solo
la comprobación de raíz; la ruta usada por Win32 se conserva sin quitar prefijo.
Prueba nativa read-only y Monitor combinado opt-in/opt-out con procesos ocultos.
No se escribió en las carpetas reales, cargó un destino ni ejecutó un archivo.

2026-10-19, revision42: la nueva regresion fallo antes del arreglo y paso despues.
Casos nuevos comprueban rechazo antes de FindFirstFile y liberacion de la ruta;
casos positivos UNC/extendidos conservan inventario y cleanup. Windows122/122
Release,20repeticiones de windows-startup-folders, consumidor instalado relinkado
y ejecutado exit0. Rutas especiales positivas/negativas son fixtures inyectadas;
la lectura nativa usa solo las carpetas conocidas existentes. No se modifica
StartupApproved ni se implementa enable/disable por esta correccion. M05 sigue
abierta; prueba separada windows-run-control confirma6801 sin escrituras.
Logs work/startup-root-*-windows.log; no es CI ni prueba nativa UNC/Volume GUID.

Resultado Release: Windows66/66 y Linux50/50; bibliotecas/headers instalados y
consumidores externos reconstruidos/ejecutados en ambos. El consumidor Windows
acepta ambos tipos de registro en Monitor. Linux verifica regresiones propias,
no Known Folders. MSVC/ARM64/x86 y redirección UNC real quedan sin verificación
nativa; la ruta extendida se verificó con API inyectada, no en un filesystem largo.

Fuentes oficiales: [SHGetKnownFolderPath](https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath),
[Known Folder IDs](https://learn.microsoft.com/en-us/windows/win32/shell/knownfolderid),
[FindFirstFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilew).
