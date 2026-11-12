# L05/Linux: preflight interno de operaciones cerradas

Revisión145. Inicio de L05, **no helper privilegiado operativo**. Se terminó antes
la [auditoría K03](LINUX_K03_AUDIT.md): K03 sigue abierta/aplazada por control no
autorizado y aceptación real, no por falta de otra proyección de inventario.

Código interno linux_privileged_preflight.hpp/.cpp, no header instalado ni nueva
API pública. Función pura sin callbacks, IPC, lectura de archivos, daemon, socket,
polkit, reglas, setuid, elevación, instalación, servicio o ejecución. No conexión
a Monitor/SystemActions ni cambio de formatos/layouts. No empieza UAC/Windows.

## Primera superficie admitida

Solo planificación de start/stop/restart systemd **del sistema** sobre nombre
canónico .service permitido literalmente por el futuro mecanismo. No comandos,
argv, PATH, shell, environment, cwd, archivos, PID/UID aportados por el cliente,
startup/reset/logout/efficiency ni scripts OpenRC/SysV en esta petición.
Esto NO elimina esos requisitos del proyecto: quedan fuera de esta primera
superficie y no se añaden por fallback.

PrivilegedServiceRequest contiene unidad, ServiceAction e intención explícita
declarada. false -> sin plan; true NO es identidad/autenticación/autorización.
Se valida nombre con el validador común de servicios y enum cerrado sin máscara
calculada a partir de un enum arbitrario. No se interpretan IDs de inventario
como autoridad de ejecución.

Reglas: configuración del **mecanismo**, no lista enviada por el cliente.
Hasta256 nombres exactos; start1/stop2/restart4, cero deniega y bits desconocidos
invalidan configuración. Nombre inválido o duplicado, incluso ajeno a la unidad
pedida, deniega todo el preflight; no se ignoran reglas defectuosas. Lista vacía,
unidad ausente u operación no concedida -> sin plan. No wildcards/regex/prefijos
ni autorización de otra instancia de una plantilla por compartir nombre base.
No cargador de configuración todavía: los tests suministran reglas privadas.

Éxito devuelve una copia propia de unidad/operación y un ID de política fijo por
verbo, provisional para el futuro mecanismo: org.pcinsight.helper.systemd.start,
stop o restart. No se registra ninguna acción con polkit. Metric available significa
**preflight sintáctico/de alcance**, no permiso del sistema, grant ni prueba de
usuario. El struct interno puede fabricarse en C++; no es una frontera de seguridad
ni token que un ejecutor deba aceptar como autorización. Hoy no hay ejecutor.
El host serializa acceso a strings/reglas; no se promete copia atómica concurrente.

## Próxima frontera de seguridad (NO implementada145)

Identidad del solicitante obtenida del transporte autenticado, nunca de UID/PID/
bus-name reclamado en payload; sujeto ligado a esa conexión y propietario válido.
Congelar solicitud completa antes de autorizar, comprobar cambios/desconexión,
respuestas del Authority correcto y resolver toda ambigüedad/error como rechazo.
No reutilizar resultados entre solicitudes ni convertir un plan en ticket.

Polkit separa subject, action_id y resultado: is_authorized es autorización;
is_challenge indica que podría obtenerse tras más información, no éxito. Interacción
de usuario es una opción distinta, no una elevación implícita. Referencia primaria:
[Authority API](https://polkit.pages.freedesktop.org/polkit/eggdbus-interface-org.freedesktop.PolicyKit1.Authority.html).

Diseño pendiente: backend de IPC, comprobación de peer/Authority, consulta por
solicitud sin caché propia, diálogo solo por evento explícito, cancelación/timeouts,
rechazos/retención/política local y resultado final nativo separado. La arquitectura
de mecanismos/subjects/agentes está documentada en
[PolkitAuthority](https://polkit.pages.freedesktop.org/polkit/PolkitAuthority.html).
No basta con comprobar un booleano aportado por el cliente o ejecutar pkexec.

También pendientes antes de despliegue: configuración root-owned/sin writable
por cliente, instalación/activación explícitas, límites de payload y concurrencia,
revalidación de manager/target, auditoría opt-in, tratamiento de efectos sobre
dependencias, pruebas de rechazo y aceptación real. Las reglas de usuario/sistema
polkit pueden afectar alcance/retención; no se promete impedir política root.
No se incorpora un helper root genérico ni se instala uno en esta unidad.

## Evidencia y estado

tests/linux_privileged_preflight.cpp: tres verbos/IDs fijos, default deny/sin
intención/target ausente/op no concedida, máscara0/bits desconocidos/enum inválido,
nombres/rutas/NUL/control no válidos, duplicados/config ajena inválida, bound256/
257, instancias exactas y copia independiente frente a cambios posteriores.
Tests puros, no polkit/daemon autenticados, acciones reales o privilegios.

Validación145: Linux171/171 +10 repeticiones de preflight, MSVC10 puro y dos
consumidores SDK relinkados exit0. Builds Linux inicial/final limpios. Logs
work/privileged-preflight-*. Sin autenticación polkit, CI remota o integración de helper root.

L05 P->E, no C: aún faltan identidad IPC, autorización, ejecución cerrada y
aceptación; UAC compartido aplazado por Linux primero. Siguiente unidad: frontera
de identidad/autorización con fallos cerrados y fixtures privados, antes de conectar
un ejecutor o instalar política. K03/OpenRC132/I07 y restantes criterios intactos.

Revisión146 añade [coordinación interna de autorización](LINUX_PRIVILEGED_AUTHORIZATION.md):
snapshot/binding/revalidación/denegaciones con fixtures, todavía sin adaptador
nativo, autenticación de peer real, política instalada o ejecutor privilegiado.
