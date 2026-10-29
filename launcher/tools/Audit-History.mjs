// WiiUNX2 GPL-3.0-or-later. Read-only targeted audit; never prints matched values.
import fs from 'node:fs';
import {spawnSync} from 'node:child_process';
const git=process.env.GIT||(process.platform==='win32'&&fs.existsSync('C:/Program Files/Git/cmd/git.exe')?'C:/Program Files/Git/cmd/git.exe':'git');
const cwd=process.argv[2]||process.cwd();
function run(args,encoding='utf8') {
  const result=spawnSync(git,args,{cwd,encoding,maxBuffer:128*1024*1024,shell:false});
  if(result.error) throw result.error;if(result.status!==0) throw Error(`git ${args[0]} failed`);return result.stdout;
}
const privateData=/[A-Z]:[\\/]Users[\\/][\w.-]+|\/(?:home|Users)\/[a-zA-Z0-9_.-]+\/|gh[pousr]_[A-Za-z0-9]{20,}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|AKIA[0-9A-Z]{16}/i;
const forbidden=/(?:^|\/)(?:wiiunx2-recent[^/]*|wiiunx2\.log|ui(?:Move|Click|Back)\.bin)$|\.(?:rpx|elf|pack|bps|wav|mp3|ogg|p12|pfx|pem|key|crt)$/i;
let scanned=0;const issues=[];
const metadata=run(['log','--all','--format=%H%n%an%n%ae%n%cn%n%ce%n%B']);
if(privateData.test(metadata)) issues.push('Commit metadata: possible sensitive value; review privately');
for(const line of run(['rev-list','--objects','--all']).trim().split('\n')) {
  const space=line.indexOf(' ');if(space<0) continue;
  const oid=line.slice(0,space),name=line.slice(space+1);
  if(run(['cat-file','-t',oid]).trim()!=='blob') continue;
  const bytes=run(['cat-file','blob',oid],null);scanned++;
  if(forbidden.test(name)) issues.push(`${oid.slice(0,12)}: excluded file category`);
  if(privateData.test(bytes.toString('latin1'))) issues.push(`${oid.slice(0,12)}: possible private path/credential`);
}
console.log(`Reviewed ${scanned} reachable file objects and commit metadata. Intentional author identities and credits are preserved.`);
if(issues.length) {console.error(issues.join('\n'));process.exitCode=1;}
else console.log('No matches in the targeted history checks. Binary metadata, unknown secret formats, legal provenance and unreferenced server objects require separate review.');
