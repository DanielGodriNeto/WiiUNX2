// WiiUNX2 GPL-3.0-or-later. Tests for the reproducible release packager. Fixtures are built in the OS temp directory.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import crypto from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {fileURLToPath, pathToFileURL} from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const toolPath = path.join(repo, 'launcher/tools/Package-Release.mjs');
const tool = await import(pathToFileURL(toolPath).href);
const {ReleaseInputError} = tool;
const GIT_AVAILABLE = spawnSync('git', ['--version'], {encoding: 'utf8'}).status === 0;
const VERSION = '0.6.11';
const ZIP = `WiiUNX2-menu-v${VERSION}.zip`;
const PREFIX = `WiiUNX2-menu-v${VERSION}/`;
const HEADER = 'launcher/src/common/common.h';
const SENTINEL = 'sentinel-release-test-value';
const PORTLIB_NAMES = ['ppc-freetype', 'ppc-harfbuzz', 'ppc-libgd', 'ppc-libpng', 'ppc-libjpeg-turbo', 'ppc-libmad', 'ppc-libvorbisidec', 'ppc-libogg', 'ppc-bzip2', 'ppc-brotli', 'ppc-zlib'];
const TOP_NAMES = ['README.md', 'LICENSE', 'SECURITY.md', 'SOUNDS.md', 'THIRD-PARTY.md', 'UPSTREAM-REVISIONS.txt', 'COMPATIBILITY.md', 'Install-SD.ps1', 'Update-SD.ps1'];
const LICENCE_NAMES = ['NotoSans-OFL.txt', 'launcher-LICENSE.txt', 'libgui-COPYING.txt', 'librpxloader-LICENSE.txt', 'libwuhbutils-LICENSE.txt', 'nlohmann-json-LICENSE.txt'];
const TEXT_FILES = {
  [HEADER]: '#pragma once\r\n#define WIIUNX2_VERSION    "0.6.11"\r\n',
  'launcher/src/main.cpp': 'int main() { return 0; }\n',
  'launcher/src/utils/nlohmann-json-LICENSE.txt': 'MIT licence text\n',
  'launcher/LICENSE': 'GPL licence text\n',
  'launcher/licenses/NotoSans-OFL.txt': 'OFL licence text\n',
  'launcher/THIRD-PARTY.md': 'Third party notices\n',
  'launcher/UPSTREAM-REVISIONS.txt': 'Upstream revisions\n',
  'launcher/COMPATIBILITY.md': 'Compatibility notes\n',
  'launcher/Install-SD.ps1': 'param()\n',
  'launcher/Update-SD.ps1': 'param()\n',
  'README.md': 'Readme\n',
  LICENSE: 'Project licence\n',
  'SECURITY.md': 'Security policy\n',
  'SOUNDS.md': 'Sounds notes\n',
  'dependencies/libgui/COPYING': 'LGPL text\n',
  'dependencies/librpxloader/LICENSE': 'LGPL text\n',
  'dependencies/libwuhbutils/LICENSE': 'LGPL text\n',
};
const SOURCE_FILES = [...Object.keys(TEXT_FILES), 'launcher/data/fonts/font.ttf'];

function sha256(data) {
  return crypto.createHash('sha256').update(data).digest('hex');
}

function elfBytes(extra = Buffer.alloc(0)) {
  return Buffer.concat([Buffer.from([0x7f, 0x45, 0x4c, 0x46, 1, 2, 1, 0]), Buffer.alloc(56), extra]);
}

function writeFile(base, rel, data) {
  const file = path.join(base, rel);
  fs.mkdirSync(path.dirname(file), {recursive: true});
  fs.writeFileSync(file, data);
  return file;
}

function validationFor(rpxSha) {
  return `Validated men.rpx ${rpxSha}\nConsole: not tested\n`;
}

function makeFixture(work, rpxBytes = elfBytes()) {
  const root = path.join(work, 'root');
  for (const [rel, text] of Object.entries(TEXT_FILES)) writeFile(root, rel, text);
  writeFile(root, 'launcher/data/fonts/font.ttf', fs.readFileSync(path.join(repo, 'launcher/data/fonts/font.ttf')));
  writeFile(root, 'launcher/men.rpx', rpxBytes);
  const validation = path.join(work, 'validation.txt');
  fs.writeFileSync(validation, validationFor(sha256(rpxBytes)));
  const portlibs = path.join(work, 'portlibs');
  for (const name of PORTLIB_NAMES) writeFile(portlibs, `${name}/LICENSE.txt`, `${name} licence\n`);
  return {work, root, portlibs, validation, out: path.join(work, 'out')};
}

