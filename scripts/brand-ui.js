#!/usr/bin/env node
// Copies the prebuilt dcx-ui bundle to build/ui and applies this fork's
// branding (see "brand" in package.json) before awot-static embeds it into
// DuinoDCX/StaticFiles.h.
//
// - Browser tab / home screen name
// - The "new firmware available" check in the UI looks at this fork's GitHub
//   releases instead of upstream's, so nobody gets offered a binary that would
//   replace this firmware.
//
// Every patch must match, otherwise the build fails loudly. That way a dcx-ui
// update can't silently drop the branding.
const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '..');
const { brand } = require(path.join(root, 'package.json'));
const source = path.join(root, 'node_modules', 'dcx-ui', 'dist');
const target = path.join(root, 'build', 'ui');

const escapeHtml = (text) =>
  text.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[c]);

function patchFile(relativePath, replacements) {
  const file = path.join(target, relativePath);
  let content = fs.readFileSync(file, 'utf8');

  for (const [search, replacement] of replacements) {
    if (!content.includes(search)) {
      throw new Error(`brand-ui: "${search}" not found in ${relativePath}. Did dcx-ui change?`);
    }
    content = content.split(search).join(replacement);
  }

  fs.writeFileSync(file, content);
}

if (!fs.existsSync(source)) {
  throw new Error('brand-ui: node_modules/dcx-ui/dist is missing. Run `npm install` first.');
}

fs.rmSync(target, { recursive: true, force: true });
fs.cpSync(source, target, { recursive: true, filter: (file) => !file.endsWith('.map') });

const title = escapeHtml(brand.name);
patchFile('index.html', [
  [
    '<title>DCX2496 Remote</title>',
    `<title>${title}</title><meta name="apple-mobile-web-app-title" content="${title}">`,
  ],
]);

const manifestFile = path.join(target, 'manifest.json');
const manifest = JSON.parse(fs.readFileSync(manifestFile, 'utf8'));
Object.assign(manifest, {
  short_name: brand.name,
  name: `${brand.name} · DCX2496 Remote`,
  theme_color: '#272b30',
  background_color: '#272b30',
});
fs.writeFileSync(manifestFile, `${JSON.stringify(manifest, null, 2)}\n`);

const jsDir = path.join(target, 'static', 'js');
const mainChunk = fs.readdirSync(jsDir).find((file) => /^main\..+\.chunk\.js$/.test(file));
if (!mainChunk) {
  throw new Error('brand-ui: main chunk not found in dcx-ui bundle.');
}
patchFile(path.join('static', 'js', mainChunk), [
  [
    'https://api.github.com/repos/lasselukkari/DuinoDCX/releases',
    `https://api.github.com/repos/${brand.releasesRepo}/releases`,
  ],
]);

console.log(`brand-ui: ${brand.name} → build/ui (releases: ${brand.releasesRepo})`);
