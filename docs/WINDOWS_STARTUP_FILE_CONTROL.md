# Control explicito de archivos Startup Windows

Revision44, 2026-10-19. Proveedor con retirada/restauracion **nativa positiva**
en archivos privados, fuera del Startup real. Independiente del proveedor Run/KTM,
que sigue rechazado con6801 en este host. M05 permanece abierta por aprobacion
efectiva, HKLM/common, integracion de respaldos y otras acciones pendientes.
El listado explicito de respaldos se completa en revision44; no depende de IDs
retenidos por el host ni se inserta silenciosamente en el inventario legacy.
Revision45 agrega [informe Monitor/Sampler opt-in](WINDOWS_STARTUP_FILE_MONITOR.md),
independiente del inventario y de las acciones. Se requiere recompilar consumidores
por campos nuevos en SystemSnapshot/SampleOptions0.x; ver el contrato enlazado.
Revision46 integra la [accion explicita en SystemActions/AuditedActions](WINDOWS_STARTUP_FILE_AUDIT.md).
La funcion libre no genera eventos por si sola. Nuevo virtual0.x: recompilar
biblioteca, consumidores y backends personalizados; no usar vtables anteriores.
Revision47 agrega [alcance common explicito](WINDOWS_COMMON_STARTUP_FILE_CONTROL.md):
lectura/accion por prefijo user/common y listado comun separado, con permisos
existentes y aviso TODOS LOS USUARIOS. No es HKLM ni aprobacion de Windows.

## Uso y significado

API adicional en `pcinsight/windows_startup_file_control.hpp`:

```cpp
auto observed = pcinsight::read_windows_startup_file_registration(entry.id);
// Mostrar estado, limites y pedir consentimiento explicito al usuario.
// Si esta registered, retirar ese archivo de las entradas directas:
// auto result = pcinsight::set_windows_startup_file_registered(*observed.value, false);
// Reconsultar por el MISMO ID conservado antes de ofrecer restauracion (true).
```

IDs `winfolder:user:`/`winfolder:common:` del inventario, en la carpeta conocida
correspondiente. Acepta rutas de unidad local normales/extendidas; rechaza UNC, unidades
de red mapeadas, otras cuentas, nombres no directos, streams, metadatos
desktop.ini y el nombre reservado del almacen. No permite crear un comando nuevo.
No modifica el registro, StartupApproved, asociaciones, permisos, procesos ni
el destino de un acceso directo. No ejecuta ni resuelve archivos. No se conecta
automaticamente al boton generico enable/disable; la auditoria es opt-in via
AuditedActions desde revision46. Monitor/Sampler
solo consultan respaldos si el host activa la opcion explicita de revision45.
El GUI debe rotular «Retirar/restaurar archivo Startup mediante PCInsight».
No presentarlo como aprobacion efectiva de Windows ni como bloqueo de ejecucion.

Estados: absent, registered (solo archivo directo), retained_by_pcinsight (solo
archivo en respaldo), conflict (ambos). Conflicto/ausencia no contienen identidad
de archivo seleccionada. La presencia en el respaldo NO autentica quien lo creo.
El estado se lee sin crear directorios, con Support::estimated: no es snapshot
atomico ni prueba de elegibilidad, confianza o lanzamiento. Ausencia en esta
fuente no significa que el programa no pueda iniciarse por otro mecanismo.

false mueve el archivo a `Startup/PCInsight.Disabled.v1/<mismo nombre>`; true lo
restaura al nivel directo de Startup. Se conserva el archivo, no se copia su
contenido ni se eliminan objetos. La subcarpeta queda incluso vacia. Se crea
solo si hace falta despues de validar la observacion; si el rename falla puede
quedar vacia. No existe limpieza automatica ni rollback destructivo. Los archivos
retirados no aparecen en el inventario directo habitual. El host puede conservar
el ID para reconsulta/restauracion o descubrirlos con el nuevo listado siguiente.
No hay persistencia automatica de IDs en el GUI. No borrar/mover manualmente esa subcarpeta si se
quiere conservar la recuperacion mediante esta API.

## Descubrimiento despues de reiniciar el consumidor

```cpp
auto backups = pcinsight::list_windows_startup_file_backups();
if (backups.value) {
    for (const auto& item : *backups.value) {
        const auto& row = item.registration;
        // Mostrar item.name (UTF-8 exacto), entry_id y estado; no restaurar automaticamente.
        // retained_by_pcinsight permite ofrecer restauracion tras consentimiento.
        // conflict exige resolver el conflicto fuera de una sobrescritura implicita.
        // auto result = pcinsight::set_windows_startup_file_registered(row, true);
    }
}
```

