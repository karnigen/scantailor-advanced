# GitHub Copilot repository instructions

## Git push rule

Never run `git push` or any equivalent remote push operation unless the user has given an explicit,
standalone request to push and explicitly confirmed that push immediately before it is executed.

A commit, successful build, completed task, or previous instruction does not grant push permission.

## Commit trailer rule

Never add a `Co-authored-by` trailer or any other added co-author attribution to commit messages
unless the user explicitly asks for it.

## Build and tooling

Use the repository's `build-vcpkg.sh` and vcpkg manifest for configuring, building, and testing.
Do not install project dependencies system-wide or use system package managers for this repository.
Use locally managed Python tools through `uv tool run`; for example, run `clang-format` with
`uv tool run clang-format`.
