# Security and reporting

**Language:** [Español](SECURITY.md) · English

PCInsight 0.2.0 is under development and has not been presented as an audited
component for a privileged security boundary. It does not implicitly elevate
privileges. When an operation is denied or unsupported, do not fall back to a
shell, signal an unverified PID, or substitute a more permissive operation.

If you find a flaw that could expose data or allow unauthorized actions, do
not include credentials, private files, other users' traces, or an exploit
against real services in a public issue. Use the repository's private
reporting channel if the maintainer has enabled it. Otherwise, ask the
maintainer for a private channel before sending sensitive details. No
separate contact address or reporting service is advertised here.

For ordinary reports about incorrect metrics or missing hardware data, follow
the [hardware validation guide](docs/LINUX_HARDWARE_EVIDENCE.en.md) and review
what you share. The hardware verifier omits several text fields, but it does
not guarantee that a hardware profile is anonymous.
