# Servicios Windows: inventario y control explícito

Implementado 2026-10-13. Se activa con `SampleOptions::include_services=true`;
por defecto permanece desactivado para evitar enumeraciones innecesarias.

Se consulta el Service Control Manager local mediante APIs Win32. No se ejecutan
`sc.exe`, PowerShell ni otras utilidades. Solo se solicitan permisos de conexión,
enumeración y lectura de configuración; este proveedor no inicia, detiene,
reinicia, elimina ni modifica servicios.

## Campos y alcance

- `id`: nombre interno del servicio; `name`: nombre mostrado por Windows.
- `status`: running, stopped, paused, start_pending, stop_pending,
  continue_pending, pause_pending o unknown.
- `pid`: solo cuando Windows proporciona un PID no nulo en un estado donde
  es válido. No se publica un PID para servicios detenidos o iniciándose/deteniéndose.
- `description`: descripción de configuración cuando es accesible. Referencias
  indirectas a recursos de idioma se conservan tal como las devuelve Windows;
  no se carga una DLL para resolverlas.
- `group`: grupo de **orden de carga** (`lpLoadOrderGroup`), no el grupo de
  alojamiento de svchost. No se infiere el grupo a partir de la línea de comandos.

Incluye servicios Win32 propios y compartidos, activos e inactivos. No enumera
drivers del kernel. Windows puede omitir servicios si el usuario carece de
permiso para consultar su estado; no se promete visibilidad administrativa.
Un servicio eliminado o con configuración inaccesible conserva el inventario
básico, con descripción/grupo vacíos. El inventario legacy no distingue una
cadena vacía de una cadena que no pudo leerse por permisos. La nueva consulta
[open_windows_service](WINDOWS_SERVICE_VIEW.md) sí proporciona disponibilidad
por campo sin modificar el layout legacy.

## Resiliencia

Enumeración paginada con buffers de 256 KiB, identificadores deduplicados,
comprobación de límites y avance del cursor. Un fallo global lanza
`std::system_error` sin presentar una lista parcial como exitosa. Máximo
100000 registros y 1024 páginas. Configuración/descripcion usa buffers de
8 KiB; si no cabe o falla, queda sin dato. Los handles se cierran en todas
las rutas, incluidas excepciones. No hay caché: cada muestra vuelve a consultar
el estado y la configuración. Los cambios concurrentes no forman un snapshot
atómico, y los timeouts de llamadas RPC dependen de Windows.

## Acciones start/stop/restart

`make_system_actions()->control_service(service.id, ServiceAction::restart)`
usa el nombre interno UTF-8, no el nombre mostrado. Las acciones son explícitas,
separadas del muestreo. No ejecutan comandos ni elevan privilegios: utilizan los
derechos del proceso llamador. SCM solicita CONNECT; el servicio solicita
QUERY_STATUS y START/STOP según la operación. El GUI debe autorizar cada acción
y ejecutarla fuera del hilo de interfaz; el helper UAC sigue pendiente.

Start/stop son idempotentes si ya se observa el estado objetivo. Restart sobre
un servicio detenido lo inicia. Se espera a que terminen las transiciones antes
de actuar y se verifica running/stopped después; aceptar la petición no equivale
a completarla. Start no reanuda un servicio pausado. Restart puede detenerlo e
iniciarlo. No se detienen dependientes automáticamente ni se termina el proceso
anfitrión. Un fallo al iniciar después de detener informa «reinicio parcial»;
no hay rollback y un timeout no cancela una petición ya aceptada por Windows.

El sondeo tiene un presupuesto compartido de 30 segundos por operación, con
intervalos de 100 ms; las llamadas RPC nativas pueden exceder ese tiempo.
El éxito solo confirma el estado observado, que puede cambiar inmediatamente.
Permisos y servicio inexistente se distinguen en ActionError; otros fallos
incluyen el código Win32 en message. Entradas vacías, NUL, barras, UTF-8 inválido,
nombres mayores de 256 caracteres UTF-16 y enums inválidos se rechazan antes
de abrir SCM. No se cambia el tipo de inicio de un servicio deshabilitado.

## Pruebas y pendientes

Fixtures inyectados cubren dos páginas, permisos de SCM y de configuración,
fallo después de la primera página, conteos/punteros inválidos, cursor detenido,
estados/PID y liberación de handles. La prueba de integración es solo lectura:
comprobó 323 servicios en la máquina de desarrollo. Windows Release 9/9;
Linux WSL 5/5. Ningún servicio fue creado ni alterado por las pruebas.

