# L05/Linux: coordinador interno de autorización

Revisión146. Segunda pieza, tras preflight145. **Solo coordinación interna y
fixtures de contrato**: no adaptador nativo, autenticación real, helper, daemon,
socket público, regla polkit, elevación o ejecución privilegiada. No instala ni
activa nada. No nueva API pública/layout/formato; Monitor/SystemActions intactos.

## Flujo implementado

observe_privileged_authorization aplica preflight a la solicitud y reglas del
mecanismo. Rechazo de scope/sintaxis/intención: cero callbacks. Faltan callbacks
o configuración de UID de autoridad: sin consulta de autorización. No backend
global ni predeterminado: una API vacía siempre rechaza.

Se obtiene un contexto available exacto del adaptador confiado: ID del bus,
nombre único del solicitante/Authority, UIDs observados, generación de conexión y
generación de política del mecanismo. Formato conservador de nombre único
:decimal.decimal y GUID32 hex minúsculo; otros formatos se rechazan, no se adivinan.
Nombres conocidos como org.freedesktop.PolicyKit1 no sustituyen al propietario
único. UID desconocido/generación0/configuración ausente son rechazo.

UID de Authority debe coincidir con **expected_authority_uid del mecanismo**,
no con UID arbitrario del cliente ni con0 hardcoded. Durante revisión se corrigió
la primera condición root-only antes de entregar la unidad. polkitd puede operar
como usuario de sistema sin privilegios; véase el
[manual oficial de polkitd](https://polkit.pages.freedesktop.org/polkit/polkitd.8.html).
Los tests cubren UID no-root y UID0 configurados explícitamente, mismatch y ausencia.
Resolver/proteger esa configuración en el adaptador real sigue pendiente.

Solicitud/plan y contexto se copian antes del check; exactamente un check con
subject, unidad y action_id fijos por verbo. Interacción permitida solo tras el
preflight de intención explícita, no para muestras de datos. El adaptador DEBE
obtener subject del transporte autenticado y vincular la respuesta a ESA llamada;
no aceptar UID/PID/busname/cookie de payload. El coordinador no puede autenticar
datos inventados por un adaptador malicioso o incorrecto.

Después se reobserva el contexto. Cambio de bus/peer/UID/owner/generación de
conexión/política, desconexión o fuente no-exacta -> resultado descartado. Para
respuesta se requiere sender=owner fijado y serial de llamada no0 igual al
reply-to serial. Es un contrato del adaptador, no parser D-Bus ni antirreplay
criptográfico. El transporte deberá aportar seriales reales y correlación causal.

Solo is_authorized true con is_challenge false y sin dismissed/retención/ID
temporal produce authorization_observed. No autorizado/challenge/desestimado/
retención -> observación false; falta de datos, binding inválido, error o excepción
-> UNKNOWN/rechazo, no false fabricado. Todas las rutas conservan check_attempted;
sin reintentos ni caché entre solicitudes.

`authorization` expresa aceptación conservadora del coordinador, NO una copia
del booleano is_authorized de polkit: por ejemplo, true+retención en la respuesta
se rechaza aquí y produce false. Sus notas explican esa distinción.

Polkit distingue autorización de challenge y define el subject system-bus-name y
AllowUserInteraction por evento de usuario. Referencia primaria:
[Authority API](https://polkit.pages.freedesktop.org/polkit/eggdbus-interface-org.freedesktop.PolicyKit1.Authority.html).
Rechazar resultados retenidos/temporales es política conservadora de ESTE
coordinador, no exigencia universal de polkit ni garantía de diálogo fresco.
La política administrativa puede autorizar implícitamente sin interacción.

## Límites de seguridad y no despliegue

Todos los structs/callbacks son internos y fabricables por tests C++; no son una
frontera de privilegios. Report true es evidencia del adaptador, **no ticket**, no
se consume por ningún ejecutor y no debe reutilizarse después. Tampoco protege
contra desconexiones/cambios posteriores al último observe ni ABA. No autenticación
de mensaje real, instalación de políticas, configuración root-owned cargada,
deadline/cancelación de IPC ni resultado de acciones implementados aquí.

El futuro adaptador debe confiar solo en conexión/bus de sistema apropiados,
credenciales obtenidas del transporte, propietario real del nombre de Authority y
configuración protegida; gestionar mensajes truncados/malformados, límites,
seriales/timeouts/cancelación y cambios de owner. No completar con booleans de
cliente o llamadas a pkexec. Los callbacks de pruebas no prueban esos requisitos.
Acceso a request/reglas/contexto serializado por el host; snapshot no es atómico
frente a escrituras concurrentes. Callbacks síncronos: sin deadline duro si bloquean.

Antes de cualquier ejecutor, volver a comprobar selección/política/contexto y
consumir autorización ligada a una sola operación en el mismo flujo. Aún falta
ese diseño/integración; ningún objeto de esta unidad autoriza servicios reales.
No se añaden operaciones fuera de la lista cerrada ni excepciones OpenRC/SysV.

## Evidencia y siguiente unidad

tests/linux_privileged_authorization.cpp: contexto válido simulado, no caché,
intención/adapter/config ausentes sin check, nombres/UID/bus/generaciones inválidos,
cambio de cada campo tras check, desconexión, sender/cookies erróneos, deny/challenge/
dismissed/retained/temporary, respuesta estimated, excepción y mutación externa de
request/reglas sin cambiar la copia. Positivos son fixtures, NO polkit autenticado.

Validación146: Linux172/172 +20 repeticiones preflight/coordinador, MSVC10 puro,
dos consumidores SDK relinkados exit0. Builds Linux inicial/final limpios. Logs
work/privileged-authorization-*. No integración nativa con Authority ni helper.

L05 sigue E, sin cerrar criterio compartido/UAC. Siguiente unidad: adaptador nativo
de identidad y Authority sobre transporte verificado, con pruebas privadas aisladas
y rechazos de protocolo, todavía sin instalar helper ni conectar ejecutor.
K03 aplazada, OpenRC132 e I07 observador intactos.

Revisión147 añade [observación nativa de identidad](LINUX_PRIVILEGED_IDENTITY.md)
desde un mensaje prestado del dispatch: checks nativos de socket/credenciales y
consultas de identidad. Validación con ABI simulada/socket real privado, no un
receptor real ni CheckAuthorization nativo; no elevar la evidencia146 retroactivamente.