// Each test gets a private TMPDIR/TMP/TEMP, so other processes writing wiiunx2-release-* dirs cannot change the assertions.
function useTempRoot(dir) {
  const saved = {TMPDIR: process.env.TMPDIR, TMP: process.env.TMP, TEMP: process.env.TEMP};
  for (const key of Object.keys(saved)) process.env[key] = dir;
  return () => {
    for (const [key, value] of Object.entries(saved)) {
      if (value === undefined) delete process.env[key];
      else process.env[key] = value;
    }
  };
}

async function withWork(run, rpxBytes) {
  const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'wiiunx2-test-tmp-'));
  const restoreTemp = useTempRoot(tempRoot);
  try {
    const work = fs.mkdtempSync(path.join(tempRoot, 'work-'));
    return await run(makeFixture(work, rpxBytes));
  } finally {
    restoreTemp();
    fs.rmSync(tempRoot, {recursive: true, force: true, maxRetries: 3});
  }
}

function standardArgs(fx) {
  return ['--root', fx.root, '--validation', fx.validation, '--portlib-licenses', fx.portlibs, '--out', fx.out];
}

function runTool(args, {cwd = os.tmpdir(), env = process.env} = {}) {
  return spawnSync(process.execPath, [toolPath, ...args], {cwd, env, encoding: 'utf8', shell: false, windowsHide: true});
}

function releaseTempEntries() {
  return fs.readdirSync(os.tmpdir()).filter(name => name.startsWith('wiiunx2-release-')).sort();
}

function outNames(out) {
  return fs.existsSync(out) ? fs.readdirSync(out) : [];
}

function runRefused(args, options) {
  const before = releaseTempEntries();
  const result = runTool(args, options);
  assert.equal(result.status, 1, `expected exit 1, got ${result.status}: ${result.stderr}`);
  assert.deepEqual(releaseTempEntries(), before, 'no wiiunx2-release temp dir may remain');
  return result;
}

function assertNoArchive(out) {
  assert.deepEqual(outNames(out).filter(name => /\.(zip|partial|sha256)$/.test(name)), []);
}

function problemsOf(fn) {
  try {
    fn();
  } catch (error) {
    assert.ok(error instanceof ReleaseInputError, `unexpected error: ${error.message}`);
    return error.problems.join('\n');
  }
  assert.fail('expected the input to be refused');
}

function symlinkOrSkip(t, target, link) {
  try {
    fs.symlinkSync(target, link, 'file');
    return true;
  } catch (error) {
    if (['EPERM', 'EACCES', 'ENOSYS', 'ENOTSUP'].includes(error.code)) {
      t.skip(`symlink not permitted here (${error.code})`);
      return false;
    }
    throw error;
  }
}

function bitwiseCrc32(buffer) {
  let crc = 0xffffffff;
  for (const byte of buffer) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) crc = (crc & 1) ? (crc >>> 1) ^ 0xedb88320 : crc >>> 1;
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function readLocal(buffer, central) {
  const at = central.localOffset;
  assert.equal(buffer.readUInt32LE(at), 0x04034b50, 'local header signature');
  assert.equal(buffer.readUInt16LE(at + 4), 10, 'local version needed');
  assert.equal(buffer.readUInt16LE(at + 6), 0, 'local flags');
  assert.equal(buffer.readUInt16LE(at + 8), 0, 'local method is stored');
  assert.equal(buffer.readUInt16LE(at + 12), 0x0021, 'local DOS date');
  assert.equal(buffer.readUInt32LE(at + 14), central.crc, 'local and central CRC agree');
  assert.equal(buffer.readUInt32LE(at + 18), central.size, 'local and central compressed size agree');
  assert.equal(buffer.readUInt32LE(at + 22), central.size, 'local and central uncompressed size agree');
  const nameLength = buffer.readUInt16LE(at + 26);
  const extraLength = buffer.readUInt16LE(at + 28);
  assert.equal(buffer.toString('ascii', at + 30, at + 30 + nameLength), central.name, 'local and central names agree');
  const start = at + 30 + nameLength + extraLength;
  const data = buffer.subarray(start, start + central.size);
  assert.equal(bitwiseCrc32(data), central.crc, `CRC-32 of ${central.name}`);
  return data;
}

