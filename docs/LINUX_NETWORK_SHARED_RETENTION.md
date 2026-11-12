# Retirada segura del productor compartido Linux

Continuacion revision92: [update autenticado](LINUX_NETWORK_SHARED_UPDATES.md)
compone esta retirada con altas/readmisiones por lotes nuevos y contadores
retenidos intactos. El backend publico aun no cambia; el pendiente91 al final
se conserva como registro historico, no como limite vigente del sampler interno.

Revision91, 2026-10-22. Unidad interna F03: `LinuxSharedNetworkSession::retain`
retira actores sin reiniciar la captura de los conservados. No es aun el update
del sampler autenticado ni del proveedor publico. F03/H04/J04 siguen abiertas.

## Contrato

Requiere consentimiento explicito y sesion en el proceso creador. Acepta solo
un subconjunto de los tokens kernel activos, con TGID/UID/inicio exactos y sin
duplicados. No admite actores nuevos ni reincorporaciones. La salida conserva
el orden original de slots, no el orden de la propuesta. Conjunto vacio cierra
toda la sesion; propuesta identica no hace syscalls ni reinicia contadores.

La propuesta se valida/aloca antes de mutar. Consentimiento, forma o ausencia
del callback de borrado dejan intacta la sesion. Un error/exception durante los
borrados cierra TODOS los FDs: no se devuelve exito para una retirada parcial.
Una copia fork cierra sus referencias sin borrar entradas del mapa del padre.

## Por que no se reciclan los slots

Cada retirada ejecuta `BPF_MAP_DELETE_ELEM` en el HASH TGID->slot. Las nuevas
invocaciones del actor salen por lookup ausente antes de contexto/contadores.
Pero una invocacion que ya obtuvo la entrada puede terminar DESPUES del
borrado. No se promete revocar eventos que ya estaban en curso.

Por eso los registros ARRAY retirados NO se borran, reasignan ni reutilizan.
Un evento anterior solo escribe su antiguo registro retirado. Los lectores
siguen recorriendo esos registros para cuadrar la secuencia global, pero no
publican sus buckets/calidad individual. La calidad GLOBAL sticky sigue
invalidando el conjunto; no se borra para ocultar perdida. El bracket global
de writers/secuencia sigue cubriendo el barrido completo, incluidos retirados.

Los actores conservados mantienen bytes/cookies/sequence y el checkpoint de
antirregresion por identidad, incluso si desaparece una fila anterior. La
retirada no cierra/recrea programas, links ni mapas: siguen siendo8FDs propios.
No hay nueva dependencia, comando externo, daemon, API publica ni persistencia.

## Evidencia

- Fixture de evento suspendido tras obtener el slot: retirada durante la
  invocacion, escritura tardia solo en registro retirado, actor ausente en
  salida y siguientes eventos sin acceso a contexto/contadores.
- Contadores retenidos1024->2048; regresion rechazada tambien despues de retirar;
  propuesta duplicada/modificada/denegada intacta; readmision rechazada; vacio
  cierra; borrado fallido/exception primero o segundo cierra todo; fork no muta.
- Una calidad individual fallida puede retirarse sin falsificar las filas que
  permanecen. La secuencia incluye sus eventos anteriores/no inventa cero.
- Nativo17->16->1: TODOS los17 hijos privados siguen enviando UDP. Solo los
  activos suman nuevos bytes/operaciones, origen TCP/UDP anterior preservado,
  mismo censo8FDs; vacio/stop-restart limpio y salida normal de todos los hijos.
  Sigue siendo un solo namespace real; no certificacion fisica64actores.

Logs `work/network-shared-retain-*`: final Linux Release130/130 sin skips,
dos focales20veces cada una (40ejecuciones), consumidor instalado `consumer`
exit0. MSVC dos focales20veces cada una; NO soporte Windows total/N03 resuelto.
Build final sin nuevos warnings C++, D9025 esperado. Advertencias iniciales de
indentacion se corrigieron sin cambio de logica; primer build preservado.
Primer intento consumer fallo por nombre ejecutable incorrecto antes de
ejecutar; log preservado y nombre real comprobado, no fallo de la biblioteca.
Espejo91 confirmado: comentario90140272294875. SEGUIMIENTO.txt sigue mandando.

## Siguiente limite pendiente

Falta componer la retirada con altas en registros NUEVOS y autenticar la
transicion completa por pidfd, conservando captura/origen de los retenidos,
excluyendo el intervalo de ausencia de los reincorporados y reiniciando todas
las tasas/epoch. No se implementara una falsa actualizacion con stop/start de
los retenidos. El backend publico sigue siendo el anterior, sin cambios.
