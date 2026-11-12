# Properties de startup: consulta de solo lectura

Unidad implementada: get_startup_properties(StartupEntry.id), expuesta en
pcinsight/startup.hpp. La consulta entrega StartupPropertiesResult: value,
ActionError y message. No abre un diálogo del SO ni ejecuta/resuelve Exec.
Permite al GUI construir su propio panel Properties sin framework impuesto.

Ampliación multiplataforma: source_file_size_bytes es el Metric de tamaño del
.desktop (available en Linux); file_size_bytes queda como campo legacy Linux.
Windows Run no tiene archivo de fuente y deja ese Metric sin valor, no cero.
Ver [Properties de Run Windows](WINDOWS_STARTUP_PROPERTIES.md). Recompilar API0.x.

```cpp
#include <pcinsight/startup.hpp>
auto properties = pcinsight::get_startup_properties(entry.id);
if (properties) {
    // properties.value->entry: estado y condiciones calculados desde estos bytes
    // properties.value->file_size_bytes: tamaño del .desktop, no del ejecutable
    // properties.value->fields: group/key/raw_value, en orden original
}
// stale_target -> recargar inventario, no sustituir silenciosamente por otra app.
```

Linux: identificador absoluto .desktop, máximo32 KiB/sin NUL, dentro de los
directorios XDG del caller y efectivo por prioridad. Un ID system oculto por una
copia personal, incluso malformada, devuelve stale_target. Inexistente -> not_found;
fuera del contexto -> invalid_argument. Inventario con errores de acceso que
impidan certificar prioridad -> system_error, sin Properties parciales falsas.

Lectura nativa O_RDONLY/CLOEXEC/NONBLOCK y archivo regular máximo1 MiB; descriptor
liberado por RAII. Compara dev/inode/size/mtime/ctime y tamaño leído, incluida ruta;
revalida prioridad y archivo después de calcular condiciones. Cambio detectado
-> stale_target. Symlink a regular se permite en esta lectura, como en inventario;
no es permiso para editar enlaces. FIFO/directorios no se abren como una app válida.
Permiso denegado al abrir -> permission_denied. Ningún archivo se modifica;
atime puede cambiar por lectura según el filesystem.

Campos group/key/raw_value conservan orden, grupos Desktop Action, nombres
localizados como Name[es], claves vendor y duplicados que el lector básico no
considera ambiguos. No eligen un ganador de claves repetidas arbitrarias. Los
valores mantienen escapes Desktop Entry, sin decodificar ni analizar como shell;
se eliminan espacios/tabs de borde y CRLF del delimitador. No incluye comentarios
ni una copia lossless para editar; usar editor de flags, no reconstruir el archivo
desde esta lista. Límites: group/key4096 bytes, value32 KiB,65536 campos; líneas
sin equals, claves vacías/estructura inválida o exceso -> invalid_argument.

entry contiene Name/Exec/status del mismo contenido leído; nombre de archivo como
fallback si falta Name. Condiciones estándar y AutostartCondition usan el contexto
nativo actual, con los mismos límites/fuentes opcionales documentados previamente.
Fields permite mostrar Comment/Icon/Path/Terminal/localizaciones/actions, pero no
ejecuta Icon/Path ni interpreta rutas relativas de Exec. No traduce automáticamente
localizaciones ni certifica validez completa de Desktop Entry. Publisher permanece
vacío y impact not_measured: estos no se deducen de nombres/vendor arbitrarios.
No lee properties/firma/versionado del binario Exec ni datos de paquetes.

Consulta opt-in: puede leer dos inventarios de hasta100000 entradas/64 MiB cada
uno además del archivo seleccionado; usar para abrir/refrescar Properties, no como
poll obligatorio por frame GUI. No es snapshot/CAS universal contra escritores que
cambien y restituyan archivos/directorios entre comprobaciones. Se confía en entorno
XDG/ancestros/loader; el host coordina cambios. Filesystems o proveedores nativos
pueden bloquear; no hay deadline universal. No es helper privilegiado ni auditoría
de una acción mutante. Otros backends devuelven not_supported; parámetros vacíos/
NUL/excesivos se rechazan antes. El lector no crea archivos ni threads propios.

Verificación 2026-10-14: Linux Release27/27, Windows38/38; instalado nuevo header y
consumidores externos compilados/ejecutados en ambos. Fixture /tmp con XDG privado:
origen readonly intacto, fields/raw escapes/localizaciones/grupos/duplicados,
flags/elegibilidad separados, custom personal prevalece, system ID stale incluso
con personal malformado, symlink lectura, permisos UID1000, FIFO sin bloqueo,
paths inválidos y prioridad incierta. Tests portables de parser/limits y API sin
backend. No startup real modificado ni app ejecutada.