function readCentral(buffer, pos) {
  assert.equal(buffer.readUInt32LE(pos), 0x02014b50, 'central header signature');
  assert.equal(buffer.readUInt16LE(pos + 8), 0, 'central flags');
  assert.equal(buffer.readUInt16LE(pos + 10), 0, 'central method is stored');
  assert.equal(buffer.readUInt16LE(pos + 14), 0x0021, 'central DOS date');
  const nameLength = buffer.readUInt16LE(pos + 28);
  assert.equal(buffer.readUInt16LE(pos + 30), 0, 'central extra length');
  return {
    crc: buffer.readUInt32LE(pos + 16),
    size: buffer.readUInt32LE(pos + 20),
    localOffset: buffer.readUInt32LE(pos + 42),
    name: buffer.toString('ascii', pos + 46, pos + 46 + nameLength),
    next: pos + 46 + nameLength,
  };
}

// Independent reader: parses the end record and central directory itself, verifies every CRC bitwise.
function readZip(buffer) {
  const end = buffer.length - 22;
  assert.equal(buffer.readUInt32LE(end), 0x06054b50, 'end record at the end of the archive');
  const count = buffer.readUInt16LE(end + 10);
  const dirSize = buffer.readUInt32LE(end + 12);
  const dirOffset = buffer.readUInt32LE(end + 16);
  assert.equal(dirOffset + dirSize, end, 'central directory ends at the end record');
  const entries = [];
  let pos = dirOffset;
  for (let index = 0; index < count; index += 1) {
    const central = readCentral(buffer, pos);
    entries.push({name: central.name, data: readLocal(buffer, central)});
    pos = central.next;
  }
  assert.equal(pos, end, 'no bytes between the central entries and the end record');
  return entries;
}

function zipMap(zipBuffer) {
  return new Map(readZip(zipBuffer).map(entry => [entry.name, entry.data]));
}

function expectedNames() {
  const licences = LICENCE_NAMES.map(name => `${PREFIX}licenses/${name}`);
  const portlibs = PORTLIB_NAMES.map(name => `${PREFIX}licenses/${name}/LICENSE.txt`);
  const source = SOURCE_FILES.filter(rel => rel !== 'launcher/men.rpx').map(rel => `${PREFIX}source/${rel}`);
  return [
    ...['men.rpx', 'men.rpx.sha256', 'SHA256SUMS', 'RELEASE-INFO.json', 'INSTALL.md', 'VALIDATION.md',
      'sd-layout/wiiu/environments/aroma-wiiunx2/men.rpx'].map(name => PREFIX + name),
    ...TOP_NAMES.map(name => PREFIX + name),
    ...licences, ...portlibs, ...source,
  ].sort((a, b) => Buffer.compare(Buffer.from(a), Buffer.from(b)));
}

function touchAll(dir, when) {
  for (const entry of fs.readdirSync(dir, {withFileTypes: true})) {
    const file = path.join(dir, entry.name);
    if (entry.isDirectory()) touchAll(file, when);
    else fs.utimesSync(file, when, when);
  }
}

function git(root, args) {
  const result = spawnSync('git', ['-C', root, ...args], {encoding: 'utf8', shell: false, windowsHide: true});
  assert.equal(result.status, 0, `git ${args[0]} failed`);
  return result.stdout;
}

function commitAll(root) {
  git(root, ['init', '-q']);
  git(root, ['add', '-A']);
  git(root, ['-c', 'user.name=t', '-c', 'user.email=t@example.invalid', 'commit', '-q', '-m', 'fixture']);
}

