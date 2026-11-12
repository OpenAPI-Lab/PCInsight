# L05 — frontera de transporte verificada y decisión de despliegue

Revision157, 2026-11-02. Prueba de contrato terminada. NO implementación de un
nuevo transporte de producción, helper instalado o protección del bus del host.
L05 sigue E. Producción no adquiere nuevas dependencias ni habilita ejecución.

## Hallazgo

Los límites156 actúan en el handler, cuando sd-bus ya recibió el mensaje. El
perfil de dbus-daemon permite limitar tamaño de mensaje y recursos del broker,
pero esos límites no se configuran por destino/helper. Aplicar un máximo pequeño
al bus compartido puede romper mensajes legítimos grandes de otras aplicaciones.
No se debe instalar este perfil en /etc/dbus-1 ni cambiar/reiniciar el bus del
sistema para satisfacer un requisito local de PCInsight.

Esta unidad no demuestra un límite de bytes PREVIO a recepción en el helper de
producción. Tampoco obtiene/atestigua límites efectivos del broker del usuario
mediante sus credenciales o identidad: SO_PEERCRED/UID no prueba la configuración.
No se convierte un umbral de cola local en una garantía dura de memoria.

## Prueba privada reproducible

tests/fixtures/privileged_transport_limits.xml es un FRAGMENTO SOLO PARA TESTS,
no una política instalable en el host. El runner lo inserta en la configuración
de su broker temporal, sin service directories/activación. Usa el ejecutable
dbus-daemon ya disponible únicamente como infraestructura de desarrollo.

Perfil: mensaje32768 bytes, colas incoming/outgoing65536 bytes, FD1 por mensaje y4
en cada cola,16 conexiones completas/por usuario,4 incompletas,128 matches,
32 replies pendientes y auth_timeout2000ms. Las colas pueden exceder su umbral
por hasta un mensaje permitido; no se afirma un máximo exacto de RAM/RSS, pues
hay overhead y otros recursos. No se estresan aquí todos esos límites.

El primer perfil con cuotas FD0 bloqueó autenticación en el runtime probado.
Se conserva el fallo en work/privileged-transport-focal.log y se corrigió el
fixture a cuotas positivas. No se generaliza esa observación a todos los brokers;
la conexión propia de PCInsight sigue negotiate_fds0. No se afirma que el broker
prohíba todos los FD por usar este perfil.

Modo --transport de tests/linux_privileged_receiver.cpp:

1. Petición pequeña legítima aceptada, con Authority SIMULADA.
2. Mensaje D-Bus válido con campo string de65536 bytes: append local correcto,
   pero llamada falla y el contador de entradas al handler NO aumenta. Tampoco
   aumentan los checks de Authority ni se llama al Manager.
3. Otro cliente aún puede hacer una petición legítima. El receptor/broker siguen
   operativos después del rechazo del cliente infractor.

Broker/libsystemd REALES privados, Authority/Manager SIMULADOS. Dos llamadas
alcanzan handler, dos checks de Authority, cero operaciones Manager. Son tres
intentos de cliente, no tres peticiones admitidas. No se ejecutan servicios
reales ni se conecta al bus/polkit del host. El padre retira broker/archivos y
el worker comprueba FD sin fuga. La prueba mide entrega al handler, no RSS ni
latencia/rechazo de cada variante de byte/FD/cola.

Linux186/186 +160 ejecuciones L05 (16tests10 veces;30brokers entre regresión,
admisión y transporte), MSVC10 argumentos/fallback,2 consumidores SDK existentes
exit0; biblioteca de producción sin cambios en esta unidad. Builds inicial/full/
final limpios. work/privileged-transport-*; positivo focal-final y suite completa.
No API/layouts/formatos públicos, Monitor/SystemActions o CI remota nuevos.

## Decisión pendiente, no inferida

No añadir un broker privado como dependencia runtime: contradice la intención
de evitar aplicaciones adicionales y amplía instalación/supervisión/seguridad.
No instalar el fragmento globalmente para aparentar protección por helper.

Se consultó al usuario entre:

- Aplazar despliegue/aceptación pendiente de L05 y continuar K03/OpenRC. Recomendado
  para seguir el alcance Linux existente; conserva L05 E y sus criterios abiertos.
- Diseñar un canal Unix privado con límites propios antes de decodificar. Es una
  ampliación de transporte, protocolo y vínculo de identidad/autorización, no un
  simple toggle de sd-bus ni autorización para instalar/elevación/acciones reales.

No se elige por silencio ni se cambia de arquitectura automáticamente. Hasta
respuesta, unidad157 cerrada como evidencia/contrato, siguiente paso de L05 en
espera de dirección. Quedan además proceso/cargador confiables, política y
consentimiento polkit reales, despliegue y aceptación. Las pruebas privadas no
cierran esas partes ni Windows/UAC.

OpenRC148 solo control explícito sigue autorizado para desarrollo/pruebas privadas;
no se amplía a recopilación/SysV/servicios reales. K03/I07 conservados.
C60/E10/P3/B2+3X excluidas:60/75=80%,15/75=20% abiertas, sin crédito parcial.

Fuente primaria: [límites de dbus-daemon](https://dbus.freedesktop.org/doc/dbus-daemon.1.html).
Las cifras y el escenario son decisiones de prueba PCInsight, no recomendaciones
de configuración para un bus de escritorio compartido.
