# I07: decision necesaria sobre ejecucion temprana

Actualizacion128: ante "prosigue" tras las opciones127 se continua de forma
conservadora por el alcance observador recomendado y K03 Linux, sin interpretar
eso como permiso para un iniciador nuevo. I07/criterio original permanecen
abiertos; causa no verificable UNKNOWN/aplazada. No se afirma que el usuario haya
autorizado gestionar sus apps ni se marca la tarea completada por este cambio
de prioridad. Primera unidad K03: LINUX_RUNIT.md, protocolo explicito.

## Revision127: limite causal y nueva decision de alcance

La arquitectura de recorder autorizada en103 ya esta implementada, incluida la
integracion manual end-to-end126. No se pide autorizarla otra vez. El nuevo
punto es DISTINTO: observar arranques ajenos vs convertirse en su iniciador.

La especificacion [XDG Autostart](https://specifications.freedesktop.org/autostart/latest/)
define ubicacion, precedencia y condiciones para que el entorno lance entradas;
no define en ese documento un registro causal autenticado de ejecuciones.
[systemd Desktop Integration](https://systemd.io/DESKTOP_ENVIRONMENTS/) describe
integracion XDG opt-in del desktop y unidades de aplicaciones. Estas incluyen
aplicaciones normales, no solo autostart; nombres de unidad, sesion activa,
cercania al login y marcadores GIO no se elevaran por ello a prueba automatica.
Es una conclusion de alcance de las fuentes revisadas, no una afirmacion de
que todos los desktops carezcan de fuentes propias mas especificas.

Un host propio que ademas SELECCIONE Y LANCE aplicaciones configuradas puede
demostrar su propia pasada gestionada y conectar recibos/recursos. Eso cambia
el papel del producto respecto al recorder observador ya aprobado. Incluso ese
host puede ser invocado manualmente: su mera presencia no demuestra que el OS
lo inicio automaticamente. No se etiquetara ese caso como automatic-origin.

Decision solicitada al usuario, SIN elegir la opcion preseleccionada por el UI:

1. Recomendacion: mantener la biblioteca observadora, conservar causa desconocida
   cuando no sea verificable y aplazar esa parte de I07; seguir K03 Linux. No
   cerrar I07 ni modificar su criterio/porcentaje antes de la respuesta.
2. Desarrollar un iniciador propio opt-in y su pasada gestionada, con pruebas
   PRIVADAS. No instalar, activar login, sustituir el desktop, lanzar apps reales
   ni afirmar automaticamente que el OS inicio el host. Esas acciones requeririan
   autorizacion posterior especifica. Tampoco garantiza apps ajenas/descendencia.

Revision127 es AUDITORIA/DECISION, no nueva funcion implementada. No se ejecutaron
tests ni sondas, no se modifico codigo/formatos ni inicio real. Ultima evidencia
de compilacion y suites sigue siendo126 (Linux154/154+30/MSVC20/SDK0).
I07 E, C60/E9/P4/B2,3X:80% cerrado/20% abierto, sin credito parcial/ID nuevo.

---

Actualizacion103: el usuario respondio "prosigue" a la propuesta concreta de
desarrollar la segunda opcion SIN instalar/activar. Decision de arquitectura
resuelta para desarrollo: host opcional propio y API reutilizable implementados;
ver STARTUP_RECORDER.md. No autorizacion de login real ni cierre de I07.
El texto102 siguiente se conserva como evidencia del checkpoint de decision.

Revision102, 2026-10-24. Auditoria de implementacion existente; sin captura,
instalacion, ejecucion de apps ni cambio del inicio de sesion del usuario.

## Evidencia actual

- StartupImpactObserver mide deltas desde una baseline ya obtenida; su asociacion
  la proporciona el caller. No inicia trabajo antes de ser construido/invocado.
- discover_startup_process_candidates lee marcadores GIO del UID del caller.
  Son evidencia declarada estimated, no prueba de causa automatica de startup.
- examples/startup_capture.cpp ya coordina scans/poll/store mientras esta en
  ejecucion. No es recorder instalado ni se ejecuta antes de su invocacion.
- read_startup_child_exit_sample recupera el final de hijos propios todavia no
  recolectados, con baseline previa; no recupera apps ajenas desaparecidas.
- apply_startup_impact_observations es enlace explicito, no captura temprana.

Fuentes revisadas: STARTUP_CAPTURE.md, STARTUP_CANDIDATES.md,
STARTUP_OBSERVER.md, STARTUP_CHILD_EXIT.md y STARTUP_OVERLAY.md; headers y
linux_startup_candidates.cpp/startup_observer.cpp correspondientes.

## Eleccion pendiente del usuario

Una biblioteca no ejecutada no puede obtener retrospectivamente contadores de
procesos ya desaparecidos. Para captura anterior a abrir el GUI se necesita
un host que este activo a tiempo, o registros de una fuente ya activa con las
identidades/contadores necesarios. No existe tal productor temprano implementado
en PCInsight; no se presupone que la distro conserve esos datos.

Alternativas de arquitectura, NO activadas ni elegidas automaticamente:

1. El futuro ejecutable consumidor/GUI arranca con la sesion e inicializa la
   biblioteca antes de mostrar la ventana. Sin ejecutable adicional; no mide lo
   anterior a su propio arranque ni garantiza precedencia sobre todas las apps.
2. Un grabador minimo opcional del mismo proyecto arranca con la sesion y guarda
   la captura para el GUI posterior. Seria un proceso adicional de PCInsight,
   no una aplicacion de terceros ni un paquete externo de telemetria. Requiere
   aprobar esa arquitectura y, POR SEPARADO, activar cualquier inicio real.

Ninguna alternativa certifica por si sola causa de autostart, captura de todas
las apps/descendientes, procesos ajenos breves o compatibilidad de todos los
desktops. Tras elegir host se debe definir el proveedor/launcher de evidencia,
su cobertura/consentimiento y pruebas nativas privadas; no cerrar I07 por un
wrapper de marcadores ni por instalar un .desktop.

Restriccion vigente: Linux primero, sin utilidades externas para recolectar ni
instalar dependencias automaticamente. No crear servicios/tareas de login,
editar XDG del usuario, lanzar apps reales, capturar entornos de otros UID ni
ampliar el permiso previo eBPF privado a vigilancia global sin autorizacion.

## Estado

I07 sigue E. No se crean nuevos IDs ni se concede credito parcial. Porcentaje
oficial60/75=80,0%;15/75=20,0% abierto. Esta revision es auditoria/decision,
no implementacion nueva: no se repitieron suites ni se reetiquetan pruebas
anteriores como resultado actual. Falta eleccion del host temprano antes de
implementar una integracion que cambie arquitectura/dependencias del consumidor.