Solo enumera archivos regulares directos en PCInsight.Disabled.v1, no entradas
activas sin respaldo ni otros mecanismos de inicio. Emite el ID original de la
entrada directa (no la ruta de respaldo) y estados retained_by_pcinsight/conflict.
Un archivo de0bytes tiene tamano conocido0. Conflictos no seleccionan identidad
de archivo ni autorizan restauracion. La lista no autentica autores del respaldo.
El host puede reconsultar un ID antes de ofrecer una accion; esta ultima vuelve
a comprobar la identidad, por lo que una fila vieja no autoriza un archivo nuevo.

GetFileInformationByHandleEx enumera por handle del respaldo fijado, con
FileIdBothDirectoryRestartInfo/FileIdBothDirectoryInfo. No utiliza FindFirstFile
por una ruta que pudiera apuntar a otra carpeta. Dos enumeraciones deben coincidir
en nombres e identidades/tamanos; se abren archivos relativos al mismo handle para
comparar la identidad enumerada y detectar conflictos con archivos directos, y se
reobserva su presencia. Se revalida la raiz Known Folder al terminar. Cambios
detectados dejan Metric sin valor, no una lista truncada o vacia inventada.
Dos observaciones iguales no prueban snapshot atomico, ausencia de ABA ni que el
estado no cambie despues del retorno; siempre Support::estimated.

Sin respaldo existente: lista vacia conocida tras comprobacion, sin crearlo.
Raiz inexistente/denegada, tipo de respaldo incompatible o fallo de enumeracion:
sin valor, con nota y permission_required si corresponde. Se ignoran '.', '..',
desktop.ini, directorios, devices y entradas reparse; no se recorre ni sigue su
contenido. Lista vacia significa ausencia de archivos regulares elegibles en este
almacen, no ausencia de todas las aplicaciones de inicio o de objetos ignorados.

Acotacion: buffer64KiB,10000 registros por pasada incluidos ignorados,10001 llamadas
maximas por pasada y16MiB de texto (IDs+nombres) de salida. Nombres hasta255 unidades UTF-16,
conversion estricta; offsets y longitudes de records comprobados antes de leer,
duplicados exactos rechazados. Los limites no son truncamiento: toda la consulta
falla sin resultado parcial. El orden es determinista por nombre UTF-16 exacto,
no localizacion ni identidad comercial. No instala dependencias, escribe archivos,
inicia transacciones ni ejecuta programas.

## Comprobacion y operacion

La observacion retiene volumen/indice/creacion de la carpeta y volumen/indice/
creacion/ultima escritura/tamano del archivo. En la accion se abren de nuevo y
se comparan, con estado coherente para la direccion solicitada. Timestamps/tamano
de la carpeta no identifican el objeto: cambian al mover archivos. Son tokens
de filesystem, no hashes, SID, autorizacion, firmas ni identidades eternas; no
se acepta un indice de objeto cero como identidad comprobable. No se fuerza
OBJ_CASE_INSENSITIVE al abrir entradas en directorios sensibles a mayusculas.
Los tokens no
protegen contra recreacion deliberada que reproduzca todos los campos. Usar en
el mismo contexto de usuario/volumen, reconsultar tras cambios y no reutilizar
observaciones antiguas ni transferirlas a otra cuenta.

Se fija la carpeta conocida mediante handle; entradas y subcarpeta se abren con
NtCreateFile relativo a ese handle. Los objetos finales reparse/device y tipos
incompatibles se rechazan. No se sigue un junction de respaldo ni un enlace final
de archivo. Componentes anteriores a la raiz conocida pueden estar redirigidos;
no se afirma que toda la ruta este libre de junctions ni que sea un limite de
seguridad frente a un atacante con permisos equivalentes. La accion exige la
identidad observada de la raiz. Durante la operacion no se comparte borrado de
directorios; el archivo seleccionado tampoco comparte escritura/borrado.

El commit es **un rename sin reemplazo del handle de archivo**, usando
NtSetInformationFile/FileRenameInformation y RootDirectory fijado al directorio
destino. No se vuelve a resolver una ruta absoluta al confirmar. Un destino
existente se rechaza, no se sobrescribe aunque los datos coincidan. No hay copia
entre volumenes, segundo intento oculto, transaccion KTM, borrado ni fallback a
MoveFile por ruta. Se conservan bytes/atributos por ser el mismo archivo; no se
promete flush durable ante perdida de energia, exclusion de todas las escrituras
por mappings previos ni proteccion ante modificaciones posteriores al retorno.

