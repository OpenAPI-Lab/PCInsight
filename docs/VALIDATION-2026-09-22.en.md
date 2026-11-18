# Validation on September 22, 2026

Work resumed on the original PCInsight plan. No AI model dependency was
added to the library.

## Fixes and deliverables

- Fixed `Metric<uint64_t>` conversions that prevented the Linux backend
  from building with GCC 13. That code was not compiled on Windows.
- Total CPU usage excludes guest/guest_nice, which are already included in
  user/nice. Per-process CPU excludes accumulated time from exited children.
- Existing test assertions remain active in Release. Previously, NDEBUG
  could remove test operations as well as checks.
- Sampler catches collector errors, exposes `last_error()`, synchronizes
  start/stop, and wakes the worker when move assignment destroys it.
- CMake installation includes Config, Version, Threads, and the 0.x ABI
  policy.

## Evidence

- Windows, GCC 16.2, Release: 4/4 tests passed.
- Ubuntu WSL, GCC 13.3, Release: 5/5 tests passed, including Linux integration.
- An external consumer using find_package built and ran on both systems.
- The Windows installation was copied to another prefix; the consumer
  built and ran against it.
- Sampler tests covered a move with the worker sleeping for one hour,
  stop/start, and concurrent start/stop calls. CTest timeout: 20 seconds.

This does not validate all GPU hardware, all init systems, or destructive
actions. No actions were performed on user services or sessions. Remote CI
was still pending at this checkpoint.

## Tracking

ClickUp was responding again. Packaging was closed: 30/53 subtasks complete
(57% by count). HistoryStore remained in progress after review: file
replacement, merging cumulative snapshots, and handling corrupt inputs
needed additional tests and fixes. Passing basic tests did not establish
those guarantees.
