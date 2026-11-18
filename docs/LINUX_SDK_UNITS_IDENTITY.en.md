# Units and identity for Linux consumers (A05)

These rules apply to SDK 0.2.0. They neither create persistent identifiers
nor guarantee 1.0 binary compatibility.

| Data | Unit and use |
| --- | --- |
| `*_bytes`, `*_bytes_per_second` | Bytes and bytes/s, respectively. For KiB/s, divide by 1024; for Mbit/s, multiply by 8 and divide by 1,000,000. Convert only for presentation. Do not derive rates from cumulative inventory values without validating identity and interval. |
| `*_percent`, `average_response_ms` | Percent (25 means 25%, not 0.25) and milliseconds, respectively. Do not apply a generic ceiling of 100; respect the provider's scope. |
| `AppHistorySample::cpu_time`, `period_start/end` | CPU duration in nanoseconds and period dates in `system_clock`. Do not confuse accumulated CPU time with wall-clock duration or `network_bytes` with a rate. |
| `SamplerSample::finished_at` | Monotonic finish time. To chart the last 60 seconds, filter by elapsed time, not sample count. Leave gaps for unavailable measurements. |
| `ProcessInfo::linux_start_ticks` | Start token in ticks since boot. Do not convert it to seconds or a date before passing it to `terminate_process_checked`. |

## Row continuity is not authorization

- **Process:** Within the same stream, boot, and PID namespace, use the PID
  and the available start token. If the token is missing or changes, do not
  continue the series as if it were the same process. The token's resolution
  does not make it globally unique. For actions, let the API verify the
  target; a display key is not sufficient.
- **Disk and network:** The `id`, name, and interface index are inventory
  references, not permanent identifiers. Break the series when the row is
  lost or the context changes. A replacement with the same name may not be
  visible to the consumer. Prefer provider-calculated rates over rates
  derived from rows joined by name. See [disks](LINUX_DISK_RATES.en.md) and
  [network](LINUX_NETWORK_IDENTITY.en.md) for their specific safeguards.
- **GPU:** PCI identifies location, not serial number. Vendor and device IDs
  identify a model or family, not an individual card. Do not join GPUs by
  those fields or compare engine indices across providers.
- **Services and history:** Keep the provider with the service identifier;
  a main PID does not identify a service. For history, preserve the source,
  capture scope, and interval. A display name is not enough to join
  applications or add overlapping periods without an explicit policy.

The installed `linux-sdk-result-contract` test verifies selected public
types and exercises synthetic process continuity: name changes, PID reuse,
and missing or estimated tokens. Presentation conversions and the other
provider rules are documented here; the test does not simulate physical
device replacement.
