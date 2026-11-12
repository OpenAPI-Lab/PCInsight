# Run new task Linux: lanzamiento y errores reales

Unidad funcional: `SystemActions::launch_process` valida el request antes de
crear un hijo y recibe errores de chdir/execve por un pipe O_CLOEXEC. La versión
anterior devolvía éxito inmediatamente después de fork, aunque execvp/chdir
fallaran; además asignaba memoria C++ en el hijo y execvp podía invocar un shell
implícito para texto ejecutable sin formato reconocido.

## Contrato

- Executable no vacío; rutas y argumentos son bytes Linux, no se exige UTF-8.
  Se rechazan NUL embebidos en executable, arguments y working_directory.
- Rutas que contienen `/` se intentan directamente. Nombres simples se buscan
  por PATH capturado del caller. PATH vacío/componentes vacíos usan cwd del hijo.
  Si PATH está ausente se consulta `_CS_PATH` a la libc; no se incluye cwd por
  suposición. Rutas relativas se resuelven después del working_directory.
- `arguments` contiene argumentos literales posteriores a argv[0]. No se
  interpreta quoting, expansión, redirección ni sintaxis de shell. Se usa
  execve, nunca execvp ni system: ENOEXEC no dispara `/bin/sh`. Un script con
  shebang sí puede invocar su intérprete mediante el kernel, o el consumidor
  puede solicitar explícitamente un intérprete; esto no es una sandbox.
- Working_directory vacío hereda cwd. No se llama chdir en el padre. No requiere
  extensiones posix_spawn_file_actions de glibc ni aplicaciones externas.
- Se prepara argv, candidatos y copia del entorno antes de fork; el hijo solo
  utiliza close/chdir/execve/write/_exit, sin allocations/streams/locks C++.
- ENOENT/ENOTDIR -> not_found, EACCES/EPERM -> permission_denied,
  E2BIG/EINVAL/ENAMETOOLONG/ENOEXEC -> invalid_argument; otros -> system_error.
  En búsqueda PATH, EACCES no impide intentar una entrada posterior; se conserva
  permission_denied si ninguna funciona y alguna fue inaccesible.
- Elevated=true se rechaza si geteuid no es 0. Si ya es 0 usa sus credenciales
  actuales; no solicita sudo/pkexec, no modifica UID/GID y no eleva privilegios.

## PID y propiedad del hijo

`ActionResult::process_id` contiene el PID en un lanzamiento exitoso (Linux y
Windows). Las otras operaciones y rechazos previos usan cero. Linux devuelve
también PID si falla inesperadamente la lectura del canal: el resultado es
system_error, el estado del hijo es desconocido y el consumidor conserva su
responsabilidad. Los errores comunicados por el hijo se recolectan mediante
waitpid del PID exacto antes de retornar, nunca con waitpid(-1).

El consumidor debe recolectar los hijos exitosos con waitpid o su supervisor
existente. PCInsight no instala SIGCHLD, no crea un reaper oculto ni cambia
configuración global. La creación no prueba readiness: la aplicación puede
fallar inmediatamente, incluso con exit 127, y eso no convierte el lanzamiento
en un error retrospectivo. PID no protege contra reutilización futura.

EOF del pipe indica que no llegó un error pre-exec. Normalmente resulta del
cierre CLOEXEC, pero una señal fatal o fallo externo antes de exec también puede
cerrar el canal sin error. No se presenta como una garantía absoluta de exec
ni de inicialización. La llamada es síncrona y no tiene deadline global:
fork/exec sobre almacenamiento lento, handlers pthread_atfork del host y la
espera de un hijo fallido detenido externamente pueden demorarla. No cancelar
automáticamente un hijo ni cerrar recursos del caller como supuesto rollback.

## Límites e integración

Máximo 4096 argumentos y candidatos PATH; cada cadena hasta 131071 bytes,
request hasta 1 MiB, candidatos hasta 1 MiB, entorno hasta 2 MiB/32768 entradas.
El kernel puede imponer un ARG_MAX menor o requisitos de formato/permisos.
PATH duplicado o entorno excesivo se rechazan antes de fork. Linux requiere
pipe2/O_CLOEXEC (kernel 2.6.27+); fallo del API se propaga, sin fallback inseguro.

El host debe coordinar setenv/unsetenv, chdir, SIGCHLD/recolección y cierre de
descriptores si otros threads los modifican. Se heredan credenciales, máscaras
de señales, grupo de procesos, stdio y descriptores sin FD_CLOEXEC; el consumidor
debe configurar sus recursos con CLOEXEC cuando no deban filtrarse. No se
ofrece terminal independiente, aislamiento, entorno personalizado ni helper UAC.
Struct pública 0.x ampliada: reconstruir librería y consumidores compatibles.

## Verificación

2026-10-14: Linux Release 16/16 y Windows Release 27/27. Paquetes instalados y
consumidores externos reconstruidos/ejecutados en ambos sistemas. El consumidor
comprueba la API nueva con request inválido, sin iniciar ninguna aplicación.

Prueba Linux con el propio ejecutable C++ de tests, nunca una aplicación del
usuario: argumentos vacíos/espacios/quotes/newlines/Unicode y sintaxis de shell
literal, cwd privado sin cambiar padre, rutas relativas/PATH, PATH vacío/ausente,
permisos, formato inválido sin shell, NUL/límites, 32 errores sin zombies,
exit 7 luego de creación correcta y cuatro llamadas concurrentes. El decorador
AuditedActions conserva PID y emite ambos eventos. Archivos privados /tmp se
eliminan al concluir. La prueba Windows de CreateProcess es inyectada y verifica
PID estructurado; no inicia ninguna aplicación Windows.

Referencias: [fork y restricciones multithread](https://man7.org/linux/man-pages/man2/fork.2.html),
[execve](https://man7.org/linux/man-pages/man2/execve.2.html),
[pipe2/CLOEXEC](https://man7.org/linux/man-pages/man2/pipe.2.html),
[execvp y fallback shell](https://man7.org/linux/man-pages/man3/exec.3.html).
