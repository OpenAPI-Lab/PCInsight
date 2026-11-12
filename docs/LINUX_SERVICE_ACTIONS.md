# Control confirmado de servicios systemd

Actualización K04: [LINUX_USER_SERVICES.md](LINUX_USER_SERVICES.md) añade
control explícito del usuario efectivo con owner único verificado; reutiliza el
corredor de jobs de esta página. La API genérica conserva ámbito de sistema.
Las exclusiones del user manager al final describen la evidencia histórica K02.

Unidad funcional terminada y verificada el 2026-10-14: start, stop y restart
mediante `SystemActions::control_service`, esperando el resultado del trabajo
systemd. Complementa el inventario real de [LINUX_SYSTEMD.md](LINUX_SYSTEMD.md).

La implementación anterior devolvía éxito al aceptar StartUnit/StopUnit/RestartUnit.
Ahora la API registra JobRemoved antes de enviar la petición, llama Subscribe y
espera la finalización del **job path devuelto para esa petición**. Un resultado
que llega durante la llamada síncrona se conserva hasta conocer ese job path.
No se confunden jobs de otra operación, ni se exige que el nombre reportado sea
igual a un alias usado para iniciar el servicio.

`done` significa éxito del trabajo. Failed, canceled, timeout, dependency,
skipped y estados desconocidos/ausentes no son éxito. Un servicio oneshot puede
terminar correctamente sin permanecer running; un servicio Type=simple puede
tener job done antes de estar listo para atender solicitudes. El resultado no
promete preparación de la aplicación ni que el estado sea permanente.

## Entrada, permisos y seguridad

- Nombre canónico `.service` de hasta 255 bytes ASCII, incluidas instancias
  y escapes `\xHH`. Vacío, NUL, barras, control chars, escapes malformados y
  enum inválido se rechazan antes de abrir el bus. No acepta paths de archivos.
- Libsystemd opcional, cargada en runtime; no SDK necesario, no systemctl/shell.
  Si systemd no es el manager activo o falta API, not_supported.
- No cambia privilegios del llamador ni crea servicios para ejecutar una acción.
  La autorización depende de D-Bus/systemd y de sus políticas. AccessDenied e
  InteractiveAuthorizationRequired se reportan permission_denied.
- NoSuchUnit se reporta not_found; fallos de configuración y de ejecución no
  se convierten en «no existe» ni en éxito. Los mensajes contienen operación y
  código D-Bus o resultado del trabajo, no comandos construidos.
- Se usa modo systemd `replace`, como antes: puede sustituir un job previo de
  esa unidad. Dependencias y propagación siguen las políticas de systemd.
  Restart fallido puede haber detenido el servicio: no se promete rollback.

## Espera y recursos

Deadline de procesamiento de 30 segundos, timeout D-Bus de 5 segundos por
llamada y esperas de hasta 1 segundo por iteración. Apertura/autenticación del
bus depende de la implementación de libsystemd; no se garantiza un tiempo
global estricto de 30 segundos para toda la operación.

Si se pierde el bus, falta identidad del job, se alcanza límite o no llega
confirmación a tiempo, se devuelve error con **resultado desconocido**.
No se cancela el job aceptado: puede finalizar después. El GUI no debe reintentar
ciegamente ni concluir que no hubo efectos.

Bus, slot, mensajes, error y biblioteca usan ownership/RAII. La conexión dedicada
se cierra al terminar y systemd retira la suscripción al desconectar. Callback C
no deja escapar excepciones. Máximo 1024 resultados tempranos, campos de 1024
bytes y 100000 mensajes procesados. Señales inconsistentes no prueban éxito.

## Criterio de cierre y evidencia

Release Linux 12/12 y Windows 24/24. Fixtures de nombres/enums, resultados
done/error, correlación por job, señal temprana/tardía, aliases, límites y
señales contradictorias. En Windows solo se prueban esos componentes portables.
Paquetes instalados y consumidor CMake externo recompilado y ejecutado con éxito
en Linux y Windows después de los cambios finales.

Integración **nativa** en Ubuntu WSL mediante ejecución explícita del fixture
como root; no forma parte del CTest normal. Dos servicios transitorios de nombres
únicos PCInsight ejecutan solo el propio binario de pruebas, sin shell ni utilidades:

- Servicio de espera: start, stop, stop idempotente, start, restart y stop;
  inventario confirma running/inactive y PID nuevo después del restart.
- Oneshot que devuelve 7: job failed se reporta como fracaso real, no aceptación
  exitosa. El inventario confirma failed.
- Ambos quedan detenidos y retirados. El fixture resetea **solo su propio**
  estado failed antes de liberar referencias para permitir GC. Ese ResetFailedUnit
  de limpieza no se agregó a la API del producto ni borra diagnósticos del usuario.

Una primera prueba de limpieza mostró que systemd retenía el fixture fallido;
se corrigió la limpieza y se repitió el ciclo completo con éxito. La unidad
residual anterior ya estaba ausente al comprobarla. No se pidieron cambios a
servicios preexistentes ni se escribieron archivos de configuración permanentes.

Este cierre no implementa control OpenRC/runit/s6, user manager, helper polkit,
cancelación de jobs ni autorización de GUI. El inventario y control **systemd**
quedan implementados/verificados; la sección multiplataforma conserva pendientes.

Referencia primaria: [systemd D-Bus: jobs, Subscribe y JobRemoved](https://wiki.freedesktop.org/www/Software/systemd/dbus/),
[espera de jobs en systemd](https://github.com/systemd/systemd/blob/main/src/shared/bus-wait-for-jobs.c),
[declaraciones sd-bus](https://github.com/systemd/systemd/blob/main/src/systemd/sd-bus.h).
