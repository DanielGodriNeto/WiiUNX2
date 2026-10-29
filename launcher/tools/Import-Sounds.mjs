// Convert user-supplied RIFF PCM/float WAV effects into bounded 32 kHz mono LPCM16 BE.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
export function readWav(b) {
  if(b.length<12||b.toString('ascii',0,4)!=='RIFF'||b.toString('ascii',8,12)!=='WAVE') throw Error('Not RIFF/WAVE');
  const end=b.readUInt32LE(4)+8;
  if(end>b.length) throw Error('Truncated RIFF');
  let fmt,data;
  for(let p=12;p+8<=end;) {
    const n=b.readUInt32LE(p+4),id=b.toString('ascii',p,p+4);
    if(n>end-p-8) throw Error('Truncated WAV chunk');
    if(id==='fmt ') {
      if(n<16) throw Error('Truncated fmt');
      fmt={format:b.readUInt16LE(p+8),channels:b.readUInt16LE(p+10),rate:b.readUInt32LE(p+12),align:b.readUInt16LE(p+20),bits:b.readUInt16LE(p+22)};
    }
    if(id==='data'&&!data) data=b.subarray(p+8,p+8+n);
    p+=8+n+(n&1);
  }
  if(!fmt||!data||![1,2].includes(fmt.channels)||fmt.rate<8000||fmt.rate>192000||!((fmt.format===1&&[16,24,32].includes(fmt.bits))||(fmt.format===3&&fmt.bits===32))||fmt.align!==fmt.channels*fmt.bits/8||data.length%fmt.align) throw Error('Unsupported WAV format');
  const frames=data.length/fmt.align;
  if(!frames||frames>fmt.rate*5) throw Error('Effect exceeds five seconds or is empty');
  const mono=new Float32Array(frames);
  for(let i=0;i<frames;i++) {
    let value=0;
    for(let c=0;c<fmt.channels;c++) {
      const p=i*fmt.align+c*fmt.bits/8;
      let sample=fmt.format===3 ? data.readFloatLE(p):fmt.bits===16 ? data.readInt16LE(p)/32768:fmt.bits===24 ? data.readIntLE(p,3)/8388608:data.readInt32LE(p)/2147483648;
      value+=Number.isFinite(sample)?Math.max(-1,Math.min(1,sample)):0;
    }
    mono[i]=value/fmt.channels;
  }
  return {mono,rate:fmt.rate};
}
export function convertWav(b) {
  const {mono,rate}=readWav(b),n=Math.max(1,Math.round(mono.length*32000/rate));
  const out=Buffer.alloc((n+32)*2); // 1 ms zero tail before AX stops the voice
  for(let i=0;i<n;i++) {
    const p=i*rate/32000,a=Math.min(mono.length-1,Math.floor(p)),f=p-a;
    let sample=mono[a]*(1-f)+mono[Math.min(a+1,mono.length-1)]*f;
    // Preserve level; only taper the last half millisecond to prevent an abrupt cut.
    sample*=Math.min(1,(n-1-i)/16);
    out.writeInt16BE(Math.max(-32768,Math.min(32767,Math.round(sample*32767))),i*2);
  }
  return out;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)) {
  const root=process.argv[2];if(!root) throw Error('Usage: node tools/Import-Sounds.mjs <folder containing WAV files>');
  const mapping={'uiMove.bin':'Select.wav','uiClick.bin':'Klick.wav','uiBack.bin':'Enter & Back.wav'};
  const report={source:'Optional user-provided audio; rights remain with the respective owners, no license is granted by importing',files:[]};
  for(const [output,input] of Object.entries(mapping)) {
    const bytes=fs.readFileSync(path.join(root,input)),pcm=convertWav(bytes);
    fs.writeFileSync(new URL('../data/sounds/'+output,import.meta.url),pcm);
    report.files.push({input,sourceSHA256:crypto.createHash('sha256').update(bytes).digest('hex'),output,bytes:pcm.length,rate:32000,channels:1,format:'signed PCM16 big-endian'});
  }
  fs.writeFileSync(new URL('../SOUND-ASSETS.json',import.meta.url),JSON.stringify(report,null,2)+'\n');
  console.log(report.files.map(f=>`${f.input} -> ${f.output} (${f.bytes} bytes)`).join('\n'));
}
