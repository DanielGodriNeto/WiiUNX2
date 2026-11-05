// WiiUNX2 GPL-3.0-or-later. Builds the reproducible menu release package from a reviewed checkout.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import crypto from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

const TOOL_DIR = path.dirname(fileURLToPath(import.meta.url));
const DEFAULT_ROOT = path.resolve(TOOL_DIR, '../..');
const CHECKER = path.join(TOOL_DIR, 'Check-Public-Source.mjs');
const HEADER = 'launcher/src/common/common.h';
const FORMAT_VERSION = 1;
const MAX_VALIDATION_BYTES = 64 * 1024;
const MAX_RPX_BYTES = 256 * 1024 * 1024;
const ARCHIVE_LIMIT = 2 ** 32;
const MAX_ENTRIES = 65535;
const VERSION_NEEDED = 10;
const VERSION_MADE_BY = 20;
const DOS_DATE = 0x0021;
const ELF_MAGIC = Buffer.from([0x7f, 0x45, 0x4c, 0x46]);
const SEMVER = /^\d+\.\d+\.\d+$/;
const VERSION_DEFINE = /^#define[ \t]+WIIUNX2_VERSION[ \t]+"([^"\r\n]*)"[ \t]*\r?$/gm;
const EXCLUDED_NAMES = new Set(['.git', '.test-output']);
const GENERATED_PATHS = new Set(['launcher/build', 'launcher/men.rpx', 'launcher/src/resources/filelist.h']);
const GENERATED_DEPENDENCY_DIR = /^dependencies\/[^/]+\/(?:build|release|lib)(?:\/|$)/;
const PORTLIB_NAMES = ['ppc-freetype', 'ppc-harfbuzz', 'ppc-libgd', 'ppc-libpng', 'ppc-libjpeg-turbo', 'ppc-libmad', 'ppc-libvorbisidec', 'ppc-libogg', 'ppc-bzip2', 'ppc-brotli', 'ppc-zlib'];
// [name in package, path in repository]
const TOP_FILES = [
  ['README.md', 'README.md'],
  ['LICENSE', 'LICENSE'],
  ['SECURITY.md', 'SECURITY.md'],
  ['SOUNDS.md', 'SOUNDS.md'],
  ['THIRD-PARTY.md', 'launcher/THIRD-PARTY.md'],
  ['UPSTREAM-REVISIONS.txt', 'launcher/UPSTREAM-REVISIONS.txt'],
  ['COMPATIBILITY.md', 'launcher/COMPATIBILITY.md'],
  ['Install-SD.ps1', 'launcher/Install-SD.ps1'],
  ['Update-SD.ps1', 'launcher/Update-SD.ps1'],
];
const LICENCE_FILES = [
  ['NotoSans-OFL.txt', 'launcher/licenses/NotoSans-OFL.txt'],
  ['launcher-LICENSE.txt', 'launcher/LICENSE'],
  ['libgui-COPYING.txt', 'dependencies/libgui/COPYING'],
  ['librpxloader-LICENSE.txt', 'dependencies/librpxloader/LICENSE'],
  ['libwuhbutils-LICENSE.txt', 'dependencies/libwuhbutils/LICENSE'],
  ['nlohmann-json-LICENSE.txt', 'launcher/src/utils/nlohmann-json-LICENSE.txt'],
];
// Keep in sync with the patterns in Check-Public-Source.mjs.
const HOME_PATTERNS = [/[A-Z]:[\\/]Users[\\/][\w.-]+/gi, /\/(?:home|Users)\/[a-zA-Z0-9_.-]+\//g];
const OTHER_PATTERNS = [
  ['token pattern', /gh[pousr]_[A-Za-z0-9]{20,}/g],
  ['private key header', /-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----/g],
];
const CRC_TABLE = Uint32Array.from({length: 256}, (_, n) => {
  let c = n;
  for (let bit = 0; bit < 8; bit += 1) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
  return c >>> 0;
});
const USAGE = 'usage: Package-Release.mjs --validation <file> [--root <dir>] [--version <x.y.z>] [--rpx <file>] [--portlib-licenses <dir>] [--out <dir>] [--allow-dirty] [--allow-rpx-home-paths]';
const VALUE_OPTIONS = new Map([
  ['--root', 'root'],
  ['--version', 'version'],
  ['--rpx', 'rpx'],
  ['--validation', 'validation'],
  ['--portlib-licenses', 'portlibLicenses'],
  ['--out', 'out'],
]);

export class ReleaseInputError extends Error {
  constructor(problems) {
    super(problems.join('\n'));
    this.name = 'ReleaseInputError';
    this.problems = [...problems];
  }
}

class UsageError extends Error {}

function inputProblem(message) {
  return new ReleaseInputError([message]);
}

function attempt(problems, check) {
  try {
    return check();
  } catch (error) {
    if (!(error instanceof ReleaseInputError)) throw error;
    problems.push(...error.problems);
    return undefined;
  }
}

function sha256Hex(data) {
  return crypto.createHash('sha256').update(data).digest('hex');
}

function bytewise(a, b) {
  return Buffer.compare(Buffer.from(a), Buffer.from(b));
}

function text(value) {
  return Buffer.from(value, 'utf8');
}

export function crc32(buffer) {
  let crc = 0xffffffff;
  for (const byte of buffer) crc = CRC_TABLE[(crc ^ byte) & 0xff] ^ (crc >>> 8);
  return (crc ^ 0xffffffff) >>> 0;
}

export function readVersion(headerText) {
  const found = [...headerText.matchAll(VERSION_DEFINE)];
  if (found.length !== 1) throw inputProblem(`${HEADER}: expected exactly one WIIUNX2_VERSION define, found ${found.length}`);
  return found[0][1];
}

function countMatches(value, pattern) {
  return (value.match(pattern) ?? []).length;
}

function homePathCount(value) {
  return HOME_PATTERNS.reduce((total, pattern) => total + countMatches(value, pattern), 0);
}

export function privateFindings(value) {
  const labels = [];
  if (homePathCount(value) > 0) labels.push('home directory path');
  for (const [label, pattern] of OTHER_PATTERNS) {
    if (countMatches(value, pattern) > 0) labels.push(label);
  }
  return labels;
}

function regularFileStat(file, label) {
  let stat;
  try {
    stat = fs.lstatSync(file);
  } catch {
    throw inputProblem(`${label}: file is missing or cannot be read`);
  }
  if (stat.isSymbolicLink()) throw inputProblem(`${label}: symbolic link is not allowed`);
  if (!stat.isFile()) throw inputProblem(`${label}: must be a regular file`);
  return stat;
}

function isDirectory(file) {
  try {
    return fs.lstatSync(file).isDirectory();
  } catch {
    return false;
  }
}

function isBigEndianElf32(data) {
  return data.length >= 6 && data.subarray(0, 4).equals(ELF_MAGIC) && data[4] === 1 && data[5] === 2;
}

export function checkRpx(file) {
  const stat = regularFileStat(file, '--rpx');
  if (stat.size < 1 || stat.size > MAX_RPX_BYTES) throw inputProblem('--rpx: size must be between 1 byte and 256 MiB');
  const data = fs.readFileSync(file);
  if (!isBigEndianElf32(data)) throw inputProblem('--rpx: not an ELF32 big-endian file');
  return {data, bytes: data.length, sha256: sha256Hex(data)};
}

function decodeUtf8(data) {
  try {
    return new TextDecoder('utf-8', {fatal: true, ignoreBOM: true}).decode(data);
  } catch {
    return null;
  }
}

function validationTextProblems(value, rpxSha256) {
  const problems = [];
  if (/[\u0000-\u0008\u000b\u000c\u000e-\u001f\u007f]/.test(value)) {
    problems.push('--validation: contains control characters other than tab, CR and LF');
  }
  for (const label of privateFindings(value)) problems.push(`--validation: contains a ${label}`);
  if (!value.includes(rpxSha256)) problems.push('--validation: must contain the men.rpx SHA-256 in lowercase hex');
  return problems;
}

export function checkValidation(file, rpxSha256) {
  const stat = regularFileStat(file, '--validation');
  if (stat.size === 0) throw inputProblem('--validation: file is empty');
  if (stat.size > MAX_VALIDATION_BYTES) throw inputProblem('--validation: file is larger than 64 KiB');
  const value = decodeUtf8(fs.readFileSync(file));
  if (value === null) throw inputProblem('--validation: must be valid UTF-8');
  const problems = validationTextProblems(value, rpxSha256);
  if (problems.length > 0) throw new ReleaseInputError(problems);
  return value;
}

function nameProblem(name) {
  const segments = name.split('/');
  const bad = !/^[A-Za-z0-9._/-]+$/.test(name) || segments.some(segment => segment === '' || segment === '.' || segment === '..');
  return bad ? `${name}: entry name is not allowed in the archive` : null;
}

export function validateEntryName(name) {
  const problem = nameProblem(name);
  if (problem !== null) throw inputProblem(problem);
}

function duplicateProblems(names) {
  const seen = new Map();
  const problems = [];
  for (const name of names) {
    const key = name.toLowerCase();
    if (seen.has(key)) problems.push(`duplicate name (case-insensitive): ${seen.get(key)} and ${name}`);
    else seen.set(key, name);
  }
  return problems;
}

function archiveEstimate(entries) {
  // 30 local + 46 central header bytes, each name twice, plus the 22-byte end record.
  return entries.reduce((total, entry) => total + 76 + 2 * Buffer.byteLength(entry.name) + entry.data.length, 22);
}

function entryProblems(entries) {
  const problems = [];
  if (entries.length > MAX_ENTRIES) problems.push(`more than ${MAX_ENTRIES} zip entries`);
  for (const entry of entries) {
    const problem = nameProblem(entry.name);
    if (problem !== null) problems.push(problem);
  }
  problems.push(...duplicateProblems(entries.map(entry => entry.name)));
  if (archiveEstimate(entries) >= ARCHIVE_LIMIT) problems.push('archive would be 4 GiB or larger');
  return problems;
}

function localHeader(name, size, crc) {
  const header = Buffer.alloc(30);
  header.writeUInt32LE(0x04034b50, 0);
  header.writeUInt16LE(VERSION_NEEDED, 4);
  header.writeUInt16LE(DOS_DATE, 12);
  header.writeUInt32LE(crc, 14);
  header.writeUInt32LE(size, 18);
  header.writeUInt32LE(size, 22);
  header.writeUInt16LE(name.length, 26);
  return header;
}

function centralHeader(name, size, crc, offset) {
  const header = Buffer.alloc(46);
  header.writeUInt32LE(0x02014b50, 0);
  header.writeUInt16LE(VERSION_MADE_BY, 4);
  header.writeUInt16LE(VERSION_NEEDED, 6);
  header.writeUInt16LE(DOS_DATE, 14);
  header.writeUInt32LE(crc, 16);
  header.writeUInt32LE(size, 20);
  header.writeUInt32LE(size, 24);
  header.writeUInt16LE(name.length, 28);
  header.writeUInt32LE(offset, 42);
  return Buffer.concat([header, name]);
}

function endRecord(count, directorySize, directoryOffset) {
  const record = Buffer.alloc(22);
  record.writeUInt32LE(0x06054b50, 0);
  record.writeUInt16LE(count, 8);
  record.writeUInt16LE(count, 10);
  record.writeUInt32LE(directorySize, 12);
  record.writeUInt32LE(directoryOffset, 16);
  return record;
}

function storedZipBytes(entries) {
  const parts = [];
  const central = [];
  let offset = 0;
  for (const entry of entries) {
    const name = Buffer.from(entry.name, 'ascii');
    const crc = crc32(entry.data);
    parts.push(localHeader(name, entry.data.length, crc), name, entry.data);
    central.push(centralHeader(name, entry.data.length, crc, offset));
    offset += 30 + name.length + entry.data.length;
  }
  const directory = Buffer.concat(central);
  parts.push(directory, endRecord(entries.length, directory.length, offset));
  return Buffer.concat(parts);
}

// Both the partial file and the final name are created exclusively: an existing partial or release is never replaced.
function publishPartial(partial, file) {
  try {
    fs.linkSync(partial, file);
  } catch (error) {
    if (error.code === 'EEXIST') throw error;
    fs.copyFileSync(partial, file, fs.constants.COPYFILE_EXCL); // file systems without hard links
  }
}

function writeNewFile(file, data) {
  const partial = path.join(path.dirname(file), `.${path.basename(file)}.partial`);
  fs.writeFileSync(partial, data, {flag: 'wx'});
  try {
    publishPartial(partial, file);
  } finally {
    fs.rmSync(partial, {force: true});
  }
}

export function writeStoredZip(filePath, entries) {
  const problems = entryProblems(entries);
  if (problems.length > 0) throw new ReleaseInputError(problems);
  const sorted = [...entries].sort((a, b) => bytewise(a.name, b.name));
  const archive = storedZipBytes(sorted);
  writeNewFile(filePath, archive);
  return sha256Hex(archive);
}

function walkFiles(base, relDir, skip, acc) {
  for (const name of fs.readdirSync(path.join(base, relDir))) {
    const rel = relDir === '' ? name : `${relDir}/${name}`;
    const kind = skip(rel, name);
    if (kind === 'excluded') continue;
    if (kind === 'generated') {
      acc.generated += 1;
      continue;
    }
    const stat = fs.lstatSync(path.join(base, rel));
    if (stat.isSymbolicLink()) acc.problems.push(`${rel}: symbolic link is not allowed`);
    else if (stat.isDirectory()) walkFiles(base, rel, skip, acc);
    else if (stat.isFile()) acc.files.push(rel);
    else acc.problems.push(`${rel}: only regular files and folders can be packaged`);
  }
}

function sourceSkip(rel, name) {
  if (EXCLUDED_NAMES.has(name)) return 'excluded';
  if (GENERATED_PATHS.has(rel) || GENERATED_DEPENDENCY_DIR.test(rel)) return 'generated';
  return null;
}

function scanTree(root, problems) {
  const acc = {files: [], generated: 0, problems: []};
  walkFiles(root, '', sourceSkip, acc);
  problems.push(...acc.problems);
  return acc;
}

function readFiles(base, files) {
  return new Map(files.map(rel => [rel, fs.readFileSync(path.join(base, rel))]));
}

// The publication check only looks at known text extensions; this looks at every packaged source file.
function privateSourceProblems(sources) {
  const problems = [];
  for (const [rel, data] of sources) {
    if (privateFindings(data.toString('latin1')).length > 0) problems.push(`${rel}: possible private path or credential (contents omitted)`);
  }
  return problems;
}

function missingProblems(sources) {
  const required = [HEADER, ...TOP_FILES.map(([, rel]) => rel), ...LICENCE_FILES.map(([, rel]) => rel)];
  return required.filter(rel => !sources.has(rel)).map(rel => `${rel}: required file is missing`);
}

function resolveVersion(requested, sources, problems) {
  const header = sources.get(HEADER);
  const headerVersion = header === undefined ? undefined : attempt(problems, () => readVersion(header.toString('utf8')));
  const version = requested ?? headerVersion;
  if (version === undefined) return undefined;
  const label = requested === undefined ? HEADER : '--version';
  if (!SEMVER.test(version)) problems.push(`${label}: must be MAJOR.MINOR.PATCH digits only, without a leading v`);
  if (requested !== undefined && headerVersion !== undefined && requested !== headerVersion) {
    problems.push(`--version: does not match WIIUNX2_VERSION in ${HEADER}`);
  }
  return version;
}

function validationFor(file, rpx, problems) {
  if (file === undefined) {
    problems.push('--validation: is required');
    return undefined;
  }
  if (rpx === undefined) return undefined;
  return attempt(problems, () => checkValidation(path.resolve(file), rpx.sha256));
}

function portlibDir(opts) {
  if (opts.portlibLicenses !== undefined) return path.resolve(opts.portlibLicenses);
  return process.env.DEVKITPRO ? path.join(process.env.DEVKITPRO, 'portlibs/ppc/licenses') : undefined;
}

function readPortlibFolder(dir, name, problems) {
  if (!isDirectory(path.join(dir, name))) {
    problems.push(`--portlib-licenses: folder ${name} is missing`);
    return [];
  }
  const acc = {files: [], generated: 0, problems: []};
  walkFiles(dir, name, () => null, acc);
  problems.push(...acc.problems);
  if (acc.files.length === 0) problems.push(`--portlib-licenses: folder ${name} has no files`);
  return acc.files.map(rel => ({path: `licenses/${rel}`, data: fs.readFileSync(path.join(dir, rel))}));
}

function readPortlibLicences(dir, problems) {
  if (dir === undefined) {
    problems.push('--portlib-licenses: is required when the DEVKITPRO variable is not set');
    return [];
  }
  return PORTLIB_NAMES.flatMap(name => readPortlibFolder(dir, name, problems));
}

function isInside(child, parent) {
  const relative = path.relative(parent, child);
  return relative === '' || (relative.split(path.sep)[0] !== '..' && !path.isAbsolute(relative));
}

function releaseNames(version) {
  const zip = `WiiUNX2-menu-v${version}.zip`;
  return {zip, sha: `${zip}.sha256`};
}

function checkOutput(root, out, version, problems) {
  if (isInside(out, root) && !isInside(out, path.join(root, '.test-output'))) {
    problems.push('--out: must be outside the source tree or under .test-output');
  }
  if (version === undefined) return;
  const names = releaseNames(version);
  for (const name of [names.zip, names.sha]) {
    if (fs.existsSync(path.join(out, name))) problems.push(`--out: ${name} already exists`);
  }
}

function git(root, args) {
  const result = spawnSync('git', ['-C', root, ...args], {encoding: 'utf8', shell: false, windowsHide: true});
  return result.error || result.status !== 0 ? null : result.stdout;
}

function canonicalPath(file) {
  const real = fs.realpathSync(file);
  return process.platform === 'win32' ? real.replaceAll('\\', '/').toLowerCase() : real;
}

function isGitTopLevel(root) {
  const top = git(root, ['rev-parse', '--show-toplevel']);
  if (top === null) return false;
  try {
    return canonicalPath(top.trim()) === canonicalPath(root);
  } catch {
    return false;
  }
}

function readGitState(root) {
  const unknown = {sourceCommit: null, sourceTreeDirty: null, dirtyPaths: [], ignoredPaths: []};
  if (!isGitTopLevel(root)) return unknown;
  const status = git(root, ['status', '--porcelain']);
  if (status === null) return unknown;
  const head = git(root, ['rev-parse', 'HEAD']);
  const dirtyPaths = status.split(/\r?\n/).filter(line => line !== '').map(line => line.slice(3));
  const ignored = git(root, ['ls-files', '-z', '--others', '--ignored', '--exclude-standard']);
  const ignoredPaths = ignored === null ? [] : ignored.split('\0').filter(rel => rel !== '');
  return {sourceCommit: head === null ? null : head.trim(), sourceTreeDirty: dirtyPaths.length > 0, dirtyPaths, ignoredPaths};
}

function gitState(root, allowDirty, files, problems) {
  const state = readGitState(root);
  const ignored = new Set(state.ignoredPaths);
  for (const rel of files) if (ignored.has(rel)) problems.push(`${rel}: ignored by .gitignore, so it is not part of the reviewed source`);
  if (state.sourceTreeDirty === true && !allowDirty) {
    problems.push(`working tree is not clean; uncommitted paths: ${state.dirtyPaths.slice(0, 20).join(', ')} (commit or remove them, or pass --allow-dirty)`);
  }
  return {sourceCommit: state.sourceCommit, sourceTreeDirty: state.sourceTreeDirty};
}

function gatherInputs(opts, problems) {
  const root = path.resolve(opts.root ?? DEFAULT_ROOT);
  if (!isDirectory(root)) throw inputProblem('--root: must be an existing folder');
  const tree = scanTree(root, problems);
  const sources = readFiles(root, tree.files);
  problems.push(...missingProblems(sources), ...privateSourceProblems(sources));
  const version = resolveVersion(opts.version, sources, problems);
  const rpx = attempt(problems, () => checkRpx(path.resolve(opts.rpx ?? path.join(root, 'launcher/men.rpx'))));
  const validation = validationFor(opts.validation, rpx, problems);
  const portlibFiles = readPortlibLicences(portlibDir(opts), problems);
  const out = path.resolve(opts.out ?? path.join(root, '.test-output/release'));
  checkOutput(root, out, version, problems);
  const homePaths = rpx === undefined ? 0 : homePathCount(rpx.data.toString('latin1'));
  if (homePaths > 0 && opts.allowRpxHomePaths !== true) {
    problems.push(`men.rpx contains ${homePaths} home-directory-like string(s); rebuild it from a neutral path (for example in the build container) or pass --allow-rpx-home-paths`);
  }
  const git = gitState(root, opts.allowDirty === true, tree.files, problems);
  return {
    version,
    out,
    rpx,
    validationText: validation,
    sources,
    portlibFiles,
    generated: tree.generated,
    git,
    homePathCount: homePaths,
  };
}

function renderInstall(version) {
  return `# WiiUNX2 menu v${version}: install and rollback

This package contains the WiiUNX2 menu binary (men.rpx) for Aroma on Wii U.
It is an unsigned build and is not official Nintendo software. It was not run
on a console unless VALIDATION.md says so.

## 1. Verify the package

From the package folder, on Linux or macOS:

    sha256sum -c SHA256SUMS

On Windows PowerShell:

    Get-FileHash -Algorithm SHA256 .\\men.rpx

Compare the hash with the first field of men.rpx.sha256. Case does not matter.
Every line printed by sha256sum must end with OK.

## 2. Install an isolated test environment

Insert the SD card, then run from the package folder:

    powershell -ExecutionPolicy Bypass -File .\\Install-SD.ps1 -SDRoot E:\\

This creates wiiu/environments/aroma-wiiunx2 as a copy of the stock aroma
environment (its plugins folder is not copied; an empty one is created) with
this men.rpx. The stock aroma environment is not changed.
The stock environment must contain modules/setup/00_mocha.rpx.
When EnvironmentLoader starts, hold X and choose aroma-wiiunx2.

## 3. Update an existing test environment

    powershell -ExecutionPolicy Bypass -File .\\Update-SD.ps1 -SDRoot E:\\ -Environment aroma-wiiunx2

The target environment must already contain modules/setup/00_mocha.rpx.
Update-SD.ps1 first copies the current men.rpx to men.rpx.backup-<date>-<id>,
then replaces the file and checks the new hash. Plugins and modules are not changed.

## 4. Manual copy (advanced)

sd-layout/wiiu/environments/aroma-wiiunx2/men.rpx is only the binary. Use it only
when the target environment already exists and contains modules/setup/00_mocha.rpx.
Copy any existing men.rpx in that environment to a backup name first, then copy
this file over it, then check the hash as in section 1.

## 5. Rollback

1. Power off the console before you change files on the SD card.
2. Boot the original aroma environment. It is the default; keep it that way.
3. To restore an earlier binary, copy a men.rpx.backup-* file back to men.rpx in the same environment.
4. Delete wiiu/environments/aroma-wiiunx2 only after power off, and only when you no longer need it.
5. Never edit NAND, system files or menu titles. This package does not touch them.

## 6. Contents

- RELEASE-INFO.json: version, binary hash and size, source commit (or null), tree state and file counts.
- source/: the publication-checked sources and notices, under their repository paths.
- licenses/: licence texts for bundled third-party code and the devkitPro portlibs.
`;
}

function renderValidation(inputs, fileCount) {
  return [
    `# Validation record for WiiUNX2 menu v${inputs.version}`,
    '',
    '## Facts checked by the packager',
    '',
    '- The source publication check passed on the packaged source tree.',
    `- Files in this package: ${fileCount} (SHA256SUMS is not counted).`,
    `- men.rpx SHA-256: ${inputs.rpx.sha256}`,
    `- men.rpx size: ${inputs.rpx.bytes} bytes`,
    `- Home-directory-like strings in men.rpx: ${inputs.homePathCount}`,
    '- Archives built from identical inputs are byte-identical. The package stores no timestamps.',
    '',
    '## Operator-supplied validation',
    '',
    inputs.validationText,
    '',
    'The packager cannot verify the operator-supplied text above. It only checked that the text',
    'contains the men.rpx SHA-256 and no private path or token pattern.',
    '',
  ].join('\n');
}

function renderReleaseInfo(inputs, fileCount) {
  return `${JSON.stringify({
    version: inputs.version,
    rpxSha256: inputs.rpx.sha256,
    rpxBytes: inputs.rpx.bytes,
    sourceCommit: inputs.git.sourceCommit,
    sourceTreeDirty: inputs.git.sourceTreeDirty,
    rpxHomePathStrings: inputs.homePathCount,
    fileCount,
    generatedExcluded: inputs.generated,
    formatVersion: FORMAT_VERSION,
  }, null, 2)}\n`;
}

function renderSha256Sums(files) {
  return [...files]
    .sort((a, b) => bytewise(a.path, b.path))
    .map(file => `${sha256Hex(file.data)}  ${file.path}\n`)
    .join('');
}

function baseFiles(inputs) {
  const rpxFiles = [
    {path: 'men.rpx', data: inputs.rpx.data},
    {path: 'men.rpx.sha256', data: text(`${inputs.rpx.sha256}  men.rpx\n`)},
    {path: 'sd-layout/wiiu/environments/aroma-wiiunx2/men.rpx', data: inputs.rpx.data},
  ];
  const top = TOP_FILES.map(([name, rel]) => ({path: name, data: inputs.sources.get(rel)}));
  const licences = LICENCE_FILES.map(([name, rel]) => ({path: `licenses/${name}`, data: inputs.sources.get(rel)}));
  const source = [...inputs.sources].map(([rel, data]) => ({path: `source/${rel}`, data}));
  return [...rpxFiles, ...top, ...licences, ...inputs.portlibFiles, ...source];
}

function documentFiles(inputs, fileCount) {
  return [
    {path: 'INSTALL.md', data: text(renderInstall(inputs.version))},
    {path: 'VALIDATION.md', data: text(renderValidation(inputs, fileCount))},
    {path: 'RELEASE-INFO.json', data: text(renderReleaseInfo(inputs, fileCount))},
  ];
}

function packageEntries(inputs) {
  const base = baseFiles(inputs);
  const described = [...base, ...documentFiles(inputs, base.length + 3)];
  const sums = {path: 'SHA256SUMS', data: text(renderSha256Sums(described))};
  const prefix = `WiiUNX2-menu-v${inputs.version}/`;
  return [...described, sums].map(file => ({name: `${prefix}${file.path}`, data: file.data}));
}

function runPublicationCheck(staging, sources) {
  const source = path.join(staging, 'source');
  for (const [rel, data] of sources) {
    const file = path.join(source, rel);
    fs.mkdirSync(path.dirname(file), {recursive: true});
    fs.writeFileSync(file, data, {flag: 'wx'});
  }
  const result = spawnSync(process.execPath, [CHECKER, source], {encoding: 'utf8', shell: false, windowsHide: true});
  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new ReleaseInputError(['source publication check failed:', ...result.stderr.split(/\r?\n/).filter(line => line !== '')]);
  }
}

