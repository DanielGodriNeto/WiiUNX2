// WiiUNX2 GPL-3.0-or-later. Packages only a clean reviewed checkout.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import crypto from 'node:crypto';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const check=spawnSync(process.execPath,[path.join(root,'launcher/tools/Check-Public-Source.mjs'),root],{stdio:'inherit'});
if(check.error) throw check.error;if(check.status!==0) process.exit(check.status||1);
const base=path.join(root,'.test-output');fs.mkdirSync(base,{recursive:true});
const output=fs.mkdtempSync(path.join(base,'source-'));
const archive=path.join(output,'WiiUNX2-source.tar.gz');
const result=spawnSync('tar',['-czf',archive,'--exclude=.git','--exclude=.test-output','-C',root,'.'],{stdio:'inherit',shell:false});
if(result.error) throw result.error;if(result.status!==0) process.exit(result.status||1);
const hash=crypto.createHash('sha256').update(fs.readFileSync(archive)).digest('hex');
fs.writeFileSync(archive+'.sha256',`${hash}  WiiUNX2-source.tar.gz\n`);
console.log(`Created ${archive}`);
