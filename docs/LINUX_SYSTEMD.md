# Inventario real de servicios systemd

Actualización K04: [LINUX_USER_SERVICES.md](LINUX_USER_SERVICES.md) añade API
separada para manager del usuario efectivo, abrir detalles y control explícito.
include_services conserva ámbito de sistema. Las exclusiones K04 que siguen
en esta página describen la unidad histórica anterior.

Actualización K01: [LINUX_SERVICE_METADATA.md](LINUX_SERVICE_METADATA.md) añade
disponibilidad por campo, propietario D-Bus único, verificación Unit.Id y Slice
como agrupación nativa. Sustituye group=systemd y añade fuente de inventario.
La evidencia de2026-10-14 abajo es histórica, previa a esa actualización.

Unidad funcional terminada y verificada el 2026-10-14: inventario systemd de
solo lectura con nombres, descripción, estados y MainPID. Se activa mediante
`SampleOptions::include_services=true`. No cambia ni inicia/detiene servicios.

```cpp
pcinsight::SampleOptions options;
options.include_services = true;
auto snapshot = monitor.sample(options);
for (const auto& service : snapshot.services) {
    // service.status: running, active/exited, inactive/dead, failed/failed, ...
    // service.pid: MainPID no nulo, si se pudo consultar.
    // service.active_state/sub_state/load_state/unit_file_state: metadatos crudos.
}
```

## Fuentes y significado

El proveedor carga `libsystemd.so.0` y usa sd-bus por D-Bus. No requiere SDK para
compilar ni ejecuta systemctl, ps u otra aplicación. Cuando systemd no es el
manager activo (`/run/systemd/system` ausente), no está libsystemd o no se puede
abrir el bus, se mantiene el inventario parcial de archivos del proveedor
anterior. En ese fallback no se infiere running ni un PID principal desde cgroups.

- `ListUnits`: unidades actualmente cargadas; se conservan las `.service`.
  Incluye unidades transitorias y generadas aunque no tengan archivo instalado.
- `ListUnitFiles`: archivos instalados, también templates, aliases y masked.
  Se combinan por nombre de unidad. Un archivo no presente en ListUnits conserva
  `status=unknown`, `load_state=not_loaded`, descripción vacía y PID ausente;
  no se presenta como stopped por estar disabled. Un alias no cargado bajo ese
  nombre no demuestra el estado del servicio al que apunta.
- `status=running` únicamente cuando ActiveState=active y SubState=running.
  Otras combinaciones se muestran sin perder información, por ejemplo
  `active/exited`: un oneshot terminado puede seguir activo sin proceso vivo.
- `pid` usa la propiedad `Service.MainPID`, no el primer proceso del cgroup.
  Cero, propiedad inaccesible o unidad desaparecida producen ausencia.
- Desde K01, group refleja Service.Slice conocida o vacío. El proveedor está
  separado en linux_details.provider; Slice no equivale a grupo Windows o Unix.
  Los nombres/descripciones son los reportados por el manager.

Se exponen cuatro cadenas nuevas en ServiceInfo: load_state, active_state,
sub_state y unit_file_state. Quedan vacías en otros proveedores cuando no están
disponibles. Este cambio requiere reconstruir consumidores de la biblioteca
estática 0.x; no se promete compatibilidad binaria con structs anteriores.

## Errores y límites

Las dos enumeraciones deben terminar antes de publicar el inventario; un fallo
de D-Bus o parsing posterior a abrir el bus lanza una excepción y no devuelve
un inventario parcial como completo. El Sampler captura esa excepción en
last_error. Desde K01, fallos individuales de MainPID/Slice distinguen disponibilidad
por campo; MainPID0 conocido se conserva tipado, nunca como PID legacy válido.

Máximo de 10000 entradas por enumeración, cadenas de hasta 64 KiB y llamadas
D-Bus con timeout de 5 segundos cada una. No es un timeout global: muchas
consultas PID pueden extender el muestreo. Interfaces/mensajes se liberan con
RAII. Los permisos son los del llamador; no hay elevación ni cambios de política.
El snapshot no es atómico; estado, PID y archivos pueden cambiar entre consultas.

## Verificación de cierre

- Fixtures: running, oneshot active/exited, inactive, failed, activating,
  templates, aliases, masked, PID cero/inaccesible, duplicados, contradicciones,
  rutas/cadenas inválidas, límites y fallo de enumeración.
- Integración real de solo lectura en Ubuntu WSL: 199 servicios, 12 running,
  16 active/exited y 82 archivos no cargados. No se creó ni modificó un servicio.
- Include_services false/true/false probado mediante Monitor.
- Release Linux 10/10 y Windows 22/22. Windows valida la lógica portable, no
  el acceso nativo a D-Bus Linux.
- Paquetes instalados y consumidor CMake externo recompilado/ejecutado en Linux
  y Windows con el nuevo ServiceInfo. No se mezclaron headers/objetos anteriores.

Este cierre es **inventario/estado systemd**, no todos los servicios de todas
las distros. Control start/stop/restart, autorización, init managers alternativos
y sesiones/servicios del manager systemd del usuario quedan fuera de esta unidad.
Tampoco convierte la tarea conjunta servicios/startup en completa.

Referencias primarias: [API D-Bus systemd](https://wiki.freedesktop.org/www/Software/systemd/dbus/),
[declaraciones sd-bus](https://github.com/systemd/systemd/blob/main/src/systemd/sd-bus.h),
[ListUnitFiles en systemd](https://cgit.freedesktop.org/systemd/systemd/tree/src/systemctl/systemctl.c).
