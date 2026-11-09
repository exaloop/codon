let cleanup;

const STAGE_DURATION_MS = 5000;
const RESUME_DELAY_MS = 6000;
const INITIAL_DELAY_MS = 1200;
const TRANSITION_EASING_MS = 150;
const PANE_FACE_OFFSET = 4.2;

export function labelConnector(start, end) {
  const bend = Math.max(0, (end.x - start.x) / 2);
  return `M${start.x} ${start.y} C${start.x + bend} ${start.y} ${end.x - bend} ${end.y} ${end.x} ${end.y}`;
}

export function pipelineBounds(element, scale) {
  const bounds = element.getBoundingClientRect();
  return Object.fromEntries(['top', 'left', 'right', 'width', 'height']
    .map(property => [property, bounds[property] / scale]));
}

export function regionLayout(centers, height) {
  const boundaries = [0, (centers[0] + centers[1]) / 2,
    (centers[4] + centers[5]) / 2, height];
  const inputHeight = Math.min(76, centers[1] - centers[0], centers[0] * 2);
  const outputHeight = Math.min(76, centers[5] - centers[4], (height - centers[5]) * 2);
  return [
    {top: centers[0] - inputHeight / 2, height: inputHeight},
    {top: boundaries[1], height: boundaries[2] - boundaries[1]},
    {top: centers[5] - outputHeight / 2, height: outputHeight},
  ];
}

export function stackLayout(width, minimumHeight = 280) {
  const height = Math.max(minimumHeight, Math.round(520 * Math.min(1, width / 350)));
  return {height, vertical: Math.max(800, 390 * height / width), spacing: 140};
}

export function paneLayout(index, activeIndex, resting, width, height, pixelsPerUnit) {
  if (activeIndex < 0) return {...resting, scale: 1, flat: false};
  if (index === activeIndex) {
    const size = Math.min(236, width - 16);
    return {
      x: width - size / 2 - 8,
      y: Math.max(size / 2 + 24, Math.min(height - size / 2 - 24, resting.y)),
      scale: size / (256 * pixelsPerUnit),
      flat: true,
    };
  }
  const scale = width < 250 ? 0.68 : 0.6;
  const halfWidth = 128 * Math.SQRT2 * pixelsPerUnit * scale;
  const faceShift = PANE_FACE_OFFSET * (1 - scale) * Math.sqrt(2 / 3) * pixelsPerUnit;
  return {...resting, x: Math.max(halfWidth + 6, resting.x - Math.min(140, width * 0.34)),
    y: resting.y - faceShift, scale, flat: false};
}

export function animationAmount(now, lastTime) {
  const elapsed = lastTime === undefined ? 16 : Math.max(0, now - lastTime);
  return 1 - Math.exp(-Math.min(64, elapsed) / TRANSITION_EASING_MS);
}

export function paneProjection(label, camera, width, height) {
  const project = (x, y) => label.localToWorld(label.position.clone().set(x, y, 0)).project(camera);
  const topLeft = project(-128, 128);
  const topRight = project(128, 128);
  const bottomLeft = project(-128, -128);
  return {
    matrix: [
      (topRight.x - topLeft.x) * width / 512,
      (topLeft.y - topRight.y) * height / 512,
      (bottomLeft.x - topLeft.x) * width / 512,
      (topLeft.y - bottomLeft.y) * height / 512,
      (topLeft.x + 1) * width / 2,
      (1 - topLeft.y) * height / 2,
    ],
    depth: project(0, 0).z,
  };
}

