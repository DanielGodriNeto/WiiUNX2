// Original WiiUNX2 ambient composition, GPL-3.0-or-later. No sampled assets.
import fs from 'node:fs';
const rate=32000, seconds=24, output=Buffer.alloc(rate*seconds*2);
const chords=[[48,55,59,62],[45,52,55,59],[41,48,52,57],[43,50,53,57]];
const freq=n=>440*Math.pow(2,(n-69)/12);
for(let i=0;i<rate*seconds;i++) {
  const t=i/rate, chord=Math.floor(t/6), local=t%6;
  const envelope=Math.pow(Math.sin(Math.PI*local/6),2);
  let sample=0;
  for(const n of chords[chord]) sample+=Math.sin(2*Math.PI*freq(n)*local)*envelope*.035;
  const step=Math.floor(local/1.5), bellT=local%1.5;
  const note=chords[chord][step]+24;
  sample+=Math.sin(2*Math.PI*freq(note)*bellT)*Math.exp(-bellT*3)*Math.min(1,bellT*80)*.045*Math.min(1,(6-local)*3);
  output.writeInt16BE(Math.round(sample*32767),i*2);
}
fs.writeFileSync(new URL('../data/sounds/menuMusic.bin',import.meta.url),output);
console.log(`Generated ${seconds}s of original ambient music (${output.length} bytes)`);
