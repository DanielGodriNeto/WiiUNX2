// WiiUNX2 GPL-3.0-or-later. Checks the shipped keyboard D-pad glyph PNG. Uses only node:zlib.
// Set DPAD_PNG to test another PNG (used for mutation checks).
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import zlib from 'node:zlib';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const dpadFile=process.env.DPAD_PNG||path.join(root,'launcher/data/images/nxButtonDpad.png');
const plusFile=path.join(root,'launcher/data/images/nxButtonPlus.png');

function paeth(a,b,c) {
  const p=a+b-c, pa=Math.abs(p-a), pb=Math.abs(p-b), pc=Math.abs(p-c);
  if(pa<=pb&&pa<=pc) return a;
  return pb<=pc?b:c;
}

// Decodes an 8-bit non-interlaced RGBA PNG (color type 6) into raw R,G,B,A bytes.
function decodeRgba(file) {
  const data=fs.readFileSync(file);
  assert.deepEqual([...data.subarray(0,8)],[0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a],`${path.basename(file)} is not a PNG`);
  let width=0,height=0,offset=8;
  const idat=[];
  while(offset<data.length) {
    const length=data.readUInt32BE(offset);
    const type=data.toString('latin1',offset+4,offset+8);
    const body=data.subarray(offset+8,offset+8+length);
    if(type==='IHDR') {
      width=body.readUInt32BE(0);
      height=body.readUInt32BE(4);
      assert.deepEqual([body[8],body[9],body[12]],[8,6,0],`${path.basename(file)}: only 8-bit RGBA, non-interlaced PNG is supported`);
    } else if(type==='IDAT') {
      idat.push(body);
    }
    offset+=12+length;
  }
  const raw=zlib.inflateSync(Buffer.concat(idat));
  const stride=width*4;
  const out=Buffer.alloc(stride*height);
  let prev=Buffer.alloc(stride);
  for(let y=0;y<height;y++) {
    const type=raw[y*(stride+1)];
    assert.ok(type<=4,`${path.basename(file)}: bad PNG filter ${type}`);
    const line=Buffer.from(raw.subarray(y*(stride+1)+1,(y+1)*(stride+1)));
    for(let i=0;i<stride;i++) {
      const a=i>=4?line[i-4]:0, b=prev[i], c=i>=4?prev[i-4]:0;
      const predictor=[0,a,b,(a+b)>>1,paeth(a,b,c)][type];
      line[i]=(line[i]+predictor)&255;
    }
    line.copy(out,y*stride);
    prev=line;
  }
  return {width,height,rgba:out};
}

const pixel=(image,x,y)=>{const i=(y*image.width+x)*4;return [...image.rgba.subarray(i,i+4)];};
const isWhite=([r,g,b,a])=>r>230&&g>230&&b>230&&a>230;
const luma=([r,g,b])=>0.2126*r+0.7152*g+0.0722*b;
const whiteCount=image=>{
  let count=0;
  for(let y=0;y<image.height;y++) for(let x=0;x<image.width;x++) if(isWhite(pixel(image,x,y))) count++;
  return count;
};

const dpad=decodeRgba(dpadFile);

test('D-pad glyph is 28x28 with transparent corners', () => {
  assert.equal(dpad.width,28);
  assert.equal(dpad.height,28);
  for(const [x,y] of [[0,0],[27,0],[0,27],[27,27]]) assert.equal(pixel(dpad,x,y)[3],0,`corner ${x},${y} is not transparent`);
});

test('D-pad centre is the dark dimple', () => {
  assert.ok(pixel(dpad,14,14)[0]<80,`centre is ${pixel(dpad,14,14)}`);
});

test('D-pad has a solid white cross of plausible size', () => {
  const count=whiteCount(dpad);
  assert.ok(count>=140&&count<=260,`near-white opaque pixel count is ${count}`);
});

test('D-pad horizontal and vertical arms are white', () => {
  for(const [x,y] of [[7,14],[20,14],[14,7],[14,20]]) assert.ok(isWhite(pixel(dpad,x,y)),`arm pixel ${x},${y} is not white`);
});

test('D-pad diagonal corners between the arms are not white', () => {
  for(const [x,y] of [[7,7],[20,7],[7,20],[20,20]]) assert.ok(!isWhite(pixel(dpad,x,y)),`diagonal pixel ${x},${y} is white`);
});

test('D-pad is 4-fold symmetric inside the central box', () => {
  for(let y=5;y<23;y++) {
    for(let x=5;x<23;x++) {
      const difference=Math.abs(luma(pixel(dpad,x,y))-luma(pixel(dpad,27-y,x)));
      assert.ok(difference<=40,`pixel ${x},${y} differs from its 90-degree rotation by ${difference.toFixed(1)}`);
    }
  }
});

test('D-pad is distinct from the plus glyph', () => {
  const plus=decodeRgba(plusFile);
  const dpadCount=whiteCount(dpad), plusCount=whiteCount(plus);
  assert.ok(plusCount<0.6*dpadCount,`plus white count ${plusCount} is not below 60% of D-pad count ${dpadCount}`);
});
