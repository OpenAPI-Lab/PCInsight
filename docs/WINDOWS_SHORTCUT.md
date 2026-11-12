# Metadatos guardados de accesos directos .lnk

Opt-in: include_startup_entries=true e include_windows_startup_shortcut_metadata=true
(por defecto false). Intenta .lnk de filas winfolder:, no comandos Run;
WINDOWS_INTERNET_SHORTCUT.md añade .url al mismo pipeline/presupuesto. Parser
propio: no usa ShellExecute, IShellLink/Resolve, COM, extensiones,
tracking, expansión de variables ni programas externos. Lee el archivo del
acceso directo; nunca abre ni comprueba su destino.

## API y significado

StartupEntry.windows_shortcut es Metric<WindowsShortcutInfo>, con métricas
stored_local_target, arguments y working_directory. Son texto guardado, NO ruta
resuelta/verificada, lista segura de argumentos ni autorización de lanzamiento.
Ausencia de HasArguments se refleja como string vacío disponible: no hay texto
de argumentos en StringData, no una certificación de la invocación efectiva.
Working directory ausente queda sin valor.

stored_local_target concatena base/sufijo locales de LinkInfo compatible, con
raíz de unidad absoluta; prefiere UTF-16 y admite ASCII legacy. ForceNoLinkInfo
suprime ese campo. IDList, UNC/network, entorno, Darwin, propiedades y otros
mecanismos no se resuelven. El texto local puede estar obsoleto o no ser el que
Windows elegiría al lanzar. No se modifica command vacío, status unknown,
publisher ni impacto not_measured. No se normalizan rutas relativas, interpretan
comillas ni ejecutan argumentos. El GUI debe etiquetar/escapar texto no confiable.

UTF-16 se valida incluidas parejas surrogate. Legacy no ASCII queda ausente:
no se adivina la página de códigos del sistema de origen.

## Límites y errores

Comprueba header/CLSID, rangos/offsets usados, IDList acotado, LinkInfo local,
StringData contada, Unicode y bloques ExtraData acotados con terminal final.
NUL incrustado, truncamiento, trailing data y overflow de rangos se rechazan.
NO es un validador semántico completo: payloads IDList, VolumeID y ExtraData no
se decodifican completamente ni prueban legitimidad/selección del destino.

Límites:1MiB por archivo,128 intentos/muestra,16MiB de payload aceptado y16MiB
de texto retenido,1024 bloques ExtraData,260 unidades de StringData salvo
argumentos (count uint16). Presupuesto agotado deja metadatos ausentes con nota;
fallo de lectura/parser afecta esa métrica, no borra el archivo del inventario.
permission_required se conserva. Lecturas fallidas/descartadas no cuentan como
payload aceptado: peor caso nativo128 intentos de1MiB de IO, no garantía16MiB de
IO estricto. No hay timeout para syscalls/UNC bloqueadas.

CreateFileW usa solo lectura y OPEN_REPARSE_POINT. Rechaza handles no disk,
directorios, reparse y tamaños excesivos. Lee desde el mismo handle y compara
tamaño/last-write antes/después. No es atomicidad ni protección contra todos
los cambios concurrentes; la raíz Startup puede estar redirigida. No modifica
el apartment COM ni eleva privilegios.

## Verificación y pendientes

Fixtures de destino local Unicode, argumentos/directorio, Unicode suplementario,
ASCII legacy y no ASCII, ForceNoLinkInfo, red sin resolución, todos los
truncamientos de una fixture, offsets/tamaños/Unicode/NUL/trailing data inválidos,
límites1MiB/128archivos/16MiB, opt-in/opt-out y permiso inyectado. Lectura Windows
real de bytes de una fixture temporal TMP/work y Monitor sobre inventario real;
no se escriben Startup ni destinos. No certifica todos los formatos del shell.
La fixture se elimina por archivo exacto y directorio vacío, sin borrado recursivo.
API0.x ampliada: recompilar biblioteca y consumidores juntos.

Resultado Release: Windows67/67 y Linux51/51, bibliotecas/headers instalados y
consumidores externos reconstruidos/ejecutados en ambos. El parser portable se
prueba en Linux; la lectura nativa de .lnk y el opt-in Monitor solo en Windows.
El consumidor instalado verifica flag por defecto false y usa el opt-in Windows.
Los metadatos añaden memoria/IO al snapshot y a su historial si el consumidor
los conserva; el presupuesto se reinicia en cada muestra, no es cuota temporal.

Pendientes: formatos/codepages adicionales y resolución efectiva,
aprobación enabled/disabled, publisher/impacto, acciones y otras fuentes autorun.

Fuentes: [MS-SHLLINK](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/747629b3-b5be-452a-8101-b9a2ec49978c),
[LinkInfo](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/6813269d-0cc8-4be2-933f-e96e8e3412dc),
[StringData](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/17b69472-0f34-4bcf-b290-eccdb8de224b),
[LinkFlags](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/ae350202-3ba9-4790-9e9e-98935f4ee5af).
