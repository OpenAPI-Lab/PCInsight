# N01: Linux verification workflow and local trial

Revision 170, 2026-11-06. The [workflow](../.github/workflows/ci.yml) leaves
Windows unchanged and expands the Linux job. Linux configures and builds
Release with examples and tests enabled. The job has a 30-minute budget
and read access to the repository. It does not publish artifacts or request
elevation. The workflow must reside at the root of the repository where
GitHub Actions runs it. Editing it locally does not trigger a remote run.

Revision 199: Linux has two independent jobs, GCC (`g++`) and Clang
(`clang++`). Each records the available version, configures from scratch,
and verifies its own SDK. If one fails, the other is not canceled. The
workflow neither pins an exact version nor installs a compiler. At this
checkpoint, WSL tests alone did not establish that both jobs passed on
GitHub Actions.

The verifier reads the compiler from the Release build cache and uses it
for examples, the header marker, and direct consumers. It does not rely on
`CXX` when verifying an existing build. This does not reproduce custom ABI flags.

After verifying its SDK, each Linux job makes a temporary archive with
`package-linux-sdk.sh`, extracts it, and compares the manifest. The archive
and hash stay in job-local temporary storage. There is no artifact upload
or release.

The reusable [verify-linux-sdk.sh](../scripts/verify-linux-sdk.sh) stage:

1. Verifies a Release build and runs CTest, treating an empty suite as a
   failure. It preserves output, including declared skips. A skip is not
   positive evidence of native capability.
2. Installs only to a dedicated prefix in a new evidence directory. It
   generates and verifies the manifest and checks rejection of a modified
   digest without writing to the package. The manifest requires the MIT
   notice; the delivery audit checks it against the project's `LICENSE`.
3. Builds six examples as an external consumer against the installed SDK
   and runs delivery checks. Revision 227 adds an address consumer with
   availability handling, bringing the total to 13 tests. It checks paths,
   installed contents, and exact package version selection.
4. Builds the marker consumer and verifies both successful linking and
   rejection of an alternate symbol. Every command has a log, and failures
   propagate even through `tee`.
5. Builds a read-only consumer with the Release build's compiler, without
   CMake on the consumer side, against the prefix's headers and library.
   It checks sampling and monotonic history within a 10-second limit.
   This tests standard direct integration; it does not reproduce custom
   ABI flags from other profiles.
6. Builds and runs the public result contract against the same SDK:
   typed units, PID continuity, availability, and two action denials
   before dispatch. It does not compare compiler-specific layouts.

After SDK installation, `check-linux-sdk-headers.sh` also compiles each
public `.hpp` in its own otherwise empty translation unit with
`-Wall -Wextra -Werror`. This prevents consumers from depending on an
accidental include order. It does not test every template instantiation
or replace link tests or Windows coverage.

The WSL host's GCC/libstdc++ baseline is not enforced on `ubuntu-latest`,
which does not pin that profile. The [ABI probe](LINUX_ABI_BASELINE.en.md)
keeps the comparison separate. No packages, `sudo`, service changes, or
installed recorder are added. Private tests retain their contracts; SDK
samples do not invoke control actions.

## Local trial

From the workspace directory containing `work/`, with the Linux Release
build already compiled:

```sh
PCINSIGHT_CI_JOBS=4 bash outputs/pcinsight/scripts/verify-linux-sdk.sh \
  work/build-linux work/ci-linux-170
```

This historical command uses the original workspace layout. In the
standalone repository, the same script is under `scripts/`.

Use a new evidence directory for each run. The script neither deletes nor
overwrites an earlier run. The local WSL build has the test recorder
enabled; the workflow keeps the default disabled. Do not extrapolate the
WSL test count or provider availability to the remote runner. This
verification does not require CMake in the consumer's GUI.

N01 does not close until successful real runs of the required profiles
have been reviewed, including their results and skips. No push or remote
CI run was triggered as part of this historical work.

Local result: `bash -n` and the complete script exited 0; Linux 200/200,
delivery 12/12, and marker 1/1. The modified manifest and alternate symbol
were rejected. Both consumers resolved the same installed prefix.
Evidence: `work/ci-linux-170/`. At this checkpoint N01 was in progress
and the official count was 60/75 (80%).
