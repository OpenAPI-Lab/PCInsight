# M05: decision pendiente sobre aprobacion efectiva de Windows

Revision49, 2026-10-19. Investigacion documental y revision de evidencia existente;
no se cambia codigo ni se ejecutan pruebas nuevas en esta unidad. M05 permanece
abierta y no se altera su criterio de cierre ni el denominador oficial.

## Resultado de la investigacion

La [clase StartupTask](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.startuptask?view=winrt-26100)
requiere una declaracion de startup en el manifiesto del paquete. Su enumeracion
GetForCurrentPackageAsync corresponde al paquete actual. No es una enumeracion
general de las entradas Run o los archivos Startup de otras aplicaciones.
Ademas, [RequestEnableAsync](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.startuptask.requestenableasync?view=winrt-26100)
no anula una desactivacion hecha por el usuario en el Administrador de tareas;
el usuario debe volver a habilitarla manualmente.

Por ello, implementar StartupTask para el paquete del host NO satisface la
aprobacion efectiva ni enable/disable general pedidos para M05. Tampoco basta
con que una entrada este presente en Run o en una carpeta de inicio. Los
proveedores explicitos de PCInsight conservan su semantica de registro/retirada,
no se renombran como aprobacion efectiva de Windows.

Microsoft documenta la configuracion de aplicaciones desde
[Configuracion/Administrador de tareas y las carpetas Startup](https://support.microsoft.com/en-us/windows/experience/startup-boot/configure-startup-applications-in-windows).
Abrir esa interfaz puede ser una integracion de GUI, pero no es un proveedor de
telemetria/control de esta biblioteca y no completa M05. No se abre ni automatiza
esa interfaz como parte de la investigacion.

No se encontro en las fuentes oficiales revisadas un contrato publico general
de StartupApproved equivalente al control del Administrador de tareas. Esto es
un resultado acotado de investigacion, NO una demostracion de que tal API no
exista en ninguna version. No se deduce el significado de bytes del registro
ni se ofrece un proveedor experimental como soporte universal certificado.

## Segundo pendiente independiente: transacciones Run

La evidencia final de revision48, work/machine-run-pinned-native-windows.log,
registra ERROR_RM_NOT_ACTIVE (6801) al abrir una clave transaccional, en las dos
ramas privadas redirigidas para probar las vistas HKLM32/64. Las consultas sin
transacciones funcionan; no hubo escrituras del proveedor en esos intentos.
El [catalogo Win32](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--6000-8199-)
describe 6801 como infraestructura de transacciones no iniciada en el gestor
de recursos especificado. Esto NO identifica por si solo la causa local ni
demuestra que elevar privilegios lo resuelva.

No se repite el sondeo, no se reconfigura KTM, no se cambian ACL ni se agrega
un fallback no transaccional. La prueba del modelo sigue sin equivaler a una
mutacion nativa positiva. Resolver StartupApproved no resolveria automaticamente
este segundo pendiente, y viceversa.

## Decision solicitada

Continuacion revision50: se interpreta el siguiente "prosigue" como continuar
con otra subtarea y aplazar M05, no como consentimiento para el proveedor
experimental. M05 conserva su estado abierto y todos los pendientes de cierre.
2026-10-21: el usuario confirma explicitamente "Aplazar M05 y continuar con otra
subtarea"; revision51 registra esa decision, sin activar el proveedor experimental.

Se presentaron al usuario dos alternativas, sin cambiar el alcance por cuenta
del asistente:

- Aplazar esta parte de M05 y continuar con otra subtarea oficial. M05 queda
  abierta, sin credito parcial. No prolongar una investigacion sin evidencia.
- Aceptar un proveedor experimental basado en estructuras internas no
  documentadas, limitado por version. Aceptarlo solo autoriza su desarrollo:
  no certifica el formato ni autoriza modificar entradas reales de otras apps,
  elevar privilegios o cambiar configuracion del sistema en las pruebas.

Si se elige la segunda opcion, antes de ofrecer mutaciones se debe establecer
un contrato explicito: opt-in, alcance/versiones comprobadas, identificacion de
fuente y vista, estado desconocido para formatos no reconocidos, observaciones
obsoletas/conflictos, preservacion de bytes y politica de recuperacion. Las
pruebas iniciales deben quedar aisladas en fixtures privadas. Una interpretacion
experimental no debe prometer elegibilidad ni lanzamiento garantizado. Los
limites restantes de M05 se mantienen visibles; no se reduce el criterio para
cerrarla artificialmente.

## Control

Sin nueva implementacion ni nueva validacion de hardware. Ultimas suites
implementadas/verificadas: Windows126/126 y Linux110/110 Release (revision48),
no reejecutadas aqui. C56/E11/P6/B2;75activas y3X excluidas:56/75=74,7% cerrado,
19/75=25,3% abierto. No representa horas restantes.
