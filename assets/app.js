const games = [
  { id: "SpyrosAdventure", label: "Spyro's Adventure", shortLabel: "Spyro", logoPaths: ["game-logos/Skylanders_Spyros_Adventure.webp", "game-logos/SpyrosAdventure.png"], order: 0 },
  { id: "Giants", label: "Giants", shortLabel: "Giants", logoPaths: ["game-logos/Skylanders_Giants.webp", "game-logos/Giants.png"], order: 1 },
  { id: "SwapForce", label: "Swap Force", shortLabel: "Swap Force", logoPaths: ["game-logos/Skylanders_Swap_Force.webp", "game-logos/SwapForce.png"], order: 2 },
  { id: "TrapTeam", label: "Trap Team", shortLabel: "Trap Team", logoPaths: ["game-logos/Skylanders_Trap_Team.webp", "game-logos/TrapTeam.png"], order: 3 },
  { id: "SuperChargers", label: "SuperChargers", shortLabel: "Chargers", logoPaths: ["game-logos/Skylanders_Super_Chargers.webp", "game-logos/SuperChargers.png"], order: 4 },
  { id: "Imaginators", label: "Imaginators", shortLabel: "Imagine", logoPaths: ["game-logos/Skylanders_Imaginators.webp", "game-logos/Imaginators.png"], order: 5 }
];

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

const proControls = [
  { id: 1, key: "A", label: "A", x: 774, y: 226, lx: 905, ly: 152 },
  { id: 2, key: "B", label: "B", x: 712, y: 286, lx: 905, ly: 220 },
  { id: 3, key: "X", label: "X", x: 714, y: 168, lx: 905, ly: 288 },
  { id: 4, key: "Y", label: "Y", x: 650, y: 226, lx: 905, ly: 356 },
  { id: 5, key: "L", label: "L", x: 260, y: 82, lx: 116, ly: 60 },
  { id: 6, key: "R", label: "R", x: 740, y: 82, lx: 884, ly: 60 },
  { id: 7, key: "ZL", label: "ZL", x: 230, y: 28, lx: 116, ly: 120 },
  { id: 8, key: "ZR", label: "ZR", x: 770, y: 28, lx: 884, ly: 120 },
  { id: 9, key: "Plus", label: "+", x: 552, y: 210, lx: 620, ly: 442 },
  { id: 10, key: "Minus", label: "-", x: 448, y: 210, lx: 380, ly: 442 },
  { id: 11, key: "Home", label: "Home", x: 500, y: 278, lx: 500, ly: 464 },
  { id: 12, key: "DPadUp", label: "D-Up", x: 250, y: 202, lx: 96, ly: 188 },
  { id: 13, key: "DPadDown", label: "D-Down", x: 250, y: 306, lx: 96, ly: 256 },
  { id: 14, key: "DPadLeft", label: "D-Left", x: 198, y: 254, lx: 96, ly: 324 },
  { id: 15, key: "DPadRight", label: "D-Right", x: 302, y: 254, lx: 96, ly: 392 },
  { id: 16, key: "LeftClick", label: "L3", x: 370, y: 330, lx: 270, ly: 72 },
  { id: 17, key: "RightClick", label: "R3", x: 630, y: 330, lx: 730, ly: 72 },
  { id: 18, key: "LeftUp", label: "LS Up", x: 370, y: 274, lx: 270, ly: 132, axis: true },
  { id: 19, key: "LeftDown", label: "LS Down", x: 370, y: 386, lx: 270, ly: 192, axis: true },
  { id: 20, key: "LeftLeft", label: "LS Left", x: 314, y: 330, lx: 270, ly: 252, axis: true },
  { id: 21, key: "LeftRight", label: "LS Right", x: 426, y: 330, lx: 270, ly: 312, axis: true },
  { id: 22, key: "RightUp", label: "RS Up", x: 630, y: 274, lx: 730, ly: 132, axis: true },
  { id: 23, key: "RightDown", label: "RS Down", x: 630, y: 386, lx: 730, ly: 192, axis: true },
  { id: 24, key: "RightLeft", label: "RS Left", x: 574, y: 330, lx: 730, ly: 252, axis: true },
  { id: 25, key: "RightRight", label: "RS Right", x: 686, y: 330, lx: 730, ly: 312, axis: true }
];

const defaultKeyboardButtons = {
  1: 90, 2: 88, 3: 67, 4: 86, 5: 81, 6: 69, 7: 49, 8: 51,
  9: 13, 10: 8, 11: 72, 12: 38, 13: 40, 14: 37, 15: 39,
  16: 160, 17: 161, 18: 87, 19: 83, 20: 65, 21: 68,
  22: 73, 23: 75, 24: 74, 25: 76
};

const defaultXInputButtons = {
  1: 13, 2: 12, 3: 15, 4: 14, 5: 8, 6: 9, 7: 42, 8: 43,
  9: 4, 10: 5, 12: 0, 13: 1, 14: 2, 15: 3, 16: 6, 17: 7,
  18: 37, 19: 42, 20: 41, 21: 36, 22: 39, 23: 44, 24: 43, 25: 38
};

const defaultSDLButtons = {
  1: 1, 2: 0, 3: 3, 4: 2, 5: 9, 6: 10, 7: 42, 8: 43,
  9: 6, 10: 4, 12: 11, 13: 12, 14: 13, 15: 14, 16: 7, 17: 8,
  18: 42, 19: 37, 20: 41, 21: 36, 22: 44, 23: 39, 24: 43, 25: 38
};

const keyNames = {
  8: "Backspace", 13: "Enter", 37: "Left", 38: "Up", 39: "Right", 40: "Down",
  49: "1", 51: "3", 65: "A", 67: "C", 68: "D", 69: "E", 72: "H", 73: "I",
  74: "J", 75: "K", 76: "L", 81: "Q", 83: "S", 86: "V", 87: "W", 88: "X",
  90: "Z", 160: "Left Shift", 161: "Right Shift"
};

const xinputNames = {
  0: "D-Pad Up", 1: "D-Pad Down", 2: "D-Pad Left", 3: "D-Pad Right",
  4: "Start", 5: "Back", 6: "Left Stick", 7: "Right Stick", 8: "LB", 9: "RB",
  12: "A", 13: "B", 14: "X", 15: "Y", 36: "Left X+", 37: "Left Y+",
  38: "Right X+", 39: "Right Y+", 41: "Left X-", 42: "Left Y-",
  43: "Right X-", 44: "Right Y-"
};

const sdlNames = {
  0: "A", 1: "B", 2: "X", 3: "Y", 4: "Back", 6: "Start", 7: "Left Stick",
  8: "Right Stick", 9: "LB", 10: "RB", 11: "D-Pad Up", 12: "D-Pad Down",
  13: "D-Pad Left", 14: "D-Pad Right", 36: "Left X+", 37: "Left Y+",
  38: "Right X+", 39: "Right Y+", 41: "Left X-", 42: "Left Y-",
  43: "Right X-", 44: "Right Y-"
};

