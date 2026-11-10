# Security and privacy

Do not post credentials, raw console dumps, usage logs, history files, keys/certificates, local account paths or identifiable screenshots in public issues. Prefer a minimal synthetic reproduction and redacted error/status. Intentional author credits, public project identities, copyright notices and repository ownership must be preserved.

WiiUNX2's custom diagnostics are disabled by default. Opt in with an empty SD file wiiu/wiiunx2-debug.enabled, then restart. Logs append locally at wiiu/wiiunx2.log up to the configured 1 MiB cap; remove the flag and restart to disable. Old logs are not automatically deleted. WiiUNX2 does not initialize the upstream UDP or external logging-module fallback. System services, plugins and third-party modules may have their own logging behavior.

Recent sorting saves per-slot title IDs and launch dates on SD in wiiu/wiiunx2-recent-SLOT.txt, with .tmp/.bak files during replacement/recovery. This is private usage data even without account names. Installed-title names are also cached locally in wiiu/wiiunx2-names.cache (title ID, version, path, and name; no icons). The launcher still enumerates installed titles each startup; this file only avoids rereading unchanged display names. An additional process-memory map is discarded when the process exits. There is no project cloud upload. Native play records are read, not edited. The project does not modify console system files.

Before publication, run the source and history checks in HOST-VALIDATION.md. Review assets, commit messages/identities, configuration, releases and archives manually too. Automated patterns cannot detect every secret, image metadata issue or licensing concern. Keep personal test fixtures outside tracked trees.

If a real leak is found, first revoke affected credentials where applicable. Identify affected commits, branches, tags and release artifacts privately; remove the data with a verified history rewrite and coordinated force-with-lease push, then re-audit. Existing clones, server caches and forks may require additional cleanup; rewriting Git does not erase every copy. Keep a private incident record without reproducing secret values. Preserve legitimate attribution.

This project does not currently offer a dedicated private vulnerability-report inbox. Do not open a public issue containing sensitive details; contact a repository maintainer through an available private channel first. No hardware safety, legal clearance or immunity from third-party claims is guaranteed.
