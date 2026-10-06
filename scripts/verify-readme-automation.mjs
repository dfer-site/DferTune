import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';

const root = resolve(import.meta.dirname, '..');
const readme = readFileSync(resolve(root, 'README.md'), 'utf8');
const makefile = readFileSync(resolve(root, 'Makefile'), 'utf8');

const versionMarker = /<!-- CURRENT_VERSION_START -->([^<\r\n]+)<!-- CURRENT_VERSION_END -->/g;
const matches = [...readme.matchAll(versionMarker)];

if (matches.length !== 1) {
  throw new Error(`Expected exactly one README version marker, found ${matches.length}.`);
}

const version = matches[0][1].trim();
if (!/^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$/.test(version)) {
  throw new Error(`README version marker must contain a semantic version, got "${version}".`);
}

const makefileVersion = makefile.match(/^export VERSION\s*:=\s*([^\s#]+)\s*$/m)?.[1];
if (!makefileVersion) {
  throw new Error('Could not find export VERSION in Makefile.');
}
if (version !== makefileVersion) {
  throw new Error(`README version ${version} does not match Makefile version ${makefileVersion}.`);
}

const requiredBadges = [
  'img.shields.io/github/downloads/dfer-site/DferTune/total',
  'hits.sh/github.com/dfer-site/DferTune.svg',
];

for (const badge of requiredBadges) {
  if (!readme.includes(badge)) {
    throw new Error(`README is missing required badge URL: ${badge}`);
  }
}

console.log(`Validated README automation markers and badges for v${version}.`);