test('T1 happy path: archive, checksums and records are complete', async () => {
  await withWork(async fx => {
    const result = runTool(standardArgs(fx));
    assert.equal(result.status, 0, result.stderr);
    const zip = fs.readFileSync(path.join(fx.out, ZIP));
    const files = zipMap(zip);
    assert.deepEqual([...files.keys()], expectedNames());
    for (const rel of SOURCE_FILES.filter(name => name !== 'launcher/men.rpx')) {
      assert.ok(files.get(`${PREFIX}source/${rel}`).equals(fs.readFileSync(path.join(fx.root, rel))), `source bytes of ${rel}`);
    }
    assert.equal(fs.readFileSync(path.join(fx.out, `${ZIP}.sha256`), 'utf8'), `${sha256(zip)}  ${ZIP}\n`);
    const rpxSha = sha256(elfBytes());
    assert.equal(files.get(`${PREFIX}men.rpx.sha256`).toString('utf8'), `${rpxSha}  men.rpx\n`);
    assertSha256Sums(files);
    assertReleaseInfo(files, rpxSha);
    assert.ok(files.get(`${PREFIX}VALIDATION.md`).toString('utf8').includes(validationFor(rpxSha)));
    assertInstall(files.get(`${PREFIX}INSTALL.md`).toString('utf8'));
  });
});

function assertSha256Sums(files) {
  const sums = files.get(`${PREFIX}SHA256SUMS`).toString('utf8');
  assert.ok(sums.endsWith('\n') && !sums.includes('\r'), 'LF-only checksum file');
  const lines = sums.trimEnd().split('\n').map(line => line.split('  '));
  const listed = lines.map(([, rel]) => rel);
  const expected = [...files.keys()].filter(name => name !== `${PREFIX}SHA256SUMS`).map(name => name.slice(PREFIX.length));
  assert.deepEqual(listed, expected.sort((a, b) => Buffer.compare(Buffer.from(a), Buffer.from(b))));
  for (const [hash, rel] of lines) assert.equal(sha256(files.get(PREFIX + rel)), hash, `checksum of ${rel}`);
}

function assertReleaseInfo(files, rpxSha) {
  const info = JSON.parse(files.get(`${PREFIX}RELEASE-INFO.json`).toString('utf8'));
  assert.deepEqual(Object.keys(info), ['version', 'rpxSha256', 'rpxBytes', 'sourceCommit', 'sourceTreeDirty', 'rpxHomePathStrings', 'fileCount', 'generatedExcluded', 'formatVersion']);
  assert.equal(info.version, VERSION);
  assert.equal(info.rpxSha256, rpxSha);
  assert.equal(info.rpxBytes, 64);
  assert.equal(info.sourceCommit, null);
  assert.equal(info.sourceTreeDirty, null);
  assert.equal(info.rpxHomePathStrings, 0);
  assert.equal(info.fileCount, files.size - 1);
  assert.equal(info.generatedExcluded, 1);
  assert.equal(info.formatVersion, 1);
}

function assertInstall(text) {
  for (const needle of ['Install-SD.ps1', 'Update-SD.ps1', '-Environment aroma-wiiunx2', 'sha256sum -c SHA256SUMS', 'men.rpx.backup-']) {
    assert.ok(text.includes(needle), `INSTALL.md mentions ${needle}`);
  }
  assert.match(text, /rollback/i);
}

test('T2 determinism: same inputs give identical bytes, regardless of mtimes and cwd', async () => {
  await withWork(async fx => {
    const outA = path.join(fx.work, 'a');
    const outB = path.join(fx.work, 'b');
    assert.equal(runTool([...standardArgs({...fx, out: outA})], {cwd: os.tmpdir()}).status, 0);
    touchAll(fx.root, new Date('2001-02-03T04:05:06Z'));
    assert.equal(runTool([...standardArgs({...fx, out: outB})], {cwd: os.tmpdir()}).status, 0);
    for (const name of [ZIP, `${ZIP}.sha256`]) {
      assert.ok(fs.readFileSync(path.join(outA, name)).equals(fs.readFileSync(path.join(outB, name))), `${name} identical`);
    }
    writeFile(fx.root, 'launcher/src/main.cpp', 'int main() { return 1; }\n');
    const outC = path.join(fx.work, 'c');
    assert.equal(runTool([...standardArgs({...fx, out: outC})], {cwd: os.tmpdir()}).status, 0);
    assert.notDeepEqual(fs.readFileSync(path.join(outC, ZIP)), fs.readFileSync(path.join(outA, ZIP)));
  });
});

