# Windows GPU engines and utilization

WindowsGpuEngines adds to Monitor the engines exposed by the local PDH
counter GPU Engine(*) / Utilization Percentage, without commands, installation,
or a graphics workload of its own. Each instance's LUID is matched to the DXGI
inventory, not to list order or the card name. The implementation uses the
same injectable PDH boundary as GPU memory to test native calls; queries and
state are independent.

## What it returns

GpuInfo.engines retains ALL observed engines and does not limit compute to
two. GpuEngine.id identifies phys:N/eng:M within that LUID; type preserves the
reported type (3D, Copy, Compute 0, Video Codec Engine, or another), and name
is a readable label that includes the ID. Multiple Copy or Compute engines
are not merged just because they have the same name. Different physical nodes
under one logical adapter remain separate.

For each engine, only the PDH percentages of distinct processes on the same
LUID/node/index are summed. The metric is estimated: it measures load
OBSERVED in those instances, not complete scheduler coverage. Overall
utilization is the maximum across those engines, never their sum or average.
If any observed engine is invalid or unknown, the overall value is absent,
but the other valid engines are retained. permission_required takes precedence.
A list with no valid instances does not mean 0% GPU use: overall use is unknown.

This does not return per-process GPU usage or retain PIDs for GUI actions.
PIDs are used internally only to prevent double counting per engine. A short-lived
app that disappears between samples may be absent from the final array. A driver
may not expose every engine or context. Observed 0% is legitimate when there are
valid zero-valued rows; it does not rule out unobserved activity. Exact agreement
with Task Manager and instantaneous percentages are not promised. The GUI
should display notes and confidence rather than converting estimated to available.

## Validation and lifecycle

Full names must match pid_N_luid_0xHHHHHHHH_0xLLLLLLLL_phys_N_eng_N_engtype_T,
with 32-bit integers and a type of 1–64 printable ASCII characters, including
spaces and underscores. Hex accepts upper and lower case. _Total is ignored.
Duplicate PID/node/engine combinations are not counted twice. Conflicting types
for the same engine leave its type empty and its value unknown. Ambiguous #N
suffixes, new grammars, and malformed names are not guessed: the array is
rejected without publishing a subset as a complete list.

Only VALID_DATA/NEW_DATA CStatus values and finite, nonnegative doubles are
used. Individual percentages and sums outside 0..100 are rejected. A tolerance
of 1e-6 percentage points is used only for numeric rounding. The sum is kept
unrounded during accumulation, so many small excesses cannot be hidden by
clamping each row. PDH_FMT_DOUBLE|NOSCALE|NOCAP100 avoids hiding invalid
values with the automatic cap at 100. An inconsistent sum is not forced to
100 as if measured.

A new query requires two collections. The first sample is unknown, not zero.
Failed open/add/collect/array operations close the query. Recovery requires
preparation and another collection, without inheriting a percentage. An
invalid item affects its engine for that sample; a subsequent valid item can
restore it, depending on PDH state. A change in LUIDs, an empty GPU list, or
include_gpus=false closes the query and clears the whole baseline. Re-enabling
does not reuse an interval from before collection was disabled.

Arrays are limited to 8 MiB, 65,536 items, 256 wchar per name, and 4,096 observed
engines. The implementation checks size/count, in-buffer pointers, alignment,
and termination. Growth between the two calls does not trigger unbounded
reallocation. There is no timer, thread, or guaranteed deadline for a slow
driver or PDH. Memory, engines, and inventory are not an atomic snapshot. The
caller serializes Monitor and the scheduler chooses the interval. GpuEngine's
0.x layout was extended: rebuild packages and consumers. Linux keeps its earlier
engines; the new id/type fields remain empty where no source exists.

## Verification

Linux 35/35 and Windows 48/48 Release; installed packages and consumers were
rebuilt and run. Injected tests cover PID grouping by engine, maximum rather
than sum or average, multiple compute/copy engines and nodes, changing values,
genuine zero, epsilon and accumulation without repeated clamping, values
above 100, NaN/Inf/negative values, permissions, duplicates and conflicting
types, _Total and no instances, invalid names and limits, malformed buffers,
and resource handling during open/add/collect/format, nulls, recovery,
disabling, and topology changes.

Live test on the RX 7900 XT: eight observed engines, including 3D, two Copy
engines, Compute 0, Timer 0, Security 1, VideoJPEG 0, and VideoCodecEngine.
In a 200 ms sample, 3D had the highest observed utilization, 0.255468%.
No benchmark or D3D device was started. The numbers are observations, not
expected constants. This test does not certify other drivers, linked or
multi-GPU setups, hotplug, or coverage of very short-lived processes.
At this stage, Windows GPU work still included temperature, location and
driver date, runtime capabilities, free memory with a valid scope, and
better coverage of unobserved load.

Sources: [engines and maximum utilization as the Task Manager summary](https://devblogs.microsoft.com/directx/gpus-in-the-task-manager/),
[PDH arrays, statuses, and flags](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw).
