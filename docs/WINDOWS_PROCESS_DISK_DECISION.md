# Disco por proceso Windows: siguiente paso de M02

Actualizacion revision59: usuario autorizo continuar; [control de sesion y
pruebas nativas](WINDOWS_DISK_TRACE_LIFECYCLE.md) implementados y verificados.
No atribucion ni integracion en Monitor todavia. El resto de este documento
conserva la decision/solicitud historica de revision58, no un bloqueo vigente.

Revision58, 2026-10-21. Investigacion documental, no proveedor implementado ni
prueba ETW ejecutada. M02 sigue E; no aumenta el porcentaje. Revision57 ya
implementa E/S general, que NO permite separar disco de red/dispositivos.

## Via evaluada

ETW es una interfaz nativa del sistema, sin ejecutar herramientas externas.
[DiskIo](https://learn.microsoft.com/en-us/windows/win32/etw/diskio) permite
habilitar eventos de disco en un logger de sistema. La clase
[DiskIo_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup1)
documenta tamano en bytes, numero de disco e hilo emisor; este ultimo no existe
en todas las versiones antiguas. No basta con asignar el PID del header de
finalizacion al proceso que originalmente solicito la operacion.

La atribucion requiere una relacion hilo/proceso valida en el tiempo del evento,
control de reutilizacion de identificadores y deteccion de eventos perdidos.
No se promete atribuir causalmente a la app original toda escritura diferida
del sistema/cache ni convertir falta de eventos en actividad cero conocida.
Eventos/esquemas no soportados y ventanas incompletas deben quedar unknown.
No sumar varias capas de un mismo I/O como si fueran transferencias distintas.

[StartTraceW](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew)
documenta restricciones de acceso y recursos limitados para sesiones; no se
puede asumir que el token actual permita iniciar un logger de sistema. Una
sesion con nombre propio no equivale a captura limitada a un unico proceso.
No se iniciara ni habilitara elevacion como efecto de Monitor.sample().

## Autorizacion solicitada para el siguiente experimento

- Una sola sesion ETW de prueba con nombre descriptivo propio, en tiempo real,
  sin fichero ETL, por un maximo de15segundos de captura.
- Solo eventos necesarios de disco y de identidad proceso/hilo. Pueden pasar
  transitoriamente eventos de otros procesos por memoria; descartar sus datos
  y conservar unicamente identidad/contadores del proceso privado de prueba.
- Sin registrar rutas, contenidos de archivos ni argumentos de otros procesos.
  Presupuesto de buffers solicitado acotado a64MiB; si no puede respetarse,
  abortar, no ampliar cuotas globales ni cambiar registro/configuracion.
- Ejecutable/archivo de prueba en work/, I/O propio acotado, sin acceder a
  archivos ajenos. Privilegios administrativos solo si son necesarios para
  esta prueba, sin instalar servicios/controladores/utilidades ni alterar grupos.
- Si ya existe la sesion solicitada, abortar: no adoptar, modificar ni parar
  sesiones ajenas. Cerrar consumidor y detener solo la sesion creada por la
  prueba en todas las salidas normales/de error; comprobar su retirada.
- Un watchdog independiente debe solicitar cierre al vencer el plazo. Si no
  puede comprobarse la limpieza, informar nombre/error y no seguir capturando.
  No prometer limpieza garantizada frente a caida del SO o terminacion forzada.

La autorizacion no habilitaria captura permanente, red ETW, cambios de cuentas,
autologgers ni integracion por defecto. Primero validar acceso/ciclo de vida;
despues atribucion con I/O propio y pruebas de perdida/reuso/errores. Todavia
no se ha escrito ni ejecutado el experimento. Esta decision no sustituye la
implementacion ni la validacion necesaria para cerrar M02.
