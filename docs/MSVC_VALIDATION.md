# Validacion local MSVC x64

Unidad M01, revision51, 2026-10-21. Este documento distingue validacion de
compilador de soporte de hardware y CI. La version de la biblioteca sigue0.2.0;
no declara API/ABI1.0 ni se habilitan proveedores experimentales.

## Entorno y reproduccion

Visual Studio18 Community, MSVC19.51.36260.0 (toolset14.51.36231), SDKWindows
10.0.26100.0, CMake4.4.2, generador Visual Studio18 2026, plataforma x64.
Se usan herramientas ya instaladas, sin descargas ni cambios de configuracion
del sistema. Todos los artefactos de verificacion quedan en work/.

Desde la raiz del workspace, con cmake/ctest en PATH:

```powershell
cmake -S outputs/pcinsight -B work/build-msvc-release -G "Visual Studio 18 2026" -A x64 -DCMAKE_CXX_FLAGS_INIT=/MP2
cmake --build work/build-msvc-release --config Release --parallel 2
ctest --test-dir work/build-msvc-release -C Release --output-on-failure
cmake --install work/build-msvc-release --config Release --prefix work/install-msvc
cmake -S outputs/pcinsight/tests/consumer -B work/consumer-msvc -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH="$PWD/work/install-msvc" -DCMAKE_CXX_FLAGS_INIT=/utf-8
cmake --build work/consumer-msvc --config Release --parallel 2
& ./work/consumer-msvc/Release/consumer.exe
```

Comprobar el codigo de salida de CADA etapa antes de continuar. El generador es
multiconfiguracion: Release se selecciona al compilar, probar e instalar, no con
CMAKE_BUILD_TYPE. Las aserciones de los tests originales se mantienen con /UNDEBUG;
el test de encoding nuevo usa static_assert, independiente de NDEBUG.

