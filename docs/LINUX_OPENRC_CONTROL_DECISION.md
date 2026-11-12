# OpenRC: decisión de control

## Avance159 — 2026-11-02

[API pública OpenRC](LINUX_OPENRC_CONTROL_PUBLIC.md) integrada y probada desde
SDK instalado, con selección original fijada/consentimiento y runner independiente.
No OpenRC real ni nuevas acciones autorizadas. Consulta SysV separada emitida,
todavía sin respuesta; NO extender el permiso148. K03 E: aceptación real y SysV
pendientes; integración pública OpenRC terminada. L05 despliegue aplazado.

## Avance158 — 2026-11-02

Se retoma la ruta K03 autorizada148 sin ampliar L05 a otro transporte. Ya existe
un [ejecutor interno opt-in](LINUX_OPENRC_CONTROL.md) con validación de rutas/FD,
entorno cerrado, hijo gated/pidfd, ACK y espera/salida separados del estado vivo.
Pruebas solo con runtime modelo privado, no OpenRC real. Falta integración pública
y aceptación aislada; K03 E, L05 despliegue aplazado. El permiso148 se conserva;
no concede acciones reales ni control SysV. Las carencias de implementación
mencionadas debajo describen el checkpoint148, no anulan este avance.

## Decisión vigente — 2026-11-02, revisión148

El usuario respondió explícitamente: «Sí, permitir scripts/runtime de OpenRC
solo para control explícito». Queda aprobada la opción1, exclusivamente para
start/stop/restart pedidos explícitamente, nunca para recopilar datos. Desarrollo
opt-in/desactivado por defecto y pruebas privadas; sin instalar dependencias,
elevar privilegios ni tocar servicios reales. No es permiso para SysV, helper root,
polkit/reglas/daemon, ejecución automática al muestrear ni iniciador autostart.

Esta aprobación retira el bloqueo de decisión OpenRC132, pero NO implementa el ejecutor,
ni valida seguridad/aceptación real ni cierra K03. Todavía falta el diseño y validación
indicados debajo. Cada acción futura requerirá consentimiento explícito de ejecución;
no se activa por la aprobación del desarrollo ni por include_services/Monitor.
La restricción de recopilación sin aplicaciones externas sigue intacta.

Registro oficial en SEGUIMIENTO.txt148. L05/Linux permanece como unidad operativa
actual; al retomar K03 ya no debe pedirse de nuevo esta misma autorización.
Los párrafos132 siguientes son históricos, anteriores a esta respuesta, no una
revocación del permiso vigente.

## Auditoría y consulta originales (histórico132)

Revisión132, 2026-10-30. Auditoría de K03, sin código ejecutor nuevo ni pruebas
de servicios reales. Las últimas pruebas de implementación siguen siendo las
de revisión131: Linux158/158, 20 repeticiones OpenRC y consumidores SDK exit0.

## Evidencia revisada

El [manual rc-service](https://raw.githubusercontent.com/OpenRC/openrc/master/man/rc-service.8)
describe localización y ejecución del servicio. El [código rc-service.c](https://raw.githubusercontent.com/OpenRC/openrc/master/src/rc-service/rc-service.c)
resuelve el script, prepara entorno y termina en execv. El [runtime openrc-run](https://raw.githubusercontent.com/OpenRC/openrc/master/src/openrc-run/openrc-run.c)
y su [wrapper shell](https://raw.githubusercontent.com/OpenRC/openrc/master/sh/openrc-run.sh.in)
participan en la ejecución de scripts y sus hooks/dependencias; no son un simple
mensaje por servicio equivalente al FIFO runit que ya implementamos.

El [header público librc revisado](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/rc.h.in)
expone funciones de consulta, marcado y configuración, pero no encontramos allí
una función pública de start/stop/restart individual equivalente al ejecutor.
Marcar started/stopped no ejecuta ni detiene el servicio: no es un sustituto.
Esto describe las fuentes revisadas, no una garantía sobre toda versión/fork.

El [dispatcher openrc-init](https://raw.githubusercontent.com/OpenRC/openrc/master/src/openrc-init/openrc-init.c)
atiende órdenes globales como apagado/reinicio/single/reexec. No se utiliza ni
se abre init.ctl para intentar controlar un servicio individual. Tampoco se
deducen PIDs de nombres/marcas ni se envían señales a procesos no identificados.

## Elección solicitada al usuario

La restricción vigente prohíbe utilizar aplicaciones externas para recopilar
información. Para evitar reinterpretarla como autorización de un ejecutor de
scripts, se solicita una decisión explícita entre:

1. Permitir scripts/runtime OpenRC existentes **solo** para acciones de control
   start/stop/restart pedidas explícitamente. Nunca para recopilar datos.
   Desarrollo opt-in/desactivado por defecto, pruebas únicamente privadas, sin
   instalar dependencias, elevar privilegios o cambiar servicios reales.
2. Aplazar ese control y avanzar otros adaptadores de K03 (s6/SysV).

Histórico132, antes de la respuesta148: hasta recibir respuesta no se implementa ni prueba esa excepción. Otro
«prosigue» sin respuesta no se interpreta como consentimiento para ejecutarla;
se puede continuar s6/SysV conservando el control OpenRC abierto. K03 permanece
E, no se cierra ni se cambia el denominador. No es un bloqueador de todo Linux.

Incluso si se aprueba, faltaría diseñar selección/validación del script y runtime,
entorno/FDs/credenciales, política de rutas/cambios de archivo, propiedad del hijo,
salida/errores/deadline y separación entre resultado del ejecutor y estado vivo.
No se añadirá un fallback shell genérico al lanzador ELF existente, ni se tratará
un índice de catálogo como autoridad para ejecutar. Las pruebas con scripts
privados no equivaldrían a aceptación OpenRC real. Los hooks pueden actuar sobre
dependencias; no se prometerá aislamiento del efecto a un solo proceso.

Esto es control de servicios solicitado desde el GUI, no un iniciador de apps
autostart. I07 sigue observador/causa UNKNOWN aplazada según decisión explícita.
Los lectores OpenRC130/131 y su contrato read-only no cambian.
