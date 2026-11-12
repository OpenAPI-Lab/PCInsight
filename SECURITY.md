# Seguridad y reportes

PCInsight 0.2.0 está en desarrollo y no se presenta como componente auditado
para una frontera de seguridad privilegiada. No ofrece elevación implícita.
Los rechazos de permisos y las funciones no soportadas no deben sustituirse
por shell, señales a PIDs no verificados u operaciones más permisivas.

Si detectas un fallo que pueda exponer datos o ejecutar acciones no autorizadas,
no adjuntes credenciales, archivos privados, trazas de otros usuarios ni un
exploit contra servicios reales en un issue público. Usa el canal privado de
reportes del repositorio si el mantenedor lo ha habilitado. Si no existe,
solicita al mantenedor un canal privado antes de enviar los detalles sensibles.
No se anuncia aquí una dirección de contacto o servicio que aún no existe.

Los informes ordinarios de métricas incorrectas o hardware sin datos pueden
seguir [la guía de validación](docs/LINUX_HARDWARE_EVIDENCE.md), tras revisar
los datos compartidos. El verificador de hardware oculta varios textos, pero
no garantiza anonimato de un perfil de hardware.
