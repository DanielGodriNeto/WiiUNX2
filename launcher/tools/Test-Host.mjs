// WiiUNX2 GPL-3.0-or-later. No console, SD card or network access.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const base=path.join(root,'.test-output');fs.mkdirSync(base,{recursive:true});
const output=fs.mkdtempSync(path.join(base,'host-'));
const compiler=process.env.CXX||(process.platform==='win32'&&fs.existsSync('C:/msys64/ucrt64/bin/g++.exe')?'C:/msys64/ucrt64/bin/g++.exe':'g++');
function run(command,args) {
  const result=spawnSync(command,args,{cwd:root,stdio:'inherit',shell:false});
  if(result.error) throw result.error;
  if(result.status!==0) throw Error(`${path.basename(command)} failed (${result.status??result.signal})`);
}
for(const name of ['navigation','library','stability','rounded','edgebleed','loading-selection','homebrew-policy','history-save','reconcile','metadata-cache','disc-state','icon-plan','launch-log','update-core','name-cache','sync-file','string-tools','selection-pulse']) {
  const executable=path.join(output,name+(process.platform==='win32'?'.exe':''));
  run(compiler,['-std=c++17','-Wall','-Wextra','-I',path.join(root,'launcher/src'),'-I',path.join(root,'launcher/tests/stubs'),path.join(root,`launcher/tests/${name}.cpp`),'-o',executable]);
  let args=[];
  if(name==='history-save') {
    const fixture=path.join(output,'history.txt');fs.mkdirSync(fixture+'-directory');
    args=[fixture.replaceAll('\\','/')];
  }
  if(name==='metadata-cache') {
    const fixture=path.join(output,'cache-fixture');fs.mkdirSync(path.join(fixture,'cache-directory'),{recursive:true});
    args=[fixture.replaceAll('\\','/')];
  }
  if(name==='launch-log') {
    const fixture=path.join(output,'launch-log');fs.mkdirSync(fixture,{recursive:true});
    args=[fixture.replaceAll('\\','/')];
  }
  run(executable,args);console.log(`PASS ${name}`);
}
const bleedFilter=path.join(output,'edgebleed-filter'+(process.platform==='win32'?'.exe':''));
run(compiler,['-std=c++17','-Wall','-Wextra',path.join(root,'launcher/tests/edgebleed-filter.cpp'),'-o',bleedFilter]);
run(process.execPath,['launcher/tests/edgebleed-assets.mjs',bleedFilter,output]);console.log('PASS edgebleed-assets');
run(process.execPath,['--test','launcher/tests/sounds.mjs']);
run(process.execPath,['--test','launcher/tests/version.mjs']);
run(process.execPath,['--test','launcher/tests/dpad-glyph.mjs']);
run(process.execPath,['--test','launcher/tests/package-release.mjs']);
console.log('All host tests passed. Hardware behavior is not covered.');