function writeRelease(inputs, entries) {
  fs.mkdirSync(inputs.out, {recursive: true});
  const names = releaseNames(inputs.version);
  const archive = path.join(inputs.out, names.zip);
  const digest = writeStoredZip(archive, entries);
  const shaFile = path.join(inputs.out, names.sha);
  writeNewFile(shaFile, text(`${digest}  ${names.zip}\n`));
  const warnings = inputs.homePathCount > 0
    ? [`men.rpx contains ${inputs.homePathCount} home-directory-like string(s); recorded in RELEASE-INFO.json and VALIDATION.md`]
    : [];
  return {archive, archiveSha256File: shaFile, fileCount: entries.length - 1, generatedExcluded: inputs.generated, warnings};
}

export function buildRelease(opts = {}) {
  const problems = [];
  const inputs = gatherInputs(opts, problems);
  if (problems.length > 0) throw new ReleaseInputError(problems);
  const entries = packageEntries(inputs);
  problems.push(...entryProblems(entries));
  if (problems.length > 0) throw new ReleaseInputError(problems);
  const staging = fs.mkdtempSync(path.join(os.tmpdir(), 'wiiunx2-release-'));
  try {
    runPublicationCheck(staging, inputs.sources);
    return {...writeRelease(inputs, entries), version: inputs.version};
  } finally {
    fs.rmSync(staging, {recursive: true, force: true, maxRetries: 3});
  }
}

