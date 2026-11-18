# Publish PCInsight as a standalone repository

**Language:** [Español](INDEPENDENT_REPOSITORY.md) · English

The repository root must be this directory: the one containing
`CMakeLists.txt`, `include/pcinsight`, `src`, `README.md`, and `LICENSE`.
Do not use the workspace or the larger GUI project as the root.
PCInsight does not depend on that consumer project.

## Before the first commit

1. Review the [support and gaps](SUPPORT_STATUS.en.md), [C++ getting-started guide](GETTING_STARTED_CPP.en.md), [contribution guide](../CONTRIBUTING.en.md), and [license](../LICENSE).
2. Check for build outputs, binary SDKs, private logs, credentials, and unreviewed hardware data. The `.gitignore` helps but does not remove already tracked files.
3. Check the exact file list with `git status --short`, then review `git diff --cached --stat` and `git diff --cached` after staging. Do not upload the whole workspace.
4. Historical documentation refers to local tests and internal logs that are not distributed. It does not present them as GitHub Actions results.

From this root, when you are ready to create the first commit:

```sh
git rev-parse --show-toplevel
git status --short
git add .
git diff --cached --stat
git diff --cached
git commit -m "Initial PCInsight SDK: Linux implementation, tests and documented limitations"
```

If this is not yet a Git repository, run `git init` first. If Git asks for a
commit name and email address, configure the identity you want to use for this
repository. You do not need to change the global identity used by other projects.

The commit is local. To publish it, create or choose a separate remote
repository, confirm its visibility, and connect its URL. These instructions
do not supply a placeholder remote URL or add a remote automatically.
Review the CI results after pushing; local verification does not predict
every result on the remote runner.

The MIT license applies only to PCInsight material. Do not include GUI code
or other projects to put them under that license. If the larger project
later consumes PCInsight as a submodule or dependency, keep the repositories
and their configurations separate.
