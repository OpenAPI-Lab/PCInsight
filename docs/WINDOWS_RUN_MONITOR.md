# Informe Run/respaldo en Monitor y Sampler

Opt-in `SampleOptions.include_windows_managed_run` (default false).
`SystemSnapshot.windows_run_registrations` contiene el Metric del listado
HKCU Run/respaldo PCInsight. Los tipos WindowsRunRegistration/State están en
types.hpp, también disponibles al incluir startup.hpp como antes.

```cpp
pcinsight::SampleOptions options;
options.include_windows_managed_run = true;
// No requiere include_startup_entries ni opciones de firmas/atajos/CompanyName.
pcinsight::Monitor monitor;
auto snapshot = monitor.sample(options);
const auto& report = snapshot.windows_run_registrations;
if (report.value) {
    // Renderizar report.value, report.support y report.note.
    // registered / disabled_by_pcinsight / conflict no son StartupApproved.
}
```

## Semántica del opt-in

Monitor llama al [listado acotado](WINDOWS_RUN_LISTING.md) una vez por muestra
cuando se solicita esta opción. No conserva un resultado previo si falla una
lectura ni si luego se desactiva. Una muestra ya entregada conserva su copia;
las muestras posteriores no la modifican.

Con la opción false, el Metric queda sin valor y con nota de opción desactivada;
no invoca **este** proveedor de listado. Otras opciones como include_startup_entries
pueden leer Run por separado. Con true en Windows: resultado estimated si hubo
dos enumeraciones coincidentes, incluido vector vacío cuando es válido; errores
del proveedor permanecen sin valor/support/note. No sustituye un error por vector
vacío. Con true en Linux: unsupported explícito, sin intentar APIs Win32.

No anexa respaldos al vector genérico startup_entries, no cambia StartupStatus,
impacto, firmas ni CompanyName. Tampoco son dos inventarios atómicamente unidos:
si se activan ambos informes, se obtienen en instantes distintos y pueden diferir.
captured_at es la marca de la muestra general, no timestamp exacto individual de
cada lectura/registro. La disponibilidad del informe no habilita por sí sola la
acción transaccional ni resuelve su error6801. Sin mutación, elevación, KTM o ejecución.

## Sampler y coste

Sampler ya conserva SystemSnapshot completos en latest/history: el informe
opt-in se propaga allí con su soporte/nota, sin lógica extra de caché ni política
de escritura. No se transforma en curva numérica ni en impacto de inicio.
Puede contener rutas/argumentos/bytes nativos sensibles; el host decide mostrar,
retener y serializar esos datos. No hay persistencia automática en disco.

El listado permite16MiB de valores de salida y hasta20000 filas, además del
overhead de objetos. Un historial de60 muestras puede multiplicar ese consumo
hacia960MiB de payload, más overhead/copias temporales de latest/history; no
afirmar una cuota global de memoria de16MiB. Recomendación: metadatos Startup en
un Monitor a demanda o sampler separado de baja frecuencia y poca retención;
dejar este flag false en el sampler rápido de CPU/RAM. El ejemplo de prueba usa
intervalo1h y capacidad1, no una recomendación de escaneo cada segundo.

Se conservan límites de entrada/salida y diagnóstico del listado, sin deadline
duro de APIs del registro. No añade dependencia runtime nueva ni cambia las
acciones existentes. API0.x ampliada: recompilar biblioteca y consumidores.

## Verificación

2026-10-15: Release Windows75/75 y Linux55/55, paquetes y consumidores externos
reconstruidos/ejecutados exit0 en ambos. Prueba nueva con assertions activos:
default opt-out, opt-in independiente de inventario genérico, desactivación sin
retener resultado, copia de snapshot previo inalterada, Sampler latest/history
con capacidad1 y parada acotada. Monitor nativo Windows observó16 filas reales,
sin acciones de escritura; Linux recibió unsupported al activar el flag.
El consumidor Windows verifica reporte estimado/IDs y default desactivado.
No se crearon/borraron claves por esta integración ni se ejecutaron entradas.

La batería conserva pruebas de fallos/permisos/límites del proveedor de listado;
esta integración no certifica retiradas/restauraciones nativas, una GUI construida,
actualización de ClickUp, persistencia del historial ni CI remoto.
