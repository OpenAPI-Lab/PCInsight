# N03: Startup Apps Linux para el GUI

Revisión 163, 2026-11-06. Traza R15 y Run new task a I01–I07/L01. Separar
configuración, elegibilidad en el contexto observado e impacto observado:
una entrada habilitada puede no ser elegible; una elegible puede no arrancar;
un impacto histórico no prueba que el login actual la haya iniciado.

## Inventario y campos

Leer `Monitor::sample` o `Sampler` con `include_startup_entries=true`. El
inventario XDG conserva el archivo efectivo por nombre y precedencia: una
copia personal que enmascara la del sistema no autoriza a saltar a esta última.
Se usa el contexto del caller; no es censo de otros usuarios ni de todos los
mecanismos de inicio. El vector no trae un Metric de completitud: vacío no
certifica que no exista autostart. [Precedencia](XDG_PRECEDENCE.md),
[pruebas](../tests/linux_startup_inventory.cpp).

| Dato | API / unidad | Alcance y evidencia |
| --- | --- | --- |
| Name / ID / fuente | `StartupEntry.name/id/source`, texto | Etiqueta de inventario e ID de la fuente `.desktop` efectiva. No usar nombre comercial como clave ni tomar un ID viejo como autorización. `command` es metadata, no `argv`. [Precedencia](XDG_PRECEDENCE.md). |
| Enabled / disabled | `status`, enum | Flags de configuración; `unknown` no equivale a disabled. No muestra procesos running ni certifica el comportamiento del escritorio. [Flags](LINUX_STARTUP.md), [pruebas](../tests/linux_startup.cpp). |
| Elegibilidad | `xdg_standard_eligible`, Metric bool | Hidden, Type, OnlyShowIn/NotShowIn y TryExec en contexto del caller, entre comprobaciones estándar. No ejecuta TryExec/Exec. Distinguir false conocido de ausencia. [Contrato](XDG_ELIGIBILITY.md), [pruebas](../tests/linux_startup_eligibility.cpp). |
| Condición adicional | `autostart_condition_satisfied`, Metric bool | Predicado separado, no prueba de soporte del gestor o lanzamiento. Sin clave: true; sintaxis/fuente sin soporte: desconocido. [Archivo](XDG_FILE_CONDITION.md), [GSettings](XDG_GSETTINGS.md), [sesión](XDG_SESSION_CONDITION.md), [pruebas](../tests/linux_startup_condition.cpp). |
| Publisher | `publisher_label`, texto | Editor/desarrollador declarado de AppStream, no firmante verificado. Opt-in `include_linux_startup_publisher` junto al inventario; metadatos locales/libxml2 opcionales. No rellenar a partir de UID o `publisher` legacy. [Contrato](LINUX_STARTUP_PUBLISHER.md), [pruebas](../tests/linux_startup_publisher.cpp). |
| Startup impact | `observed_startup_impact`, categoría; `impact_scope`, texto | Mostrar soporte y alcance; el shorthand `impact` no sustituye el Metric. Not measured no significa none; una medición none no demuestra impacto cero en todo el login. [Medición](STARTUP_IMPACT.md), [pruebas](../tests/startup_impact.cpp). |
| Recursos de impacto | `observed_startup_cpu_time` ns, `observed_startup_disk_bytes` B, `observed_startup_interval` duración monotónica | Deltas de ventana observada; CPU no es porcentaje, disco no es velocidad física, intervalo no es fecha. Un overlay histórico puede dejarlo ausente. No sumar múltiples grupos para fabricar un total. [Overlay](STARTUP_OVERLAY.md), [overlay histórico](STARTUP_BIRTH_OVERLAY.md), [pruebas](../tests/startup_birth_overlay.cpp). |

La biblioteca conserva el papel de observadora. Las APIs explícitas de
medición y overlay requieren asociaciones y contextos aceptados; no inferir
causalidad de inicio por nombre, PID, ruta, proximidad al login o coincidencia
de ID histórico. La captura temprana automática con causa no verificable
permanece abierta en I07. No se instala ni activa un iniciador/recorder desde
el ejemplo ni desde una lectura de inventario. [Decisión I07](STARTUP_EARLY_DECISION.md).

## Acciones y Properties

| Acción | API | Qué comunicar al usuario |
| --- | --- | --- |
| Properties | `get_startup_properties(entry.id)` | Solo consulta la fuente efectiva, no el ejecutable ni un diálogo del SO. `source_file_size_bytes` mide el `.desktop`; `fields` conserva orden, duplicados, grupo y valores raw. No interpretar esos valores como shell ni registrarlos por defecto. Tratar `stale_target` recargando inventario, no escogiendo otro destino. [Contrato](XDG_PROPERTIES.md), [pruebas](../tests/linux_startup_properties.cpp). |
| Enable / Disable personal | `set_startup_enabled_for_current_user(id, enabled)` | Acción explícita: crea o edita el override personal, no el origen system. Un override ya existente conserva su personalización; recargar inventario porque el ID efectivo puede cambiar. No lanza, detiene ni vuelve elegible la aplicación por sí mismo. [Contrato](XDG_OVERRIDE.md), [pruebas](../tests/linux_startup_override.cpp). |
| Edición directa | `set_startup_enabled(path, enabled)` | Edita precisamente el archivo indicado; con permisos sobre system puede afectar varios usuarios. No usar como fallback automático del toggle personal. [Contrato](LINUX_STARTUP.md). |
| Restablecer a system | `reset_startup_for_current_user(personal_id)` | Acción explícita distinta de habilitar; requiere contraparte system válida, retira la copia personal de futuros logins y conserva respaldo en `startup_backup_path`. No restablece una entrada exclusivamente personal ni fuerza enabled. [Contrato](XDG_RESET.md), [pruebas](../tests/linux_startup_reset.cpp). |
| Run new task | `launch_process(LaunchRequest)` | Acción separada y confirmada; no pasar `command` o `raw_value` directamente como shell/argv. Lanzar manualmente no demuestra origen de autostart. [Contrato](LINUX_LAUNCH.md). |

En mutaciones, un error posterior al cambio no siempre significa que no se
aplicó: respetar diagnósticos y respaldo, recargar y no reintentar a ciegas.
Ninguna acción se ejecuta por consultar o renderizar estas filas.

## Ejemplo SDK

[linux_startup.cpp](../examples/linux_startup.cpp) usa el SDK público. Sin
argumentos muestra ayuda. `--sample` lee inventario Linux sin activar publisher
opcional ni medición; `--properties ID` consulta de solo lectura un ID efectivo
e imprime claves y grupos, sin valores raw ni comando. `--self-test` verifica
desconocido, enabled pero no elegible, impacto none y CPU cero conocidos,
tamaño legacy no promovido, claves duplicadas y redacción. El
[consumidor externo](../tests/startup_consumer/CMakeLists.txt) usa el SDK instalado;
su CMake no impone el del GUI. No hay cambios de ABI ni de biblioteca productiva.

Verificación: Linux Release 196/196, dos pruebas del ejemplo repetidas diez
veces, diez recorridos MSVC portables, 29 enlaces locales comprobados y
consumidor del SDK instalado con self-test y muestra exit0. Logs
`work/startup-contract-*`. Esto no valida un nuevo inicio gráfico ni cierra I07.
