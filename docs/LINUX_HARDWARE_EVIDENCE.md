# Verificar PCInsight en hardware Linux

El verificador consume PCInsight 0.2.0 desde un SDK ya instalado. Está separado de la biblioteca y del futuro GUI; no añade dependencias al usuario de la biblioteca. Una muestra indica qué pudo observar el SDK en ese entorno, no si un campo pasó su validación en hardware físico.

## Ejecutar la comprobación

Desde la raíz de PCInsight, compila e instala el SDK en un prefijo local. Si ya
tienes el SDK de esa misma revisión, omite los tres primeros comandos y apunta
`CMAKE_PREFIX_PATH` a su directorio. No uses `sudo` para esta comprobación.

```sh
cmake -S . -B build/hardware-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build build/hardware-sdk
cmake --install build/hardware-sdk --prefix "$PWD/build/hardware-prefix"
cmake -S tests/linux_hardware_evidence -B build/hardware-verifier \
  -DCMAKE_PREFIX_PATH="$PWD/build/hardware-prefix"
cmake --build build/hardware-verifier
ctest --test-dir build/hardware-verifier --output-on-failure
./build/hardware-verifier/linux-hardware-evidence --sample > evidencia.txt
```

La muestra no cambia permisos ni configuración; no inicia servicios, no activa captura eBPF y no solicita contextos OpenGL ni sondeo de clientes DRM. El informe omite textos, notas, nombres de interfaces, direcciones IP y ubicaciones PCI exactas. Los identificadores numéricos y las capacidades pueden identificar el perfil del equipo: revisa el archivo antes de compartirlo. `unknown` no es cero; `estimated` no es una medición directa. Los índices de GPU, disco e interfaz solo identifican filas de esa muestra.

## Criterios de aceptación física

- **N02 — sistema.** Anotar por separado si la ejecución fue nativa o virtualizada, distribución, kernel y controlador relevantes. Contrastar los positivos con sus fuentes del sistema: topología y frecuencias CPU; tipo, velocidad y ocupación de RAM cuando el firmware los exponga; capacidad y transporte de discos SATA/NVMe; identidad de interfaces físicas y USB cuando existan. Mantener separadas la API de KVM y la configuración de virtualización en firmware. Distribuir las pruebas en distintos equipos si es necesario; registrar ausencias o restricciones sin contarlas como positivos.
- **G07 — GPU.** Registrar positivos por proveedor (AMD, Intel, NVIDIA), dispositivo y controlador. Contrastar inventario, memoria y telemetría que la fuente correspondiente exponga. Una versión de API observada no demuestra renderizado ni cobertura de todos los motores. El informe conserva todas las filas de motores y no presupone que sus porcentajes estén disponibles. Las vías opt-in requieren pruebas separadas; una muestra vacía en WSL no cubre ningún proveedor físico.

Los casos sintéticos verifican el contrato del informe, no el hardware. Mantener N02 y G07 abiertos hasta que cada uno tenga su evidencia física correspondiente.

## Compartir resultados

No hace falta verificar todo el equipo: un solo dispositivo o campo sirve. Abre
un issue de hardware Linux en el repositorio con este formato. Incluye también
los campos inesperadamente ausentes; no cambies permisos para forzar un positivo.

```text
Versión y commit de PCInsight:
Entorno (Linux nativo / VM / WSL), distribución, kernel y arquitectura:
Dispositivo y driver (modelo y versión si se conocen; sin número de serie):
Campo revisado (CPU/RAM/disco/red o GPU):
Estado observado en PCInsight (disponible / estimado / desconocido):
Valor y unidad observados, si se pueden compartir:
Fuente independiente consultada y resultado esperado:
Resultado (coincide / difiere / no se pudo comprobar):
Fragmento revisado del informe, si ayuda:
```

Comparte solo las filas necesarias. No adjuntes volcados completos de firmware,
archivos binarios DMI, logs del sistema ni capturas con identificadores personales.
Si la fuente independiente no está disponible, describe la limitación sin marcar
la observación como validación positiva.

**Entrega Linux (revisión 177):** no esperar equipo físico para entregar el alcance verificable. Registrar por separado «probado en WSL», «probado con datos simulados» y «no validado en hardware». Los casos simulados deben cubrir datos válidos, ausencia y fallos relevantes; no cuentan como evidencia de que el firmware o driver real publique el dato. Si falta la fuente, conservar el valor desconocido. N02/G07 siguen en el seguimiento para validación posterior y B08 no presume un proveedor OEM por aprobar un fixture.
