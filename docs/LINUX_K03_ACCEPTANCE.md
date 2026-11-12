# K03: aceptación privada de supervisores Linux

Esta comprobación está separada de las pruebas de protocolo existentes. No instalar ni activar un init, usar servicios personales, invocar el gestor del host o ejecutar scripts SysV para completar esta lista. Si no hay runtime ni entorno adecuado, registrar «no ejecutado» y pasar a otra subtarea.

## Preparación y límites

1. Usar un entorno de prueba que ya tenga el runtime. Registrar distribución, versión del supervisor y cómo se aisló. Para runit/s6, usar un directorio privado nuevo y un solo supervisor hijo de la prueba, no el árbol de servicios de la máquina. Para OpenRC, usar una instancia desechable con catálogo y estado propios y un servicio sintético; no apuntar al OpenRC del host. No reutilizar rutas de pruebas previas.
2. El servicio sintético debe permanecer activo hasta que lo detengan, sin red ni acceso a datos del usuario. Conservar control independiente del hijo lanzado para limpiar con tiempo límite aunque falle la biblioteca. No señalar PIDs obtenidos del inventario. Si no se puede garantizar aislamiento y limpieza, no ejecutar.
3. Consultar inventario y estado por la biblioteca; contrastar con el ciclo de vida observado del servicio de prueba. Registrar separadamente la entrega de cada solicitud, el estado posterior y, cuando proceda, la nueva identidad del proceso. Una entrega satisfactoria sin transición no cuenta como aceptación.

| Proveedor | Evidencia positiva necesaria |
| --- | --- |
| runit | `runsv` real; inventario del directorio privado y start, stop, restart de la entrada seleccionada. Comprobar que el reinicio reemplaza al hijo anterior, no solo que permanece `running`. |
| s6 | `s6-supervise` real; comprobar inventario y transiciones de la entrada privada. Registrar readiness solo si el servicio de prueba la anuncia y se observa independientemente. |
| OpenRC | Runtime real y selección pública explícita de la entrada privada; inventario sin scripts, luego una solicitud consentida por cada start/stop/restart. No derivar vida o PID de marcas. |
| SysV | Inventario de solo lectura separado de control. Control no ejecutado: falta autorización para la excepción de scripts; la autorización OpenRC no se extiende a SysV. |

Los IDs `sysv-file:...` son referencias de inventario, no nombres systemd ni
autorización para ejecutar scripts. El consumidor no debe quitar el prefijo y
reenviar el nombre a otra ruta de control. La prueba de selección comprueba el
rechazo de start/stop/restart al recibir un ID SysV en la API de control systemd.

Para los controles autorizados, conservar además un rechazo sin despacho (por ejemplo, sin consentimiento) y constancia de que no quedaron procesos o servicios de prueba. Los [fixtures ya existentes](LINUX_K03_AUDIT.md) prueban guardas y proyección, no sustituyen estos resultados.

Revisión 173: Ubuntu WSL, 18/18 pruebas focales (runit, s6, OpenRC, SysV y selección). La primera consulta de rutas con variables fue inválida por expansión en el shell de lanzamiento. La consulta corregida con argumentos literales (`command -v`, `whereis -b` y comprobación de rutas específicas OpenRC) no encontró ejecutables. Esto no demuestra ausencia fuera de las rutas consultadas. Aceptación real no ejecutada; no se instaló ni arrancó un runtime.
