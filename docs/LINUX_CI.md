# N01: flujo de verificación Linux y ensayo local

Revisión 170, 2026-11-06. El [workflow](../.github/workflows/ci.yml) mantiene
Windows sin cambios y amplía el job Linux. Linux configura y compila Release,
con ejemplos y pruebas activados. El job tiene presupuesto de 30 minutos y
acceso de lectura al repositorio; no publica artefactos ni solicita elevación.
El workflow debe vivir en la raíz del repositorio donde se ejecute GitHub
Actions. Editarlo aquí no dispara una ejecución remota.

Revisión 199: Linux tiene dos trabajos independientes, GCC (`g++`) y Clang
(`clang++`). Cada uno registra la versión disponible, configura desde cero y
verifica su propio SDK. Si uno falla, no se cancela el otro. No se fija una
versión exacta ni se instala un compilador en el workflow. Las pruebas en WSL
no demuestran todavía que ambos trabajos pasen en GitHub Actions.

El verificador lee el compilador de la caché del build Release y lo usa para
los ejemplos, el marcador y los consumidores directos. No depende de `CXX` al
verificar un build ya creado. Esto no replica flags ABI personalizados.

Después de verificar su SDK, cada trabajo Linux crea un archivo temporal con
`package-linux-sdk.sh`, lo extrae y compara el manifiesto. El archivo y su hash
quedan en el espacio temporal del trabajo: no hay subida de artefactos ni release.

La etapa reutilizable [verify-linux-sdk.sh](../scripts/verify-linux-sdk.sh):

1. Verifica que el build esté configurado en Release y ejecuta CTest con fallo
   si no hay pruebas. Conserva la salida, incluidos skips declarados por los
   tests; un skip no es evidencia positiva de capacidad nativa.
2. Instala solo en un prefijo dedicado dentro de una carpeta de evidencia
   nueva. Genera y verifica el manifiesto; comprueba el rechazo de un digest
   alterado sin escribir en el paquete. El manifiesto exige el aviso MIT y
   la auditoría de entrega comprueba que coincide con `LICENSE` del proyecto.
3. Compila seis ejemplos como consumidor externo contra el SDK instalado y
   corre las verificaciones de entrega. La revisión 227 añade un consumidor
   de direcciones con disponibilidad (13 pruebas en total). Revisa paths, contenido instalado
   y la selección exacta de versión del paquete.
4. Compila el consumidor del marcador y comprueba el enlace correcto y el
   rechazo del símbolo alternativo. Cada comando tiene log; un fallo se
   propaga incluso si se usa `tee`.
5. Compila un consumidor de lectura con el compilador del build Release,
   sin CMake en el consumidor, contra los headers y la biblioteca del prefijo.
   Comprueba el muestreo y el historial monotónico con un límite de 10 segundos.
   Este paso prueba la integración directa estándar; no replica flags ABI
   personalizados de otros perfiles.
6. Compila y ejecuta el contrato público de resultados contra ese mismo SDK:
   unidades tipadas, continuidad de PID, disponibilidad y dos rechazos de
   acciones antes del despacho. No compara layouts de un compilador específico.

Además, tras instalar el SDK, `check-linux-sdk-headers.sh` compila cada `.hpp`
público en una unidad vacía distinta, con `-Wall -Wextra -Werror`. Evita que
el consumidor dependa de un orden accidental de includes. No prueba todos los
templates posibles ni sustituye las pruebas de enlace o la cobertura Windows.

No se fuerza la línea base GCC/libstdc++ del host WSL sobre `ubuntu-latest`:
ese runner no fija el mismo perfil. La [sonda de ABI](LINUX_ABI_BASELINE.md)
mantiene esa comparación separada. No se añaden paquetes, `sudo`, cambios de
servicios ni un recorder instalado. Los tests privados de la suite conservan
sus contratos; las muestras del SDK no ejecutan acciones de control.

## Ensayo local

Desde el directorio de trabajo que contiene `work/`, con el build Linux
Release ya compilado:

```sh
PCINSIGHT_CI_JOBS=4 bash outputs/pcinsight/scripts/verify-linux-sdk.sh \
  work/build-linux work/ci-linux-170
```

Usar una carpeta de evidencia nueva en cada ejecución; el script no borra
ni sobreescribe una ejecución anterior. El build local de WSL tiene el
recorder de pruebas habilitado; el workflow conserva su default deshabilitado.
Por ello no se extrapola el número de pruebas ni la disponibilidad de los
proveedores de WSL al runner remoto. Esta verificación no requiere que el
GUI del usuario use CMake.

N01 no se cierra hasta tener ejecuciones reales exitosas de los perfiles
requeridos, con sus resultados y skips revisados. No se hizo push ni se
disparó CI remota en esta unidad.

Resultado local: `bash -n` y script completos exit0, Linux 200/200, entrega
12/12 y marcador 1/1. Manifiesto alterado y símbolo alternativo rechazados.
Ambos consumidores resolvieron el mismo prefijo instalado. Evidencia en
`work/ci-linux-170/`; N01 en curso y avance oficial 60/75=80%.
