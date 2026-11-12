# End task Linux: identidad, pidfd y confirmación de salida

Unidad funcional: `terminate_process_checked(pid, linux_start_ticks, force)`.
El monitor expone `ProcessInfo::linux_start_ticks` usando la misma lectura de
`/proc/PID/stat` que alimenta sus contadores, sin una lectura posterior que pueda
pertenecer a otro proceso. Es el campo 22, ticks de reloj desde boot: no segundos,
no nanosegundos ni FILETIME Windows. Conservar el entero sin conversiones.

```cpp
// p pertenece a una muestra reciente del mismo sistema/boot/PID namespace.
if (p.linux_start_ticks.value && *p.linux_start_ticks.value != 0) {
    auto result = actions->terminate_process_checked(
        p.pid, *p.linux_start_ticks.value, false); // SIGTERM, no kill automático
    // result.error == none implica salida confirmada, no solo señal aceptada.
}
```

## Orden verificado

1. Rechazar PID 0/1, propio PID, números mayores que INT_MAX y token cero.
2. Comprobar procfs nativo y que `/proc/self/stat` usa el PID del caller. Fallo
   de contexto/lectura no se convierte en un permiso para enviar señales.
3. Abrir pidfd con flags 0 (proceso/thread group leader, no grupo de procesos).
4. Leer identidad actual y comparar PID + start_ticks. Diferencia -> stale_target
   sin señal. Lectura denegada/inexistente/inválida -> error explícito.
5. Poll de pidfd sin espera: proceso ya salido -> not_found, sin señal.
6. pidfd_send_signal SIGTERM o SIGKILL, nunca kill(PID) después del chequeo.
7. Poll del mismo pidfd durante hasta 5s para confirmar salida. Si vence o falla,
   el resultado indica señal aceptada y salida desconocida/no confirmada. No
   se cambia a SIGKILL automáticamente ni se promete rollback.

El descriptor se cierra en todos los caminos posteriores a su apertura, incluso
si una excepción interrumpe la consulta. No recolecciona el proceso con waitpid:
su padre o supervisor conserva esa responsabilidad. Confirmación de salida
no determina código de salida, éxito funcional ni limpieza de recursos por la app.

## Límites de identidad y compatibilidad

pidfd mantiene fijo el destinatario **desde su apertura** y evita que un PID
reutilizado después redirija la señal a otro proceso. start_ticks detecta una
identidad distinta entre muestra y apertura, pero tiene resolución de ticks:
PID reciclado con idéntico start_ticks es indistinguible con este token. No es
un identificador global, no sobrevive reinicios, ni es válido en otro host o
PID namespace. Para garantías más fuertes desde el instante de la muestra se
necesitaría retener una referencia pidfd en esa muestra; esta API no lo ofrece.
Tampoco fija el executable frente a exec del mismo proceso.
Un start_ticks real de cero puede aparecer en arranque muy temprano; el monitor
lo conserva, pero el checked action lo rechaza como token no utilizable.

Se requiere kernel Linux 5.3+ para pidfd_open (pidfd_send_signal existe desde 5.1)
y permisos/seccomp apropiados. Se invocan syscalls nativos con constantes de los
headers, sin requerir wrapper nuevo de libc ni CLI. Headers/kernel sin soporte
devuelven not_supported; seccomp puede devolver permission_denied u otro errno.
Nunca se degrada silenciosamente a señal por PID. Sin root/helpers/elevación:
se aplican las reglas nativas de permisos para la señal.

La variante PID-only `terminate_process` también usa pidfd y confirma salida,
pero no protege el intervalo anterior a la apertura con un token de la muestra.
Antes usaba kill con uint32 convertido a pid_t: números fuera de rango podían
volverse negativos y significar grupos de procesos. Ahora se rechazan.
Esta mejora requiere pidfd también en esa variante; kernels viejos no conservan
el fallback inseguro anterior. No hay process-tree kill ni exclusión universal
de procesos críticos, salvo self y PID 0/1: el host debe aplicar su política.

La lectura stat se limita a 64 KiB. El presupuesto de 5s corresponde al poll de
confirmación, no a un deadline total para apertura/lectura/syscall; kernel o
filesystem lento pueden demorar otros pasos. Unmount/cambios adversariales del
procfs, modificación de descriptores por otros threads o reutilizar muestras
de distintos contextos quedan fuera del contrato: el consumidor debe confiar
en su procfs y coordinar esas operaciones.

## Auditoría y ABI

AuditedActions registra token Linux en `expected_linux_start_ticks` para esta
acción; el campo Windows queda cero en Linux. El token de efficiency_checked
sigue siendo Windows porque ese backend Linux aún no está implementado.
ProcessInfo/AuditEvent son structs públicas 0.x ampliadas: reconstruir librería
y consumidores juntos; no se promete ABI binaria estable.

## Verificación

2026-10-14: Linux Release 17/17, Windows Release 28/28. Paquetes instalados y
consumidores externos reconstruidos/ejecutados en ambos; consumidor Linux
comprueba el nuevo token del monitor y Windows su ausencia.

Lógica inyectada: identidad cambiada/PID distinto, salida antes de enviar,
permisos/ENOSYS/ESRCH, lectura inválida, API incompleta, timeout y fallo después
de aceptar señal; sin envío indebido, cierre de fd y sin fallback.
Linux nativo: cuatro hijos temporales creados por el test, readiness propia,
token obtenido mediante Monitor, mismatch deja hijo vivo, SIGTERM/SIGKILL
correctos, SIGTERM ignorado produce timeout y el hijo sigue vivo, luego SIGKILL
explícito. Se recolectan todos los hijos; no se toca un proceso del usuario.
Auditoría real contiene el token Linux. En Windows solo corre lógica inyectada:
no se presume cobertura nativa Linux por ese build.

Referencias: [pidfd_open](https://man7.org/linux/man-pages/man2/pidfd_open.2.html),
[pidfd_send_signal](https://man7.org/linux/man-pages/man2/pidfd_send_signal.2.html),
[proc_pid_stat](https://man7.org/linux/man-pages/man5/proc_pid_stat.5.html).
