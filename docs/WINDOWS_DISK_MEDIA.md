# Windows: medio reportado HDD/SSD/SCM

Implementado 2026-10-13 con COM/WMI nativo y el proveedor local de almacenamiento
MSFT_PhysicalDisk. No se ejecutan PowerShell/Get-PhysicalDisk ni programas externos.
Se enlazan ole32/oleaut32/wbemuuid del SDK; el runtime utiliza componentes del OS.

## Fuente e identidad

La consulta es fija: SerialNumber, Size, BusType y MediaType de MSFT_PhysicalDisk
en ROOT\Microsoft\Windows\Storage. MediaType=3/4/5 produce HDD/SSD/SCM; 0 u otros
valores no se convierten en una clasificación. Estos atributos son reportados
por Windows/proveedor y algunos pueden modificarse administrativamente; no son
verificación independiente del hardware físico.

La asociación exige serie no vacía, tamaño y bus iguales a los obtenidos por
Storage IOCTL del inventario nativo, con una sola coincidencia en cada fuente.
Solo se recortan espacios externos en la serie; no se adivinan intercambios
de bytes ni se toma DeviceId WMI como número PhysicalDrive. Diferencias,
duplicados, identificadores incompletos y dispositivos virtuales/RAID que no
publiquen datos correlacionables quedan sin clasificación.

DiskInfo.reported_media_type expone valor/soporte/nota. Cuando se obtiene un
tipo reportado, media_type también recibe HDD/SSD/SCM. Cuando no se puede leer,
el campo antiguo puede conservar seek-penalty/no-seek-penalty como pista literal
del controlador; esa pista no se presenta como una clasificación garantizada.
La serie se utiliza internamente para asociación/caché y no se publica ni
persiste como identificador nuevo en la API pública.

## Permisos, COM y coste

El proveedor no eleva privilegios. E_ACCESSDENIED/WBEM_E_ACCESS_DENIED se
presentan como permission_required. En la máquina de desarrollo, la consulta
real devolvió **0x80041003, acceso denegado**, de modo que 0/5 discos obtuvieron
tipo WMI. Inventario, capacidad, volúmenes y rendimiento siguen disponibles.
Esto no significa que esos discos no tengan tipo de medio.

Se inicializa COM en el hilo llamador y se equilibra únicamente la inicialización
propia. Si el hilo ya pertenece a un STA, se usa ese apartamento sin deshacerlo.
No se llama CoInitializeSecurity para imponer política global al host; solo
CoSetProxyBlanket en los proxies locales. Si el host necesita configurar seguridad
COM global, debe hacerlo antes de sus primeras operaciones COM; COM puede
inicializar sus valores predeterminados implícitamente al usar servicios.

La caché dura 30 segundos o hasta cambiar identidades observadas, desactivar
discos/metadata o retroceder el reloj de muestreo. También se cachean fallos para
evitar repetir RPC en cada muestra. SampleOptions.include_disk_media=false
evita COM/WMI y reinicia esa caché, conservando el resto de los datos de disco.
Es una opción de metadata Windows; no cambia el proveedor de discos Linux.

Enumeración hasta 4096 filas, strings BSTR hasta 4096 caracteres, conversión
entera sin valores negativos/truncados/overflow y descarte de respuesta incompleta.
Next usa esperas de 1 segundo y presupuesto observado de 5 segundos para recorrer
filas. **No es un timeout total**: ConnectServer puede esperar hasta el máximo
nativo de 2 minutos y otras llamadas COM/propiedades pueden bloquear más tiempo.
Recolectar en worker, como Sampler, evita bloquear el GUI. Los proxies/BSTR/VARIANT
se liberan mediante RAII y no se invocan métodos que modifiquen almacenamiento.

## Verificación y estado

Fixtures de VARIANT/BSTR, medios conocidos/desconocidos, identidad distinta,
duplicados en ambas fuentes, datos ausentes, error de permisos, renovación/reset
de caché y eliminación de clasificación anterior. Integración real comprueba
la lectura fallida por permisos y que se preserva un STA inicializado por el
consumidor. La lectura real observó el fallo por permisos descrito arriba.
No hubo cambios de privilegios ni de atributos del disco.
Windows Release 19/19, Linux WSL 6/6. El campo/opción nuevos requieren recompilar
consumidores; no hay ABI estable en 0.x.

Actualización2026-10-19 revision38: respuesta WMI positiva5/5,2HDD/3SSD reales
en SATA/NVMe, sin UAC ni cambios de permisos/configuración. El fallo0x80041003
descrito arriba es histórico, no estado actual ni causa atribuida/corregida por
esta revisión. Código WMI sin cambios. M03 cierra proveedores soportados con
evidencia en WINDOWS_NETWORK_IDENTITY.md; no se anuncian probados todos los
transportes COM, timeouts extremos ni otras topologías/versiones físicas.

Referencias: [MSFT_PhysicalDisk](https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/msft-physicaldisk),
[identidad de hardware](https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/msft-storagefaultdomain),
[COM para WMI](https://learn.microsoft.com/en-us/windows/win32/wmisdk/initializing-com-for-a-wmi-application).
