# Editor declarado de startup Linux

I05: proveedor opcional AppStream, integrado en Monitor. No ejecuta programas,
gestores de paquetes, appstreamcli, shells ni utilidades externas; no instala
software ni descarga metadatos. La biblioteca sigue funcionando sin libxml2.

## Uso y significado

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/linux_startup_publisher.hpp>

pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
options.include_startup_entries = true;
options.include_linux_startup_publisher = true; // opt-in independiente
auto snapshot = monitor.sample(options);
for (const auto& entry : snapshot.startup_entries) {
    // Comprobar entry.publisher_label.value y mostrar support/note.
    // publisher es solo shorthand; cadena vacia NO significa editor inexistente.
}

// Alternativa explicita: consultar y aplicar al inventario que ya tiene el GUI.
auto declared = pcinsight::read_linux_startup_publishers();
pcinsight::apply_linux_startup_publishers(snapshot.startup_entries, declared);
```

Solo se une el basename de la entrada XDG efectiva con un launchable AppStream
de tipo desktop-id exactamente igual, case-sensitive. No se busca coincidencia
por Name, Exec, PID, username, nombre del paquete ni nombre del archivo XML.
No se ejecuta ni resuelve el comando para demostrar quien lo publico.
Una entrada desconocida/malformada no recibe una etiqueta por esa coincidencia.
Una entrada disabled valida si puede tener editor declarado.

El resultado es estimated: developer_label es el desarrollador/equipo declarado,
no necesariamente una empresa editora, firmante ni titular del ejecutable actual.
Un .desktop personalizado puede conservar el basename y cambiar Exec. La etiqueta
NO autentica esa entrada. publisher_source_executable queda ausente en Linux.
publisher_label.note y LinuxStartupPublisher conservan componente/fuente.
No usar estos datos para autorizar ejecucion, elevacion ni confiar en un binario.

Se lee developer/name moderno y developer_name legacy; se selecciona solo la
forma predeterminada sin xml:lang, incluida herencia del idioma. Traducciones no
se eligen implicitamente segun el locale. Etiquetas duplicadas, vacias, con
controles o modernas/legacy incompatibles se rechazan. Identidades y etiquetas
admitidas hasta4096B. El lector no es un validador completo del esquema AppStream.

## Fuentes, condiciones y ausencias

Se resuelven XDG_DATA_HOME (fallback HOME/.local/share) y XDG_DATA_DIRS (fallback
/usr/local/share:/usr/share) del proceso llamador. HOME ausente/invalido consulta
geteuid/passwd; no se obtiene el entorno de otras sesiones. Los componentes
relativos/vacios se ignoran; directorios normalizados se deduplican.
En cada base se consulta metainfo y el directorio legacy appdata; extensiones
.metainfo.xml/.appdata.xml. Esto es una politica de fuentes explicita de PCInsight,
no una promesa de reproducir todos los caches/precedencias de AppStream.

Un solo component upstream por XML, sin namespace, type=desktop-application.
No se leen catalogos components, YAML DEP-11, archivos comprimidos, caches,
datos remotos ni bases internas de gestores de paquetes. XML con namespace,
DTD/declaraciones de entidades o instrucciones de procesamiento queda ausente;
no se infiere un editor alternativo. Otros tipos de component no aportan filas.
Flatpak/Snap solo se ven si publican metadatos regulares en los directorios
consultados y con una coincidencia explicita; no se recorren sus instalaciones
privadas ni se siguen exportaciones symlink como fallback.

Las condiciones startup ya soportadas siguen separadas del editor: flags,
OnlyShowIn/NotShowIn/TryExec, AutostartCondition por archivos/GSettings/GNOME3
session. Ver XDG_ELIGIBILITY.md, XDG_FILE_CONDITION.md, XDG_GSETTINGS.md y
XDG_SESSION_CONDITION.md. No se inventa soporte para condiciones vendor nuevas.
Otros origenes de arranque (cron @reboot, perfiles shell, scripts de sesion KDE,
jobs de otros init, unidades de usuario o extensiones propias) no se mezclan
como apps XDG equivalentes. Servicios pertenecen a su inventario K03/K04;
atribucion automatica/captura temprana de impacto sigue en I07. Esta unidad
amplia el proveedor de metadatos y documenta ausencias; no cierra esas tareas
ni afirma un inventario universal de todos los mecanismos de inicio Linux.

Si mas de un documento declara el mismo desktop-id, el overlay no selecciona
un ganador ni toma el primer editor: ese match queda desconocido. Un scan con
errores de parseo/lectura/limite no publica un conjunto parcial como unico; se
rechaza el informe entero. Asi, un archivo defectuoso no relacionado tambien
puede dejar sin etiquetas todo el inventario en esa muestra: politica conservadora.
Directorio inexistente es fuente sin documentos; EACCES/EPERM preserva
permission_required. Una lista vacia solo describe estas fuentes, no demuestra
que las aplicaciones carezcan de editor. No se conserva una etiqueta vieja si
el nuevo informe es ausente, ambiguo o invalido.

## Biblioteca XML y limites de lectura

Se carga libxml2.so.2 (fallback libxml2.so) del sistema mediante dlopen solo al
consultar este proveedor. No requiere headers/paquete de desarrollo para compilar.
Sin biblioteca/simbolos necesarios devuelve unsupported; no instala un reemplazo.
El host administra su biblioteca y rutas de carga de confianza. No se cambian
callbacks, opciones globales ni se llama xmlCleanupParser en una biblioteca ajena.

Entrada UTF-8 acotada, sin NUL; DTD/ENTITY se rechazan antes de entrar al parser.
Lector en memoria sin URL base, NONET/NOERROR/NOWARNING y sin NOENT, DTDLOAD,
DTDATTR, DTDVALID, XINCLUDE, HUGE ni RECOVER. No se siguen URLs de releases,
screenshots, iconos o includes. El limite/guardas son propios; la correccion del
parser y su mantenimiento dependen de la biblioteca del sistema.

Hasta128directorios,8192entradas visitadas,1MiB por archivo,16MiB de bytes de
entrada y16MiB de cadenas de salida,8192filas,100000nodos/archivo y profundidad64.
No son un limite exacto de RSS ni deadline de syscalls. No hay cache persistente;
cada consulta vuelve a muestrear. El loader se resuelve una vez por proceso.

Directorios fijados mediante fd, archivos openat/O_NOFOLLOW/O_NONBLOCK; solo
regulares. No se sigue el symlink final y un FIFO no bloquea. Metadatos de fd
y pathname relativos se revalidan (dev/inode/mode/nlink/size/mtime/ctime).
La enumeracion no es atomica: altas/bajas, sustitucion de directorios/ancestros
y cambios ABA pueden escapar; estimated no es una garantia de identidad.
No es sandbox para helpers elevados con rutas hostiles. Consultar en contexto
de usuario y coordinar cambios de environ del propio proceso.

## Compatibilidad y verificacion

API adicional linux_startup_publisher.hpp; nuevo flag al final de SampleOptions
en version0.2 en desarrollo: recompilar TODOS los consumidores por cambio de
layout de opciones. No cambia el layout de StartupEntry (se reutiliza el Metric
de etiqueta con su nota), ABI virtual ni formato de historial. Windows conserva
su proveedor VERSIONINFO/Authenticode y no consulta AppStream implicitamente.

Pruebas: tests/linux_startup_publisher.cpp (assertions activas en Release),
fixtures privados para XML moderno/legacy, entidades predefinidas/numericas,
CDATA, idioma heredado, duplicados/conflictos, XML invalido, DTD/XXE rechazado,
limites, permisos, symlink/FIFO; scan publico y Monitor opt-in/opt-out con XDG
privado del proceso. No se inicia ninguna app ni se modifica autostart real.
Prueba separada de biblioteca XML con simbolos incompletos: unsupported sin
scan/etiqueta. Windows verifica join/contexto portables y ausencia del proveedor,
no captura XML Linux. Si falta libxml2 el positivo XML se omite explicitamente
con exit77, nunca se declara positivo nativo por un skip.

Verificado2026-10-19: positivo nativo Ubuntu WSL con libxml2 ya instalada,
consulta publica/Monitor y archivos regulares privados reales; exit0, no skip.
Linux104/104 y Windows117/117 Release aprobadas; builds finales sin warnings.
Paquetes reinstalados y consumidores externos de ambos sistemas recompilados/
relinkados/ejecutados exit0 con el header/API nuevos. Logs work/startup-publisher-*
(native-linux, build/suite/install/consumer por plataforma). Se corrigio tambien
una advertencia preexistente de NOMINMAX duplicado en tests/windows_app_history.cpp
con guard de macro, sin modificar la logica/aserciones de esa prueba.

Cierre I05: ampliacion de proveedores (AppStream moderno/legacy) y politica
explicita de fuentes/condiciones ausentes, con positivo publico y rechazos.
No cambia los criterios de I07/K03 ni declara soporte universal de vendors.
Riesgos historicos N03 siguen abiertos; estas suites verdes no prueban su causa.

Fuentes primarias: [AppStream upstream, developer y launchable](https://www.freedesktop.org/software/appstream/docs/chap-Metadata.html),
[libxml2 XMLReader](https://gnome.pages.gitlab.gnome.org/libxml2/html/xmlreader_8h.html),
[opciones del parser](https://gnome.pages.gitlab.gnome.org/libxml2/html/parser_8h.html).