test('T3 each missing input is refused and named; all of them are reported together', async () => {
  const cases = [
    ['--rpx', fx => ['--rpx', path.join(fx.work, 'missing.rpx')]],
    ['--validation', fx => ['--validation', path.join(fx.work, 'missing.txt')]],
    ['ppc-zlib', fx => { fs.rmSync(path.join(fx.portlibs, 'ppc-zlib'), {recursive: true}); return []; }],
    ['launcher/LICENSE', fx => { fs.rmSync(path.join(fx.root, 'launcher/LICENSE')); return []; }],
    [HEADER, fx => { fs.rmSync(path.join(fx.root, HEADER)); return []; }],
  ];
  for (const [culprit, prepare] of cases) {
    await withWork(async fx => {
      const extra = prepare(fx);
      const result = runRefused([...standardArgs(fx), ...extra]);
      assert.ok(result.stderr.includes(culprit), `stderr names ${culprit}: ${result.stderr}`);
      assertNoArchive(fx.out);
    });
  }
});

test('T3 DEVKITPRO unset without --portlib-licenses names the option and the variable', async () => {
  await withWork(async fx => {
    const env = Object.fromEntries(Object.entries(process.env).filter(([key]) => key.toUpperCase() !== 'DEVKITPRO'));
    const args = standardArgs(fx);
    args.splice(args.indexOf('--portlib-licenses'), 2);
    const result = runRefused(args, {env});
    assert.ok(result.stderr.includes('--portlib-licenses'));
    assert.ok(result.stderr.includes('DEVKITPRO'));
    assertNoArchive(fx.out);
  });
});

test('T3b two missing inputs produce one message that lists both', async () => {
  await withWork(async fx => {
    fs.rmSync(path.join(fx.root, 'launcher/LICENSE'));
    fs.rmSync(path.join(fx.root, 'README.md'));
    const result = runRefused(standardArgs(fx));
    assert.ok(result.stderr.includes('launcher/LICENSE'));
    assert.ok(result.stderr.includes('README.md'));
    assertNoArchive(fx.out);
  });
});

test('T4 validation text: each defect is refused and named without echoing content', async () => {
  const rpxSha = sha256(elfBytes());
  const homePath = 'C:' + '\\Users\\' + 'someone\\x';
  const cases = {
    empty: Buffer.alloc(0),
    tooLarge: Buffer.alloc(64 * 1024 + 1, 0x61),
    invalidUtf8: Buffer.from([0xff, 0xfe, 0x41, 0x0a]),
    homePath: Buffer.from(`${validationFor(rpxSha)}${homePath}\n`),
    nulByte: Buffer.from(`${validationFor(rpxSha)}\u0000\n`),
    lacksRpxSha: Buffer.from('Validated an older binary\n'),
  };
  for (const [label, bytes] of Object.entries(cases)) {
    await withWork(async fx => {
      const file = writeFile(fx.work, `${label}.txt`, bytes);
      const text = problemsOf(() => tool.checkValidation(file, rpxSha));
      assert.match(text, /--validation/, label);
      assert.doesNotMatch(text, /someone/, label);
    });
  }
});

test('T5 rpx: empty, directory, ELF64, little-endian, non-ELF and oversized are refused', async t => {
  await withWork(async fx => {
    const dir = path.join(fx.work, 'dir.rpx');
    fs.mkdirSync(dir);
    const big = path.join(fx.work, 'big.rpx');
    fs.writeFileSync(big, elfBytes());
    fs.truncateSync(big, 256 * 1024 * 1024 + 1);
    const cases = {
      'empty.rpx': Buffer.alloc(0),
      'elf64.rpx': Buffer.from([0x7f, 0x45, 0x4c, 0x46, 2, 2, 1, 0]),
      'little.rpx': Buffer.from([0x7f, 0x45, 0x4c, 0x46, 1, 1, 1, 0]),
      'text.rpx': Buffer.from('not an executable\n'),
    };
    for (const [name, bytes] of Object.entries(cases)) {
      const file = writeFile(fx.work, name, bytes);
      assert.match(problemsOf(() => tool.checkRpx(file)), /--rpx/, name);
    }
    assert.match(problemsOf(() => tool.checkRpx(dir)), /--rpx/, 'directory');
    assert.match(problemsOf(() => tool.checkRpx(big)), /--rpx/, 'oversized');
    const link = path.join(fx.work, 'link.rpx');
    if (symlinkOrSkip(t, writeFile(fx.work, 'real.rpx', elfBytes()), link)) {
      assert.match(problemsOf(() => tool.checkRpx(link)), /--rpx/, 'symlink');
    }
  });
});

