# Runit en ServiceInfo: estado reportado, no identidad de proceso

Revisión141/K03. Consulta explícita de un árbol runit de confianza del llamador;
no cambia Monitor ni las consultas systemd/SysV. No autodetección de init, SVDIR,
PATH, shell, utilidades externas, scripts o solicitudes de control.

```cpp
#include <pcinsight/linux_runit_services.hpp>
auto result = pcinsight::query_linux_runit_service_inventory("/var/service");
if (pcinsight::metric_is_known(result)) {
    for (const auto& service : result.value->services) {
        // status: reported-run, reported-down, reported-finish, /paused, o unknown.
        // No llamar "running comprobado" a reported-run.
        const auto& report = service.linux_details.value->status;
        // Report es estimated cuando hay una etiqueta; comprobar disponibilidad.
        (void)report;
    }
}
```

El path es un ejemplo: el host selecciona su árbol real, sin suponer una ruta
universal. `source` conserva LinuxRunitInventory, sus filas originales y diagnósticos;
`services[i]` corresponde solo a `source.services[i]` de la misma llamada. El
proyector privado es puro y no repite lecturas ni une snapshots por nombre.

## Qué se proyecta

`linux_details.provider` = `runit-reported-inventory`. Su Support es estimated:
hay reportes de protocolo, no enumeración autenticada de procesos del kernel.
Nombre: filename observado, available con censo coherente o estimated si parcial.
IDs: `runit-report:` más hex de los bytes del nombre. Son opacos, sin colisiones
dentro de ese inventario y limitados al ámbito de su root, no identificadores
globales ni rutas/comandos/tokens. Nombres pueden contener bytes no UTF-8 o
controles: el consumidor debe escaparlos al mostrarlos.

Con inventario coherente y record available válido:

- state down -> `reported-down`;
- state run -> `reported-run`;
- state finish -> `reported-finish`;
- paused añade `/paused` a la etiqueta del estado reportado.

Metric status es estimated y su nota identifica explícitamente el origen. No
se usan `running`, `stopped`, `enabled` o `ready` como conclusiones de vida,
política o readiness. want_up, term_sent y TAI64N crudos quedan en source, sin
convertirse a enablement, uptime ni hora Unix. TAI nanosegundos/PID/enum y
coherencia down/run/PID inválidos hacen status UNKNOWN. Fuente sin record
available, permiso denegado o inventario parcial/inestable -> status unknown;
el Metric conserva el motivo/permiso. No se reutiliza el estado anterior.

**PID común ausente y main_pid UNKNOWN**, incluso si reported_pid es positivo:
el formato informa un número en el namespace del supervisor, no una identidad
del proceso en el host. El número original, incluido0 válido en down/finish,
se conserva en source. No se busca /proc por nombre ni se usa el número para
señales o terminar procesos. No se interpreta finish como daemon vivo.

Descripción/group vacíos; description/slice/load_state/active_state/sub_state/
unit_file_state UNKNOWN, group_kind vacío. Sin inventar descripción por nombre,
metadatos de scripts, grupo por root ni propiedades de systemd/Windows.
Support exterior describe coherencia del censo, no disponibilidad de todos los
reportes. Una fila unmanaged puede ser UNKNOWN dentro de un censo completo.

## I/O, alcance y acciones

Se reutiliza exactamente query_linux_runit_inventory: límite1..4096 filas,
visitas16384/payload8MiB, endpoints fijados, comprobaciones de contexto/metadata
y status20 bytes doble. El lector requiere un lector en supervise/ok; un fichero
status solo no basta. Abrir el lado escritor de ok para comprobarlo NO autentica
al peer ni comprueba el proceso reportado. No se escribe ningún byte de control.
Sin nuevo worker/caché/persistencia/lectura de run/check/finish o deadline duro.
No snapshot atómico ni garantía de identidad de un pathname después de la consulta.

El proyector rechaza raíces/nombres/filas duplicadas/directorios incoherentes y
payload excesivo, sin asociar por basename con otro árbol. Preserva evidencia;
no mejora retroactivamente las garantías del inventario original.

La vista no contiene handles ni capability de control. Para una acción runit
explícita se usa LinuxRunitService con su propio pin/consentimiento y contrato
de entrega; NO se pasa este ID a las acciones systemd ni reported_pid a acciones
de proceso. Refrescar y fijar la selección nueva; el pathname viejo no autoriza
a actuar sobre cualquier reemplazo. Entrega y condición observada siguen sin
equivaler a completed/readiness/causa. No se cambian esos contratos128/129.

## Evidencia y siguiente unidad

tests/linux_runit_services.cpp: proyección pura portátil, estados/paused, PID
siempre ausente en modelo común, flags/record conservados, rangos/enum inválidos,
permisos/Support/partial, directorio fuera de scope/duplicados/IDs rawbytes/vacío.
Linux privado: alias de directorio, status20 con lector ok, fila unmanaged,
bound y fichero obsoleto sin lector ok, FIFO control sin bytes, sentinel de
script nunca ejecutado y descriptores sin fuga. No runsv real ni servicios reales.
MSVC solo proyección/argumentos/fallback; no prueba backend Linux. SDK separado
incluye y enlaza el nuevo header/símbolo instalado.

Validación141: Linux167/167 +30 repeticiones runit, MSVC10 puro/fallback y dos
consumidores SDK instalado exit0. Builds Linux inicial/final limpios. Logs
work/runit-services-*. Pruebas locales, no CI remota ni aceptación runsv real.

K03 sigue E por otros proveedores, integración y aceptación con gestores reales.
Siguiente unidad funcional: proyección explícita s6 al modelo común, sin convertir
un archivo status persistente en prueba de supervisor/proceso vivo. OpenRC132 y
controlscript SysV sin excepción autorizada; I07 observador intacto.
