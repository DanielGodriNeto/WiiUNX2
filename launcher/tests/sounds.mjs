import test from 'node:test';
import assert from 'node:assert/strict';
import {readWav,convertWav} from '../tools/Import-Sounds.mjs';
function wave({channels=1,format=3,bits=32,rate=48000,frames=480}={}) {
  const bytes=bits/8*channels*frames,b=Buffer.alloc(44+bytes);
  b.write('RIFF');b.writeUInt32LE(b.length-8,4);b.write('WAVEfmt ',8);b.writeUInt32LE(16,16);
  b.writeUInt16LE(format,20);b.writeUInt16LE(channels,22);b.writeUInt32LE(rate,24);
  b.writeUInt32LE(rate*channels*bits/8,28);b.writeUInt16LE(channels*bits/8,32);b.writeUInt16LE(bits,34);
  b.write('data',36);b.writeUInt32LE(bytes,40);
  return b;
}
test('float stereo downmix, duration and native big-endian output',()=>{
  const b=wave({channels:2});
  for(let i=44;i<b.length;i+=8){b.writeFloatLE(.5,i);b.writeFloatLE(-.25,i+4);}
  const result=convertWav(b);
  assert.equal(result.length,(320+32)*2);
  assert.ok(Math.abs(result.readInt16BE(0)-4096)<=1);
  assert.ok(result.subarray(-64).every(v=>v===0));
  assert.equal(result.readInt16BE(638),0);
});
test('PCM16 amplitude and NaN sanitization',()=>{
  const b=wave({format:1,bits:16,rate:32000,frames:100});
  b.writeInt16LE(-16384,44);assert.equal(readWav(b).mono[0],-.5);
  const f=wave();f.writeFloatLE(NaN,44);assert.equal(readWav(f).mono[0],0);
});
test('reject truncated chunks, invalid alignment and unsupported formats',()=>{
  const b=wave();
  for(const n of [0,8,12,25,b.length-1]) assert.throws(()=>readWav(b.subarray(0,n)));
  const bad=Buffer.from(b);bad.writeUInt16LE(2,32);assert.throws(()=>readWav(bad));
  assert.throws(()=>readWav(wave({format:6})));
  assert.throws(()=>readWav(wave({frames:0})));
});
