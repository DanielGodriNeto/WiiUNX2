# Contributor and AI instructions

Read MAINTAINING.md before changing code. Preserve project authorship and intentional public identities. Never commit secrets, credentials, private accounts/usage records, local home paths, console dumps, Nintendo recordings or assets without verified redistribution permission. Retain upstream licenses and attribution.

Do not modify console system files. Keep native history reads read-only; save project state only on SD. Do not retain graphics resources across foreground loss. Make bounded allocations/reads, handle service failures and keep slow work off the rendering thread.

Review changed source, assets, documentation, configuration, commit metadata and reachable history before publication. Use the provided checks, but do not describe them as exhaustive security/legal audits. If sensitive history is discovered, remove it from all affected refs and release artifacts, preserving intended identity and licensing information. Document findings without copying private values. Revoke leaked credentials; history rewriting alone does not undo disclosure.

Update MAINTAINING.md and relevant user instructions when behavior, dependencies, workflows or validation status changes. Distinguish host checks, cross-compilation, hardware results and pending tests. Never claim testing that did not complete. Keep changes focused and preserve unrelated edits.