function createScene(diagram, three, onRender) {
  const {AmbientLight, BoxGeometry, DirectionalLight, Mesh, MeshBasicMaterial, MeshLambertMaterial,
    Object3D, OrthographicCamera, Scene, Vector3, WebGLRenderer} = three;
  const stages = diagram.querySelector('.pipeline-stages');
  const panes = Array.from(stages.querySelectorAll('.pipeline-stage'));
  let renderer;
  try {
    renderer = new WebGLRenderer({alpha: true, antialias: true, preserveDrawingBuffer: true});
  } catch {
    return {setActive() {}, dispose() {}};
  }
  const scale = parseFloat(getComputedStyle(diagram).zoom) || 1;
  renderer.setPixelRatio(Math.min(window.devicePixelRatio * scale, 2));
  renderer.autoClear = false;
  renderer.domElement.className = 'pipeline-canvas';
  renderer.domElement.setAttribute('aria-hidden', 'true');
  const labels = document.createElement('div');
  labels.className = 'pipeline-projection';
  const scene = new Scene();
  const camera = new OrthographicCamera(-250, 250, 355, -355, 1, 3000);
  camera.position.set(900, 900, 900);
  camera.lookAt(0, 0, 0);
  const right = new Vector3(1, 0, 0).applyQuaternion(camera.quaternion);
  const up = new Vector3(0, 1, 0).applyQuaternion(camera.quaternion);
  const front = new Vector3(0, 0, 1).applyQuaternion(camera.quaternion);
  scene.add(new AmbientLight(0xffffff, 0.85));
  const light = new DirectionalLight(0xffffff, 1.1);
  light.position.set(-300, 500, 800);
  scene.add(light);
  const geometry = new BoxGeometry(256, 256, 8);
  const depthMaterial = new MeshBasicMaterial({colorWrite: false});
  const items = panes.map(pane => {
    const material = new MeshLambertMaterial();
    const mesh = new Mesh(geometry, [material, material, material, material, depthMaterial, depthMaterial]);
    mesh.rotation.x = -Math.PI / 2;
    const label = new Object3D();
    label.rotation.copy(mesh.rotation);
    pane.style.position = 'absolute';
    pane.style.left = '0';
    pane.style.top = '0';
    pane.style.transformOrigin = '0 0';
    pane.style.willChange = 'transform';
    pane.style.pointerEvents = 'auto';
    labels.append(pane);
    scene.add(mesh);
    return {pane, mesh, material, label, base: new Vector3(), target: new Vector3(),
      restingRotation: mesh.quaternion.clone(), targetRotation: mesh.quaternion.clone(), targetScale: 1};
  });
  stages.append(renderer.domElement, labels);
  stages.dataset.renderer = 'three';
  const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
  let active;
  let frame;
  let lastTime;
  let anchors = [];

  function updateTargets() {
    const width = stages.clientWidth;
    const height = stages.clientHeight;
    const pixelsPerUnit = height / (camera.top - camera.bottom);
    const activeIndex = items.findIndex(item => item.pane.dataset.pipelineNode === active);
    for (const [index, item] of items.entries()) {
      const projected = item.base.clone().project(camera);
      const resting = {x: (projected.x + 1) * width / 2, y: (1 - projected.y) * height / 2};
      const target = paneLayout(index, activeIndex, resting, width, height, pixelsPerUnit);
      item.target.copy(item.base)
        .addScaledVector(right, (target.x - resting.x) / pixelsPerUnit)
        .addScaledVector(up, (resting.y - target.y) / pixelsPerUnit)
        .addScaledVector(front, target.flat ? 900 : 0);
      item.targetRotation.copy(target.flat ? camera.quaternion : item.restingRotation);
      item.targetScale = target.scale;
    }
  }

  function render(now = performance.now(), immediate = false) {
    frame = undefined;
    const amount = animationAmount(now, lastTime);
    lastTime = now;
    let moving = false;
    for (const item of items) {
      const settled = item.mesh.position.distanceTo(item.target) < 0.05
        && item.mesh.quaternion.angleTo(item.targetRotation) < 0.001
        && Math.abs(item.mesh.scale.x - item.targetScale) < 0.001;
      if (immediate || reducedMotion.matches || document.hidden || settled) {
        item.mesh.position.copy(item.target);
        item.mesh.quaternion.copy(item.targetRotation);
        item.mesh.scale.setScalar(item.targetScale);
      } else {
        item.mesh.position.lerp(item.target, amount);
        item.mesh.quaternion.slerp(item.targetRotation, amount);
        item.mesh.scale.setScalar(item.mesh.scale.x + (item.targetScale - item.mesh.scale.x) * amount);
        moving = true;
      }
      item.label.position.copy(item.mesh.position);
      item.label.quaternion.copy(item.mesh.quaternion);
      item.label.scale.copy(item.mesh.scale);
      item.label.translateZ(PANE_FACE_OFFSET * item.mesh.scale.x);
    }
    renderer.clear();
    scene.overrideMaterial = depthMaterial;
    renderer.render(scene, camera);
    scene.overrideMaterial = null;
    renderer.render(scene, camera);
    for (const item of items) {
      const projection = paneProjection(item.label, camera, stages.clientWidth, stages.clientHeight);
      item.pane.style.transform = `matrix(${projection.matrix.join(',')})`;
      item.depth = projection.depth;
    }
    items.slice().sort((first, second) => second.depth - first.depth)
      .forEach((item, index) => { item.pane.style.zIndex = String(index); });
    for (const [index, item] of items.entries()) {
      const tilt = Math.min(1, item.mesh.quaternion.angleTo(camera.quaternion)
        / item.restingRotation.angleTo(camera.quaternion));
      anchors[index].connector.set(-128, -128 * tilt, 0)
        .applyMatrix4(item.label.matrixWorld).project(camera);
    }
    onRender(anchors);
    if (moving) frame = requestAnimationFrame(render);
  }

  function requestRender(immediate = false) {
    cancelAnimationFrame(frame);
    lastTime = undefined;
    render(performance.now(), immediate);
  }

  function resize() {
    const width = stages.clientWidth;
    const minimumHeight = window.matchMedia('(max-width: 760px)').matches ? 280 : 360;
    const {height, vertical, spacing} = stackLayout(width, minimumHeight);
    stages.style.height = `${height}px`;
    stages.dataset.layout = 'stack';
    for (const [index, item] of items.entries()) {
      item.base.set(0, (2.5 - index) * spacing, 0);
      item.mesh.position.copy(item.base);
    }
    const aspect = width / height;
    camera.left = -vertical * aspect / 2;
    camera.right = vertical * aspect / 2;
    camera.top = vertical / 2;
    camera.bottom = -vertical / 2;
    camera.updateProjectionMatrix();
    camera.updateMatrixWorld();
    anchors = items.map(item => ({
      node: item.pane,
      point: item.base.clone().add(new Vector3(0, PANE_FACE_OFFSET, 0)).project(camera),
      connector: new Vector3(),
    }));
    renderer.setSize(width, height);
    updateTargets();
    requestRender(true);
  }

  function updateTheme() {
    for (const item of items) {
      item.material.color.set(getComputedStyle(item.pane).getPropertyValue('--stage-accent').trim());
    }
    requestRender(true);
  }

  const observer = new ResizeObserver(resize);
  observer.observe(stages);
  window.addEventListener('resize', resize);
  const themeObserver = new MutationObserver(updateTheme);
  themeObserver.observe(document.documentElement, {attributes: true, attributeFilter: ['data-theme']});
  resize();
  updateTheme();
  return {
    setActive(kind, immediate = false) {
      if (active === kind && !immediate) return;
      active = kind;
      updateTargets();
      requestRender(immediate);
    },
    dispose() {
      observer.disconnect();
      window.removeEventListener('resize', resize);
      themeObserver.disconnect();
      cancelAnimationFrame(frame);
      for (const item of items) {
        stages.append(item.pane);
        item.pane.removeAttribute('style');
        item.material.dispose();
      }
      geometry.dispose();
      depthMaterial.dispose();
      renderer.dispose();
      renderer.forceContextLoss();
      renderer.domElement.remove();
      labels.remove();
      stages.style.height = '';
      delete stages.dataset.renderer;
      delete stages.dataset.layout;
    },
  };
}