const toyCategories = [
  { kind: "Skylander", label: "Skylanders" },
  { kind: "Item", label: "Items" },
  { kind: "Trap", label: "Traps" },
  { kind: "Vehicle", label: "Vehicles" },
  { kind: "Trophy", label: "Trophies" }
];

let nativeState = {};
let activeGame = "Giants";
let activeToyKind = "Skylander";
let activeSlot = 0;
let activeIndex = 0;
let placedSlots = ["Empty", "Empty", "Empty", "Empty"];
let placedSlotCharacters = [null, null, null, null];
let activeMappingPlayer = 1;
let inputDevices = [];
let devicesScanned = false;
let devicesScanning = false;
let deviceScanTimer = 0;
let pendingInitialDeviceScan = true;
let mappingState = {
  players: {
    1: { enabled: true, device: { api: "Keyboard", uuid: "keyboard", displayName: "Keyboard" }, entries: {} },
    2: { enabled: false, device: null, entries: {} }
  },
  overlayToggle: { bindings: [{ type: "xinput", xinputMask: 48, label: "Start + Back" }] }
};
let captureTarget = null;
let activeCaptureSession = null;
let saveTimer = 0;
let overlayXinputCapture = null;
let hasReceivedNativeState = false;
const imageExtensions = ["png", "jpg", "jpeg", "webp"];

const qs = (selector) => document.querySelector(selector);
const qsa = (selector) => Array.from(document.querySelectorAll(selector));

function gameMeta(gameId) {
  return games.find((game) => game.id === gameId) || games[0];
}

function allowedGamesForStub(currentStubGame) {
  const current = gameMeta(currentStubGame);
  if (!current || !games.some((game) => game.id === currentStubGame)) {
    return games;
  }
  return games.filter((game) => game.order <= current.order);
}

function clampActiveGame() {
  const allowed = allowedGamesForStub(nativeState.game);
  if (!allowed.some((game) => game.id === activeGame)) {
    activeGame = allowed[allowed.length - 1].id;
    activeIndex = 0;
  }
}

function catalogEntries() {
  return nativeState.characters || [];
}

function characterById(id) {
  return catalogEntries().find((entry) => entry.id === id) || null;
}

function entriesReleasedIn(game) {
  return catalogEntries().filter((entry) => entry.releaseGame === game);
}

function entriesForCategory(game, toyKind) {
  return entriesReleasedIn(game).filter((entry) => entry.toyKind === toyKind);
}

function clampActiveToyKind() {
  if (entriesForCategory(activeGame, activeToyKind).length > 0) {
    return;
  }
  const firstAvailable = toyCategories.find((category) => entriesForCategory(activeGame, category.kind).length > 0);
  if (firstAvailable) {
    activeToyKind = firstAvailable.kind;
    activeIndex = 0;
  }
}

function charactersReleasedIn(game) {
  return entriesForCategory(game, activeToyKind);
}

function charactersForGame(game) {
  return charactersReleasedIn(game);
}

function initials(value) {
  return String(value || "?")
    .split(/\s+/)
    .filter(Boolean)
    .map((part) => part[0])
    .join("")
    .replace(/[^a-z0-9]/gi, "")
    .slice(0, 3)
    .toUpperCase() || "?";
}

