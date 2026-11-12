# Estado y cobertura de sesiones Linux (J01)

Actualización posterior J04 parcial: [LINUX_USER_IO.md](LINUX_USER_IO.md) añade
informe E/S por UID numérico con cobertura y control de UID en los extremos del
intervalo de disco. CPU/RAM conservan username; no completa red F03 ni J04.

Esta unidad completa el contrato de inventario/estado logind y registros utmp,
con recursos visibles por username. No agrega control de cuentas, detach ni
acciones destructivas: J02/J05/J06 permanecen separados. Los recursos de disco/
red y su completitud se cuentan en J04/F03, no se cierran aquí.

## API y semántica

UserSession.linux_details es Metric<LinuxSessionDetails>. provider identifica
login1 o utmp. Cada campo conserva presencia, soporte y nota; comprobar Metric,
no solo el texto legacy UserSession.status.

| Fuente/campo | Lo que realmente informa |
|---|---|
| login1 state | Propiedad State nativa, sin convertirla a WTS connected/disconnected |
| login1 active | Propiedad Active de primer plano; false es un valor conocido, no desconexión |
| login1 remote | Propiedad Remote de origen remoto, no salud actual de la conexión |
| login1 unix_uid | UID del tuple ListSessions; no identidad global entre namespaces |
| utmp state | reported_login: registro USER_PROCESS, no prueba de sesión viva/activa |
| utmp active/remote/unix_uid | Desconocidos; no se infieren de PID, hostname ni NSS |

State y Active se consultan por separado. No se fuerza concordancia ni se deriva
uno del otro: sus lecturas no son atómicas. Se conservan estados futuros que sean
tokens ASCII válidos de hasta128B, sin inventar su significado. Estado inválido,
ausente o permiso denegado es Metric sin valor; el status legacy da unknown.
Un error de State no borra un Active válido de la misma identidad y viceversa.
Estas propiedades tampoco prueban presencia humana, pantalla desbloqueada o
actividad de CPU. [Contrato nativo login1](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.login1.xml).

## Identidad durante la consulta

Todas las propiedades usan el unique owner obtenido antes de ListSessions.
Se consultan TimestampMonotonic antes y después de Id/Name/User/State/Active/
Remote. Se exige timestamp positivo estable, Id/Name/UID coincidentes con la
fila enumerada y disponibilidad de esas propiedades de identidad. Si falta algo,
cambia o no coincide, se conservan la fila/UID reportados por ListSessions pero
state/active/remote quedan desconocidos y no se publica logout_token. Permiso
denegado observado se distingue de ausencia/error genérico.

El timestamp usado en el token es el verificado, no un PID/TTY. El inventario
rechaza IDs o rutas de objeto duplicadas, y comprueba al final que sigue el mismo
owner. Fallo de enumeración/owner/parseo/límite se propaga: no se publica una lista
parcial como completa ni se oculta como un fallback satisfactorio.

El control J02 conserva sus comprobaciones frescas antes de actuar. Un token
publicado no garantiza autorización. Estas consultas no son transacción ni una
referencia inmutable: reutilización A->B->A y cambios fuera del intervalo leído
siguen siendo posibles. UID de la enumeración tampoco es prueba de la identidad
actual cuando las propiedades no se verificaron.

## Cobertura e inventario vacío

SystemSnapshot.linux_session_inventory_source es un Metric separado de las filas:

- login1 conocido: se completó ListSessions del proveedor seleccionado, aunque
  devolviera cero filas. No significa que estén todos los logins del host.
- utmp conocido: libc completó la lectura de registros, incluso archivo vacío.
  No certifica que utmp esté actualizado ni que registre sesiones gráficas/contenedores.
- Sin valor: captura desactivada o fuente utmp ausente. La nota distingue el caso;
  una lista legacy vacía no se presenta como “ningún usuario conectado”.