test('T5 rpx: a valid ELF32 big-endian file is accepted with its size and hash', async () => {
  await withWork(async fx => {
    const file = writeFile(fx.work, 'ok.rpx', elfBytes());
    const rpx = tool.checkRpx(file);
    assert.equal(rpx.bytes, 64);
    assert.equal(rpx.sha256, sha256(elfBytes()));
  });
});

test('T6 stray private files are refused, named, and their contents never printed', async t => {
  const pemLine = '-----BEGIN ' + 'PRIVATE KEY-----';
  const cases = [
    ['launcher/data/sounds/uiMove.bin', SENTINEL],
    ['dependencies/libgui/stray.pem', `${pemLine}\n${SENTINEL}\n`],
    ['launcher/wiiunx2-recent-0.txt', SENTINEL],
    ['.env', `TOKEN=${SENTINEL}\n`],
    ['launcher/src/leak.cpp', `const char *p = "${'C:' + '\\Users\\' + 'mallory\\x'}";\n`],
  ];
  for (const [rel, content] of cases) {
    await withWork(async fx => {
      writeFile(fx.root, rel, content);
      const result = runRefused(standardArgs(fx));
      assert.ok(result.stderr.includes(rel), `stderr names ${rel}`);
      assert.ok(!result.stderr.includes(SENTINEL) && !result.stderr.includes('mallory'), 'no content printed');
      assertNoArchive(fx.out);
    });
  }
  await withWork(async fx => {
    const link = path.join(fx.root, 'launcher/src/link.cpp');
    if (!symlinkOrSkip(t, path.join(fx.root, 'README.md'), link)) return;
    const result = runRefused(standardArgs(fx));
    assert.ok(result.stderr.includes('launcher/src/link.cpp'));
  });
});

test('T7 generated paths are excluded and counted; a stray object outside build is refused', async () => {
  await withWork(async fx => {
    for (const rel of ['launcher/build/x.o', 'launcher/src/resources/filelist.h', 'dependencies/libgui/release/x.a', 'dependencies/libgui/lib/libgui.a']) {
      writeFile(fx.root, rel, 'generated\n');
    }
    assert.equal(runTool(standardArgs(fx)).status, 0);
    const files = zipMap(fs.readFileSync(path.join(fx.out, ZIP)));
    assert.equal([...files.keys()].filter(name => /x\.o|filelist|\.a$|libgui\.a/.test(name)).length, 0);
    const info = JSON.parse(files.get(`${PREFIX}RELEASE-INFO.json`).toString('utf8'));
    assert.equal(info.generatedExcluded, 5);
  });
  await withWork(async fx => {
    writeFile(fx.root, 'launcher/src/x.o', 'stray\n');
    const result = runRefused(standardArgs(fx));
    assert.ok(result.stderr.includes('launcher/src/x.o'));
  });
});

test('T8 version: malformed, prefixed, pre-release or mismatched values are refused', async () => {
  for (const version of ['1.2', 'v0.1.0', '0.1.0-rc1', '0.1.0']) {
    await withWork(async fx => {
      const result = runRefused([...standardArgs(fx), '--version', version]);
      assert.ok(result.stderr.includes('--version'), version);
      assertNoArchive(fx.out);
    });
  }
  await withWork(async fx => {
    writeFile(fx.root, HEADER, `#define WIIUNX2_VERSION "${VERSION}"\n#define WIIUNX2_VERSION "${VERSION}"\n`);
    const result = runRefused(standardArgs(fx));
    assert.ok(result.stderr.includes(HEADER));
  });
});

