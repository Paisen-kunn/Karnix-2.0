const stages = [
  'Reading 64×64 grayscale pixels',
  'Extracting edges with 3×3 kernels',
  'Selecting strongest local activations',
  'Building sixteen deep feature maps',
  'Combining learned features in dense units',
  'Calculating OA class probabilities'
];

const state = { data: [], index: 0, stage: 0, playing: true, timer: null, delay: 700, frames: new Map() };
const el = id => document.getElementById(id);

async function decodePgm(url) {
  const bytes = new Uint8Array(await (await fetch(url)).arrayBuffer());
  let offset = 0;
  const token = () => {
    while (bytes[offset] <= 32) offset++;
    if (bytes[offset] === 35) { while (bytes[offset++] !== 10); return token(); }
    let value = '';
    while (offset < bytes.length && bytes[offset] > 32) value += String.fromCharCode(bytes[offset++]);
    return value;
  };
  if (token() !== 'P5') throw new Error('Expected binary PGM image');
  const width = Number(token());
  const height = Number(token());
  const max = Number(token());
  while (bytes[offset] <= 32) offset++;
  return { width, height, pixels: bytes.slice(offset, offset + width * height), max };
}

function drawPixels(canvas, frame, transform = 'normal') {
  const temp = document.createElement('canvas');
  temp.width = frame.width; temp.height = frame.height;
  const context = temp.getContext('2d');
  const image = context.createImageData(frame.width, frame.height);
  for (let i = 0; i < frame.pixels.length; i++) {
    const value = frame.pixels[i] * 255 / frame.max;
    const left = frame.pixels[Math.max(0, i - 1)] * 255 / frame.max;
    const above = frame.pixels[Math.max(0, i - frame.width)] * 255 / frame.max;
    let shown = value;
    if (transform === 'edge') shown = Math.min(255, Math.abs(value - left) * 4 + Math.abs(value - above) * 4);
    if (transform === 'bright') shown = Math.min(255, Math.max(0, (value - 75) * 1.55));
    if (transform === 'inverse') shown = 255 - value;
    image.data[i * 4] = shown;
    image.data[i * 4 + 1] = transform === 'edge' ? Math.min(255, shown * 1.08) : shown;
    image.data[i * 4 + 2] = transform === 'edge' ? Math.min(255, shown * .78) : shown;
    image.data[i * 4 + 3] = 255;
  }
  context.putImageData(image, 0, 0);
  const target = canvas.getContext('2d');
  target.imageSmoothingEnabled = transform === 'normal';
  target.clearRect(0, 0, canvas.width, canvas.height);
  target.drawImage(temp, 0, 0, canvas.width, canvas.height);
}

async function loadFrame(item) {
  if (!state.frames.has(item.file)) state.frames.set(item.file, await decodePgm(item.url));
  return state.frames.get(item.file);
}

async function buildFilmstrip() {
  const strip = el('filmstrip');
  for (const [index, item] of state.data.entries()) {
    const button = document.createElement('button');
    button.className = 'thumb'; button.type = 'button'; button.dataset.index = index;
    const canvas = document.createElement('canvas'); canvas.width = 100; canvas.height = 100;
    const label = document.createElement('span'); label.textContent = String(index + 1).padStart(2, '0');
    button.append(canvas, label); strip.append(button);
    drawPixels(canvas, await loadFrame(item));
    button.addEventListener('click', () => { state.index = index; state.stage = 0; state.playing = true; el('playButton').textContent = 'Pause'; render(); schedule(); });
  }
}

function paintFeatures(frame, stage) {
  const transforms = stage < 1 ? ['normal','normal','normal','normal'] : ['edge','bright','inverse','edge'];
  document.querySelectorAll('#featureGrid canvas').forEach((canvas, index) => drawPixels(canvas, frame, transforms[index]));
  el('featureGrid').classList.toggle('processing', stage > 0);
}

async function render() {
  const item = state.data[state.index];
  const frame = await loadFrame(item);
  drawPixels(el('xrayCanvas'), frame);
  paintFeatures(frame, state.stage);
  el('imageName').textContent = item.file.replace('.pgm', '').replaceAll('_', ' ');
  el('currentIndex').textContent = String(state.index + 1).padStart(2, '0');
  el('knownLabel').textContent = item.knownLabel;
  el('stageTitle').textContent = stages[Math.min(state.stage, stages.length - 1)];
  document.querySelectorAll('.node').forEach((node, index) => {
    node.classList.toggle('active', index === state.stage);
    node.classList.toggle('complete', index < state.stage);
  });
  document.querySelectorAll('.thumb').forEach((thumb, index) => {
    thumb.classList.toggle('active', index === state.index);
    thumb.classList.toggle('done', index < state.index);
  });
  const ready = state.stage === stages.length - 1;
  el('result').classList.toggle('ready', ready);
  el('normalValue').textContent = `${(item.normal * 100).toFixed(2)}%`;
  el('oaValue').textContent = `${(item.oa * 100).toFixed(2)}%`;
  el('normalBar').style.width = ready ? `${item.normal * 100}%` : '0';
  el('oaBar').style.width = ready ? `${item.oa * 100}%` : '0';
  const positive = item.oa > item.normal;
  el('classification').textContent = ready ? (positive ? 'OSTEOARTHRITIS' : 'NO OA') : 'ANALYZING';
  el('classification').classList.toggle('positive', ready && positive);
  const progress = ((state.index * stages.length + state.stage + 1) / (state.data.length * stages.length)) * 100;
  el('progressBar').style.width = `${progress}%`;
}

function advance() {
  if (!state.playing) return;
  if (state.stage < stages.length - 1) state.stage++;
  else { state.stage = 0; state.index = (state.index + 1) % state.data.length; }
  render(); schedule();
}
function schedule() { clearTimeout(state.timer); if (state.playing) state.timer = setTimeout(advance, state.delay); }

async function init() {
  const response = await fetch('predictions.json');
  if (!response.ok) throw new Error('C++ prediction data is unavailable');
  state.data = (await response.json()).images;
  await buildFilmstrip(); await render(); schedule();
}

el('playButton').addEventListener('click', () => { state.playing = !state.playing; el('playButton').textContent = state.playing ? 'Pause' : 'Play'; schedule(); });
el('restartButton').addEventListener('click', () => { state.index = 0; state.stage = 0; state.playing = true; el('playButton').textContent = 'Pause'; render(); schedule(); });
el('speedSelect').addEventListener('change', event => { state.delay = Number(event.target.value); schedule(); });

init().catch(error => { el('stageTitle').textContent = error.message; console.error(error); });