Usa funciones del ntdll existente cargadas por GetProcAddress, Win32 y Known
Folders del sistema. No instala herramientas, lanza comandos, carga un driver,
solicita UAC ni ignora ACL. Funciones ausentes o filesystem que no permita la
operacion producen fallo; no se fuerza disponibilidad. Errores conservan codigo
Win32 traducido de NTSTATUS; permisos se distinguen de no encontrado/conflicto.
Tras cualquier error reconsultar, no asumir que una operacion de filesystem o
el estado externo siguen inalterados. El host es responsable de consentimiento
y auditoria: la funcion libre no registra por si sola una accion sensible.

## Verificacion

`tests/windows_startup_file_control.cpp`: redireccion interna unicamente de la
raiz Known Folder a una carpeta temporal unica; operaciones de archivos/rename
son **reales**, no un modelo inyectado. No escribe en el inicio automatico real.
Retirada/restauracion positiva, bytes e identidad preservados, consulta fresca
de respaldo, conflictos sin sobrescribir, tokens de archivo/carpeta incorrectos,
archivo/respaldo cambiado, sharing incompatible, readonly conservado, backup
reemplazado por archivo, entrada convertida en directorio, ausencia, Unicode,
IDs/rutas/tamanos invalidos y prefijo extendido. Pruebas de ACL arbitrarias,
junction nativo, FAT/exFAT/ReFS, MSVC/ARM64/x86 y equipos distintos quedan sin
validacion nativa en esta unidad. No se presume esa cobertura por el chequeo de
atributos del codigo. Sin garantia de timeout duro de las llamadas nativas.

La primera expectativa de test «readonly impide rename» era incorrecta en este
host. Se comprobo que la operacion conserva readonly al retirar/restaurar;
no se cambio la biblioteca para fingir un error de permisos. Las dos carpetas
de esas pruebas abortadas se inspeccionaron y limpiaron por sus nombres exactos,
sin borrado recursivo; las ejecuciones finales limpian su propia fixture.
Avisos iniciales de casts de GetProcAddress/indentacion corregidos; log inicial
conservado. Logs finales `work/startup-file-control-*-windows.log`, regresiones
Linux y consumidores en `work/startup-file-control-*-linux.log`.

Resultado final Release: Windows123/123 y Linux108/108,20repeticiones de la
prueba nueva Windows, ambos consumidores instalados recompilados/relinkados y
ejecutados exit0. Builds finales sin warnings. Linux verifica el stub no soportado
y regresiones propias, no el proveedor NT. Nueva API adicional, sin modificar
layouts existentes/ABI virtual/serializacion de historial. No equivale a CI remota.

Revision44: pruebas del listado agregadas a windows-startup-file-control. Positivos
nativos de descubrimiento sin IDs guardados, restauracion desde fila descubierta,
conflictos, varios archivos/Unicode, tamano0, orden y exclusiones de metadata/
subcarpeta. Consulta sin respaldo no lo crea. API de enumeracion inyectada solo
para fallos: permisos inicial/tardio, longitud impar, offset fuera de buffer,
UTF-16 invalido, exito sin record valido, limite10000, API nula; todos sin lista
parcial. Se modifica un respaldo privado entre pasadas para verificar rechazo,
y se simula cambio de contexto Known Folder. No se generaliza esa inyeccion a
validacion fisica de otros filesystems. Logs work/startup-backup-listing-*.log.

Fuentes del contrato nativo:
[NtCreateFile y RootDirectory](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntcreatefile),
[FILE_RENAME_INFORMATION](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information).

Listado: [GetFileInformationByHandleEx](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex),
[FILE_ID_BOTH_DIR_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_both_dir_info).

## Actualizacion de compatibilidad MSVC (revision51)

MSVC19.51/SDK26100 x64 ahora compila y verifica el mismo proveedor con una
declaracion ABI interna documentada de NtSetInformationFile/RenameInformation,
sin requerir WDK ni cambiar a operaciones por ruta. Retiro/restauracion en
fixture privada positivos,20repeticiones de control (80totales junto con otros
tests focalizados) y suite127/127; no Startup
real modificado. Ver [evidencia y limites](MSVC_VALIDATION.md). La mencion
historica MSVC pendiente mas arriba no representa esta validacion posterior;
ARM64/x86, otros filesystems y hardware conservan sus limites.