test('T9 output: existing zip or checksum is refused and left unchanged; out inside launcher is refused', async () => {
  for (const existing of [ZIP, `${ZIP}.sha256`]) {
    await withWork(async fx => {
      writeFile(fx.out, existing, 'previous release\n');
      const result = runRefused(standardArgs(fx));
      assert.ok(result.stderr.includes(existing));
      assert.equal(fs.readFileSync(path.join(fx.out, existing), 'utf8'), 'previous release\n');
      assert.deepEqual(outNames(fx.out), [existing]);
    });
  }
  await withWork(async fx => {
    const result = runRefused(standardArgs({...fx, out: path.join(fx.root, 'launcher/release-out')}));
    assert.ok(result.stderr.includes('--out'));
    assert.equal(fs.existsSync(path.join(fx.root, 'launcher/release-out')), false);
  });
});

test('T10 names: non-ASCII, spaces and case-only duplicates are refused', async t => {
  const cases = [
    ['launcher/src/café.cpp', 'café.cpp'],
    ['launcher/src/a b.txt', 'a b.txt'],
  ];
  for (const [rel, culprit] of cases) {
    await withWork(async fx => {
      writeFile(fx.root, rel, 'name test\n');
      const result = runRefused(standardArgs(fx));
      assert.ok(result.stderr.includes(culprit), culprit);
      assertNoArchive(fx.out);
    });
  }
  await withWork(async fx => {
    writeFile(fx.root, 'launcher/src/Dup.txt', 'one\n');
    writeFile(fx.root, 'launcher/src/dup.txt', 'two\n');
    if (fs.readdirSync(path.join(fx.root, 'launcher/src')).filter(name => name.toLowerCase() === 'dup.txt').length < 2) {
      t.skip('file system folds case; duplicate names cannot exist here');
      return;
    }
    const result = runRefused(standardArgs(fx));
    assert.ok(result.stderr.includes('Dup.txt') && result.stderr.includes('dup.txt'));
  });
});

test('T11 writer: CRC-32 matches the check value and a bitwise reference; invalid entries are rejected', async () => {
  assert.equal(tool.crc32(Buffer.from('123456789')), 0xcbf43926);
  for (let round = 0; round < 5; round += 1) {
    const sample = crypto.randomBytes(round * 997);
    assert.equal(tool.crc32(sample), bitwiseCrc32(sample));
  }
  await withWork(async fx => {
    const file = path.join(fx.work, 'writer.zip');
    const many = Array.from({length: 65536}, (_, index) => ({name: `f${index}`, data: Buffer.alloc(0)}));
    const rejected = [
      many,
      [{name: 'a/../b', data: Buffer.alloc(1)}],
      [{name: '/abs', data: Buffer.alloc(1)}],
      [{name: 'a\\b', data: Buffer.alloc(1)}],
      [{name: 'A.txt', data: Buffer.alloc(1)}, {name: 'a.txt', data: Buffer.alloc(1)}],
    ];
    for (const entries of rejected) {
      assert.throws(() => tool.writeStoredZip(file, entries), ReleaseInputError);
    }
    assert.equal(fs.existsSync(file), false, 'nothing written for rejected entries');
    tool.writeStoredZip(file, [{name: 'b.txt', data: Buffer.from('bb')}, {name: 'a.txt', data: Buffer.from('a')}]);
    assert.deepEqual(readZip(fs.readFileSync(file)).map(entry => entry.name), ['a.txt', 'b.txt']);
  });
});

test('T12 readVersion: LF and CRLF accepted; zero or two defines refused', () => {
  assert.equal(tool.readVersion('#define WIIUNX2_VERSION "1.2.3"\n'), '1.2.3');
  assert.equal(tool.readVersion('#define WIIUNX2_VERSION    "1.2.3"\r\n'), '1.2.3');
  assert.throws(() => tool.readVersion('// no version here\n'), ReleaseInputError);
  assert.throws(() => tool.readVersion('#define WIIUNX2_VERSION "1.2.3"\n#define WIIUNX2_VERSION "1.2.4"\n'), ReleaseInputError);
});

test('T13 self-scan: the tool and this test contain no home-directory-like text', () => {
  const toolText = fs.readFileSync(toolPath, 'utf8');
  assert.equal(tool.privateFindings(toolText).includes('home directory path'), false);
  assert.equal(tool.privateFindings(fs.readFileSync(fileURLToPath(import.meta.url), 'utf8')).includes('home directory path'), false);
  assert.ok(tool.privateFindings('C:' + '\\Users\\' + 'tester\\x').includes('home directory path'), 'positive control');
  assert.ok(tool.privateFindings('/' + 'home' + '/' + 'tester' + '/x').includes('home directory path'), 'positive control, Unix form');
});