function connectFlow(diagram) {
  const flow = diagram.querySelector('.pipeline-flow');
  const track = diagram.querySelector('.pipeline-track');
  const mask = track.querySelector('mask');
  const rail = diagram.querySelector('.pipeline-labels');
  const labels = Array.from(rail.querySelectorAll('[data-pipeline-label]'));
  const connectors = diagram.querySelector('.pipeline-connectors');
  const foreground = connectors.cloneNode(false);
  foreground.classList.add('pipeline-connectors-active');
  connectors.after(foreground);
  const connections = labels.map(label => {
    const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    path.dataset.pipelineConnector = label.dataset.pipelineLabel;
    return path;
  });
  connectors.replaceChildren(...connections);
  const regions = Array.from(diagram.querySelectorAll('[data-pipeline-region]'));
  const notes = diagram.querySelector('.pipeline-notes');
  let anchors = [];
  function update(nextAnchors) {
    if (nextAnchors) anchors = nextAnchors;
    const scale = diagram.getBoundingClientRect().width / parseFloat(getComputedStyle(diagram).width) || 1;
    const measure = element => pipelineBounds(element, scale);
    const bounds = measure(flow);
    const diagramBounds = measure(diagram);
    const stageBounds = measure(diagram.querySelector('.pipeline-stages'));
    const railBounds = measure(rail);
    for (const layer of [connectors, foreground]) {
      layer.setAttribute('viewBox', `0 0 ${diagramBounds.width} ${diagramBounds.height}`);
    }
    const centers = [];
    for (const [index, label] of labels.entries()) {
      const anchor = anchors.find(item => item.node.dataset.pipelineNode === label.dataset.pipelineLabel);
      const node = anchor?.node || diagram.querySelector(`[data-pipeline-node="${label.dataset.pipelineLabel}"]`);
      const rect = measure(node);
      const top = anchor ? stageBounds.top + (1 - anchor.point.y) * stageBounds.height / 2 : rect.top + rect.height / 2;
      centers.push(top - railBounds.top);
      label.style.setProperty('--pipeline-label-top', `${top - railBounds.top}px`);
      label.style.setProperty('--stage-accent', getComputedStyle(node).getPropertyValue('--stage-accent'));
      label.toggleAttribute('data-active', diagram.dataset.active === label.dataset.pipelineLabel);
      const labelBounds = measure(label);
      const edge = anchor?.connector;
      const end = {
        x: (edge ? stageBounds.left + (edge.x + 1) * stageBounds.width / 2 : rect.left) - diagramBounds.left - 4,
        y: (edge ? stageBounds.top + (1 - edge.y) * stageBounds.height / 2 : rect.top + rect.height / 2) - diagramBounds.top,
      };
      connections[index].setAttribute('d', labelConnector({
        x: labelBounds.right - diagramBounds.left + 4,
        y: top - diagramBounds.top,
      }, end));
      connections[index].style.setProperty('--stage-accent', label.style.getPropertyValue('--stage-accent'));
      connections[index].toggleAttribute('data-active', label.hasAttribute('data-active'));
      const layer = label.hasAttribute('data-active') ? foreground : connectors;
      if (connections[index].parentNode !== layer) layer.append(connections[index]);
    }
    for (const [index, region] of regionLayout(centers, railBounds.height).entries()) {
      regions[index].style.setProperty('--region-top', `${region.top}px`);
      regions[index].style.setProperty('--region-height', `${region.height}px`);
    }
    track.setAttribute('viewBox', `0 0 ${bounds.width} ${bounds.height}`);
    const points = Array.from(diagram.querySelectorAll('.pipeline-port'), port => {
      const rect = measure(port);
      return [rect.left + rect.width / 2, rect.top + rect.height / 2];
    });
    const path = points.map(([left, top], index) =>
      `${index ? 'L' : 'M'}${left - bounds.left} ${top - bounds.top}`).join(' ');
    for (const line of track.querySelectorAll(':scope > path')) line.setAttribute('d', path);
    const backdrop = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    backdrop.setAttribute('width', bounds.width);
    backdrop.setAttribute('height', bounds.height);
    backdrop.setAttribute('fill', 'white');
    const exclusions = Array.from(diagram.querySelectorAll('.pipeline-operation, .pipeline-operand, .pipeline-source-code, .pipeline-native-code, .pipeline-python-logo'), element => {
      const boundsToHide = measure(element);
      const exclusion = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      exclusion.setAttribute('x', boundsToHide.left - bounds.left - 3);
      exclusion.setAttribute('y', boundsToHide.top - bounds.top - 3);
      exclusion.setAttribute('width', boundsToHide.width + 6);
      exclusion.setAttribute('height', boundsToHide.height + 6);
      exclusion.setAttribute('fill', 'black');
      return exclusion;
    });
    mask.replaceChildren(backdrop, ...exclusions);
    const active = Array.from(diagram.querySelectorAll('[data-pipeline-node]'))
      .find(node => node.dataset.pipelineNode === diagram.dataset.active);
    if (active) {
      const rect = measure(active);
      const note = notes.querySelector(`[data-pipeline-info="${active.dataset.pipelineNode}"]`);
      const noteHeight = measure(note).height;
      const noteBounds = measure(notes);
      const top = Math.max(8, Math.min(noteBounds.height - noteHeight - 8,
        rect.top + rect.height / 2 - noteBounds.top - noteHeight / 2));
      notes.style.setProperty('--pipeline-note-top', `${top}px`);
    }
  }
  const resize = () => update();
  const scroll = () => { if (diagram.dataset.active) update(); };
  const observer = new ResizeObserver(resize);
  observer.observe(diagram);
  window.addEventListener('resize', resize);
  window.addEventListener('scroll', scroll, {passive: true});
  update();
  return {
    update,
    dispose() {
      observer.disconnect();
      window.removeEventListener('resize', resize);
      window.removeEventListener('scroll', scroll);
      connectors.replaceChildren();
      foreground.remove();
    },
  };
}

