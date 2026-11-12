# Destino guardado de Internet Shortcuts (.url)

El opt-in include_windows_startup_shortcut_metadata ahora admite .lnk y .url de
filas winfolder:. También requiere include_startup_entries. .url aporta
windows_shortcut.value->stored_url; stored_local_target, arguments y
working_directory quedan ausentes. No se rellena command ni se cambia status
unknown, publisher o impacto not_measured. No es un ejecutable local.

El lector reconoce un subconjunto conservador de texto INI: una sección
InternetShortcut con una clave URL, comparadas sin distinguir mayúsculas ASCII.
Ignora otras secciones y propiedades no utilizadas. No carga IconFile, iconos
remotos, handlers, navegador ni COM. No usa GetPrivateProfileString ni sus posibles
mapeos del registro; analiza solo los bytes del archivo observado. No ejecuta,
visita, resuelve DNS ni valida disponibilidad o seguridad del destino.

## Texto y valores admitidos

ASCII sin BOM, UTF-8 estricto con BOM y UTF-16LE con BOM. No adivina páginas de
códigos para legacy no ASCII ni UTF-8 sin BOM. UTF-16BE no soportado. Rechaza
Unicode malformado, surrogates inválidos, overlong UTF-8 y NUL en cualquier parte.
Admite LF/CRLF, comentarios de línea ;/# y espacio/tab alrededor de claves,
secciones y valores. No elimina comentarios al final de la URL: ; y # pueden
pertenecer al valor. No quita comillas ni expande porcentajes/variables.

Rechaza sección InternetShortcut o clave URL repetidas, incluso idénticas;
secciones malformadas, líneas sin '=' dentro de la sección relevante y URL
ausente/vacía quedan sin dato, no string vacío disponible. No replica toda la
gramática/precedencia que pueda usar una versión del shell o una API INI.

Exige un prefijo de esquema ASCII con forma letra[letra/dígito/+.-]*: y contenido
no vacío, sin whitespace ASCII no escapado o controles ASCII/DEL. Es un control
de forma, NO validación completa de URL/URI/IRI ni allowlist de esquemas. Conserva
texto Unicode y protocolos arbitrarios (por ejemplo file: o custom-app:), sin
decodificar, normalizar, comprobar destino o darle permiso de apertura.

stored_url es texto NO confiable. El GUI debe presentarlo escapado como metadato,
no convertirlo automáticamente en un enlace seguro/clicable ni pasarlo a un shell.
Puede incluir credenciales, tokens, caracteres confusos o esquemas peligrosos.
La política de navegación/autorización es del consumidor y no está implementada.

## Presupuestos y errores

Máximo1MiB por archivo,4096 líneas,64KiB por línea y32768 bytes de URL UTF-8.
Comparte con .lnk los128 intentos,16MiB de payload aceptado y16MiB de texto
retenido por muestra; no son presupuestos separados. Peor caso128MiB de lectura
de contenido si lecturas se descartan/fallan; no hay deadline de IO. Ver
WINDOWS_SHORTCUT.md para el lector nativo (OPEN_REPARSE_POINT, mismo handle,
tamaño/last-write revalidados sin garantía atómica). No se sigue el destino URL.

Errores de parser/permisos/presupuesto dejan solo windows_shortcut ausente con
nota, manteniendo el archivo inventariado. Inventario sin opt-in no lee su
contenido. La ampliación API0.x requiere recompilar biblioteca y consumidores.

## Verificación

Fixtures de ASCII, UTF-8 BOM, UTF-16LE/supplementary Unicode, case-insensitivity,
comentarios, query/fragment preservados, iconos remotos ignorados y esquemas
arbitrarios almacenados sin apertura. Rechazo de duplicados, URL ausente/vacía,
quotes/controles/espacios no escapados, codepage desconocida, Unicode/UTF-8
malformado, tamaños/líneas/URL excesivos, prefijos truncados y permisos. Pruebas
del pipeline y límite128; conserva las regresiones .lnk y presupuesto compartido.
Windows lee bytes de fixture .url temporal en TMP/work y muestrea Monitor opt-in/
opt-out. Fixture eliminada por archivo exacto y directorio vacío. No se modifican
Startup reales, cargan iconos ni se abre URL. No certifica todos los .url del shell.

Resultado Release: Windows68/68 y Linux52/52; bibliotecas/headers instalados y
consumidores externos reconstruidos/ejecutados en ambos. El consumidor Windows
comprueba stored_url separado de command/ruta local. Linux verifica el parser
portable y regresiones, no lectura nativa de archivos Windows. API0.x ampliada:
recompilar consumidores junto con la biblioteca.

Referencias oficiales: [Internet Shortcuts y propiedad URL](https://learn.microsoft.com/en-us/windows/win32/lwef/internet-shortcuts),
[modelo de secciones/claves INI y diferencias de la API Windows](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilestringw).
