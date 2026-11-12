# Ventanas autenticadas sobre el productor compartido Linux

Revision93: [activacion publica observada](LINUX_NETWORK_SHARED_PUBLIC.md)
verificada en standalone/Monitor/Sampler/owner. Los limites de activacion de
revisiones90/92 siguientes son historicos; admision general pendiente.

Continuacion revision92: [updates autenticados](LINUX_NETWORK_SHARED_UPDATES.md)
ya conservan retenidos y admiten/reincorporan con captura fresca por lotes
disjuntos acotados. Activacion publica pendiente. Seleccion fija y coste N+14
de revision90 debajo son historicos: tras updates el coste es N+6+8G, G lotes.

Revision90, 2026-10-22. Unidad interna F03 terminada: LinuxSharedSelectedNetworkSampler
conecta el productor compartido a pidfds propios, asociaciones verificadas,
ventanas nominales, cuatro tasas estimadas y sumas UID parciales. Seleccion FIJA
observada1..64, cookie de entrada0. NO sustituye todavia al proveedor publico,
Monitor/Sampler ni su update_selection con contadores retenidos.

## Autenticacion y propiedad

Start exige consentimiento, lista completa sin duplicados y PID/start/pidfd
validos. Antes de tocar fuentes rechaza errores de forma/capacidad/cookies.
Retiene su propia copia de cada pidfd; el caller puede liberar las originales.
Lee identidad del productor, valida cada asociacion kernel/procfs, boot/observador
comunes, unicidad kernel y revalida TODOS despues de iniciar la captura. Fallo
de cualquier paso retira captura, consulta y todas las referencias propias.

Usa UNA LinuxPidfdIdentityQuery reutilizable para toda la seleccion cuando el
source ofrece join_query. Los sources anteriores join/join_layout siguen
funcionando. No se guardan permisos ni identidades como resultados actuales.
Cada sample hace join de TODOS antes y despues del barrido; cada join nativo
conserva DOS consultas kernel independientes y sus contextos procfs por extremos.
Se comprueban PID/start/UID/inicio kernel, vigencia del pidfd, permiso actual y
boot/observador/contexto. No hay conversion de enteros entre UID/PID locales y
dominios iniciales: se conservan por separado.

Son tres links de trafico compartidos y un link de consulta de identidad gated
(desarmado fuera de la consulta), no tres links de trafico POR proceso. Despues
de liberar el owner externo, hay N+14 FDs propios: N pidfds,8 del productor y6
de consulta. El guard de fork rechaza muestreo y cierra solo referencias de la
copia, sin alterar mapas/captura del padre. Stop/destruccion cierra todo.

## Ventanas y calidad

El barrido completo ya posee writer/sequence global, validacion por actor,
inmutabilidad de registros y bracket monotonic<=1s. Los joins quedan fuera del
bracket de CONTADORES; no hay deadline duro ni garantia de vida futura/estado
atomico del sistema. La ventana nominal usa el final del barrido para todos
los buckets seleccionados. El primer bucket no tiene tasa ni intervalo, no0.
Actores aun sin trafico conservan metadata, sin inventar buckets/usuarios.

Si falla el ultimo join, una identidad, permisos, calidad, contexto o reloj,
no se publica ninguna parte y se descartan TODAS las baselines. La proxima
muestra aceptada vuelve sin tasas. Los controles antirregresion del productor
no se borran al descartar las ventanas. discard_window hace rebaseline explicito.
Se usa el reductor existente con selected_observed_actor_set, epoch propio,
dominios kernel_initial y sumas por UID parcial con known/missing/overflow.

El resultado es PrivateCommonNetworkWindow; la proyeccion existente
publish_linux_process_network valida sus campos y produce el mismo esquema
publico. Probar esa proyeccion NO activa el backend en LinuxProcessNetworkSampler.
Start sobre sesion activa se rechaza. Stop/start usa contadores y epoch nuevos:
NO es equivalente a actualizar seleccion conservando origen de contadores.

## Evidencia

- Fixture nuevo tests/linux_network_shared_selected.cpp:64 actores/1024 buckets,
  cuatro tasas20B/s por bucket en intervalo2s, dos UID iniciales distintos del
  UID local,512 conocidos/UID/lane y suma10240B/s/UID/lane; metadata vacia,
  primer bucket sin tasa, proyeccion validada, reset/recovery y ultimo AFTER
  join rechazando todo. Solo tres cargas/links de trafico para64 actores.
- Fallos/excepciones en cada operacion de inicio con rollback, join/layout
  anterior conservado, kernel duplicado/inicio/contexto/boot/observador/calidad/
  permiso/clock/fork y fallo por extremos probados. Contadores no se reabren
  por discard_window; detener y reiniciar si se cambia la seleccion.
- Manual WSL privado --authorized-private-shared-windows17 en el ejecutable
  pcinsight-linux-network-capacity-native (fuera de CTest):17 hijos, owner
  liberado,31FD propios, vacio/primer bucket/tasas y UID contra oracle entero.
  UDP64..80B IPv4/IPv6 y TCP cuatro veces64..80B por direccion, cuatro lanes,
  cambio main-worker-main sin perder identidad/contador; padre TCP/UDP excluido,
  fork rechazado/padre intacto, actor salido rechaza TODO dos veces. Stop y
  start16 con nueva seleccion/epoch vacio, contadores frescos, discard/recovery,
 30FD propios, todos hijos salida normal y censo FD final limpio.
  Preparado16ms/start420ms/medido517ms/stop2262ms desde inicio: incluye joins,
  IPC/trafico/restart; NO benchmark ni promesa de latencia. Un namespace real
  compartido, SO_NETNS_COOKIE inicializado por oracle.64/1024 son fixtures.
- Linux130/130 Release sin skips; tres tests20veces=60ejecuciones; consumidor
  externo instalado reconstruido/ejecutado exit0. MSVC3/3 focales y nueva prueba
 20repeticiones; sin nuevos C++ warnings/D9025 esperado. Fallo inicial MSVC
  SegFault del fixture preservado en network-shared-window-final-targeted-msvc.log:
  range-for sobre datos de resultado temporal ya destruido. Se retiene la muestra
  en variable antes de recorrerla; final2-targeted-msvc.log y repeat-msvc.log pasan.
  NO se atribuye a esto el fallo consumidor75/N03, que sigue abierto.
  Logs work/network-shared-window-*, suite Linux final-suite y final-repeat.

## Pendiente inmediato

Actualizacion dinamica de la seleccion compartida: conservar captura/origen de
retenidos, retirar actores y admitir nuevos sin incluir huecos ni reutilizar
slots de forma insegura ante eventos en vuelo. Solo despues activar backend en
el proveedor publico/Monitor/Sampler, y despues admision automatica con cobertura.
No se cambia ahora el contrato publico de update_selection ni se introduce un
fallback que reinicie silenciosamente contadores de procesos retenidos.

Sin API/layout publico/GUI/persistencia/dependencias/CLI/config permanente nuevos.
Las pruebas BPF siguen limitadas a procesos privados autorizados, sin captura
general. F03/H04/J04 abiertas;57/75=76,0% cerrado,18/75=24,0% abierto.