export function connectInteractions(diagram, display, signal) {
  const options = {signal};
  const document = diagram.ownerDocument;
  const viewport = document.defaultView;
  const reducedMotion = viewport.matchMedia('(prefers-reduced-motion: reduce)');
  const nodes = Array.from(diagram.querySelectorAll('[data-pipeline-node]'));
  const labels = Array.from(diagram.querySelectorAll('[data-pipeline-label]'));
  let hovered;
  let focused;
  let current;
  let timer;
  let inView = false;
  let windowFocused = true;

  function stop() {
    viewport.clearTimeout(timer);
    timer = undefined;
  }

  function canPlay() {
    return !signal.aborted && nodes.length > 0 && inView && windowFocused
      && !document.hidden && !reducedMotion.matches && !hovered && !focused;
  }

  function schedule(delay) {
    stop();
    if (!canPlay()) return;
    timer = viewport.setTimeout(() => {
      timer = undefined;
      if (!canPlay()) return;
      current = nodes[(nodes.indexOf(current) + 1) % nodes.length];
      display(current, false, true);
      schedule(STAGE_DURATION_MS);
    }, delay);
  }

  function inspect() {
    stop();
    const node = hovered || focused;
    if (node) {
      current = node;
      display(current);
    } else {
      schedule(RESUME_DELAY_MS);
    }
  }

  for (const node of nodes) node.tabIndex = -1;
  for (const label of labels) {
    const node = nodes.find(pane => pane.dataset.pipelineNode === label.dataset.pipelineLabel);
    if (!node) continue;
    label.tabIndex = 0;
    label.addEventListener('pointerenter', () => {
      hovered = node;
      inspect();
    }, options);
    const leave = () => {
      if (hovered !== node) return;
      hovered = undefined;
      inspect();
    };
    label.addEventListener('pointerleave', leave, options);
    label.addEventListener('pointercancel', leave, options);
    label.addEventListener('focus', () => {
      focused = node;
      inspect();
    }, options);
    label.addEventListener('blur', () => {
      if (focused === node) focused = undefined;
      inspect();
    }, options);
  }
  diagram.addEventListener('pointerleave', () => {
    if (!hovered) return;
    hovered = undefined;
    inspect();
  }, options);

  const suspend = () => {
    stop();
    hovered = undefined;
    display(current, true, true);
  };
  diagram.addEventListener('keydown', event => {
    if (event.key !== 'Escape') return;
    hovered = undefined;
    focused = undefined;
    current = undefined;
    stop();
    display(undefined, true);
    schedule(RESUME_DELAY_MS);
  }, options);
  viewport.addEventListener('blur', () => {
    windowFocused = false;
    suspend();
  }, options);
  viewport.addEventListener('focus', () => {
    windowFocused = true;
    schedule(RESUME_DELAY_MS);
  }, options);
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) suspend();
    else {
      windowFocused = true;
      schedule(current ? RESUME_DELAY_MS : INITIAL_DELAY_MS);
    }
  }, options);
  reducedMotion.addEventListener('change', () => {
    suspend();
    schedule(RESUME_DELAY_MS);
  }, options);
  const themeObserver = new viewport.MutationObserver(() => display(current, true, !hovered && !focused));
  themeObserver.observe(document.documentElement, {attributes: true, attributeFilter: ['data-theme']});
  const visibilityObserver = new viewport.IntersectionObserver(entries => {
    const visible = entries.some(entry => entry.isIntersecting && entry.intersectionRatio >= 0.2);
    if (visible === inView) return;
    inView = visible;
    if (inView) schedule(current ? RESUME_DELAY_MS : INITIAL_DELAY_MS);
    else suspend();
  }, {threshold: [0, 0.2]});
  visibilityObserver.observe(diagram);
  signal.addEventListener('abort', () => {
    stop();
    themeObserver.disconnect();
    visibilityObserver.disconnect();
  }, {once: true});
}

