// WiiUNX2 GPL-3.0-or-later. Focused publication checks, not a legal/security audit.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const root=path.resolve(process.argv[2]||path.join(path.dirname(fileURLToPath(import.meta.url)),'../..'));
const errors=[];
function visit(directory) {
  for(const entry of fs.readdirSync(directory,{withFileTypes:true})) {
    if(['.git','.test-output'].includes(entry.name)) continue;
    const file=path.join(directory,entry.name),relative=path.relative(root,file).replaceAll('\\','/');
    if(entry.isSymbolicLink()) {errors.push(`${relative}: symbolic link requires review`);continue;}
    if(entry.isDirectory()) {visit(file);continue;}
    if(/\.(rpx|wuhb|wps|wms|elf|map|log|exe|o|a|d|pack|bps|wav|mp3|ogg|zip|p12|pfx|pem|key|crt|cer|der|tik|tmd|wud|wux|iso)$/i.test(entry.name)||/^wiiunx2-recent/i.test(entry.name)||entry.name==='wiiunx2-debug.enabled'||(entry.name.startsWith('.env')&&entry.name!=='.env.example')||/^ui(Move|Click|Back)\.bin$/i.test(entry.name)) errors.push(`${relative}: private or generated artifact`);
    if(/\.ttf$/i.test(entry.name)) {
      const hash=crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
      if(relative!=='launcher/data/fonts/font.ttf'||hash!=='b85c38ecea8a7cfb39c24e395a4007474fa5a4fc864f6ee33309eb4948d232d5') errors.push(`${relative}: unreviewed font`);
    }
    if(/\.(cpp|h|hpp|c|mjs|js|ps1|json|md|txt|ya?ml)$/i.test(entry.name)) {
      const text=fs.readFileSync(file,'utf8');
      if(/[A-Z]:[\\/]Users[\\/][\w.-]+/i.test(text)||/\/(?:home|Users)\/[a-zA-Z0-9_.-]+\//.test(text)||/gh[pousr]_[A-Za-z0-9]{20,}/.test(text)||/-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----/.test(text)) errors.push(`${relative}: possible private path or credential (contents omitted)`);
    }
  }
}
visit(root);
for(const required of ['launcher/data/fonts/font.ttf','launcher/licenses/NotoSans-OFL.txt','launcher/LICENSE','dependencies/libgui/COPYING','dependencies/librpxloader/LICENSE','dependencies/libwuhbutils/LICENSE','launcher/src/utils/nlohmann-json-LICENSE.txt']) if(!fs.existsSync(path.join(root,required))) errors.push(`${required}: missing reviewed asset/license`);
if(errors.length) {console.error(errors.join('\n'));process.exitCode=1;}
else console.log('Publication checks passed: checked artifact types, obvious private paths/token patterns, pinned font and required licenses. This is not exhaustive security or legal clearance.');
