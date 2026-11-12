# Accion explicita y auditoria de archivos Startup

Revision47 amplia lectura/accion a IDs common, sin nuevo virtual/operacion;
ver [impacto en todos los usuarios y permisos](WINDOWS_COMMON_STARTUP_FILE_CONTROL.md).
El host debe mostrar ese alcance antes del consentimiento. ID/actor no autorizan
a un helper elevado a publicar archivos comunes de terceros.

Revision46, 2026-10-19. SystemActions agrega
`set_windows_startup_file_registered(observed, registered)` y AuditedActions
envuelve ese metodo. Windows delega al [proveedor checked](WINDOWS_STARTUP_FILE_CONTROL.md)
de la funcion libre; Linux/otros y backends personalizados sin override declaran
not_supported. No se redirige set_startup_enabled generico ni la accion Run/KTM.
Construir cualquiera de estos objetos no cambia archivos ni registra solicitudes.

```cpp
pcinsight::AuditedActions actions(pcinsight::make_system_actions(),
    [](const pcinsight::AuditEvent& event) {
        // Copiar/guardar estructuradamente con las garantias que requiera el host.
    }, "GUI local");
auto backups = pcinsight::list_windows_startup_file_backups();
// Mostrar nombres/estados y obtener consentimiento explicito ANTES de actuar.
// Tras seleccionar una fila retained_by_pcinsight y reconsultarla si hace falta:
// auto result = actions.set_windows_startup_file_registered(row.registration, true);
```

El ejemplo de sink vacio NO es almacenamiento durable: el host debe sustituirlo
por un sink real y gestionar diagnostics(). La funcion libre sigue disponible
sin auditoria; el decorador no captura llamadas que lo evitan ni acciones externas.
No es autorizacion, identidad autenticada, helper UAC, consentimiento automatico,
logging global ni un registro inviolable. Tampoco cambia StartupApproved o la
elegibilidad efectiva de Windows.

## Evento y politicas

AuditOperation::windows_startup_file_registration se agrega al final del enum;
ordinales previos conservados. Requested/completed comparten ID local al wrapper.
enabled=false solicita retirar el archivo directo al respaldo; true restaurar.
Ese flag es la solicitud, no aprobacion Windows ni prueba de una escritura.
backend_invoked solo certifica llamada al backend, no rename realizado.

Target contiene solo entry_id, limitado a4096bytes con metadata_truncated; actor
tiene el limite existente. No se añaden identidad de archivo/carpeta, contenidos,
diagnosticos ni texto de excepciones al evento. **El ID contiene la ruta completa
codificada en hex, que puede revelar usuario/nombres sensibles: no es anonimo ni
redactado.** El host controla acceso, retencion, serializacion y sanitizacion.
No concatenar cadenas no confiables en lineas de log; la truncacion puede cortar
una secuencia UTF-8 y los strings pueden contener bytes arbitrarios.

Se copia la observacion completa antes de emitir requested. El backend recibe
esa copia sin truncar (ID, estado, identidad de carpeta y archivo opcional),
incluso si un callback modifica despues el objeto original. Esto no autoriza el
destino ni sustituye su revalidacion de filesystem. El caller debe evitar carreras
concurrentes durante la copia; no es permiso para mutar objetos compartidos sin
sincronizacion. La mejora se limita a este metodo: no cambia el contrato previo
de otros metodos auditados que conservan referencias a argumentos del caller.
Un fallo de asignacion antes del invoke puede propagar sin eventos ni backend.

Politicas de [auditoria general](AUDIT.md) conservadas:

- require_request_record por defecto: si falla requested, backend no invocado;
  system_error y se intenta completed con backend_invoked=false.
- best_effort solo por eleccion explicita: puede ejecutar aunque falle requested.
- Si falla completed, se conserva el resultado ya devuelto por el backend y se
  incrementa failed_completed_records. Nunca se afirma rollback ni se repite.
- Excepcion del backend: se intenta completed/system_error/backend_threw=true y
  se propaga la excepcion original; no demuestra ausencia de efectos.

Sink sincronico/thread-safe, sin timeout, y backend serializado por instancia,
no entre procesos o wrappers. No hay elevacion, nuevos ficheros de log, reintentos,
colas ni cambios de configuracion. El host conserva la responsabilidad de pedir
consentimiento y de tratar resultados desconocidos/perdida del registro.

## Compatibilidad0.x

Nuevo virtual al final de SystemActions, override de AuditedActions y enum
ampliado: **recompilar/relinkar biblioteca y TODOS los consumidores/backends
personalizados juntos**. El default not_supported conserva compatibilidad de
fuente de subclases existentes, no de vtables binarias antiguas. No mezclar
plugins/objetos anteriores. No cambian layouts de AuditEvent/SystemSnapshot/
SampleOptions ni formatos de historial en esta unidad; no se declara ABI1.0
estable ni se cierra A05.

## Verificacion

tests/windows_startup_file_audit.cpp (portable): forwarding de todos los campos,
ambas direcciones y todos los ActionError, operacion separada/sin fallback
generico, dos etapas/ID, metadata limitada solo en evento, preservacion de
resultado/mensaje y excepcion, politicas de sink requested/completed/best-effort,
copia previa a callback que modifica el objeto caller. Factory real solo recibe
una observacion invalida: no se solicita una modificacion de Startup real.

tests/windows_startup_file_control.cpp (Windows): backend de prueba redirige
solo Known Folder a fixture temporal; el decorador real envuelve el proveedor
nativo. Retirada/restauracion positivas con archivos reales y eventos correlados,
request rechazado por sink sin invocar proveedor/crear respaldo, y completed
fallido con rename real exitoso/resultado conservado/diagnostico incrementado.
No se modifican entradas de inicio automatico reales ni se ejecutan archivos.
El consumidor instalado invoca el nuevo virtual auditado solo con observacion
invalida y comprueba sus dos eventos. Logs work/startup-file-audit-*.log.

Release: Windows125/125 y Linux110/110,20repeticiones de audit/control Windows
(40ejecuciones) y20audit Linux. Ambos consumidores instalados recompilados/
relinkados/ejecutados exit0. Windows y compilacion incremental final Linux sin
warnings; log inicial WSL conserva avisos Clock skew en compiler_depend.make
(0.023-0.047s futuro), sin cambios de reloj/configuracion ni causa atribuida.
Sin fixtures residuales ni Startup real modificado. No es CI remota ni validacion
de otros filesystems/arquitecturas. M05 sigue por HKLM/common/aprobacion efectiva
y acciones genericas; no se afirma que esta auditoria solucione Run/KTM6801.
