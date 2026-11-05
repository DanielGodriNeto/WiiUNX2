// WiiUNX2 GPL-3.0-or-later. Source-wiring check for the single version constant.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const read=file=>fs.readFileSync(path.join(root,file),'utf8');
const header=read('launcher/src/common/common.h');
test('common.h defines WIIUNX2_VERSION as x.y.z',()=>{
  assert.match(header,/^#define WIIUNX2_VERSION\s+"\d+\.\d+\.\d+"\r?$/m);
});
test('pause menu takes its version from WIIUNX2_VERSION, not a literal',()=>{
  const source=read('launcher/src/menu/MainWindow.cpp');
  assert.match(source,/#include "common\/common\.h"/);
  assert.match(source,/"WiiUNX2 " WIIUNX2_VERSION " experimental/);
  assert.doesNotMatch(source,/"[^"\n]*\d+\.\d+\.\d+[^"\n]*"/);
});
test('packaging default and SD message do not hardcode a version',()=>{
  const wrapper=read('launcher/tools/Package.ps1');
  assert.doesNotMatch(wrapper,/\d+\.\d+\.\d+/);
  assert.doesNotMatch(wrapper,/common\.h/);
  const packager=read('launcher/tools/Package-Release.mjs');
  assert.match(packager,/WIIUNX2_VERSION/);
  assert.match(packager,/common\.h/);
  assert.doesNotMatch(read('launcher/Update-SD.ps1'),/\d+\.\d+\.\d+/);
});