Las acciones se prueban con APIs inyectadas: orden stop/start,
permisos mínimos, fallos, handles cerrados, entrada inválida, pausas, transiciones,
idempotencia, dependientes activos, inicio fallido y presupuesto compartido.
Además, el2026-10-19 se ejecutó el harness nativo manual descrito abajo,
con autorización explícita y confirmación UAC. No se tocaron servicios existentes.

Revision37 completa la consulta dedicada de detalles y disponibilidad tipada
de descripción/grupo: [WINDOWS_SERVICE_VIEW.md](WINDOWS_SERVICE_VIEW.md).
Junto con las acciones nativas sobre proceso hijo privado cierra M07 en su
alcance documentado. El helper UAC productivo permanece L05; no se anuncian
resolución de descripciones indirectas, grupo de alojamiento svchost, modo
de eficiencia combinado ni cobertura de más versiones/equipos.

## Validación nativa manual y acotada (2026-10-19)

El target `pcinsight-service-native-test` NO está registrado con CTest. Sin
`--run-private-service-test` devuelve2 sin mutaciones; sin privilegios devuelve1
y registra Win32=5 antes de crear nada. No se eleva por sí mismo. Para ejecutarlo,
el operador debe autorizar la prueba y elevar explícitamente el proceso.

Usa un nombre único `PCInsight_Private_SCM_<pid>_<tick>`, CreateService exclusivo,
inicio manual, proceso propio, cuenta LocalService y sin dependencias. No cambia
servicios existentes ni políticas. Copia el ejecutable estático MinGW a un
directorio exclusivo de ProgramData con DACL protegida: SYSTEM/Administrators
control total, LocalService lectura/ejecución. No abre permisos del perfil del
usuario ni ejecuta el servicio como LocalSystem. Fija ambos ejecutables contra
reemplazo ordinario durante la prueba. Este harness no es un helper de producción
ni una frontera de seguridad frente a otros administradores.

El servicio privado solo acepta stop/interrogate, no abre red y se apaga solo
en60s si pierde al controlador. El controlador prueba la API pública de acciones:
stop detenido, start, start idempotente conservando PID, restart corriendo,
stop, restart detenido y stop final. En cada fase verifica SCM y el inventario
real de la librería: nombre interno, estado, PID válido/ausente, descripción y
grupo de carga vacío. No exige un PID distinto tras restart: Windows puede
reutilizarlo. Fixtures conservan pausas, dependientes y fallos; no se provocan
esos escenarios en servicios reales de terceros.

La limpieza usa únicamente el handle devuelto por SU CreateService exitoso:
espera stop, DeleteService, cierra handle y verifica ERROR_SERVICE_DOES_NOT_EXIST;
borra su copia y su directorio no recursivamente. Cualquier limpieza incompleta
deja RESULT FAIL con diagnóstico. Un apagón/terminación forzada del controlador
puede dejar registro/archivos para limpieza administrativa; el watchdog solo
limita el proceso vivo, no promete limpieza ante un crash del controlador.

Resultado real: servicio `PCInsight_Private_SCM_18672_36777171`, todas las fases
PASS, registro/copia/directorio eliminados, RESULT PASS. Reconsulta independiente
confirmó registro/directorio ausentes. Evidencia:
`work/build-release/pcinsight-service-native-test.exe.result_18672_36777171.txt`.
Primer intento elevado desde perfil de usuario falló StartService Win32=5;
registro eliminado, fallo conservado en result_2664_36728984.txt. La copia con
ACL acotada resolvió el arranque sin modificar ACLs existentes.

Regresión: Windows119/119 Release y Linux108/108. Solo cambia el harness/CMake/
documentación; no cambia código productivo, API/ABI/layout ni historial, por lo
que esa unidad no requería migrar consumidores. Revision36 no cerraba M07 entero, L05,
CI remota ni cobertura de otras versiones/VMs/hardware.

Fuentes: [enumeración SCM](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-enumservicesstatusexw),
[configuración](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfigw)
y [descripción](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfig2w).
Control: [StartServiceW](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-startservicew),
[detener servicios](https://learn.microsoft.com/en-us/windows/win32/services/stopping-a-service),
[consultar estado](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryservicestatusex).
