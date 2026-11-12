# Captura final de hijos no recolectados

read_startup_child_exit_sample(pid,expected_linux_start_ticks) permite recuperar
una muestra final después de que un hijo del caller termine entre dos polls.
No ejecuta utilidades, no instala componentes ni altera SIGCHLD. Requiere Linux
5.4+ (pidfd_open y waitid P_PIDFD), procfs genuino en la misma PID namespace y
un hijo que el consumidor todavía pueda esperar. Windows devuelve not_supported.

El resultado bool significa operación válida, no salida: si exited=false, el
hijo sigue sin una salida observable y sample/exit_code/termination_signal están
vacíos. Si exited=true y la operación es correcta, sample contiene los contadores
finales y una de exit_code/termination_signal describe la terminación. Errores
no ofrecen un sample; exited puede conservar true si la salida fue observada
antes de un fallo posterior. No hay zero ficticio cuando una métrica no existe.

El proveedor abre pidfd, valida PID/start_ticks/context mediante el lector de
contadores, consulta WEXITED|WNOHANG|WNOWAIT, obtiene la muestra final y vuelve a
comprobar la salida contra el mismo pidfd. No espera por PID desnudo ni consume
la salida. Un PID/token incorrecto se rechaza. Un proceso ajeno, kernel sin API
o un hijo ya recolectado no se convierte en éxito ni usa fallback inseguro.

El caller debe coordinar waitpid, cualquier supervisor y handlers SIGCHLD: otra
hebra puede recolectar el hijo durante las comprobaciones, en cuyo caso se
rechaza la muestra cuando se detecta. WNOWAIT no bloquea otros reapers ni crea
una propiedad exclusiva. SIG_IGN/SA_NOCLDWAIT pueden impedir conservar el zombie.
Después de tomar la muestra, el caller sigue obligado a recolectar su hijo;
PCInsight no deja zombies automáticamente ni asume esa responsabilidad.

CPU/E/S son los contadores procfs ya documentados en STARTUP_IMPACT.md: CPU
utime+stime cuantizado, disco read_bytes+write_bytes si accesible. No se mezclan
con rusage de wait4, no se convierten bloques de I/O en bytes ni se suman tiempos
de descendientes. Permisos/contadores ausentes permanecen desconocidos.

## Incorporación al observador

```cpp
// Requiere baseline previa ya asociada explícitamente al observer.
auto result = pcinsight::read_startup_child_exit_sample(pid, start_ticks);
if (result && result.exited && result.sample)
    observer.finish_process(*result.sample);
// El supervisor del caller hace waitpid(pid, ...) en su propio flujo.
```

finish_process valida/incorpora la última muestra y cierra SOLO esa identidad;
las otras siguen abiertas. Una muestra inválida no se aplica ni cierra el
registro. No hace I/O, comprueba salida ni autentica el origen de la muestra.
Una identidad cerrada no admite otro final ni se contabiliza dos veces.
Cobertura corta/gapped sigue sin certificar low/medium; high observado puede
conservarse como límite inferior. Luego puede usarse el overlay explícito.

Esto resuelve el hueco entre baseline y salida para hijos todavía no recolectados,
no la captura universal de procesos breves: si no hubo baseline, si otro programa
era su padre o si ya desapareció, no reconstruye CPU/E/S perdida. Tampoco prueba
launch automático ni observa descendientes. La integración del supervisor debe
tomar el final ANTES de reaping/cierre global del observer. No crea un launcher
de autostart ni activa ningún archivo de startup del usuario.

## Pruebas

Fixture C++ propia Linux: hijo bloqueado, baseline, CPU de 50ms, salida 7 antes
del segundo muestreo, captura final/repetida, finish de una identidad y waitpid
posterior con el mismo exit code. También salida por SIGTERM, no captura tras
reaping, rechazo de token incorrecto/no-hijo y entrada inválida. Sin apps reales.
Tests portables de cierre individual/otras filas abiertas/invalid chronology,
no double count/cierre global y contrato unsupported Windows. Consumidor instalado
usa ambas APIs con entradas inválidas, sin crear procesos.

Verificado 2026-10-14: Linux32/32 y Windows43/43 Release; instalación,
reconstrucción y ejecución de consumidores externos en ambos sistemas.

Fuentes: [waitid, WNOWAIT y P_PIDFD](https://man7.org/linux/man-pages/man2/waitpid.2.html),
[pidfd_open y condiciones de retención del hijo](https://man7.org/linux/man-pages/man2/pidfd_open.2.html).