/MP2 es solo una opcion de esta verificacion, no un requisito de la biblioteca.
Con dos proyectos simultaneos limita a cuatro procesos de compilacion. Se
inicializa CMAKE_CXX_FLAGS_INIT, conservando los flags de plataforma/excepciones;
no sustituir CMAKE_CXX_FLAGS completo por /MP2. Una cache antigua puede requerir
reconfiguracion explicita; no reutilizar el mismo build para MinGW y MSVC.
[Contrato /MP](https://learn.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes).

## Correcciones verificables

La compilacion inicial nativa detecto que winternl.h del SDKMicrosoft no declara
NtSetInformationFile, FILE_RENAME_INFORMATION ni FileRenameInformation que el
proveedor de Startup usaba a traves de los headers MinGW. No se instala WDK:
el proveedor ahora declara internamente la firma/estructura ABI documentadas,
carga el MISMO simbolo dinamicamente y mantiene clase10, RootDirectory fijado
por handle y ReplaceIfExists=false. Se comprueban tamano/offsets de los campos
y del selector de clase. En MinGW se comparan tambien con sus declaraciones
independientes. No se cambia a MoveFile/rutas, copy/delete ni otro fallback.
No se cambia la identidad comprobada, alcance, privilegios o consentimiento.
Referencias: [NtSetInformationFile](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntsetinformationfile)
y [FILE_RENAME_INFORMATION](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information).
No usar el formato wire SMB/RDP como sustituto de este layout nativo.

CMake fija /utf-8 para C++ en los targets de este proyecto, no para el compilador
de recursos ni como requisito exportado al consumidor. Evita depender de la
pagina de codigos del usuario para fuentes/literales narrow de protocolo.
tests/utf8_literals.cpp verifica bytes UTF-8 esperados de literales directos y
escapados, ademas de wide/UTF-32, al compilar. Es una proteccion explicita de
portabilidad, no una afirmacion de que se reprodujo corrupcion de texto en este
host antes del cambio. [Contrato /utf-8](https://learn.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8).

Se corrigen advertencias de tipos sin desactivar warnings: tamanos de arrays
NVML fijos128/256 convertidos explicitamente a unsigned, constantes de IDs y
buffers con tipo unsigned/uint8_t, variable local de frecuencia renombrada y
conversion explicita del GUID ASCII devuelto por StringFromGUID2. No se cambian
algoritmos de telemetria ni se marcan estimaciones como valores exactos.
No cambian headers publicos, firmas, layouts, vtables ni formatos persistidos.
Actualizar biblioteca/consumidores juntos sigue siendo la politica0.x existente.

MSVC tambien detecto un fallo de COMPILACION en el test windows_history_store:
construia un system_clock::time_point desde -1ns, no representable implicitamente
con el clock100ns de esa STL. Ahora usa un tick nativo negativo; conserva la
exigencia de rechazarlo antes de normalizar a milisegundos. No se usa un cast
que lo redondee a0 ni se cambia el almacen, su formato o sus validadores.
Se revisaron HISTORY.md y WINDOWS_HISTORY_STORE.md antes de ajustar la prueba.
El warning de conversion ASCII del nombre de servicio privado se corrige de
forma explicita; el ejecutable SCM manual se compila, NO se ejecuta en esta
unidad ni se reutiliza consentimiento anterior para crear otro servicio.

## Evidencia y limites

Resultados finales Release: MSVC127/127, MinGW127/127 y Linux111/111. Tres
instalaciones separadas, consumidores externos recompilados/relinkados/ejecutados
exit0, sin mezclar .lib MSVC con .a MinGW. Repeticiones20 de UTF8/control de
archivo Startup/historialWindows/PDH en MSVC(80ejecuciones); UTF8/frecuenciaCPU/
NVML/discoveryDRM/historialWindows en Linux(100ejecuciones).

work/msvc-verified-build-windows.log y msvc-native-suite-windows.log son el build
MSVC completo corregido y su suite; msvc-repeat-windows.log las80repeticiones,
msvc-native-positive-windows.log el control de archivo privado positivo y
lectura CPU/firmware, msvc-install-windows.log y msvc-consumer-*-windows.log el
paquete/consumidor separado. Regresion final MinGW/Linux en
msvc-post-clock-{build,suite}-{mingw,linux}.log y consumidores finales en
msvc-final-consumer-{mingw,linux}.log. Los intentos previos se preservan y NO
son suites aprobadas. El build antes de arreglar el test del reloj acabo con
errorC2440; el build posterior termina exit0.

No se desactivan warnings. Las advertencias C de tipos/declaraciones detectadas
se corrigieron. MSVC mantiene el diagnostico de linea de comandos D9025 al
sobrescribir /DNDEBUG con /UNDEBUG en tests para mantener assertions Release;
no se vende como build sin ninguna advertencia. MinGW/Linux finales sin warnings.

El control Startup MSVC se verifica positivamente solo en sus fixtures privadas:
retiro/restauracion, bytes, recovery/conflictos/identidad/auditoria y limpieza.
No Startup real modificado, no accion SCM manual ni mutacion Run HKLM real.
El test de integracion reporta Ryzen9 7950X,32procesadores logicos,RAM DDR5 DIMM,
5200MT/s y2/4dispositivos representados por firmware; datos de este host, no
matriz universal. El mecanismo multigrupo PDH se prueba con modelo y consulta
nativa, no con un equipo multigrupo fisico certificado. M01 sigue abierta por
compresion estable/validacion multigrupo; no hay nuevo cierre ni credito parcial.

El primer build dentro del sandbox no avanzo de la regla inicial y fue
interrumpido; el build con acceso nativo al compilador si avanzo. No se ha
demostrado la causa concreta del bloqueo ni se configura UAC/ACL/antivirus para
sortearlo. El primer build nativo tambien se interrumpio despues de registrar
errores de declaraciones y warnings. Un intento de añadir /MP2 sustituyendo
CMAKE_CXX_FLAGS perdio /EHsc y genero C4530: se interrumpio y reconfiguro
con FLAGS_INIT conservando excepciones. No se certifica aquel intento.

Una suite local MSVC no es CI remota (N01), ni valida ARM64/x86, otras versiones
de SDK/MSVC o un equipo con varios grupos de CPU. No convierte el proveedor
experimental de compresion en estable/documentado. En esta configuracion no
se encontraron headers Vulkan con MSVC; las funciones dependientes conservan
su disponibilidad declarada. No certifica todas las APIsGPU, K03 ni matrices
fisicas G07/N02. M05 continua aplazada por eleccion explicita del usuario.