function parseArgs(argv) {
  const opts = {};
  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    if (arg === '--allow-dirty' || arg === '--allow-rpx-home-paths') {
      opts[arg === '--allow-dirty' ? 'allowDirty' : 'allowRpxHomePaths'] = true;
      continue;
    }
    const key = VALUE_OPTIONS.get(arg);
    if (key === undefined || index + 1 >= argv.length) throw new UsageError(`unknown option or missing value: ${arg}`);
    opts[key] = argv[index + 1];
    index += 1;
  }
  if (opts.validation === undefined) throw new UsageError('--validation is required');
  return opts;
}

function printResult(result) {
  for (const warning of result.warnings) console.error(`warning: ${warning}`);
  console.log(`Wrote ${result.archive}`);
  console.log(`Wrote ${result.archiveSha256File}`);
  console.log(`Files: ${result.fileCount}; generated paths excluded: ${result.generatedExcluded}`);
}

function runCli(argv) {
  let opts;
  try {
    opts = parseArgs(argv);
  } catch (error) {
    if (!(error instanceof UsageError)) throw error;
    console.error(`${USAGE}\n${error.message}`);
    return 2;
  }
  try {
    printResult(buildRelease(opts));
    return 0;
  } catch (error) {
    if (error instanceof ReleaseInputError) console.error(error.problems.join('\n'));
    else console.error(`error: ${error.message}`);
    return 1;
  }
}

function isDirectRun() {
  if (process.argv[1] === undefined) return false;
  try {
    return fs.realpathSync(fileURLToPath(import.meta.url)) === fs.realpathSync(process.argv[1]);
  } catch {
    return false;
  }
}

if (isDirectRun()) process.exitCode = runCli(process.argv.slice(2));
