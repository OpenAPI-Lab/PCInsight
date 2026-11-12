# Etiqueta de fabricante de inicio Windows (VERSIONINFO)

Opt-in independiente: include_startup_entries=true e
include_windows_startup_publisher=true (por defecto false). Lee CompanyName de
VERSIONINFO del ejecutable candidato, sin ejecutar código de la aplicación,
verificar firmas, elevar ni instalar dependencias. Usa Version.dll ya enlazada.
Este opt-in SÍ consulta el archivo candidato: es distinto del lector .lnk/.url,
que solo lee el acceso directo y no consulta destinos.

## Datos del GUI y confianza

StartupEntry.publisher_label contiene CompanyName con soporte estimated y
advertencia de procedencia; publisher_source_executable conserva la ruta
candidata también estimated. El string legacy publisher refleja esa etiqueta
solo cuando hay dato. Sin el opt-in, las entradas Windows siguen sin editor.

CompanyName es texto autodeclarado en un recurso, NO editor Authenticode,
firmante válido, prueba de integridad/confianza ni autoría del payload. Si el
EXE es un lanzador/intérprete, describe ese archivo, no sus argumentos, DLL,
script o aplicación cargada. El GUI debe mostrar la procedencia/estimación;
no añadir insignia de editor verificado. No se modifica status ni startup impact.

La ruta procede de un candidato de recursos, no identidad de lanzamiento
comprobada. Un archivo sustituido, enlace obsoleto, alias o cambio concurrente
puede cambiar el resultado. No se valida existencia/versión en el instante de
lanzamiento ni se habilita una acción de ejecución/control.

## Selección conservadora

- Run: prefijo EXE entre comillas cerradas seguido de whitespace/fin, o primer
  token sin comillas cuando ya es una ruta .exe absoluta sin espacios.
- Archivo .exe directo en Startup: su ruta source.
- .lnk: stored_local_target available de windows_shortcut available; requiere
  también el opt-in de metadatos y un LinkInfo local compatible. No hace Resolve.

Solo rutas explícitas de unidad letra:\ y extensión .exe (case-insensitive ASCII).
Sin búsqueda PATH, expansión de variables/porcentajes, UNC, prefijos extendidos,
drive-relative, slashes alternativos, streams, comodines ni comandos ambiguos.
No scripts/DLL, .url ni consulta de manejadores. Casos no cubiertos quedan sin
etiqueta; no se adivina el binario a partir del nombre de aplicación.

No es una promesa de IO exclusivamente local: una unidad puede estar mapeada,
una ruta atravesar reparse points y la API de versión puede consultar recursos
MUI. El proveedor no hace peticiones explícitas de firma/revocación/red ni carga
el ejecutable para lanzarlo. El host debe considerar políticas de rutas y el
costo de IO antes de activar el opt-in. No hay deadline de API.

## Errores y límites

Máximo50000 filas,128 rutas textuales distintas consultadas por muestra y16MiB
de valores de salida (contando dos veces la etiqueta reflejada en publisher).
Caché solo durante esa llamada, también para ausencias/errores; no persiste
CompanyName de archivos que podrían cambiar entre muestras. Diferencias de
case/alias de ruta no se canonicalizan ni deduplican por identidad de archivo.
Ruta hasta32768 bytes UTF-8 y etiqueta hasta16384 bytes, sin NUL incrustado.

Recursos nativos: buffer VERSIONINFO hasta4MiB, Translation hasta4096 bytes
(1024 pares idioma/codepage), valores hasta32768 wchar_t con terminador dentro
del recurso y conversión UTF-8 estricta. Recorre traducciones declaradas, no
fuerza inglés ni confunde ProductName/FileDescription con CompanyName.
Estos son límites de buffers/consultas, no cuotas estrictas de bytes de IO.

Sin recursos/campo/acceso/formato soportado, no hay valor. El lector nativo
reutilizado devuelve ausencia con causa no clasificada; no dice que toda ausencia
sea falta de permisos. Un proveedor inyectado que informa permission_required
lo conserva. Fallo en etiqueta no elimina la entrada de inventario. Otros
proveedores/plataformas no se modifican. API0.x ampliada: recompilar consumidores.

## Verificación

Pruebas portables de rutas citadas/no citadas, ambigüedad, .exe/.lnk/.url/scripts,
variables/UNC/streams/comodines, confianza de metadatos, caché, permisos/ausencia,
valores inválidos, límites128/50000/16MiB y preservación de filas ajenas.
Prueba Windows con CompanyName Unicode en segundo idioma del recurso de su propio
ejecutable de test: lectura nativa, selección Run inyectada sin escribir registro,
integración Monitor opt-in/opt-out y regresión FileDescription preservada.
No se cambió Startup real ni se ejecutó ninguna de sus entradas.

Resultado Release: Windows69/69 y Linux53/53, bibliotecas/headers instalados y
consumidores externos reconstruidos/ejecutados en ambos. El consumidor instalado
verifica default opt-out y coherencia de publisher/label/source en opt-in Windows.
Linux ejecuta selección/caché/suma de presupuesto portables y regresiones, no
el lector nativo VERSIONINFO. MSVC, otros equipos y otras variantes de recursos
requieren validación; esto no completa verificación de firmas/editor confiable.
La consulta Authenticode posterior es independiente y no cambia la confianza de
CompanyName: [firma por política local](WINDOWS_STARTUP_SIGNATURE.md).

Fuentes oficiales: [CompanyName en StringFileInfo](https://learn.microsoft.com/en-us/windows/win32/menurc/stringfileinfo-block),
[GetFileVersionInfoW y recursos MUI](https://learn.microsoft.com/en-us/windows/win32/api/winver/nf-winver-getfileversioninfow).
