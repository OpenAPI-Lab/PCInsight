# Recursos visibles por sesión y cuenta Windows

Monitor ahora calcula CPU y memoria de las sesiones WTS opt-in usando los
procesos visibles. include_user_sessions exige recopilar procesos internamente
aunque include_processes=false; la lista se oculta después de calcular recursos.
Desactivar ambos descarta las bases de CPU. No se añade polling/daemon ni CLI.

## Asociación y alcance

ProcessInfo.windows_session_id contiene TokenSessionId del token primario leído
desde el handle del proceso, no una consulta posterior por PID numérico.
La cuenta y el ID de sesión se leen del mismo handle de token. Es un Metric:
permiso denegado/consulta inválida produce ausencia, nunca sesión0 inventada.
En Linux queda ausente. El campo no es un token persistente para acciones.

Para sumar un proceso se exige ID de sesión available, creación FILETIME válida
y available, y cuenta exactamente igual a la cuenta de la fila WTS. No se usa
username como único criterio: dos sesiones de la misma cuenta tienen sumas
independientes. Procesos con otra cuenta dentro de la misma sesión (servicios,
RunAs), cuenta desconocida, otra sesión o identidad no comprobable no se atribuyen.
Mayúsculas, aliases y diferencias de formato entre WTS y LookupAccountSid pueden
dejar procesos sin atribuir: no se adivina equivalencia ni se afirma validación SID.
El PID0 se trata como pseudo-proceso idle, no como trabajo de un usuario.

resource_scope=windows-visible-processes-by-wts-session-and-account. Las métricas
son estimated: se suman solo valores conocidos, con conteos conocidos/ausentes
y procesos sin atribuir globalmente en las notas. No equivalen al total completo
de la cuenta, al total de sesión del Administrador de tareas ni a responsabilidad
causal. Enumeración de procesos, WTS, permisos y ventanas temporales son parciales
y no atómicos. Reutilización concurrente de sesión/cambio de token no se eliminan.

CPU suma porcentajes por proceso normalizados por todos los procesadores lógicos;
la primera muestra no tiene delta. No se fuerza cero. RAM suma working sets,
incluyendo páginas compartidas que pueden contarse varias veces; no es memoria
física única, private working set ni PSS. Disco/red solo se agregan si un proveedor
ya produjo tasas válidas: el backend Windows actual no lo hace por proceso y
deja ambas ausentes. La lógica de suma no convierte E/S genérica en disco/red.

Cada métrica se trata independientemente. Sin valores conocidos: nullopt;
permission_required se conserva si algún valor atribuido fue denegado. Con parte
conocida: suma estimated, sin extrapolar. NaN/infinito/CPU fuera0..100 y soporte
incompatible se omiten. Suma CPU>100 o overflow uint64: ausente sin clamp/wrap.
Solo un cero leído válido puede contribuir a un cero medido. Una sesión sin
procesos atribuibles no se presenta como consumo0.

## Límites y verificación

Núcleo sin IO portable: fill_windows_user_resources, límites10000 sesiones,
100000 procesos,32768 bytes por identidad,16MiB de identidades recibidas. IDs
WTS canónicos/únicos y PID no duplicados; entradas inválidas lanzan excepción
sin publicar cambios parciales. No acepta tokens/IDs implícitos ni listas
truncadas como completas. No muta procesos o sesiones ni necesita elevación.

Pruebas: sesiones separadas de una cuenta, cuentas distintas, cero real,
permisos/parcialidad/ausencia, NaN/infinito y overflow, duplicados/límites/IDs
malformados, identidad/cuenta/sesión no válidas y publicación transaccional.
Prueba nativa WTS/Monitor: RAM de sesión actual, CPU en segunda muestra, procesos
ocultos, reset tras opt-out y TokenSessionId del proceso caller. No sesiones
reales cerradas/desconectadas. RDS multiusuario real y otros permisos/equipos
siguen requiriendo validación. API0.x ampliada: recompilar biblioteca/consumidores.

Resultado Release: Windows64/64 y Linux50/50. Bibliotecas/headers instalados y
consumidores externos reconstruidos/ejecutados en ambos. La prueba portable de
sumas corre también en Linux; la asociación TokenSessionId y WTS nativa solo
se verificó en Windows, sin extrapolar la prueba portable a ese proveedor.

Fuentes oficiales: [TokenSessionId](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ne-winnt-token_information_class),
[GetTokenInformation](https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-gettokeninformation).
El inventario y sus límites se documentan en [WINDOWS_USERS.md](WINDOWS_USERS.md).