Se prefiere login1 opcional mediante sd-bus ya instalado. Si no hay biblioteca,
bus o servicio compatible, se usa utmp; prefer_logind_sessions=false elige utmp
explícitamente. No se mezclan fuentes ni se deducen equivalencias entre sus IDs.
No se instalan dependencias ni se ejecutan loginctl/who/w/otros programas.
Compatibilidad elogind depende de que implemente esa API y exista el transporte
sd-bus: no se afirma validación nativa de todas las distros.

La lectura utmp distingue ENOENT observado de fin normal/ESRCH de libc. Otros
errores de lectura se propagan. No valida el archivo byte a byte ni elimina
registros viejos. El mutex cubre este proveedor, no otros usos utmpx/NSS del host.

## Recursos y límites

Las sumas CPU/RAM siguen por username de procesos visibles, no por sesión ni por
UID validado globalmente. Varias sesiones del mismo usuario repiten ese total;
no sumarlas dos veces. include_processes=false oculta la lista, no evita recopilar
los procesos necesarios para esos totales. Primera CPU sin baseline permanece
desconocida. Métricas conocidas parciales son estimated con conteos/nota; ausencia,
NaN/rango inválido, overflow o todos los datos inaccesibles no se convierten a0.
RSS puede contar páginas compartidas varias veces. Ver LINUX_USERS.md.

Se comprueban también contradicciones de metadatos entre filas duplicadas, para
no conservar arbitrariamente un UID/estado. Se mantienen los límites10000sesiones,
100000registros/procesos y payload16MiB de agregación. Cadenas D-Bus hasta4096B,
rutas1024, owner255. D-Bus5s por llamada y presupuesto de enumeración5s revisado
entre filas: ahora hay más consultas por fila, NO es deadline global de5s.
NSS/libc/bus opening tampoco tienen un deadline total en esta unidad.

API0.x cambia layout UserSession/SystemSnapshot: recompilar biblioteca y TODOS
los consumidores. No cambia ABI virtual de acciones ni formato de historial.
Windows deja los campos Linux ausentes y conserva su proveedor WTS.

## Pruebas

tests/linux_session_state.cpp verifica estados nativos/futuros, false conocido,
campos independientes, permisos, identidad ID/name/UID/timestamp cambiada o ausente,
metadatos duplicados incompatibles y ausencia en backends no Linux. En Linux
consulta login1 real de forma read-only y el Monitor público, sin invocar métodos
de control. tests/linux_users.cpp conserva las pruebas de sumas visibles y usa
utmp privado para sesiones repetidas, datos desconocidos, archivo vacío y archivo
ausente. Solo el proceso de prueba cambia su selector utmp y lo restaura.

El cierre funcional J01 cubre esos contratos/providers, no sesiones certificadas
en todas las distros, recursos de procesos ocultos ni estado de una conexión
remota. No se crea/cierra/modifica ninguna sesión real para probarlo. Hardware y
distros adicionales siguen N02; acciones no verificadas no se cuentan terminadas.

2026-10-16: Release Linux85/85 y Windows105/105 aprobadas. Paquetes reinstalados
y consumidores externos recompilados/ejecutados exit0; consumidor Linux consulta
también los nuevos campos mediante la API instalada. Lectura nativa Ubuntu WSL:
1sesión login1,1State conocido y1flag Active conocido (conteo de disponibilidad,
no afirmación de que la sesión esté en primer plano). No se invocaron controles
de sesión. Fixture utmp privado verificó3registros, sumas CPU/RAM, archivo vacío
con fuente conocida y archivo ausente sin fuente conocida. Windows ejecutó las
pruebas portables, no un proveedor logind/utmp nativo. Build incremental final
sin avisos; los avisos transitorios iniciales de reloj MSFS y NOMINMAX existente
no se confundieron con validación de otro entorno ni motivaron cambiar el reloj.
Los riesgos intermitentes previos CPU/historial de N03 no se declaran resueltos
porque esta suite haya pasado. No se agregan IDs al conteo del proyecto.
