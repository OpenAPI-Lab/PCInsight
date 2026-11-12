# Identidad PnP de adaptadores Windows y cierre M03

Implementado2026-10-19. API adicional `query_windows_network_identity(adapter.id)`
en `include/pcinsight/windows_network_identity.hpp`. Es una consulta explícita
read-only para el GUI, separada de Monitor para no escanear PnP por adaptador
en cada muestra dinámica. No cambia NetworkAdapter/SystemSnapshot ni historial.

Devuelve Metric de WindowsNetworkIdentity con adapter_id, interface_guid,
manufacturer y device_description. Estas dos propiedades también son Metric:
vacío válido conocido no equivale a campo ausente. Comprobar metric_is_known
antes de mostrar valores. Fabricante significa declarado por PnP/INF, no marca
física autenticada, proveedor independiente del driver ni marca OEM del equipo.

## Fuente, identidad y errores

Recibe id luid:<decimal> de Monitor, sin signos, padding, NUL, overflow ni0.
GetIfEntry2 obtiene GUID/tipo del LUID. SetupAPI enumera clase Net presente,
abre SU software key existente(DIREG_DRV,KEY_QUERY_VALUE) y lee NetCfgInstanceId
como GUID estricto. Exige una coincidencia única por GUID: no nombre, MAC,
descripción ni orden. Lee SPDRP_MFG y SPDRP_DEVICEDESC, no construye rutas de
registro con entrada del usuario, no publica MAC/serie/instance-id PnP.

Reabre la key del mismo nodo y relee ambas keys; al terminar toda la enumeración
revalida LUID/GUID/tipo con GetIfEntry2. Cambios, duplicado o fallo global de
enumeración/asociación invalidan el resultado. No equivale a snapshot atómico,
autenticación de un host adversarial ni token de acciones. Id local al host/boot,
no identidad permanente. Ausencia documentada de key/NetCfgInstanceId se omite
como dispositivo sin asociación. Una interfaz lógica puede no tener nodo PnP.
Sin match, la identidad IP Helper sigue conocida y las propiedades ausentes.
Un fallo de propiedad no elimina la otra; permission_required distingue acceso
denegado; otros fallos usan unsupported con operación/código. Conservar note.

4096nodos máximo, GUID80bytes, cada string4096bytes. Tipos, tamaños, NUL y
UTF-16/UTF-8 comprobados, sin crecimiento/reintento ilimitado. Rechaza
REG_EXPAND_SZ/MULTI_SZ; no expande ni interpreta strings. Keys/set RAII.
Sin CLI, WMI, COM global, descargas, SDKs nuevos, escrituras, cambios de red
o elevación. Fuera de Windows devuelve unsupported. Llamadas OS síncronas
sin deadline propio: usar worker del GUI. Sin caché ni efectos implícitos.

## Verificación

tests/windows_network_identity.cpp: entrada inválida antes de I/O, derechos
mínimos, permisos, scan parcial, duplicado, inexistencia, GUID distinto/
malformado/tipo incorrecto, cambio de key fijada/reabierta, cambio GUID/tipo
de interfaz/fallo final, límite4096nodos/registro inválido, propiedad parcial/
denegada/vacía, UTF-16/terminación/tipo inválidos y cierre de handles.
Positivo read-only:55interfaces,9fabricantes,8con ifType Ethernet; Realtek /
Realtek PCIe 2.5GbE Family Controller asociado por GUID real. También hay
Microsoft/Bluetooth, Kernel Debug y PdaNet: no se anuncian todos como NIC físico.
20repeticiones de identidad y20de red(40ejecuciones) aprobadas.

M03 verifica también WMI5/5discos,2HDD/3SSD reales en SATA/NVMe, sin UAC ni
cambios de permisos/configuración. El antiguo0x80041003 es histórico, no el
estado actual ni causa demostrada/corregida por esta revisión; código WMI intacto.
Capacidad filesystem5/5,un disco Windows,uno con pagefile. Lectura/escritura/
latencia5/5; active_percent válido1/5 en muestra detallada: otros valores
%IdleTime inválidos/fuera de0..100 se rechazan, sin clamp/cero inventado. Su
disponibilidad cambia por muestra y no se exige un positivo permanente.
Diagnóstico posterior sin cambio de proveedor:active5/5, conservado en
work/network-identity-active-diagnostic-windows.log. No demuestra la causa
concreta de las ausencias de la muestra anterior ni las oculta.
Segunda muestra real de red con send/receive conocidos55/55, sin generar tráfico;
0B/s conocido no se confunde con indisponibilidad.

Fixtures existentes cubren deltas/regresiones/identidad/IP, volúmenes multidisco/
cuotas/duplicados/errores: no se afirma hotplug/RAID/MPIO/Storage Spaces ni todos
los filesystems físicos probados. No se reparten capas lógicas arbitrariamente.
M03 cierra proveedores soportados disco/red e identidad con disponibilidad
explícita y positivos; M02/M06 procesos/sesiones, F03 Linux, N01 CI/toolchains
y N03 auditoría siguen abiertos. No nuevas dependencias de terceros ni cambios
de almacenamiento/conexiones. API adicional sin layouts/ABI virtual/historial
previos cambiados. Windows122/122,Linux108/108 Release, consumidores instalados
ambos recompilados/relinkados/ejecutados exit0. Logs work/network-identity-*.log.

Referencias primarias:
[identificadores](https://learn.microsoft.com/en-us/windows/win32/network-interfaces),
[software key](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/opening-a-device-s-software-key),
[fabricante PnP](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/devpkey-device-manufacturer).