export function onRouteDidUpdate() {
  cleanup?.();
  cleanup = undefined;
  const diagram = document.querySelector('[data-codon-pipeline]');
  if (!diagram) return;

  const controller = new AbortController();
  const pins = diagram.querySelector('.pipeline-chip-pins');
  const contacts = [];
  for (const position of [64, 76, 88, 100, 148, 160, 172, 184, 232, 244, 256, 268]) {
    const contact = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    contact.setAttribute('d', `M${position} 4V20M${position} 324V340M4 ${position}H20M324 ${position}H340`);
    contacts.push(contact);
  }
  pins.replaceChildren(...contacts);
  const flow = connectFlow(diagram);
  let scene;
  import('three')
    .then(three => {
      if (controller.signal.aborted) return;
      scene = createScene(diagram, three, flow.update);
      scene.setActive(diagram.dataset.active);
    })
    .catch(error => console.warn('The compiler diagram is using its static layout.', error));
  const notes = Array.from(diagram.querySelectorAll('[data-pipeline-info]'));
  function display(node, immediate = false, automatic = false) {
    diagram.querySelector('.pipeline-notes').setAttribute('aria-live', automatic ? 'off' : 'polite');
    for (const note of notes) {
      note.hidden = note.dataset.pipelineInfo !== node?.dataset.pipelineNode;
    }
    diagram.dataset.active = node?.dataset.pipelineNode || '';
    if (scene) scene.setActive(node?.dataset.pipelineNode, immediate);
    else flow.update();
  }
  connectInteractions(diagram, display, controller.signal);
  cleanup = () => {
    controller.abort();
    flow.dispose();
    scene?.dispose();
  };
}