test('T14 cleanup: a failed build leaves no release temp directory behind', async () => {
  await withWork(async fx => {
    fs.rmSync(path.join(fx.root, 'launcher/LICENSE'));
    runRefused(standardArgs(fx));
  });
});

test('rpx home-path strings: refused by default; allowed with an explicit flag and recorded', async () => {
  const rpx = elfBytes(Buffer.from('C:' + '\\Users\\' + 'tester\\x'));
  await withWork(async fx => {
    const result = runRefused(standardArgs(fx));
    assert.match(result.stderr, /--allow-rpx-home-paths/);
    assert.ok(!result.stderr.includes('tester'), 'no content printed');
    assertNoArchive(fx.out);
    const allowed = runTool([...standardArgs(fx), '--allow-rpx-home-paths']);
    assert.equal(allowed.status, 0, allowed.stderr);
    assert.match(allowed.stderr, /warning/);
    const info = JSON.parse(zipMap(fs.readFileSync(path.join(fx.out, ZIP))).get(`${PREFIX}RELEASE-INFO.json`).toString('utf8'));
    assert.equal(info.rpxHomePathStrings, 1);
  }, rpx);
});

test('a private string in a file with an unchecked extension is refused without printing it', async () => {
  await withWork(async fx => {
    writeFile(fx.root, 'launcher/src/notes.ini', `path=${'C:' + '\\Users\\' + 'mallory\\x'}\n`);
    const result = runRefused(standardArgs(fx));
    assert.ok(result.stderr.includes('launcher/src/notes.ini'));
    assert.ok(!result.stderr.includes('mallory'));
  });
});

test('writeStoredZip never replaces an existing archive and removes its partial file', async () => {
  await withWork(async fx => {
    fs.mkdirSync(fx.out, {recursive: true});
    const target = path.join(fx.out, 'x.zip');
    fs.writeFileSync(target, 'original');
    assert.throws(() => tool.writeStoredZip(target, [{name: 'a/b.txt', data: Buffer.from('x')}]), {code: 'EEXIST'});
    assert.equal(fs.readFileSync(target, 'utf8'), 'original');
    assert.deepEqual(fs.readdirSync(fx.out), ['x.zip']);
  });
});

test('git: a gitignored file inside the source tree is refused', {skip: GIT_AVAILABLE ? false : 'git is not installed'}, async () => {
  await withWork(async fx => {
    writeFile(fx.root, '.gitignore', 'launcher/data/sounds/ui*.bin\n');
    commitAll(fx.root);
    writeFile(fx.root, 'launcher/data/sounds/uiSelect.bin', 'x');
    const result = runRefused(standardArgs(fx));
    assert.ok(result.stderr.includes('launcher/data/sounds/uiSelect.bin'));
    assertNoArchive(fx.out);
  });
});

test('git: a clean committed tree records the commit', {skip: GIT_AVAILABLE ? false : 'git is not installed'}, async () => {
  await withWork(async fx => {
    commitAll(fx.root);
    assert.equal(runTool(standardArgs(fx)).status, 0);
    const info = JSON.parse(zipMap(fs.readFileSync(path.join(fx.out, ZIP))).get(`${PREFIX}RELEASE-INFO.json`).toString('utf8'));
    assert.match(info.sourceCommit, /^[0-9a-f]{40}$/);
    assert.equal(info.sourceTreeDirty, false);
  });
});

test('git: an untracked file refuses the build unless --allow-dirty is given', {skip: GIT_AVAILABLE ? false : 'git is not installed'}, async () => {
  await withWork(async fx => {
    commitAll(fx.root);
    writeFile(fx.root, 'launcher/notes.txt', 'local notes\n');
    const result = runRefused(standardArgs(fx));
    assert.ok(result.stderr.includes('launcher/notes.txt'));
    assertNoArchive(fx.out);
    assert.equal(runTool([...standardArgs(fx), '--allow-dirty']).status, 0);
    const info = JSON.parse(zipMap(fs.readFileSync(path.join(fx.out, ZIP))).get(`${PREFIX}RELEASE-INFO.json`).toString('utf8'));
    assert.equal(info.sourceTreeDirty, true);
  });
});
