# Condiciones de startup por nombre de sesión GNOME

Unidad verificada 2026-10-14: AutostartCondition=GNOME3 if-session <name> y
GNOME3 unless-session <name> usan el nombre reportado por el gestor del bus del
caller. Se devuelven en autostart_condition_satisfied, sin cambiar status ni
xdg_standard_eligible. Comparación de nombre case-sensitive; tipo/predicado
completos sin distinguir mayúsculas ASCII. Predicado inválido, falta de operandos,
nombre vacío/control/NUL o más de1024 bytes -> unknown. No se ejecuta nada.

La fuente nativa es la propiedad [SessionName de org.gnome.SessionManager](https://gnome.pages.gitlab.gnome.org/gnome-session/re04.html),
leída mediante libsystemd/sd-bus ya instalado, sin CLI ni dependencia nueva de
compilación. GetNameOwner resuelve el propietario actual; Properties.Get se envía
a ese nombre único, no a un servicio activable. No se arranca GNOME ni se usa
XDG_CURRENT_DESKTOP/DESKTOP_SESSION como sustituto del nombre real.

Transporte local: si DBUS_SESSION_BUS_ADDRESS está presente, debe ser una sola
dirección unix: de máximo32 KiB; se rechazan autolaunch/unixexec/TCP/listas antes
de abrir. Si falta, sd_bus_open_user usa su socket local por defecto, no un
subprocess de PCInsight. El entorno/loader/socket son contexto confiable del host,
no una barrera para un helper privilegiado. No busca la sesión gráfica de otro
usuario ni garantiza que el bus elegido pertenezca a la sesión deseada.

Se valida el propietario único, variante string, nombre no vacío/acotado/sin
controles. Runtime, bus, propietario o propiedad ausentes, respuesta malformada
y otros fallos -> unknown; acceso denegado -> permission_required. unless-session
no invierte un unknown en true. El valor conocido es available: nombre observado,
no garantía de arranque ni identidad de sesión persistente.

Consulta lazy una vez por contexto de inventario (también errores), compartida
por todas sus entradas y protegida por mutex. Un inventario nuevo crea otro
contexto y vuelve a consultar; no mantiene watcher/caché global. Dos llamadas de
D-Bus con timeout2s cada una; no se promete un deadline global para apertura,
handshake ni consultas de filesystem de otros proveedores. El propietario/nombre
pueden cambiar después de la consulta; el caller debe recargar el inventario.
No hay métodos de logout/registro/arranque ejecutados por este proveedor.

Verificación Release: Linux26/26 y Windows37/37, paquetes instalados y consumidores
externos ejecutados. Parser portable verifica case-sensitive name, diferencias
respecto al desktop, operandos/límites, proveedor ausente y permission_required.
Linux usa servicio D-Bus privado org.pcinsight.SessionFixture.p<PID> en el bus
local existente; nombre propio adquirido sin reemplazo, sin reclamar el nombre
GNOME real. El mismo proveedor nativo (destino interno de test) lee variantes,
observa cambios y rechaza variante errónea, acceso denegado, cadenas excesivas/
control, propietario inexistente y unixexec antes de apertura. Al cerrar su
conexión desaparece solo el servicio de prueba; ningún daemon/CLI fue lanzado.
En Ubuntu24.04 WSL la fixture se ejecutó, no se saltó. En hosts sin bus puede
saltar la porción nativa explícitamente; Windows no emula un gestor GNOME.

No se ha validado con un gestor GNOME gráfico real ni todas sus versiones. La
fixture verifica el contrato nativo de transporte/propiedad, no GNOME global.
GConf y otros tipos/vendor policies/startup impact siguen pendientes.
