# L03: alcance avanzado limitado aceptado

Actualización revisión41,2026-10-19: el usuario respondió «prosigue» a la
propuesta concreta de conservar únicamente una opción avanzada desactivada por
defecto, con reconocimiento del riesgo numérico, sin checked/EcoQoS, cobertura
de nuevos threads ni restauración garantizada. Aceptación solo L03, no licencia
para modificar procesos ajenos en tests ni instalar helpers/tracing/cgroups.
El análisis revisión18 de abajo se conserva como evidencia histórica.

Revisión18, 2026-10-17. Diagnóstico del diseño, no implementación ni nueva
validación nativa. Las pruebas85/85 Linux,105/105 Windows y20 repeticiones son
evidencia de revisión17, no se han repetido en esta revisión documental.

## Resultado

El preset por threads y el recibo de restauración ya existen y están probados
con los límites de [LINUX_EFFICIENCY.md](LINUX_EFFICIENCY.md). No se encontró en
las interfaces documentadas revisadas una sustitución directa que haga esos
writes de prioridad sobre la referencia pidfd en lugar del PID/TID numérico.
No se afirma imposibilidad de cualquier extensión futura del kernel.

`setpriority(PRIO_PROCESS, who, prio)` e `ioprio_set(which, who, ioprio)` reciben
identificadores numéricos. Obtener/verificar un pidfd en otra llamada no cambia
el destinatario del write posterior. Un chequeo posterior puede detectar un
cambio, pero no deshacer que se haya afectado una identidad incorrecta. Esta es
la razón de no promover la variante actual a checked.
Fuentes: [setpriority](https://man7.org/linux/man-pages/man2/setpriority.2.html),
[ioprio_set](https://www.man7.org/linux/man-pages/man2/ioprio_set.2.html),
[pidfd_open](https://man7.org/linux/man-pages/man2/pidfd_open.2.html).

Repetir la enumeración tampoco constituye una garantía de observar todos los
threads nacidos/terminados entre lecturas. La herencia normal de prioridades
puede ayudar, pero no sustituye un mecanismo de participación/control ni una
política de restauración para threads que no estaban en el recibo. No se inventa
para ellos un valor original. El actual recibo no debe ampliarse con ceros ni
con los valores ya modificados fingiendo que son los originales.

## Alternativas

| Alternativa | Qué permite | Límite / decisión necesaria |
| --- | --- | --- |
| Conservar el proveedor actual como opcional y limitado | Preset de threads capturados, comprobaciones observadas y restauración explícita del recibo | Aceptar expresamente este alcance para L03; checked continúa no disponible y el riesgo numérico se muestra antes del opt-in |
| Diseñar un proveedor cooperativo | Investigar que cada thread participante ejecute su propio cambio, sin seleccionar otro proceso por PID numérico | Requiere integrar código en la aplicación destino y definir protocolo/autorización, registro de threads y restauración; no cubre aplicaciones arbitrarias ni está implementado |
| Investigar control externo invasivo / grupos propios | Estudiar tracing de creación de threads o políticas sobre grupos administrados | No es sustitución transparente: permisos, pausas/efectos y semántica distintas; requiere aprobación de ese diseño y pruebas adicionales, no una garantía ya demostrada |

La recomendación para el pedido original («efficiency mode si está disponible»
y sin nuevas aplicaciones instaladas) es la primera alternativa, SOLO si el
usuario acepta el límite. El programa que consume la biblioteca no debería
anunciarla como Windows EcoQoS ni seleccionar unchecked automáticamente cuando
checked falla. Sin aceptación no se reduce el criterio ni se cierra L03.

Para la tercera alternativa, ptrace tiene operaciones de parada y seguimiento
de clone, comprobaciones de acceso y efectos sobre los tracees. No se ha
adjuntado un debugger ni se ha suspendido ningún proceso en esta revisión.
[ptrace](https://man7.org/linux/man-pages/man2/ptrace.2.html).
En cgroup v2, migrar un proceso existente escribiendo un PID tampoco transforma
esa selección en pidfd; `CLONE_INTO_CGROUP` permite creación con un FD de grupo,
pero cambia el escenario a tareas creadas/administradas y conserva restricciones
de acceso. No equivale a restaurar nice/ioprio de cualquier proceso existente.
[cgroup v2](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html),
[clone](https://man7.org/linux/man-pages/man2/clone.2.html).

## Estado histórico del control (revisión18)

L03 pasa E->B por decisión de arquitectura/alcance no disponible; se conserva
todo el código y la evidencia previa. No se incorpora helper, protocolo,
tracing, cgroup ni proveedor cooperativo sin la decisión del usuario. No hay
nuevos IDs, fracciones ni aceptación inferida de «prosigue». El conteo sigue
42/75=56,0%; C42/E17/P13/B3. La respuesta deberá registrarse antes de cambiar
el criterio o iniciar un diseño que requiera integración/control adicional.

## Implementación del alcance aceptado (revisión41)

No se ha arreglado la carrera PID/TID ni añadido un flujo checked. La salida
genérica SystemActions::set_efficiency_mode en Linux ahora devuelve not_supported
sin leer/cambiar el destino, incluso por AuditedActions. La variante checked
continúa not_supported y nunca cae a unchecked. ProcessInfo publica
efficiency_mode_available=false/available para ese camino genérico, no una
heurística propietario/root que invite al GUI a activarlo automáticamente.

La única API pública Linux que modifica el preset es el flujo avanzado
apply_linux_efficiency_unchecked/restore_linux_efficiency_unchecked. Conserva
acknowledge_pid_reuse_risk=false por defecto; ambos pasos requieren opt-in
explícito. El GUI debe explicar primero que puede afectarse una identidad
distinta si se reutiliza un PID/TID, que puede haber efectos parciales/CLONE_IO
compartido y que restaurar nice original puede requerir permisos. Un bool no
autentica consentimiento humano ni sustituye autorización por operación.

Cambio incompatible de COMPORTAMIENTO0.x: consumidores que antes usaban el
método genérico para cambiar prioridades ya no las cambian. No deben detectar
not_supported y reintentar automáticamente con ack=true. Deben migrar solo una
opción avanzada explícitamente aceptada al flujo con token/recibo, conservando
recibo incluso tras fallo. Firmas, vtables, layouts y formatos no cambian; no se
anuncia ABI1.0 estable. Windows EcoQoS sigue igual. El algoritmo leader legacy
se conserva interno/no instalado solo para regresión, no como bypass del GUI.

Tests nativos únicamente sobre hijos privados: genérico on/off y checked
rechazados sin cambiar nice/E/S, auditoría del rechazo, apply y restore sin ack
rechazados sin mutación, apply avanzado opt-in de2threads y restauración exacta
cuando posible, caso mixto con restauración CPU denegada/recibo pendiente.
Las pruebas confirman que el caller queda intacto y los hijos se recolectan.
Fixtures mantienen cobertura de errores/identidad observada/threads cambiantes/
recibos parciales/CLONE_IO/conflictos; NO prueban eliminación de la carrera.
L03 se cierra por la alternativa limitada aceptada, no como modo completo
checked/todos los threads. L05/autorización,N02/hardware,N03/riesgos históricos
siguen separados. No nuevos IDs ni crédito fraccionario.
Verificación final: Linux108/108,Windows122/122 Release;20repeticiones de
linux-efficiency y consumidores instalados ambos recompilados/relinkados/
ejecutados exit0. Builds finales sin warnings; el aviso inicial uid_known
sin uso tras retirar la heurística se corrigió y el log inicial se conserva.
Evidencia en work/efficiency-optin-*.log. Son pruebas del alcance limitado,
no de ausencia de carreras PID/TID ni cobertura de threads futuros.
