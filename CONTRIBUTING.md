# Contributing to Wevoa OS

Thanks for your interest in contributing to Wevoa OS, a Devdopz project.

## Before You Start

- Read [README.md](README.md) for the current project scope and commands.
- Review [docs/SETUP.md](docs/SETUP.md) for toolchain and environment setup.
- Check existing issues or open a new one before starting larger work.

## Ways to Contribute

- Report bugs and unexpected behavior
- Improve documentation or setup guides
- Fix low-level kernel, boot, UI, tooling, or build issues
- Propose well-scoped new features aligned with the project direction

## Development Guidelines

- Keep changes focused and easy to review
- Prefer small pull requests over large unrelated changes
- Follow the existing code style and file layout
- Avoid introducing third-party code that conflicts with the project's
  from-scratch direction
- Preserve the security-first and ethical-use-first design principles

## Local Validation

Run the checks that make sense for your environment before opening a pull
request:

- `make`
- `make hash-check`
- `powershell -ExecutionPolicy Bypass -File .\tools\build-image.ps1`
- `powershell -ExecutionPolicy Bypass -File .\tools\build-iso.ps1`

If you cannot run all of these, include the commands you did run and anything
you could not verify.

## Commit Style

- Use clear commit messages
- Keep each commit focused on one logical change
- Include documentation updates when behavior or developer workflow changes

Examples:

- `feat(kernel): add ...`
- `fix(gui): correct ...`
- `docs: update ...`
- `chore(tools): improve ...`

## Pull Request Expectations

Please include:

- A short summary of the change
- Why the change is needed
- Any testing or validation you performed
- Screenshots or logs if the change affects the UI or boot flow

## Security Reporting

Please do not disclose security issues publicly first. See
[SECURITY.md](SECURITY.md) for the reporting policy.
