// WiiUNX2 GPL-3.0-or-later. Checks the edge bleed on the shipped card and focus PNGs. Uses only node:zlib.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const [filter,workDir]=process.argv.slice(2);
assert.ok(filter&&workDir,'usage: edgebleed-assets.mjs <edgebleed-filter executable> <work directory>');
const assets=['nxAccountCard.png','nxAccountFocus.png','nxKeyCard.png','nxKeyFocus.png'];

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

const luma=(rgba,i)=>0.2126*rgba[i]+0.7152*rgba[i+1]+0.0722*rgba[i+2];

// Byte offsets of the 8-neighbours of (x,y) that exist inside the image.
function neighbourOffsets(width,height,x,y) {
  const offsets=[];
  for(let dy=-1;dy<=1;dy++) {
    for(let dx=-1;dx<=1;dx++) {
      const nx=x+dx, ny=y+dy;
      if((!dx&&!dy)||nx<0||ny<0||nx>=width||ny>=height) continue;
      offsets.push((ny*width+nx)*4);
    }
  }
  return offsets;
}

// Counts alpha-0 texels whose luminance is below the darkest neighbour with alpha >= minAlpha.
function darkerThanNeighbours(rgba,width,height,minAlpha) {
  let count=0;
  for(let y=0;y<height;y++) {
    for(let x=0;x<width;x++) {
      const i=(y*width+x)*4;
      if(rgba[i+3]!==0) continue;
      const sources=neighbourOffsets(width,height,x,y).filter(j=>rgba[j+3]>=minAlpha);
      if(!sources.length) continue;
      if(luma(rgba,i)<Math.min(...sources.map(j=>luma(rgba,j)))) count++;
    }
  }
  return count;
}

// Exact rule: alpha unchanged, visible texels unchanged, alpha-0 texels copy the RGB of a most-opaque neighbour or stay.
function assertBleedRule(before,after,width,height) {
  for(let y=0;y<height;y++) {
    for(let x=0;x<width;x++) {
      const i=(y*width+x)*4;
      assert.equal(after[i+3],before[i+3],`alpha changed at ${x},${y}`);
      if(before[i+3]!==0) {
        assert.deepEqual([...after.subarray(i,i+4)],[...before.subarray(i,i+4)],`visible texel changed at ${x},${y}`);
        continue;
      }
      const sources=neighbourOffsets(width,height,x,y).filter(j=>before[j+3]>0);
      if(!sources.length) {
        assert.deepEqual([...after.subarray(i,i+3)],[...before.subarray(i,i+3)],`invented colour at ${x},${y}`);
        continue;
      }
      const maxAlpha=Math.max(...sources.map(j=>before[j+3]));
      const candidates=sources.filter(j=>before[j+3]===maxAlpha).map(j=>[...before.subarray(j,j+3)]);
      assert.ok(candidates.some(c=>c.every((v,k)=>v===after[i+k])),`texel ${x},${y} did not take RGB from a most-opaque neighbour`);
    }
  }
}

let changed=0;
for(const name of assets) {
  const {width,height,rgba}=decodeRgba(path.join(root,'launcher/data/images',name));
  const input=path.join(workDir,`${name}.in.rgba`), output=path.join(workDir,`${name}.out.rgba`);
  fs.writeFileSync(input,rgba);
  const result=spawnSync(filter,[input,output,String(width),String(height)],{stdio:'inherit'});
  assert.equal(result.status,0,`${name}: edge bleed filter failed (${result.status??result.signal})`);
  const bled=fs.readFileSync(output);
  assert.equal(bled.length,rgba.length,`${name}: output size differs`);
  assertBleedRule(rgba,bled,width,height);
  const before={opaque:darkerThanNeighbours(rgba,width,height,255),visible:darkerThanNeighbours(rgba,width,height,1)};
  const after={opaque:darkerThanNeighbours(bled,width,height,255),visible:darkerThanNeighbours(bled,width,height,1)};
  assert.equal(after.opaque,0,`${name}: alpha-0 texels darker than an opaque neighbour`);
  assert.equal(after.visible,0,`${name}: alpha-0 texels darker than a visible neighbour`);
  for(let i=0;i<rgba.length;i+=4) if(rgba[i+3]===0&&rgba.compare(bled,i,i+3,i,i+3)!==0) changed++;
  console.log(`PASS ${name} ${width}x${height}: darker than opaque neighbour ${before.opaque} -> ${after.opaque}; darker than visible neighbour ${before.visible} -> ${after.visible}`);
}
console.log(`Edge bleed on shipped PNGs: ${changed} transparent texels had RGB rewritten; alpha and visible texels verified unchanged.`);
