# Firma del ejecutable candidato de inicio Windows

Opt-in independiente: `include_startup_entries=true` e
`include_windows_startup_signature=true` (por defecto false). Devuelve
`StartupEntry.windows_signature` sin promover CompanyName a editor verificado.
Usa Wintrust/Crypt32 del sistema: sin CLI, SDK instalado por el usuario,
ejecución de la entrada, elevación ni modificación de su configuración de inicio.

## Resultado y confianza

`native_status` conserva el LONG de WinVerifyTrust. Solo **cero exacto** significa
aceptación para esta política; no se interpreta como HRESULT con SUCCEEDED.
`status` distingue trusted_local_policy, rejected, no_embedded_signature y
unavailable. Se clasifican como rechazo los errores conocidos de digest,
desconfianza explícita/de raíz/de sujeto, expiración, revocación y uso incorrecto.
Otros errores, incluidos proveedores no disponibles y valores positivos,
son unavailable, no una aceptación ni una prueba de ausencia de firma.

TRUST_E_NOSIGNATURE se etiqueta no_embedded_signature únicamente para esta
consulta en modo archivo. No descubre catálogos ni prueba que el archivo esté
globalmente sin firmar. Un resultado nativo observado puede tener soporte
available pero status unavailable: el dato conocido es el error del proveedor.
Si no se puede abrir/consultar el archivo, falta el valor; ACCESS_DENIED conserva
permission_required. El inventario no desaparece por ausencia de firma.

`signer_name` es el nombre de presentación del certificado hoja del primer
firmante, obtenido del estado de WinTrust **solo tras aceptación**. No es identidad
única, CompanyName, garantía de autoría del payload, reputación ni permiso de
ejecución. Una firma aceptada puede carecer de nombre legible. No hay fallback
a recursos VERSIONINFO. `source_executable` y `policy` hacen explícita la fuente.
Un EXE lanzador no acredita los argumentos, scripts, DLL o programas que cargue.

## Política y selección

WINTRUST_ACTION_GENERIC_VERIFY_V2, WTD_CHOICE_FILE, WTD_UI_NONE y ventana -1.
WTD_CACHE_ONLY_URL_RETRIEVAL, WTD_REVOCATION_CHECK_NONE, WTD_REVOKE_NONE y
WTD_DISABLE_MD2_MD4. Política local y caché Windows, sin recuperación de red
del proveedor solicitada por esta consulta y sin comprobación de revocación.
No equivale a revocación vigente, SmartScreen, antivirus, política MOTW, todas
las firmas múltiples, catálogos ni verificación independiente de la política
de confianza del host. No altera el almacén de certificados.

Reutiliza la selección conservadora de [CompanyName](WINDOWS_STARTUP_PUBLISHER.md):
Run con EXE literal inequívoco, .exe directo o destino local guardado compatible
de .lnk (requiere además el opt-in de metadatos). Sin PATH, variables, UNC,
prefijos extendidos, scripts, DLL, handler .url ni resolución del shell.

Se abre el candidato con GENERIC_READ/FILE_SHARE_READ y OPEN_REPARSE_POINT;
se rechazan directorios, reparse points finales y archivos mayores de128MiB.
El mismo handle abierto se entrega a WinTrust; se cierra siempre, al igual que
el estado de verificación mediante WTD_STATEACTION_CLOSE incluso ante errores
de extracción. Certificados/datos del proveedor no se liberan por separado.
Compartición restrictiva puede impedir leer archivos con escritores abiertos.
No es garantía de identidad de lanzamiento futura, snapshot universal ni de
inmutabilidad posterior. Una unidad mapeada o un reparse point intermedio puede
implicar IO de red: cache-only no convierte cualquier ruta en almacenamiento local.

## Límites y uso

Máximo50000 filas,16 rutas textuales distintas por muestra,128MiB de tamaño de
archivo nativo y16MiB de valores de texto retenidos (ruta/política/nombre).
Caché por llamada incluye fallos; no reutiliza firmas de muestras anteriores.
No canonicaliza alias/case. Rutas hasta32768B UTF-8 y nombres hasta16384B;
nombre nativo hasta4097 wchar_t incluyendo terminador, UTF-8 estricto sin NUL
intermedio; cadena de certificados y firmantes hasta64 para extracción.
Estos son límites de entradas/buffers, no cuota de IO del proveedor ni deadline.
Es una consulta costosa: no activarla en cada tick de CPU/RAM; el host debe elegir
una cadencia de metadatos y presentar los resultados como observaciones fechadas.

No cambia startup status (unknown), impacto (not_measured), CompanyName ni
acciones. Linux no interpreta estas firmas; el lector nativo allí es unsupported.
API0.x ampliada: recompilar consumidores contra headers/biblioteca instalados.

## Verificación

Pruebas portables: códigos cero/positivos/rechazados/desconocidos, clasificación
derivada ignorada, nombres eliminados en fallo, proveedor/permisos/estado ausente,
independencia de CompanyName, selección conservadora, caché incluyendo errores,
límites16/50000/16MiB y etiquetas inválidas.
API Windows inyectada: flags/GUID/UI, mismo handle, extracción solo tras éxito,
certificado del proveedor, nombres inválidos/ausentes y cierre en éxito/error y
excepción. Consulta nativa del propio EXE de prueba sin firmar y lectura opcional
de explorer.exe para observar la política real sin ejecutarlo. Integración Monitor
opt-in independiente/opt-out y consumidor instalado con coherencia de resultado.
La prueba no modifica registro, carpetas Startup ni almacenes de confianza.

Resultado Release 2026-10-15: Windows70/70 y Linux54/54; instalación y consumidores
externos reconstruidos/ejecutados en ambos, exit0. En el host Windows, el EXE
fixture devolvió no_embedded_signature; explorer.exe devolvió LONG0 y firmante
Microsoft Windows. Esto observa la política local de ese equipo, no prueba
revocación actual ni validez en otros hosts. Linux ejecutó el pipeline portable
y regresiones, no WinTrust nativo. MSVC, catálogos, firmas múltiples y más
equipos/políticas Windows siguen pendientes.

Fuentes oficiales:
[WinVerifyTrust: estado LONG y cero exacto](https://learn.microsoft.com/en-us/windows/win32/api/wintrust/nf-wintrust-winverifytrust),
[WINTRUST_DATA: flags y cierre](https://learn.microsoft.com/en-us/windows/win32/api/wintrust/ns-wintrust-wintrust_data),
[WINTRUST_FILE_INFO: handle del archivo](https://learn.microsoft.com/en-us/windows/win32/api/wintrust/ns-wintrust-wintrust_file_info),
[WTHelperGetProvSignerFromChain](https://learn.microsoft.com/en-us/windows/win32/api/wintrust/nf-wintrust-wthelpergetprovsignerfromchain),
[CertGetNameStringW](https://learn.microsoft.com/en-us/windows/win32/api/wincrypt/nf-wincrypt-certgetnamestringw).
