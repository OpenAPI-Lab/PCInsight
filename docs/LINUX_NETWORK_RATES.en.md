# Linux network rates: continuity and resets

This work corrects `NetworkAdapter::receive_bytes_per_second` and
`send_bytes_per_second`, which Monitor obtains from sysfs without external
commands. It does not add per-process, per-user, or per-app accounting or
notification history.

## Corrections

Previously the backend divided differences in retained counters by the
global interval since Monitor's last sample. That clock advanced even with
include_networks=false, so reenabling could divide traffic from a long
pause by a short interval. It also subtracted uint64 counters without
checking for resets and retained disappeared interfaces. Network now has a
per-interface baseline with its own steady_clock, replaced in full on
each successful sample.

A rate requires two consecutive samples with the same name and nonzero
ifindex, increasing time, and two nondecreasing counters. A reset in
either direction invalidates both rates for the interval without wraparound
and establishes a new baseline. The same applies to identity changes or
equal or backward time. Absent interfaces are forgotten, and renaming
does not preserve continuity. Disabling include_networks clears every
baseline. The first sample after reenabling has no rates. Failures that
abort fill_networks also clear the tracker.

Rates are bytes/s rounded down, not bits/s or percentages. A zero
difference between valid samples yields a known zero; a missing baseline
does not. Values outside uint64 are absent and unsupported for that
direction, rather than saturated or converted out of range. Conversion
uses an exclusive upper bound of 2^64 and avoids subtracting signed
integer durations across their full range. Valid rates carry
Support::estimated and a note: they are interface counter differences
divided by sample time. sysfs reads are not atomic.

## Reading and limits

For each row, the reader reads ifindex, rx_bytes, and tx_bytes, then reads
ifindex again to reject a visible change during collection. Numbers must
be complete decimal uint64 values, with optional trailing whitespace.
Negatives, signs, overflow, trailing text, and empty reads are rejected.
Because the legacy NetworkAdapter totals are not Metrics, a row without
valid counters or identity is omitted rather than given zeros. Omission
clears its baseline, so recovery requires another sample before rates can
be published. Consumers must not interpret an absent adapter as zero
traffic. A global enumeration error still propagates to the caller.

The tracker rejects empty IDs, NUL, names longer than 4,096 B, duplicates,
and more than 100,000 rows, without publishing partial results.
Enumeration accepts up to 100,000 rows. This is not a total RAM budget
or a filesystem deadline. The getifaddrs list is released by RAII, also
if copying addresses throws. The public layout is unchanged and no new
dependencies are introduced.

Name plus ifindex is not persistent identity. An interface recreated
between polls with both values unchanged and increasing counters may go
undetected. Not all resets between samples are detected. Use a Monitor
within the same network namespace and recreate it when switching
namespaces. Counters, addresses, and metadata do not form an atomic
snapshot. Data reflects what is visible through the caller's sysfs and
getifaddrs and can include other processes' traffic. It is not an
application's payload and does not establish negotiated speed or physical
link capacity. F02 update: model and connection type no longer use the
old heuristic. See [network identity](LINUX_NETWORK_IDENTITY.en.md) for
availability, ARPHRD and markers, PCI/USB, and provenance. It does not
provide universal exact commercial identity or infer identity by name.
F01 retains its rate calculation and counter policy.

## Verification

On 2026-10-15, Linux Release passed 59/59 and Windows Release passed
79/79. Packages were reinstalled and rebuilt external consumers exited 0
on both platforms. The linux-network-rates test keeps assertions enabled
in Release and covers intervals, measured zero, each counter resetting,
ifindex or name changes, disappearance, explicit reset, equal or backward
time, out-of-range rates, fraction truncation, row and name limits,
duplicates, uint64 parsing, and full clock range without duration overflow.

The native Linux test sent 16 UDP datagrams of 1,024 B from a private
socket to itself on 127.0.0.1, with poll-bounded receipt. Loopback totals
increased and rates were positive. Monitor opt-out and reenabling leave
rates absent initially. The test does not configure interfaces, send
external traffic, require root, or install applications. Windows runs
portable calculation fixtures, not native sysfs collection. Physical
adapter hotplug or reset was not tested; those cases use fixtures.
