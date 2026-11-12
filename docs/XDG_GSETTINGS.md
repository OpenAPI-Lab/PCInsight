# Startup condicionado por GSettings

Unidad verificada 2026-10-14: AutostartCondition=GSettings <schema> <key> se
evalúa mediante GIO nativo en StartupEntry.autostart_condition_satisfied.
No ejecuta gsettings/dconf/shell ni instala paquetes. Bibliotecas ya instaladas
se cargan en runtime; sin libgio-2.0.so.0 o símbolos requeridos -> unknown.
No hay dependencia de headers GLib/GIO ni enlace obligatorio a esas bibliotecas.
Esta fuente opcional es necesaria para conocer semántica del backend GSettings;
no se inventa el valor ni se sustituye por una búsqueda de XML/default como si
fuera configuración almacenada del usuario.

Parser: tipo completo sin distinguir mayúsculas ASCII, exactamente schema y key;
identificadores no vacíos, máximo1024 bytes, letras ASCII/dígitos/punto/guion/
underscore. Rechaza args extra, slash, NUL, controles y espacios internos.
Se mantienen validación de Desktop Entry/escapes/grupo principal/duplicados
del evaluador anterior. No ejecuta el contenido del campo.

Antes de construir GSettings se busca el schema en la fuente por defecto
(lookup recursivo), se exige path fijo, se comprueba has_key y el tipo booleano.
Es necesario porque [new_full](https://docs.gtk.org/gio/ctor.Settings.new_full.html)
no permite un schema relocatable sin path. Schema/key inexistente, relocatable,
tipo no booleano o datos ausentes -> unknown, sin invocar constructores con
precondiciones inválidas. Tipo se compara con g_variant_type_equal, no con strcmp
de peek_string: [GLib no garantiza terminación NUL](https://docs.gtk.org/glib/method.VariantType.peek_string.html).
Se comprueba también el tipo del valor devuelto antes de leer su booleano.

Resultado conocido tiene Support::estimated y nota explícita: valor efectivo
del caller, que puede ser default/fallback de schema/backend. No certifica origen
persistido, backend disponible o permisos de lectura del almacenamiento. Ausencia
de schema/runtime no equivale a false. El decorador no escribe ningún setting.
Schema/key/value/GSettings se liberan por RAII; default schema source es prestado.
La biblioteca dinámica se retiene durante la vida del proceso: GIO puede registrar
tipos/modules/callbacks y estado global que no es seguro descargar entre muestras.

Cada muestra construye/consulta nuevamente GSettings. Actualizaciones externas
dependen del backend/cache y del procesamiento de eventos GLib en el host: no se
promete refresco externo instantáneo sin su main context. No se instala un loop
oculto ni se modifica el entorno del consumidor. GIO puede inicializar módulos,
threads, conexiones/cache propios de su backend; no se promete cero efectos de
inicialización de bibliotecas de terceros ni timeout universal. Entorno, schemas,
loader y módulos son confiables del host, no inputs seguros para un helper root.
GSETTINGS_BACKEND/GSETTINGS_SCHEMA_DIR pueden estar cacheados desde la primera
inicialización por el host; PCInsight no los cambia ni selecciona otra sesión.

Status enabled, elegibilidad XDG estándar y predicado GSettings siguen separados.
Una condición false no cambia flags ni detiene la app. No es garantía de arranque
ni cubre GNOME/GConf u otras políticas vendor. GNOME3/session tiene un proveedor
posterior separado, ver XDG_SESSION_CONDITION.md. Otros backends no
intentan emular GIO nativo; tipos no cubiertos permanecen unknown.

Verificación: Linux Release25/25 y Windows36/36; instalación y consumidores
externos ejecutados en ambos. Pruebas portables simulan fallos de runtime/source/
schema/path/key/type/settings/value, comprobando que constructores/boolean getters
no se llaman antes de validar y que se liberan objetos. Parser verifica tipos,
aridad, límites y propagación de permission_required del proveedor inyectado.
Linux usa GIO/schema instalado real y GSETTINGS_BACKEND=memory en proceso privado
antes de su primera carga: Monitor observa false/true mientras flags siguen enabled;
schema/key ausentes y key string se rechazan sin abortar. No preferencias reales,
HOME, autostart real ni servicios modificados. El test puede saltar la porción
nativa si falta runtime/schema; en Ubuntu24.04 WSL se ejecutó, no se saltó.
No se validó refresco cross-process de un backend dconf real ni todos los backends.
