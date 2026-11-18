# Startup conditioned on GSettings

Verified on 2026-10-14: `AutostartCondition=GSettings <schema> <key>` is
evaluated through native GIO in StartupEntry.autostart_condition_satisfied.
The provider does not run gsettings, dconf, or a shell, or install
packages. Already installed libraries are loaded at runtime. Missing
libgio-2.0.so.0 or required symbols yields unknown. There are no GLib/GIO
header dependencies or mandatory links to those libraries. This
optional source is needed for GSettings backend semantics. The provider
does not invent a value or substitute an XML or default lookup as if
it were the user's stored setting.

The parser recognizes the full type with ASCII case-insensitive comparison
and requires exactly a schema and key. Identifiers must be nonempty,
at most 1,024 bytes, and use ASCII letters, digits, dots, hyphens, or
underscores. Extra arguments, slashes, NUL, controls, and internal spaces
are rejected. Desktop Entry, escape, main-group, and duplicate checks
from the earlier evaluator remain in place. The field's contents are
not executed.

Before constructing GSettings, the provider recursively looks up the
schema in the default source, requires a fixed path, checks has_key,
and verifies the boolean type. This is necessary because
[new_full](https://docs.gtk.org/gio/ctor.Settings.new_full.html) does
not accept a relocatable schema without a path. Missing schema or key,
a relocatable schema, a nonboolean type, or absent data yields unknown
without invoking constructors with invalid preconditions. Types are
compared with g_variant_type_equal rather than strcmp on peek_string:
[GLib does not guarantee NUL termination](https://docs.gtk.org/glib/method.VariantType.peek_string.html).
The returned value's type is also checked before reading its boolean.

A known result has Support::estimated and an explicit note: the caller's
effective value may be a schema or backend default or fallback. It does
not certify persisted provenance, backend availability, or permission
to read storage. Missing schema or runtime does not mean false. The
provider writes no settings. Schema, key, value, and GSettings are
released through RAII; the default schema source is borrowed. The
dynamic library is retained for the process lifetime because GIO can
register types, modules, callbacks, and global state that cannot safely
be unloaded between samples.

Each sample constructs and queries GSettings again. External updates
depend on the backend, caches, and the host's GLib event processing.
Instant external refresh without its main context is not promised.
No hidden loop is installed and the consumer environment is not changed.
GIO can initialize modules, threads, connections, and backend caches.
There is no promise of zero third-party initialization effects or a
universal timeout. Environment, schemas, loader, and modules are trusted
host context, not safe inputs for a root helper. The host may have cached
GSETTINGS_BACKEND and GSETTINGS_SCHEMA_DIR at first initialization.
PCInsight neither changes them nor selects another session.

Enabled status, standard XDG eligibility, and the GSettings predicate
remain separate. A false condition neither changes flags nor stops the
app. It does not guarantee launch or cover GNOME/GConf or other vendor
policies. The later [GNOME session provider](XDG_SESSION_CONDITION.en.md)
is separate. Other backends do not emulate native GIO, and unsupported
types stay unknown.

Linux Release passed 25/25 and Windows Release passed 36/36, with
installation and external consumers run on both. Portable tests simulate
runtime, source, schema, path, key, type, settings, and value failures.
They verify that constructors and boolean getters are not called
before validation and that objects are released. Parser tests cover
types, arity, limits, and propagation of permission_required from an
injected provider. Linux uses real installed GIO and a schema with
GSETTINGS_BACKEND=memory in a private process before its first load.
Monitor observes false and true while flags remain enabled. Missing
schema or key and a string-valued key are rejected without aborting.
Real preferences, HOME, real autostart, and services are not changed.
The native portion can skip when runtime or schema is absent; it ran
on Ubuntu 24.04 WSL. Cross-process refresh with a real dconf backend
and coverage of all backends were not validated.
