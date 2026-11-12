# Metadatos y agrupación de servicios systemd (K01)

Actualización K04: [LINUX_USER_SERVICES.md](LINUX_USER_SERVICES.md) implementa
la API separada de usuario/abrir detalles/control; las exclusiones K04 de la
evidencia histórica siguiente no describen el estado actual de esa unidad.

Lectura del manager systemd del sistema: nombre, descripción, estado, MainPID,
agrupación Slice y estado de instalación, con disponibilidad explícita. No se
ejecutan systemctl/ps ni se instalan dependencias. No se inicia/detiene/reinicia
ningún servicio para enumerar ni para validar esta unidad. Control K02, otros
init K03 y managers de usuario/open-service K04 permanecen separados.

## Contrato público

ServiceInfo.linux_details es Metric<LinuxServiceDetails>. provider identifica
el origen; group_kind define la semántica de agrupación, no el valor de grupo.
Comprobar cada Metric: la estructura presente no significa todos los campos conocidos.

| Campo | Fuente y significado |
|---|---|
| name/description | Nombre primario y descripción reportados por ListUnits; descripción vacía puede ser conocida |
| load_state/active_state/sub_state | Cadenas nativas de ListUnits, no estado de instalación |
| status | Presentación estimated de ActiveState/SubState; active+running -> running, resto active/sub |
| main_pid | Propiedad Service.MainPID;0 conocido significa ningún PID principal reportado |
| slice | Service.Slice: agrupación nativa de control de recursos |
| unit_file_state | ListUnitFiles: política de instalación, no running/stopped |

ServiceInfo.group refleja slice o vacío; ya no vale systemd para identificar
al proveedor. group_kind=systemd-slice y provider=systemd-system-manager están
separados. Una Slice NO es un grupo Windows de orden de carga, svchost ni el
grupo Unix de credenciales Service.Group. El GUI debe etiquetarla “Slice de
systemd”, no asumir equivalencia entre plataformas. Sin propiedad válida no se
inventa system.slice por defecto.
Una propiedad Slice leída como cadena vacía conserva un valor conocido vacío
(ningún nombre de Slice reportado), distinto de una consulta inaccesible.

