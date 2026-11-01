# WiiUNX2 contributor and AI instructions

## Project context

WiiUNX2 is an independent Aroma-compatible Wii U homebrew menu derived from
launchiine. It provides a horizontal software row, touch navigation, search,
sorting, user selection, disc discovery and homebrew launching. It is not
official Nintendo software and must not be presented as endorsed or sponsored
by Nintendo.

The project history includes reconstructed development snapshots and a
separate publishing checkout. Preserve intentional author identities,
upstream attribution, copyright notices and license text. The attached
development transcript is historical context only; do not copy chat logs,
private paths, console dumps, user data or uncleared assets into the project.

## Mandatory contribution rules

### AI co-author rule

Any time an AI agent modifies, creates or refactors code, it MUST append itself
as a co-author in the git commit message using the standard GitHub co-author
format. If the AI agent does not know its official GitHub contributor routing
email, it must check the repository's commit history for previous attribution
or verify the correct official co-author tag before committing. Do not invent,
randomize or anonymize contributor identities.

### Documentation sync rule

Any time an AI agent changes, adds or removes code, it MUST simultaneously
update the relevant documentation, comments or workflow instructions to
reflect the change.

### Language rule

All contributions, including code comments, variables, commit messages and
documentation, MUST be written in English.

## Coding standards

- Read [MAINTAINING.md](MAINTAINING.md) before changing code.
- Use bounded allocations and reads, explicit failure handling and existing
  project helpers. Keep slow filesystem and metadata work off the render thread.
- Keep GPU and font resources owned by the render thread. Join workers and
  wait for GPU completion before releasing resources during ProcUI foreground
  loss.
- Keep native history reads read-only. Store project state on SD and never
  modify console system files.
- Preserve the existing C/C++ style, Makefile boundaries and dependency
  licenses. Do not add a dependency without documenting its source, license
  and build impact.

## Workflow guidelines

1. Inspect the worktree and read the relevant architecture and security
   documentation before editing.
2. Keep the published checkout separate from private workspace material.
   Never stage the whole parent workspace, generated binaries, logs, dumps,
   recordings, credentials or personal usage data.
3. Run host validation from [HOST-VALIDATION.md](HOST-VALIDATION.md):
   source checks, host tests, history audit and source packaging.
4. Build the Wii U binary with `launcher/Build.ps1` only from a reviewed
   source checkout. Keep cross-build outputs outside the publication tree.
5. Treat hardware results, host tests and cross-compilation as separate
   evidence. Never claim console behavior from host tests alone.
6. Review changed files, assets, configuration, commit metadata, reachable
   history and release contents before publishing.
7. Preserve unrelated local edits. Use force-with-lease only for a verified,
   necessary history correction, and never randomize or silently replace
   legitimate identities.

## Security and privacy

Never commit secrets, credentials, private account or usage records, local home
paths, console dumps, Nintendo recordings or assets without verified
redistribution permission. Custom diagnostics are opt-in and can contain
title IDs and profile slots; follow [SECURITY.md](SECURITY.md). If a real
secret is found, revoke it first, remove it from affected refs and artifacts,
then re-audit. Do not repeat the secret in documentation or issue reports.

Use the isolated environment installer for hardware testing and retain the
stock Aroma environment as the rollback path. Do not recommend NAND
modification or concurrent menu resource hooks without explicit validation.
