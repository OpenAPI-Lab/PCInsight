# Abrir detalles de un servicio Windows

Implementado2026-10-19. `open_windows_service(internal_name)`, en
`pcinsight/windows_services.hpp`, consulta SCM directamente para un panel propio
del GUI. No inicia procesos, editores, services.msc, shells ni UAC. Funciona con
los permisos actuales. No necesita enumerar todos los servicios primero.

```cpp
const auto opened = pcinsight::open_windows_service(selected.id);
if (opened.result && opened.view) {
    const auto& details = *opened.view;
    if (pcinsight::metric_is_known(details.description)) {
        // Incluso "" es una descripción vacía conocida.
        show_description(*details.description.value);
    }
}
```

El resultado exterior contiene ActionResult y una vista opcional. En una
respuesta de esta función, éxito implica vista presente, y error implica vista
ausente. Entradas inválidas, no encontrado, permiso denegado, otros errores
Win32 y plataforma no soportada se distinguen. Se rechazan vacío, NUL, barras,
UTF-8 inválido, más de1024bytes o más de256unidades UTF-16 antes de abrir SCM.
Fuera de Windows devuelve not_supported sin consultar el sistema.

La vista aporta nombre interno solicitado, display_name, description,
load_order_group, status, native_service_type y process_id. Todos salvo el
identificador usan Metric. SCM se abre CONNECT; se fija primero un handle con
QUERY_STATUS y se pide QUERY_CONFIG por separado. Así, configuración denegada
no oculta un estado legible ni dispara elevación. El handle de estado impide
reabrir un objeto reemplazado con el mismo nombre mientras vive la consulta.

Campos de configuración vacíos/nulos son cadenas vacías conocidas; denegación,
fallo, datos inválidos y límites agotados son ausencia con motivo, no vacío.
permission_required distingue permisos. Otros fallos de campo usan unsupported
con operación/código: ese enum no distingue por sí solo todos los errores;
conservar note. Un fallo global de estado no devuelve una vista parcialmente
exitosa. No hay fallback a caché, registro, archivos ni utilidades.

El grupo es explícitamente **de orden de carga**, no de alojamiento svchost.
La descripción es texto SCM crudo; referencias indirectas `@...` se conservan,
no se cargan DLLs ni se obtiene texto de rutas remotas. El GUI debe tratarlo
como texto, nunca como comando/HTML. No se anuncia resolución amigable universal.
No se exponen rutas de ejecutables/cuentas/secretos como parte de esta API.

process_id es Metric de optional: valor exterior conocido e interior vacío
significa que el estado observado no proporciona un PID publicable (stopped,
start_pending, stop_pending), NO PID0 ni prueba de que no exista ningún proceso.
Running/paused/pause_pending/continue_pending publican solo PID no nulo; estado
desconocido o PID0 inesperado quedan sin dato. Un PID no autentica un proceso
ni es un token checked para terminarlo. Para acciones sobre servicios, usar el
nombre interno con control_service, nunca terminar el anfitrión por este PID.

Se admiten servicios Win32 propios/compartidos, no drivers del kernel. Dos
buffers8KiB, punteros/rangos/alineación/terminadores y UTF-16 comprobados,
sin crecimiento/reintentos no acotados. La salida necesaria de QueryServiceConfig
solo está documentada para insuficiencia de buffer, por lo que no se utiliza
como longitud de datos válidos en éxito. Inicializar a0xFF evita que ceros del
buffer previo inventen terminadores. Handles RAII incluso en excepciones.
No hay snapshot atómico: estado y campos de configuración se consultan en
instantes distintos. Windows controla los timeouts RPC; no es una API realtime.

## Verificación y cierre M07

`tests/windows_service_view.cpp`: fixtures de derechos mínimos, entrada inválida
antes de SCM, no encontrado/denegación/estado truncado/driver rechazado,
configuración parcial, buffer insuficiente, strings nulas/vacías, punteros
fuera de rango/desalineados/sin terminador y UTF-16 inválido, referencias
indirectas, estados1..7/desconocido/PID0 y cierre de handles. Consulta nativa
read-only positiva:8servicios abiertos,8descripciones y8grupos conocidos.

`tests/windows_actions_native.cpp`: lanza solo su propio ejecutable con un
modo hijo privado que valida argumentos literales (espacios, vacío, comillas,
barra final) y confirma por evento único. El hijo no hace E/S de usuario/red
y se apaga a15s si pierde al padre. Se obtiene FILETIME en handle propio,
se rechazan tokens obsoletos para EcoQoS/terminación, se comprueba hijo vivo,
se activa/desactiva EcoQoS checked y se relee el bit solicitado. Después se
termina checked y se confirma señalización en el mismo handle/exit1.
RAII intenta limpiar el hijo privado también ante excepción. No toca procesos
ajenos, no eleva ni cambia la política del padre. EcoQoS sigue siendo solicitud
energética, no ahorro efectivo ni modo combinado/prioridad del Task Manager.

Positivos nativos de ambas pruebas;20repeticiones de cada una(40ejecuciones).
La validación de start/stop/restart SCM de revision36 se conserva en
[WINDOWS_SERVICES.md](WINDOWS_SERVICES.md), con servicio privado autorizado,
registro/copia/directorio eliminados. Esta revisión NO repite la mutación SCM.
M07 cierra inventario/control/abrir detalles/launch/end/EcoQoS soportados y su
validación aislada. El helper UAC productivo sigue L05; M02 recursos/red por
proceso, M06 sesiones, N01 CI/toolchains y N03 auditoría final no se cierran.
Más versiones/hardware y mejoras como descripción localizada, grupo svchost,
cierre cooperativo específico de aplicación o modo combinado no se anuncian
implementadas. No se sustituye una acción pendiente por un éxito simulado.

API adicional sin cambios de layouts anteriores/ABI virtual/SystemSnapshot/
historial; se recompilan/relinkan consumidores instalados en Linux y Windows
para comprobar exportación/enlace. Evidencias en work/service-view-*.log.
Regresión final: Windows121/121 y Linux108/108 Release; consumidores instalados
ambos recompilados/relinkados/ejecutados exit0. Builds finales sin warnings.

Referencias primarias:
[QueryServiceConfigW](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfigw),
[QueryServiceStatusEx](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryservicestatusex).
