# Inventario de sesiones Windows WTS

Monitor consulta sesiones locales cuando include_user_sessions=true. El valor
por defecto sigue siendo false. El lector WTS es independiente de procesos;
Monitor añade ahora la agregación documentada en WINDOWS_USER_RESOURCES.md,
recopilando procesos internamente aunque se oculte su lista.
No usa programas externos, PowerShell, WMI, elevación ni archivos temporales.
wtsapi32 es una biblioteca incluida con Windows, no una instalación adicional.

## Contrato público

Una fila UserSession por ID WTS con cuenta reportada no vacía. id tiene formato
wts:N; username es DOMAIN\usuario si existe dominio, o usuario si está vacío.
display_name es el nombre de cuenta sin dominio, NO el nombre completo del perfil.
Varias sesiones de la misma cuenta se conservan, no se sobrescriben. Se omiten
listeners (no representan un login) y entradas sin cuenta reportada.

status conserva el estado de conexión WTS: active, connected, connect_query,
shadow, disconnected, idle, listen, reset, down o init. listen tiene mapeo pero
sus filas se omiten del inventario de cuentas. Estados futuros/desconocidos
conservan su número en unknown(N); no se convierten en active. El estado de
conexión no certifica desbloqueo, actividad humana ni consumo de aplicaciones.

El lector WTS aislado usa resource_scope=windows-wts-inventory-only. CPU, RAM, disco y red quedan sin valor
con una nota de ausencia de contabilidad. No se suman procesos por username ni
se atribuye consumo a una sesión por compartir una cuenta. Estos recursos son
una unidad separada: WINDOWS_USER_RESOURCES.md implementa CPU/RAM observadas
en Monitor, sin afirmar cobertura completa y manteniendo disco/red ausentes.

disconnect_available y logout_available son false: este proveedor es solo
inventario. logout_token queda vacío. wts:N NO es una identidad persistente
verificada para acciones; un ID puede reutilizarse. No se implementa aquí
disconnect/logout, gestión de cuentas, ni lanzamiento bajo credenciales ajenas.
La acción de lanzamiento existente sigue usando el contexto del caller.

## Fallos y límites

Los fallos de enumeración/consulta propagan std::system_error con código Win32,
en vez de devolver una lista parcial/vacía como si fuera un inventario completo.
Monitor propaga ese fallo. Una sesión que desaparece durante la consulta puede
invalidar esta muestra: el consumidor puede volver a solicitarla. No hay cambio
de permisos ni reintentos ocultos. Un resultado vacío solo significa que esta
consulta no reportó cuentas, no que no existan usuarios en todo el equipo.

Límites:10000 entradas,4096 unidades UTF-16 por campo,16MiB de texto final de
identidades. Se comprueban puntero nulo con count no cero, ID duplicado/reservado,
longitud/alineación en bytes, terminador dentro del buffer, UTF-16 válido y API
inyectada completa. Se valida toda la lista antes de devolverla. WTSFreeMemory
libera todos los buffers, también ante fallos/excepciones. Se confía en los
buffers entregados por la API del sistema; esto no es un parser de punteros de
memoria arbitrarios. Los límites no imponen timeout a llamadas WTS bloqueadas.

La enumeración y las consultas de identidad no son un snapshot atómico. No se
afirma protección contra reutilización concurrente del ID o cambios de cuenta.
Tampoco hay garantía de cobertura completa cuando el caller carece de permisos
de consulta, hay restricciones del servicio o de la versión de Windows.

## Verificación

Pruebas inyectadas: múltiples sesiones de una cuenta, dominio vacío, Unicode,
estados conocidos/futuros, listener/sin cuenta, lista vacía, errores API y de
permisos, identidad truncada/malformada, límites y duplicados, limpieza de buffers
incluso en errores, API incompleta. Prueba nativa de solo lectura: sesión actual
local identificada por ProcessIdToSessionId; Monitor opt-in/opt-out con procesos
ocultos. No se desconecta ni termina ninguna sesión real.

Resultado Release: Windows63/63 y Linux49/49; bibliotecas/headers instalados y
consumidores externos reconstruidos/ejecutados en ambos. El consumidor Windows
también consulta usuarios con procesos ocultos. Linux verifica regresiones del
backend propio; no se afirma cobertura WTS por compilar en Linux. MSVC, ARM64,
RDS multiusuario real y denegaciones reales del servicio siguen sin validación;
los errores/permisos y varias sesiones se verificaron con API inyectada.

Fuentes oficiales: [enumeración WTS](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsenumeratesessionsw),
[consulta de identidad](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsquerysessioninformationw),
[estados de conexión](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/ne-wtsapi32-wts_connectstate_class).