El PID legacy opcional solo se publica cuando MainPID está disponible y es
positivo dentro del rango signed32 de Linux. main_pid=0 permanece conocido en
el campo tipado, sin presentarlo como un proceso válido; consulta inaccesible o
valor fuera de rango permanece desconocido, no0. MainPID no equivale al primer
proceso del cgroup ni a ExecMainPID histórico. [API del manager y MainPID](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.systemd1.xml),
[propiedad nativa Slice](https://github.com/systemd/systemd/blob/main/src/core/dbus-unit.c),
[publicación en la interfaz de tipo Service](https://github.com/systemd/systemd/blob/main/src/core/dbus.c).

La combinación active/exited puede ser un oneshot activo sin proceso principal.
Enabled/disabled/masked/alias no demuestra ejecución. Los estados futuros se
conservan como tokens ASCII válidos hasta128B, no se traducen arbitrariamente.
Estado inválido/ausente deja su Metric sin valor; status legacy da unknown si
no están disponibles tanto ActiveState como SubState.

## Identidad y cobertura

Se obtiene el unique owner de systemd por D-Bus, y ambas enumeraciones y todas
las propiedades se consultan contra esa instancia. Se verifica de nuevo el owner
al terminar. Error de enumeración/parseo/límite o cambio de owner impide publicar
la lista como éxito; no hay fallback silencioso después de un fallo así.

Unit.Id debe coincidir con el nombre primario antes y después de MainPID/Slice.
Si no se puede verificar, PID y Slice quedan desconocidos, manteniendo la nota
de permiso cuando corresponde. Si falla el primer Id no se solicitan esas
propiedades. Los metadatos previos de ListUnits se conservan como lo reportado
en esa enumeración, no como estado actual certificado tras el fallo.

Id no identifica una invocación del servicio ni la generación de un PID. Un
reinicio/reemplazo que conserve el nombre entre consultas puede no detectarse;
no hay snapshot atómico ni garantía de que el PID siga vivo. Estos datos no son
un token de terminación de proceso ni autorización para actuar. K02 mantiene su
contrato separado de control por nombre y confirmación del job.

Los archivos instalados que no figuraron en ListUnits conservan estado de
ejecución desconocido. load_state=not_loaded es una etiqueta estimated de esa
vista, NO un valor nativo de LoadState ni prueba de stopped. Alias/templates no
se resuelven ni se cargan deliberadamente para obtener un PID/Slice. No se llama
LoadUnit/StartUnit. Consultas a objetos enumerados pueden interactuar con cachés
internas del manager; no se promete ausencia de todo efecto interno del daemon.

SystemSnapshot.linux_service_inventory_source distingue:

- systemd-system-manager/available: completó ListUnits+ListUnitFiles sobre el
  mismo owner; puede ser un inventario vacío válido, no todos los init/managers.
- partial-file-scan/estimated: fallback anterior de nombres de archivos, con
  cobertura/errores posiblemente parciales, no estado de ejecución.
- Sin valor: captura desactivada o backend no Linux; consultar la nota.

En fallback, provider de cada fila es systemd-file-scan o sysv-file-scan; group
y descripción efectiva quedan vacíos y su disponibilidad tipada es desconocida.
Se retiró el parser ingenuo de líneas Description=, que no resolvía sección,
precedencia, aliases ni overrides efectivos. Encontrar un archivo no demuestra
que systemd/OpenRC/SysV lo gestione ni lo esté ejecutando. Esto no implementa K03.

## Validación, errores y compatibilidad

Nombres .service con el validador canónico compartido del proyecto (máximo255B,
ASCII/escapes hex). Rutas de objeto bajo /org/freedesktop/systemd1/unit/ con un
único componente válido, sin self. Se rechazan duplicados cargados y rutas
repetidas, estados de archivos contradictorios incluso si uno era vacío.
Slice se valida como nombre .slice acotado; no acepta rutas, instancias @,
NUL ni escapes incompletos. Cadenas generales hasta64KiB sin NUL,10000filas por
enumeración y payload conjunto16MiB. El lector nativo limita también campos
de ListUnits que no se publican. No es límite exacto de memoria del proceso ni
del buffer interno sd-bus; el callback inyectado ya puede haber creado su vector.

Errores observados EACCES/EPERM/AccessDenied/InteractiveAuthorizationRequired se
conservan como permission_required por campo; no se adivinan desde una ausencia.
Autorización interactiva desactivada, sin elevación. RAII libera mensajes, errores,
bus y biblioteca. ABI dinámica verificada contra [sd-bus.h](https://github.com/systemd/systemd/blob/main/src/systemd/sd-bus.h).
Timeout5s por llamada, no global: cuatro consultas por unidad cargada pueden
extender el muestreo. No garantiza latencia universal en managers grandes/lentos.

API0.x cambia layout ServiceInfo/SystemSnapshot: recompilar biblioteca y TODOS
los consumidores. Los strings legacy group/description del fallback cambian por
corrección semántica; consumers que usaban group==systemd deben migrar a provider.
Windows conserva sus campos y semántica SCM, con linux_details ausente. No se
cambia la interfaz virtual de acciones ni el formato de historial.

## Pruebas

tests/linux_systemd.cpp cubre running/oneshot/inactive/failed/activating,
instalados no cargados, metadatos por campo, descripción vacía conocida, cero PID
vs denegado/ausente/inválido, Slice válida/ausente/denegada/malformada, identidad
anterior/posterior inválida o cambiada, duplicados, payload excesivo y fallback
sin grupo/descripción inventados. La integración real es de solo lectura, con
Monitor include_services false/true/false; no crea ni controla servicios.

2026-10-17: Release final Linux85/85 y Windows105/105 aprobadas, sin fallos
en estas ejecuciones. Paquetes reinstalados y ambos consumidores instalados
recompilados/relinkados y ejecutados exit0, incluido el recorrido de nuevos campos
desde headers instalados. Recompilación incremental final sin avisos.
Ubuntu WSL real:199servicios,11running,16active/exited,82no observados cargados,
117reportes MainPID (incluido0),117Slice conocidos (15vacíos,102con nombre).
Son datos de esa enumeración dinámica, no valores permanentes ni garantía de
que el proceso siga vivo. No se crearon ni controlaron servicios del host.
Windows ejecutó fixtures del proveedor portable y sus regresiones SCM, no D-Bus
nativo Linux. Logs del workspace: work/service-metadata-verified-suite-linux.log
y work/service-metadata-verified-suite-windows.log. Los riesgos intermitentes
CPU/historial previos N03 no se declaran resueltos porque esta suite pasó.

Cierre K01 de este proveedor/contrato con límites explícitos. No cierra otros
init K03, managers de usuario/open-service K04, autorización L05, CI N01 ni
cobertura de distribuciones N02. No se agregan subtareas al conteo por estas pruebas.
