# Servicios del usuario y abrir detalles (K04)

API adicional en `pcinsight/linux_services.hpp`. No cambia el layout existente
de ServiceInfo/SystemSnapshot ni la ABI virtual de SystemActions. Recompilar y
relinkar la biblioteca estática para usar los nuevos símbolos.

```cpp
#include <pcinsight/linux_services.hpp>
using namespace pcinsight;
auto inventory = query_linux_service_inventory(LinuxServiceScope::current_user);
LinuxServiceTarget target{LinuxServiceScope::current_user, "example.service"};
auto view = open_linux_service(target); // datos para el GUI propio, sin lanzarlo
// Solo por una decisión explícita de la aplicación/usuario:
// auto result = control_linux_service(target, ServiceAction::restart);
```

## Destino e identidad

system y current_user son ámbitos separados incluso con el mismo nombre de
unidad o de owner D-Bus (estos últimos son locales a cada bus). Inventario y
vista conservan scope, user_uid y manager_owner. No deducir ámbito a partir
de PID/nombre/descripción. user_uid es el UID **efectivo** actual, no el usuario
de escritorio supuesto, el propietario de un proceso ni un UID arbitrario.
En system queda ausente. Monitor include_services y SystemActions control_service
conservan ámbito de sistema; esta API separada no mezcla filas en sus snapshots.

current_user construye únicamente `unix:path=/run/user/<euid>/bus` con
sd_bus_new/set_address/set_bus_client/start. No usa DBUS_SESSION_BUS_ADDRESS,
XDG_RUNTIME_DIR, sd_bus_open_user, transporte remoto, shell ni autolaunch.
Verifica SO_PEERCRED del socket contra euid y GetConnectionUnixUser del owner
único de org.freedesktop.systemd1. Fija ese owner para propiedades/llamadas/
señales y comprueba de nuevo su titularidad al terminar. Confía en el broker
local y su registro: no certifica criptográficamente el ejecutable systemd ni
protege de un broker/biblioteca maliciosos bajo el mismo usuario. Un bus no
estándar o broker con otro UID se declara indisponible/denegado; no se adivina
otro destino ni se cae al manager de sistema.

Es una observación, no un token duradero de autorización. Cada nueva llamada
current_user selecciona el euid vigente en esa llamada. El llamador no debe
cambiar credenciales concurrentemente ni usar una vista como identidad de un
proceso; MainPID no es un pidfd ni un token checked para otras acciones.

## Inventario y abrir servicio

Reutiliza las dos enumeraciones completas y verificaciones Unit.Id de K01.
provider=systemd-user-manager distingue las filas; Slice sigue siendo agrupación
nativa, no grupo Windows/Unix. Conserva MainPID0 conocido y disponibilidad por
campo. No llama LoadUnit ni inicia servicios para resolver una vista.

open_linux_service valida nombre .service antes de abrir el bus y devuelve una
copia de detalles para el GUI propio. No abre editor, archivo, URL ni aplicación
externa. Solo resuelve nombres observados; un nombre no encontrado lleva nota,
no prueba que nunca exista. Aliases/templates instalados no cargados permanecen
sin resolver y con ejecución desconocida, no stopped.

Metric separa inventario completado vacío de bus/manager ausente, permiso
denegado o fallo de parsing/identidad. No hay scan de archivos sustituto. Los
errores de esta API libre vuelven como ausencia con nota; el recorrido interno
histórico de Monitor conserva propagación de errores posteriores a la apertura.
No es snapshot atómico. Límites heredados:10000 entradas por enumeración,
cadenas64KiB, payload16MiB, timeout5s por llamada, sin timeout global prometido.

## Control explícito

control_linux_service(target,start/stop/restart) valida ámbito/nombre/enum antes
del bus. system delega al controlador K02; current_user abre/verifica su destino
y usa el mismo corredor de jobs con destino/filtro fijados al owner único.
No hay elevación, autorización interactiva, sudo, UID remoto, fallback de ámbito
ni arranque de un manager/broker ausente. Start de una unidad concreta sí puede
cargarla, iniciar dependencias y sustituir jobs (`replace`): es acción mutante
explícita, distinta de consultar/abrir detalles.

Registra JobRemoved antes de Subscribe y de solicitar el cambio. Espera el job
path devuelto, incluidas señales tempranas. Solo result=done es éxito, no prueba
de readiness ni running permanente. Denegación, NoSuchUnit y job fallido
conservan categorías. Cambio de owner, job inválido, desconexión, límites o
timeout tras intentar la acción pueden dejar **resultado desconocido**. No se
cancela ni reintenta; restart puede haber detenido la aplicación aunque falle.
Espera30s, llamadas5s, máximo100000 mensajes y1024 señales tempranas/campos1024
bytes. Apertura/autenticación no tiene límite global estricto.

## Evidencia (2026-10-17)

- Release Linux91/91 y Windows108/108; consumidores externos instalados de ambos
  sistemas recompilados/relinkados/ejecutados exit0 con el header/símbolos nuevos.
- Portable: ámbitos incompatibles, identidad incompleta/duplicada, ausencia vs
  denegación, nombre/enum inválidos y selección de detalles.
- Loader Linux aislado: libsystemd simulada y socketpair local para credenciales;
  verifica dirección fija, UID, cliente bus, timeout y autorización interactiva
  desactivada; variables de entorno deliberadamente ajenas ignoradas. Inventario
  vacío/con datos, manager ausente, UID incorrecto, denegación, parsing fallido y
  cambio de owner. Control start/stop/restart, señal temprana/tardía, job failed,
  NoSuchUnit, denegación, job inválido y desconexión. Comprueba destino y sender
  únicos; nunca conecta al D-Bus del host.
- Nativo WSL solo lectura: sistema199 unidades y usuario19, detalles abiertos en
  ambos ámbitos y UID del usuario verificado. Owner :1.2 en cada bus no significa
  identidad compartida. No se controló ningún servicio real. Positivos nuevos
  de control son simulados; evidencia nativa histórica de sistema sigue en K02.
  No se ejecutó --native-fixture en esta unidad.
- Logs: work/user-services-build-{linux,windows}.log,
  work/user-services-suite-{linux,windows}.log y
  work/user-services-native-linux.log. Builds sin avisos de compilación.

Cierra este proveedor systemd del usuario y abrir detalles, no otros init K03,
todos los usuarios del host, gestor externo de servicios, helper L05, CI N01 ni
validación universal de distros N02. Riesgos históricos CPU/historial N03 no se
declaran corregidos por pasar esta suite. Sin nuevos IDs/créditos fraccionarios.

Referencias primarias: [dirección sd-bus](https://www.freedesktop.org/software/systemd/man/253/sd_bus_set_address.html),
[cliente de bus](https://www.freedesktop.org/software/systemd/man/253/sd_bus_set_server.html),
[D-Bus e identidad Unix](https://dbus.freedesktop.org/doc/dbus-specification.html),
[declaraciones sd-bus](https://github.com/systemd/systemd/blob/main/src/systemd/sd-bus.h).
