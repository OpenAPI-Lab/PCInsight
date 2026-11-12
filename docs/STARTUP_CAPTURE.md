# Capturador de referencia end-to-end

examples/startup_capture.cpp conecta Monitor, candidatos nativos, observer y
StartupImpactStore en un flujo ejecutable. Se compila como pcinsight-startup-capture
si PCINSIGHT_BUILD_EXAMPLES=ON (default). Es un consumidor de ejemplo incluido en
el proyecto, NO una aplicación externa que la librería necesite ejecutar o instalar.
El GUI puede usar esas mismas APIs y su propio scheduler sin este ejecutable.

```text
pcinsight-startup-capture FILE CAPTURE_ID [DURATION_MS]
pcinsight-startup-capture --help
```

La invocación explícita acepta asociaciones ESTIMADAS por marcadores GIO del UID
actual. No activa .desktop, no interpreta Exec, no inicia aplicaciones ni modifica
configuración de login. No es daemon, servicio ni recorder instalado de arranque.
Linux utiliza fuentes nativas existentes; Windows compila/expone ayuda pero la
captura devuelve código3 (proveedor no disponible), sin escribir archivo.

## Flujo y ventana

Valida argumentos, comprueba destino/directorio y rehúsa sobrescribir un archivo
existente que StartupImpactStore no pueda cargar. El directorio debe existir.
Un archivo existente válido se reemplaza con la nueva captura: no es un merge
ni backup/retención. Esa comprobación previa NO es CAS contra otros escritores:
usar una ruta privada y serializar invocaciones conforme a STARTUP_STORE.md.

Obtiene snapshot con procesos/startup y contexto de la propia identidad, descubre
candidatos, registra baselines y empieza la fase temporizada DESPUÉS de ese primer
scan. Así las identidades iniciales reciben una ventana entera. Por defecto60s,
duración entera configurable10ms..1h. Cada250ms vuelve a descubrir solo identidades
aún no registradas y hace poll; no re-registra identidades cerradas ni duplica CPU.
Los procesos que aparecen tarde tendrán ventanas más cortas al terminar la fase:
se conserva unknown/partial o high lower bound, no se inventa low/medium completo.

Al acabar, hace un último poll, close y save de captura cerrada, con recorded_at y
capture_id explícitos. Las llamadas nativas pueden ser lentas: tiempo de warmup,
descubrimiento/I/O y save no están sujetos a cancelación ni deadline absoluto.
No se acumulan polls atrasados; el siguiente scan se programa desde el reloj
actual. No escribe nada automáticamente antes del save final. No maneja SIGINT
como supuesto cierre/guardado seguro ni altera señales del proceso consumidor.

## Interpretación

Salida resume cantidad de observaciones, ventanas completas individuales, scans
parciales, intentos inaccesibles y registros rechazados. Esas cuentas de descubrimiento
son diagnóstico del ejecutable, no un certificado global persistido de cobertura
del boot. window_complete en el archivo significa duración muestreada del proceso
sin gaps, no que todas las apps se encontraron ni que arrancaron automáticamente.
El GUI debe seguir mostrando contexto/fecha histórica/confianza/scope del archivo.

Marcadores GIO pueden provenir de launch manual y no existen en todos los desktops.
No se verifica la versión/causa de launch del .desktop. Una captura vacía significa
que NO se encontraron candidatos aceptables, no impacto cero. PID/token/context
cambiados no se atribuyen a la observación vieja. Si un proceso desaparece entre
polls se retienen las muestras previas con gap; no se reconstruye su consumo.
Este ejecutable no es padre de las apps observadas, por lo que no usa waitid/reap
de procesos ajenos. STARTUP_CHILD_EXIT.md es para supervisores que sí poseen hijos.

Códigos:0 captura guardada;2 argumentos inválidos;3 proveedor/contexto nativo
no disponible;4 ruta/carga previa/guardado fallidos (puede haberse publicado si
falla confirmación de durabilidad);5 excepción/fallo de ejecución sin reclamar
completitud. Una captura puede guardarse con scans parciales: sus límites siempre
son los descritos arriba, incluso si el proceso devuelve0.

## Verificación

Linux35/35 y Windows45/45 Release, paquetes/consumidores instalados ejecutados.
Prueba Linux lanza SOLO este consumidor y un fixture C++ propio con marcadores
antes de exec, XDG/directorio/archivo privados. Flujo real con300ms: encuentra la
identidad exacta, obtiene delta estimado, cierra ventana, guarda y carga archivo.
Comprueba que .desktop sigue idéntico y que su Exec nunca se ejecutó. Duraciones
inválidas/ID vacío no sobrescriben la captura, archivo corrupto permanece intacto,
directorio ausente se rechaza, nueva captura vacía reemplaza sin acumular filas.
Todos los hijos del test son recolectados por el test, no por el consumidor.
Ayuda funciona en ambos sistemas; Windows se verificó también código3 sin archivo.

Pendiente: iniciar recorder temprano mediante integración explícita elegida por el
consumidor, probar causa fiable de autostart por proveedor y capturar procesos
ajenos breves/descendientes. No cerrar startup impact completo por este ejemplo.