function elementKey(element) {
  return String(element || "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "_")
    .replace(/^_+|_+$/g, "");
}

function renderImageCandidates(character) {
  if (!character) return [];
  const paths = [];
  const key = character.assetKey || character.id;
  if (character.renderPath) {
    paths.push(character.renderPath);
  }
  if (character.imagePath && character.imagePath !== character.renderPath) {
    paths.push(character.imagePath);
  }
  if (character.releaseGame && key) {
    const base = `characters/renders/_review/${character.releaseGame}/${key}`;
    imageExtensions.forEach((extension) => paths.push(`${base}.${extension}`));
  }
  return [...new Set(paths)];
}

function baseVariantFor(character) {
  if (!character || !character.baseName || !character.isVariant) return null;
  const sameHalf = (entry) => (entry.swapHalf || "") === (character.swapHalf || "");
  const sameKindAndBase = (entry) =>
    entry !== character &&
    entry.toyKind === character.toyKind &&
    entry.baseName === character.baseName &&
    !entry.isVariant &&
    sameHalf(entry);

  return catalogEntries().find((entry) => sameKindAndBase(entry) && entry.releaseGame === character.releaseGame) ||
    catalogEntries().find(sameKindAndBase) ||
    null;
}

function renderImageCandidatesWithFallback(character) {
  const paths = renderImageCandidates(character);
  const fallback = baseVariantFor(character);
  if (fallback) {
    paths.push(...renderImageCandidates(fallback));
  }
  return [...new Set(paths)];
}

function iconImageCandidates(character) {
  if (!character || !character.releaseGame) return [];
  const key = character.assetKey || character.id;
  if (!key) return [];
  const paths = [];
  if (character.iconPath) {
    paths.push(character.iconPath);
  }
  const base = `characters/icons/${character.releaseGame}/${key}`;
  const reviewBase = `characters/icons/_review/${character.releaseGame}/${key}`;
  return [
    ...paths,
    ...imageExtensions.map((extension) => `${base}.${extension}`),
    ...imageExtensions.map((extension) => `${reviewBase}.${extension}`),
    ...renderImageCandidates(character),
    ...(baseVariantFor(character) ? iconImageCandidates(baseVariantFor(character)) : [])
  ].filter((value, index, all) => all.indexOf(value) === index);
}

function elementIconCandidates(element) {
  const key = elementKey(element);
  if (!key) return [];
  return [
    ...imageExtensions.map((extension) => `elements/${key}.${extension}`),
    ...imageExtensions.map((extension) => `elements/_review/${key}.${extension}`)
  ];
}

function loadImageOnce(img, paths, key, onSuccess, onFailure) {
  const candidates = (paths || []).filter(Boolean);
  const imageKey = `${key}|${candidates.join("|")}`;

  if (img.dataset.imageKey === imageKey) {
    if (img.dataset.imageState === "loaded") {
      onSuccess();
    } else if (img.dataset.imageState === "failed") {
      onFailure();
    }
    return;
  }

  let index = 0;
  img.dataset.imageKey = imageKey;
  img.dataset.imageState = "loading";
  img.onload = () => {
    img.dataset.imageState = "loaded";
    onSuccess();
  };
  img.onerror = () => {
    index += 1;
    if (index < candidates.length) {
      img.src = candidates[index];
      return;
    }
    img.removeAttribute("src");
    img.dataset.imageState = "failed";
    onFailure();
  };

  if (candidates.length === 0) {
    img.removeAttribute("src");
    img.dataset.imageState = "failed";
    onFailure();
    return;
  }
  img.src = candidates[index];
}

function setElementIcon(img, element) {
  if (!img) return;
  const key = elementKey(element);
  img.alt = key ? `${element} element` : "";
  loadImageOnce(
    img,
    elementIconCandidates(element),
    `element:${key}`,
    () => img.classList.remove("hidden"),
    () => img.classList.add("hidden"));
}

function selectedCharacter() {
  const list = charactersForGame(activeGame);
  if (list.length === 0) return null;
  activeIndex = ((activeIndex % list.length) + list.length) % list.length;
  return list[activeIndex];
}

function syncPortalSlotsFromNative() {
  const slots = Array.isArray(nativeState.portalSlots) ? nativeState.portalSlots : [];
  for (let index = 0; index < placedSlots.length; index += 1) {
    const slot = slots.find((entry) => Number(entry.slot) === index);
    if (!slot || slot.empty) {
      placedSlots[index] = "Empty";
      placedSlotCharacters[index] = null;
      continue;
    }

    const character = characterById(slot.characterId);
    placedSlots[index] = slot.displayName || (character ? character.displayName : "Placed");
    placedSlotCharacters[index] = character || {
      id: slot.characterId,
      displayName: slot.displayName || "Placed",
      element: slot.element || ""
    };
  }
}

window.setNativeState = (state) => {
  const firstState = !hasReceivedNativeState && state && state.game;
  if (state && state.game) {
    hasReceivedNativeState = true;
  }
  nativeState = state || {};
  if (firstState && games.some((game) => game.id === nativeState.game)) {
    activeGame = nativeState.game;
  }
  syncPortalSlotsFromNative();
  clampActiveGame();
  renderAll();
  maybeStartDeviceScanning();
};

function renderAll() {
  clampActiveGame();
  clampActiveToyKind();
  renderGameSelector();
  renderCategorySelector();
  renderSlots();
  renderCharacter();
  renderDiagnostics();
  renderCarousel();
}

function renderDiagnostics() {
  const status = qs("#graphicPackStatus");
  const launch = qs("#launchDiagnostics");
  if (!status && !launch) return;
  const packs = nativeState.graphicPacks || {};
  const prep = nativeState.cemuPreparation || {};
  const warnings = Array.isArray(prep.warnings) ? prep.warnings : [];
  const selected = Array.isArray(prep.selectedGraphicPacks) ? prep.selectedGraphicPacks.length : 0;
  const missingImportant = Array.isArray(prep.graphicPacks)
    ? prep.graphicPacks.filter((pack) => pack && pack.important && !pack.selected).length
    : 0;

  if (status && packs.status === "installed") {
    status.textContent = `Graphic packs installed (${packs.installedPackCount || 0}); ${selected} active for this game.`;
  } else if (status && packs.status === "error") {
    status.textContent = packs.lastError || "Graphic pack update failed; game will launch with standard Cemu settings.";
  } else if (status && (missingImportant > 0 || warnings.length > 0)) {
    status.textContent = "Graphic packs missing or outdated; game will launch with standard Cemu settings.";
  } else if (status && (packs.status === "checking" || packs.status === "extracting")) {
    status.textContent = "Graphic packs update in progress...";
  } else if (status) {
    status.textContent = `Graphic packs: ${packs.status || "not checked"}.`;
  }

  if (launch) {
    const lines = [
      `Cemu: ${nativeState.cemuSource || "unresolved"} (${nativeState.cemuBackendMode || "bundled_external"})`,
      `ROM: ${nativeState.hasGamePath ? "resolved" : "missing"}`,
      `Keys: ${nativeState.keysStatus || "unknown"}`,
      `Portal bridge: ${nativeState.portalReady ? "ready" : "waiting"}`,
      `Input bridge: ${nativeState.inputBridgeReady ? "ready" : "waiting"}`
    ];
    if (Array.isArray(warnings) && warnings.length > 0) {
      lines.push(`Warnings: ${warnings.length}`);
    }
    launch.innerHTML = lines.map((line) => `<span>${escapeHtml(line)}</span>`).join("");
  }
}

function renderCategorySelector() {
  qsa(".category-button").forEach((button) => {
    const kind = button.dataset.toyKind;
    const count = entriesForCategory(activeGame, kind).length;
    const meta = toyCategories.find((category) => category.kind === kind);
    button.classList.toggle("active", kind === activeToyKind);
    button.disabled = count === 0;
    button.textContent = `${meta ? meta.label : kind} ${count}`;
  });
}

function renderGameSelector() {
  const meta = gameMeta(activeGame);
  const allowed = allowedGamesForStub(nativeState.game);
  const logo = qs("#gameLogo");
  const fallback = qs("#gameLogoFallback");
  const current = qs("#currentGame");

  current.title = meta.label;
  fallback.textContent = meta.label;
  loadImageOnce(
    logo,
    meta.logoPaths || [],
    `logo:${meta.id}`,
    () => {
      logo.classList.remove("hidden");
      fallback.classList.add("hidden");
    },
    () => {
      logo.classList.add("hidden");
      fallback.classList.remove("hidden");
    });
  logo.alt = meta.label;

  qs("#prevGame").disabled = allowed.length <= 1;
  qs("#nextGame").disabled = allowed.length <= 1;
}

function renderSlots() {
  qsa(".portal-slot").forEach((slot) => {
    const index = Number(slot.dataset.slot);
    const character = placedSlotCharacters[index];
    slot.classList.toggle("active", index === activeSlot);
    slot.dataset.element = character ? character.element || "" : "";
    qs(`#slot${index}`).textContent = character ? character.displayName : placedSlots[index];
    setElementIcon(qs(`#slotElement${index}`), character ? character.element : "");
  });
  qs("#clearSlotButton").disabled = placedSlots[activeSlot] === "Empty" || !nativeState.portalReady;
  const placeButton = qs("#placeButton");
  if (placeButton) {
    placeButton.disabled = !nativeState.portalReady || !selectedCharacter();
  }
}

function renderCharacter() {
  const character = selectedCharacter();
  const render = qs("#characterRender");
  const fallback = qs("#renderFallback");

  if (!character) {
    qs("#characterName").textContent = "No entries";
    qs("#characterElement").textContent = "-";
    setElementIcon(qs("#characterElementIcon"), "");
    qs("#characterRole").textContent = "-";
    qs("#characterIds").textContent = "-";
    qs("#characterSource").textContent = "-";
    render.removeAttribute("src");
    render.classList.add("hidden");
    fallback.dataset.name = "?";
    fallback.classList.remove("hidden");
    return;
  }

  qs("#characterName").textContent = character.displayName;
  qs("#characterElement").textContent = character.element || "-";
  setElementIcon(qs("#characterElementIcon"), character.element);
  qs("#characterRole").textContent = character.swapHalf
    ? `${character.role} (${character.swapHalf})`
    : character.role || character.toyKind || "-";
  qs("#characterIds").textContent = `${character.skyId}:${character.skyVar}`;
  qs("#characterSource").textContent = character.toyKind || "Cemu";
  fallback.dataset.name = initials(character.displayName);
  fallback.classList.remove("hidden");
  render.classList.add("hidden");

  render.alt = character.displayName;
  loadImageOnce(
    render,
    renderImageCandidatesWithFallback(character),
    `render:${character.assetKey || character.id}`,
    () => {
      render.classList.remove("hidden");
      fallback.classList.add("hidden");
    },
    () => {
      render.classList.add("hidden");
      fallback.classList.remove("hidden");
    });
}

function makePortraitIcon(character) {
  const icon = document.createElement("div");
  icon.className = "portrait-icon";

  const img = document.createElement("img");
  img.alt = "";
  img.classList.add("hidden");

  const fallback = document.createElement("i");
  fallback.textContent = initials(character.displayName);

  icon.appendChild(img);
  icon.appendChild(fallback);

  loadImageOnce(
    img,
    iconImageCandidates(character),
    `icon:${character.assetKey || character.id}`,
    () => {
      img.classList.remove("hidden");
      fallback.classList.add("hidden");
    },
    () => {
      img.remove();
      fallback.classList.remove("hidden");
    });
  return icon;
}

function makeElementBadge(character) {
  const badge = document.createElement("img");
  badge.className = "portrait-element hidden";
  setElementIcon(badge, character.element);
  return badge;
}

function renderCarousel() {
  const root = qs("#carousel");
  const list = charactersForGame(activeGame);
  const listKey = `${activeToyKind}:${list.map((character) => character.assetKey || character.id).join("|")}`;
  root.style.setProperty("--active-index", activeIndex);
  root.classList.remove("fit");

  if (root.dataset.listKey !== listKey) {
    root.innerHTML = "";
    root.dataset.listKey = listKey;

    list.forEach((character, index) => {
      const button = document.createElement("button");
      button.className = "portrait";
      button.title = character.displayName;
      button.dataset.index = String(index);
      button.dataset.element = character.element || "";

      const label = document.createElement("span");
      label.textContent = character.displayName;

      button.appendChild(makePortraitIcon(character));
      button.appendChild(label);
      button.appendChild(makeElementBadge(character));
      button.addEventListener("click", () => {
        activeIndex = index;
        renderCharacter();
        renderCarousel();
      });
      root.appendChild(button);
    });
  }

  const cards = Array.from(root.querySelectorAll(".portrait"));
  cards.forEach((button) => {
    const index = Number(button.dataset.index);
    button.classList.toggle("active", index === activeIndex);
  });

  const viewport = root.parentElement;
  const viewportWidth = viewport ? viewport.clientWidth : 0;
  const selected = cards[activeIndex];
  const activeOverflow = selected ? Math.ceil(selected.offsetWidth * 0.16) + 18 : 0;
  const fadePadding = viewportWidth > 0 ? Math.ceil(viewportWidth * 0.04) + 10 : 0;
  const totalWidth = root.scrollWidth;

  if (viewportWidth > 0 && totalWidth + activeOverflow <= viewportWidth) {
    root.classList.add("fit");
    root.style.transform = "translateX(-50%)";
    return;
  }

  if (!selected || viewportWidth <= 0) {
    root.style.transform = "translateX(0px)";
    return;
  }

  const selectedLeft = selected.offsetLeft;
  const selectedRight = selectedLeft + selected.offsetWidth;
  const maxOffset = Math.max(0, totalWidth + activeOverflow + fadePadding - viewportWidth);
  const desiredOffset = Math.max(
    0,
    selectedLeft - (viewportWidth * 0.38),
    selectedRight + activeOverflow + fadePadding - viewportWidth);
  const offset = Math.min(maxOffset, desiredOffset);
  root.style.transform = `translateX(-${Math.round(offset)}px)`;
}

function cycle(delta) {
  const list = charactersForGame(activeGame);
  if (list.length === 0) return;
  activeIndex = (activeIndex + delta + list.length) % list.length;
  renderAll();
}

function setSelectedGameByDelta(delta) {
  const allowed = allowedGamesForStub(nativeState.game);
  if (allowed.length === 0) return;
  const currentIndex = Math.max(0, allowed.findIndex((game) => game.id === activeGame));
  const nextIndex = (currentIndex + delta + allowed.length) % allowed.length;
  activeGame = allowed[nextIndex].id;
  activeIndex = 0;
  renderAll();
}

async function placeSelectedCharacter() {
  const character = selectedCharacter();
  if (!character) return;

  const ok = window.portal && typeof window.portal.placeOrCreate === "function"
    ? window.portal.placeOrCreate(activeSlot, character.id)
    : false;

  if (ok) {
    syncPortalSlotsFromNative();
    qs("#statusText").textContent = `Slot ${activeSlot + 1}: ${character.displayName}`;
    if (window.hideOverlay && typeof window.hideOverlay === "function") {
      window.hideOverlay();
    }
  } else {
    qs("#statusText").textContent = nativeState.lastPortalError || nativeState.lastError || "Portal bridge unavailable";
  }
  renderSlots();
}

function clearSlot(slot) {
  const ok = window.portal && typeof window.portal.remove === "function"
    ? window.portal.remove(slot)
    : false;
  if (ok) {
    syncPortalSlotsFromNative();
    qs("#statusText").textContent = `Slot ${slot + 1} cleared`;
  } else {
    qs("#statusText").textContent = nativeState.lastPortalError || nativeState.lastError || "Portal bridge unavailable";
  }
  renderSlots();
}

function clearActiveSlot() {
  clearSlot(activeSlot);
}

function showSelector() {
  qs("#selectorView").classList.remove("hidden");
  qs("#mappingView").classList.add("hidden");
}

function showMapping() {
  qs("#selectorView").classList.add("hidden");
  qs("#mappingView").classList.remove("hidden");
  renderMapping();
}

function loadMappings() {
  if (!window.config || typeof window.config.loadInputMappings !== "function") return;
  try {
    const loaded = JSON.parse(window.config.loadInputMappings() || "{}");
    mappingState = normalizeMappingState(loaded);
  } catch {
    mappingState = normalizeMappingState({});
  }
}

function normalizeMappingState(value) {
  const state = {
    players: {
      1: { enabled: true, device: { api: "Keyboard", uuid: "keyboard", displayName: "Keyboard" }, entries: defaultKeyboardEntries() },
      2: { enabled: false, device: null, entries: {} }
    },
    overlayToggle: { bindings: [{ type: "xinput", xinputMask: 48, label: "Start + Back" }] }
  };
  if (value && value.players) {
    [1, 2].forEach((player) => {
      const incoming = value.players[player] || value.players[String(player)];
      if (!incoming) return;
      state.players[player] = {
        enabled: player === 1 ? true : Boolean(incoming.enabled),
        device: incoming.device || state.players[player].device,
        entries: { ...state.players[player].entries, ...(incoming.entries || {}) }
      };
    });
  }
  if (value && value.overlayToggle) {
    if (Array.isArray(value.overlayToggle.bindings)) {
      state.overlayToggle = {
        bindings: value.overlayToggle.bindings
          .filter((binding) => binding && (binding.type === "keyboard" || binding.type === "xinput" || binding.type === "cemu"))
          .map((binding) => ({ ...binding }))
      };
    } else {
      const bindings = [];
      if (value.overlayToggle.keyboardVk) {
        bindings.push({
          type: "keyboard",
          keyboardVk: Number(value.overlayToggle.keyboardVk),
          label: value.overlayToggle.label || `Key ${value.overlayToggle.keyboardVk}`
        });
      }
      if (value.overlayToggle.xinputMask) {
        bindings.push({
          type: "xinput",
          xinputMask: Number(value.overlayToggle.xinputMask),
          label: value.overlayToggle.label || "Controller Combo"
        });
      }
      state.overlayToggle = { bindings: bindings.length > 0 ? bindings : state.overlayToggle.bindings };
    }
  }
  return state;
}

function defaultKeyboardEntries() {
  return defaultEntriesForDevice(keyboardDevice());
}

function defaultEntriesForDevice(device) {
  const api = device && device.api;
  if (api === "Keyboard") {
    return entriesFromButtonMap(device, defaultKeyboardButtons, keyNames);
  }
  if (api === "XInput") {
    return entriesFromButtonMap(device, defaultXInputButtons, xinputNames);
  }
  if (api === "SDLController") {
    return entriesFromButtonMap(device, defaultSDLButtons, sdlNames);
  }
  return {};
}

function entriesFromButtonMap(device, buttonMap, labels) {
  const entries = {};
  proControls.forEach((control) => {
    const button = buttonMap[control.id];
    if (button === undefined) return;
    entries[control.id] = {
      proControlId: control.id,
      sourceApi: device.api,
      sourceUuid: device.uuid,
      sourceInputId: button,
      displayLabel: labels[button] || `Input ${button}`,
      deadzone: control.axis ? 0.25 : undefined,
      range: control.axis ? 1 : undefined
    };
  });
  return entries;
}

function playerProfile() {
  return mappingState.players[activeMappingPlayer];
}

function deviceKey(device) {
  return `${device ? device.api : ""}:${device ? device.uuid : ""}`;
}

function keyboardDevice() {
  return { api: "Keyboard", uuid: "keyboard", displayName: "Keyboard", connected: true, hasAxis: false };
}

function sameDeviceList(left, right) {
  if (left.length !== right.length) return false;
  return left.every((device, index) => {
    const other = right[index];
    return other &&
      device.api === other.api &&
      device.uuid === other.uuid &&
      device.displayName === other.displayName &&
      Boolean(device.connected) === Boolean(other.connected);
  });
}

function refreshInputDevices() {
  const nextDevices = [keyboardDevice()];
  if (!window.inputBridge || typeof window.inputBridge.listDevices !== "function") {
    const changed = !sameDeviceList(inputDevices, nextDevices);
    inputDevices = nextDevices;
    devicesScanned = true;
    return { ok: false, changed };
  }
  try {
    const response = JSON.parse(window.inputBridge.listDevices() || "{}");
    if (response.ok && Array.isArray(response.devices)) {
      const seen = new Set(nextDevices.map(deviceKey));
      response.devices.forEach((device) => {
        const key = deviceKey(device);
        if (seen.has(key)) return false;
        seen.add(key);
        nextDevices.push(device);
      });
    }
    const changed = !sameDeviceList(inputDevices, nextDevices);
    inputDevices = nextDevices;
    devicesScanned = true;
    return { ok: true, changed };
  } catch {
    const changed = !sameDeviceList(inputDevices, nextDevices);
    inputDevices = nextDevices;
    devicesScanned = true;
    return { ok: false, changed };
  }
}

function scanInputDevices(statusText = "Scanning input devices...", options = {}) {
  if (devicesScanning) return;
  if (captureTarget || activeCaptureSession || overlayXinputCapture) return;
  devicesScanning = true;
  const silent = Boolean(options.silent);
  if (!silent) renderMappingStatus(statusText);
  const refreshButton = qs("#refreshDevices");
  if (refreshButton) refreshButton.disabled = true;
  setTimeout(() => {
    const result = refreshInputDevices();
    devicesScanning = false;
    if (refreshButton) refreshButton.disabled = false;
    if (result.changed || !devicesScanned) {
      renderDeviceSelect();
    }
    if (!silent) {
      renderMappingStatus(result.ok ? "Input devices refreshed." : "Using keyboard mapping only.");
    }
  }, 20);
}

function maybeStartDeviceScanning() {
  if (!window.inputBridge || !nativeState.running) return;
  if (pendingInitialDeviceScan && nativeState.inputBridgeReady) {
    pendingInitialDeviceScan = false;
    scanInputDevices("Scanning input devices...", { silent: qs("#mappingView").classList.contains("hidden") });
  }
  if (deviceScanTimer) return;
  deviceScanTimer = setInterval(() => {
    if (!nativeState.running || !nativeState.inputBridgeReady) return;
    scanInputDevices("Scanning input devices...", { silent: true });
  }, 30000);
}

function renderMapping() {
  qsa(".player-tab").forEach((button) => {
    button.classList.toggle("active", Number(button.dataset.player) === activeMappingPlayer);
  });

  renderDeviceSelect();
  renderMappingList();
  renderOverlayToggleList();
  renderMappingStatus();
}

function renderDeviceSelect() {
  const select = qs("#deviceSelect");
  const profile = playerProfile();
  if (!profile.device) {
    profile.device = keyboardDevice();
  }
  const currentKey = deviceKey(profile.device);
  if (!inputDevices.some((device) => deviceKey(device) === currentKey)) {
    inputDevices = [...inputDevices, { ...profile.device, displayName: `${profile.device.displayName || profile.device.uuid} (saved)` }];
  }

  select.innerHTML = "";
  inputDevices.forEach((device) => {
    const option = document.createElement("option");
    option.value = deviceKey(device);
    option.textContent = `${device.displayName || device.uuid} (${device.api})`;
    select.appendChild(option);
  });
  select.value = currentKey;
  qs("#stickDeadzone").value = String(stickSetting("deadzone", 0.25));
  qs("#stickRange").value = String(stickSetting("range", 1));
}

function stickSetting(field, fallback) {
  const profile = playerProfile();
  const axisEntry = Object.values(profile.entries || {}).find((entry) => entry && entry.deadzone !== undefined);
  return axisEntry && axisEntry[field] !== undefined ? axisEntry[field] : fallback;
}

function controlGroup(control) {
  if ([1, 2, 3, 4].includes(control.id)) return "Face Buttons";
  if ([5, 6, 7, 8].includes(control.id)) return "Shoulders";
  if ([12, 13, 14, 15].includes(control.id)) return "D-Pad";
  if ([9, 10, 11].includes(control.id)) return "System";
  if ([16, 18, 19, 20, 21].includes(control.id)) return "Left Stick";
  return "Right Stick";
}

function renderMappingList() {
  const root = qs("#mappingList");
  const profile = playerProfile();
  const listKey = `${activeMappingPlayer}:${deviceKey(profile.device || keyboardDevice())}`;
  const groups = ["Face Buttons", "Shoulders", "D-Pad", "Left Stick", "Right Stick", "System"];

  if (root.dataset.listKey === listKey) {
    proControls.forEach((control) => updateMappingRow(control.id));
    return;
  }

  root.dataset.listKey = listKey;
  root.innerHTML = "";
  groups.forEach((group) => {
    const section = document.createElement("section");
    section.className = "mapping-group";
    const title = document.createElement("h3");
    title.textContent = group;
    section.appendChild(title);

    proControls.filter((control) => controlGroup(control) === group).forEach((control) => {
      const entry = profile.entries[String(control.id)] || profile.entries[control.id];
      const row = document.createElement("button");
      row.className = "mapping-row-button";
      row.dataset.mapping = String(control.id);
      row.innerHTML = `<span>${control.label}</span><strong>${entry ? entry.displayLabel : "Unmapped"}</strong>`;
      row.addEventListener("click", () => beginCapture(control.id));
      section.appendChild(row);
    });

    root.appendChild(section);
  });
}

function updateMappingRow(mappingId) {
  const row = qs(`.mapping-row-button[data-mapping="${mappingId}"]`);
  if (!row) return;
  const profile = playerProfile();
  const entry = profile.entries[String(mappingId)] || profile.entries[mappingId];
  const value = row.querySelector("strong");
  if (value) value.textContent = entry ? entry.displayLabel : "Unmapped";
  row.classList.toggle("capturing", captureTarget && captureTarget.mappingId === mappingId);
}

function renderMappingStatus(text) {
  const bridgeReady = Boolean(nativeState.inputBridgeReady);
  const profile = playerProfile();
  qs("#mappingStatus").textContent = text || (bridgeReady
    ? `${profile.device ? profile.device.displayName : "No device"} selected for Player ${activeMappingPlayer}`
    : "Controller mapping requires the bundled Cemu build. Keyboard defaults can still be saved.");
}

function overlayBindings() {
  if (!mappingState.overlayToggle || !Array.isArray(mappingState.overlayToggle.bindings)) {
    mappingState.overlayToggle = { bindings: [{ type: "xinput", xinputMask: 48, label: "Start + Back" }] };
  }
  return mappingState.overlayToggle.bindings;
}

function renderOverlayToggleList() {
  const root = qs("#overlayToggleList");
  if (!root) return;
  root.innerHTML = "";

  const locked = document.createElement("div");
  locked.className = "toggle-binding locked";
  locked.innerHTML = "<span>F1</span><strong>Always active</strong>";
  root.appendChild(locked);

  overlayBindings().forEach((binding, index) => {
    const row = document.createElement("div");
    row.className = "toggle-binding";
    const label = binding.label || (binding.type === "keyboard" ? `Key ${binding.keyboardVk}` : "Controller Input");
    row.innerHTML = `<span>${label}</span><button type="button" data-toggle-index="${index}">Remove</button>`;
    row.querySelector("button").addEventListener("click", () => {
      overlayBindings().splice(index, 1);
      renderOverlayToggleList();
      scheduleAutosave("Overlay toggle removed. F1 still works.");
    });
    root.appendChild(row);
  });
}

function setSaveState(text) {
  const node = qs("#mappingSaveState");
  if (node) node.textContent = text;
}

function autosaveMappings(statusText) {
  const profile = playerProfile();
  profile.enabled = true;
  if (!window.inputBridge || typeof window.inputBridge.saveProfile !== "function") {
    setSaveState("Not saved");
    renderMappingStatus(statusText || "Unable to save mapping.");
    return false;
  }
  setSaveState("Saving...");
  const ok = window.inputBridge.saveProfile(
    activeMappingPlayer,
    cemuProfileXml(activeMappingPlayer),
    JSON.stringify(mappingState, null, 2));
  setSaveState(ok ? "Autosaved" : "Save failed");
  renderMappingStatus(ok ? (statusText || "Mapping saved.") : "Unable to save mapping.");
  return ok;
}

function scheduleAutosave(statusText) {
  clearTimeout(saveTimer);
  setSaveState("Saving...");
  saveTimer = setTimeout(() => autosaveMappings(statusText), 250);
}

function beginCapture(mappingId) {
  const control = proControls.find((entry) => entry.id === mappingId);
  const profile = playerProfile();
  const device = profile.device || inputDevices[0];
  if (!device) return;

  cancelActiveCapture();
  suppressOverlayToggle();
  captureTarget = { mappingId, device };
  renderMappingStatus(device.api === "Keyboard"
    ? `Press a key for ${control.label}`
    : (control && control.axis ? `Move a stick for ${control.label}` : `Press a button for ${control.label}`));
  updateMappingRow(mappingId);

  if (device.api === "Keyboard") {
    return;
  }

  if (!window.inputBridge || typeof window.inputBridge.startCapture !== "function") {
    renderMappingStatus("Controller capture requires the bundled Cemu build.");
    captureTarget = null;
    updateMappingRow(mappingId);
    return;
  }

  setTimeout(() => {
    try {
      const response = JSON.parse(window.inputBridge.startCapture(
        device.api,
        device.uuid,
        device.displayName || "",
        mappingId,
        control && control.axis ? "axis" : "button") || "{}");
      if (!response.ok) {
        renderMappingStatus(response.error || "No input captured.");
        captureTarget = null;
        updateMappingRow(mappingId);
        return;
      }
      activeCaptureSession = {
        id: response.captureId,
        mappingId,
        startedAt: Date.now(),
        timer: setInterval(() => pollCaptureSession(), 35)
      };
    } catch {
      renderMappingStatus("No input captured.");
      captureTarget = null;
      updateMappingRow(mappingId);
    }
  }, 30);
}

function pollCaptureSession() {
  if (!activeCaptureSession || !window.inputBridge || typeof window.inputBridge.pollCapture !== "function") {
    return;
  }

  if (Date.now() - activeCaptureSession.startedAt > 5500) {
    cancelActiveCapture();
    renderMappingStatus("No input captured.");
    return;
  }

  try {
    const response = JSON.parse(window.inputBridge.pollCapture(activeCaptureSession.id) || "{}");
    if (response.pending) return;
    const session = activeCaptureSession;
    clearInterval(session.timer);
    activeCaptureSession = null;

    if (!response.ok) {
      renderMappingStatus(response.error || "No input captured.");
      captureTarget = null;
      updateMappingRow(session.mappingId);
      return;
    }
    applyCapturedInput(session.mappingId, captureTarget ? captureTarget.device : playerProfile().device, response.sourceInputId, response.displayLabel, response.axisSign);
  } catch {
    cancelActiveCapture();
    renderMappingStatus("No input captured.");
  }
}

function cancelActiveCapture() {
  if (!activeCaptureSession) return;
  const session = activeCaptureSession;
  clearInterval(session.timer);
  activeCaptureSession = null;
  if (window.inputBridge && typeof window.inputBridge.cancelCapture === "function") {
    try { window.inputBridge.cancelCapture(session.id); } catch {}
  }
  const mappingId = captureTarget ? captureTarget.mappingId : session.mappingId;
  captureTarget = null;
  updateMappingRow(mappingId);
}

function applyCapturedInput(mappingId, device, sourceInputId, displayLabel, axisSign) {
  const control = proControls.find((entry) => entry.id === mappingId);
  const profile = playerProfile();
  profile.enabled = true;
  profile.device = device;
  profile.entries[mappingId] = {
    proControlId: mappingId,
    sourceApi: device.api,
    sourceUuid: device.uuid,
    sourceInputId,
    displayLabel,
    axisSign,
    deadzone: control && control.axis ? Number(qs("#stickDeadzone").value || 0.25) : undefined,
    range: control && control.axis ? Number(qs("#stickRange").value || 1) : undefined
  };
  captureTarget = null;
  updateMappingRow(mappingId);
  scheduleAutosave(`${control ? control.label : "Input"} mapped to ${displayLabel}`);
}

function cemuProfileXml(playerIndex) {
  const profile = mappingState.players[playerIndex];
  const device = profile.device || { api: "Keyboard", uuid: "keyboard", displayName: "Keyboard" };
  const profileName = playerIndex === 1 ? "skylanders_player1" : "skylanders_player2";
  const entries = proControls
    .map((control) => profile.entries[String(control.id)] || profile.entries[control.id])
    .filter(Boolean);
  const deadzone = Number(qs("#stickDeadzone").value || 0.25);
  const range = Number(qs("#stickRange").value || 1);

  const escapeXml = (value) => String(value || "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");

  return `<?xml version="1.0" encoding="UTF-8"?>
<emulated_controller>
  <type>Wii U Pro Controller</type>
  <profile>${profileName}</profile>
  <controller>
    <api>${escapeXml(device.api)}</api>
    <uuid>${escapeXml(device.uuid)}</uuid>
    <display_name>${escapeXml(device.displayName || device.uuid)}</display_name>
    <axis><deadzone>${deadzone}</deadzone><range>${range}</range></axis>
    <rotation><deadzone>${deadzone}</deadzone><range>${range}</range></rotation>
    <trigger><deadzone>${deadzone}</deadzone><range>${range}</range></trigger>
    <mappings>
${entries.map((entry) => `      <entry><mapping>${entry.proControlId}</mapping><button>${entry.sourceInputId}</button></entry>`).join("\n")}
    </mappings>
  </controller>
</emulated_controller>
`;
}

function resetCurrentMapping() {
  mappingState.players[activeMappingPlayer] = activeMappingPlayer === 1
    ? { enabled: true, device: { api: "Keyboard", uuid: "keyboard", displayName: "Keyboard" }, entries: defaultKeyboardEntries() }
    : { enabled: true, device: inputDevices[0] || null, entries: {} };
  qs("#mappingList").dataset.listKey = "";
  renderMapping();
  scheduleAutosave("Player mapping reset.");
}

function selectedDeviceFromPicker() {
  const key = qs("#deviceSelect").value || "";
  return inputDevices.find((device) => deviceKey(device) === key) || inputDevices[0] || null;
}

function keyToVirtualKey(event) {
  if (/^Key[A-Z]$/.test(event.code)) return event.code.charCodeAt(3);
  if (/^Digit[0-9]$/.test(event.code)) return event.code.charCodeAt(5);
  const map = {
    Enter: 13, Backspace: 8, ShiftLeft: 160, ShiftRight: 161, ArrowUp: 38,
    ArrowDown: 40, ArrowLeft: 37, ArrowRight: 39, Space: 32, Escape: 27
  };
  return map[event.code] || event.keyCode || 0;
}

function suppressOverlayToggle() {
  if (window.inputBridge && typeof window.inputBridge.suppressOverlayToggle === "function") {
    try { window.inputBridge.suppressOverlayToggle(); } catch {}
  }
}

function beginOverlayXinputCapture() {
  if (!window.inputBridge || typeof window.inputBridge.readXinputButtons !== "function") {
    renderMappingStatus("Controller overlay toggle capture requires XInput support.");
    return;
  }

  renderMappingStatus("Hold the controller button combo for the overlay toggle. F1 will remain active.");
  suppressOverlayToggle();
  let bestMask = 0;
  let bestLabel = "";
  const startedAt = Date.now();
  overlayXinputCapture = setInterval(() => {
    try {
      const response = JSON.parse(window.inputBridge.readXinputButtons() || "{}");
      if (response.ok && response.xinputMask) {
        bestMask = Number(response.xinputMask);
        bestLabel = response.label || `XInput ${bestMask}`;
      }
    } catch {}

    if (bestMask && Date.now() - startedAt > 450) {
      clearInterval(overlayXinputCapture);
      overlayXinputCapture = null;
      overlayBindings().push({ type: "xinput", xinputMask: bestMask, label: bestLabel });
      renderOverlayToggleList();
      renderMappingStatus(`Overlay toggle added: ${bestLabel}. F1 still works.`);
      scheduleAutosave("Overlay toggle saved. Release the buttons before toggling.");
      return;
    }

    if (Date.now() - startedAt > 3500) {
      clearInterval(overlayXinputCapture);
      overlayXinputCapture = null;
      renderMappingStatus("No controller combo captured.");
    }
  }, 35);
}

function beginOverlayCemuControllerCapture(device) {
  if (!device || device.api === "Keyboard") {
    renderMappingStatus("Choose a controller device before adding a controller toggle.");
    return;
  }
  if (!window.inputBridge || typeof window.inputBridge.startCapture !== "function") {
    renderMappingStatus("Controller overlay toggle capture requires the bundled Cemu build.");
    return;
  }

  suppressOverlayToggle();
  renderMappingStatus(`Press a button on ${device.displayName || device.uuid} for the overlay toggle.`);
  try {
    const response = JSON.parse(window.inputBridge.startCapture(
      device.api,
      device.uuid,
      device.displayName || "",
      1,
      "button") || "{}");
    if (!response.ok) {
      renderMappingStatus(response.error || "No input captured.");
      return;
    }
    activeCaptureSession = {
      id: response.captureId,
      mappingId: 0,
      overlayDevice: device,
      startedAt: Date.now(),
      timer: setInterval(() => pollOverlayCemuCapture(), 35)
    };
  } catch {
    renderMappingStatus("No input captured.");
  }
}

function pollOverlayCemuCapture() {
  if (!activeCaptureSession || !activeCaptureSession.overlayDevice) {
    return;
  }
  if (Date.now() - activeCaptureSession.startedAt > 5500) {
    cancelActiveCapture();
    renderMappingStatus("No controller toggle captured.");
    return;
  }

  try {
    const response = JSON.parse(window.inputBridge.pollCapture(activeCaptureSession.id) || "{}");
    if (response.pending) return;
    const session = activeCaptureSession;
    clearInterval(session.timer);
    activeCaptureSession = null;

    if (!response.ok) {
      renderMappingStatus(response.error || "No controller toggle captured.");
      return;
    }

    overlayBindings().push({
      type: "cemu",
      api: session.overlayDevice.api,
      uuid: session.overlayDevice.uuid,
      displayName: session.overlayDevice.displayName || "",
      sourceInputId: Number(response.sourceInputId),
      label: `${response.displayLabel || "Controller Input"} (${session.overlayDevice.displayName || session.overlayDevice.api})`
    });
    renderOverlayToggleList();
    renderMappingStatus("Controller overlay toggle saved. F1 still works.");
    scheduleAutosave("Overlay toggle saved. Release the button before toggling.");
  } catch {
    cancelActiveCapture();
    renderMappingStatus("No controller toggle captured.");
  }
}

qs("#prevCharacter").addEventListener("click", () => cycle(-1));
qs("#nextCharacter").addEventListener("click", () => cycle(1));
qs("#prevGame").addEventListener("click", () => setSelectedGameByDelta(-1));
qs("#nextGame").addEventListener("click", () => setSelectedGameByDelta(1));
qs("#placeButton").addEventListener("click", placeSelectedCharacter);
qs("#clearSlotButton").addEventListener("click", clearActiveSlot);
qs("#updateGraphicPacksButton").addEventListener("click", () => {
  const button = qs("#updateGraphicPacksButton");
  button.disabled = true;
  qs("#graphicPackStatus").textContent = "Updating graphic packs...";
  setTimeout(() => {
    const ok = window.updateGraphicPacks && typeof window.updateGraphicPacks === "function"
      ? window.updateGraphicPacks()
      : false;
    button.disabled = false;
    if (!ok) {
      qs("#graphicPackStatus").textContent = nativeState.lastError || "Graphic pack update failed.";
    }
    renderDiagnostics();
  }, 20);
});
qs("#mappingButton").addEventListener("click", showMapping);
qs("#backToSelector").addEventListener("click", showSelector);
qs("#resetMapping").addEventListener("click", resetCurrentMapping);
qs("#refreshDevices").addEventListener("click", () => {
  scanInputDevices();
});
qs("#deviceSelect").addEventListener("change", () => {
  const profile = playerProfile();
  const previousApi = profile.device ? profile.device.api : "";
  const device = selectedDeviceFromPicker();
  profile.device = device;
  if (activeMappingPlayer === 2) playerProfile().enabled = true;
  if (device && device.api !== previousApi) {
    profile.entries = defaultEntriesForDevice(device);
  }
  qs("#mappingList").dataset.listKey = "";
  renderMapping();
  scheduleAutosave(device ? `Default ${device.api} mapping applied.` : "Input device changed.");
});
qs("#stickDeadzone").addEventListener("change", () => {
  Object.values(playerProfile().entries).forEach((entry) => {
    const control = proControls.find((item) => item.id === Number(entry.proControlId));
    if (control && control.axis) entry.deadzone = Number(qs("#stickDeadzone").value || 0.25);
  });
  scheduleAutosave("Stick deadzone saved.");
});
qs("#stickRange").addEventListener("change", () => {
  Object.values(playerProfile().entries).forEach((entry) => {
    const control = proControls.find((item) => item.id === Number(entry.proControlId));
    if (control && control.axis) entry.range = Number(qs("#stickRange").value || 1);
  });
  scheduleAutosave("Stick range saved.");
});
qs("#addKeyboardToggle").addEventListener("click", () => {
  cancelActiveCapture();
  suppressOverlayToggle();
  captureTarget = { overlay: true };
  renderMappingStatus("Press a keyboard key for the overlay toggle. F1 will remain active.");
});
qs("#addControllerToggle").addEventListener("click", () => {
  cancelActiveCapture();
  const device = selectedDeviceFromPicker();
  if (device && device.api === "XInput") {
    beginOverlayXinputCapture();
  } else {
    beginOverlayCemuControllerCapture(device);
  }
});

qsa(".player-tab").forEach((button) => {
  button.addEventListener("click", () => {
    activeMappingPlayer = Number(button.dataset.player);
    if (activeMappingPlayer === 2) {
      mappingState.players[2].enabled = true;
      if (!mappingState.players[2].device) {
        mappingState.players[2].device = inputDevices.find((device) => device.api !== "Keyboard") || inputDevices[0] || null;
      }
    }
    renderMapping();
  });
});

qsa(".portal-slot").forEach((slot) => {
  slot.addEventListener("click", () => {
    activeSlot = Number(slot.dataset.slot);
    renderSlots();
  });
  slot.addEventListener("contextmenu", (event) => {
    event.preventDefault();
    clearSlot(Number(slot.dataset.slot));
  });
});

qsa(".category-button").forEach((button) => {
  button.addEventListener("click", () => {
    if (button.disabled) return;
    activeToyKind = button.dataset.toyKind;
    activeIndex = 0;
    renderAll();
  });
});

document.addEventListener("keydown", (event) => {
  if (captureTarget && qs("#mappingView").classList.contains("hidden") === false) {
    event.preventDefault();
    const vk = keyToVirtualKey(event);
    if (captureTarget.overlay) {
      const label = event.key && event.key.length === 1 ? event.key.toUpperCase() : (event.key || `Key ${vk}`);
      if (vk === 112) {
        captureTarget = null;
        renderMappingStatus("F1 is already always active.");
        return;
      }
      overlayBindings().push({ type: "keyboard", keyboardVk: vk, label });
      captureTarget = null;
      renderOverlayToggleList();
      renderMappingStatus(`Overlay toggle added: ${label}. F1 still works.`);
      scheduleAutosave("Overlay toggle saved. Release the key before toggling.");
      return;
    }
    if (captureTarget.device && captureTarget.device.api === "Keyboard") {
      applyCapturedInput(
        captureTarget.mappingId,
        captureTarget.device,
        vk,
        event.key && event.key.length === 1 ? event.key.toUpperCase() : (event.key || `Key ${vk}`));
      return;
    }
  }

  if (event.key === "ArrowLeft") cycle(-1);
  if (event.key === "ArrowRight") cycle(1);
  if (event.key === "ArrowUp") setSelectedGameByDelta(-1);
  if (event.key === "ArrowDown") setSelectedGameByDelta(1);
  if (event.key === "Enter") placeSelectedCharacter();
  if (event.key === "Delete" || event.key === "Backspace") clearActiveSlot();
  if (/^[1-4]$/.test(event.key)) {
    activeSlot = Number(event.key) - 1;
    renderSlots();
  }
  if (event.key === "Escape") showSelector();
});

window.addEventListener("resize", renderCarousel);

window.prewarmOverlay = () => {
  renderAll();
  qs("#mappingList").dataset.listKey = "";
  renderMapping();
  showSelector();
};

loadMappings();
window.setNativeState(window.__nativeState || {});
