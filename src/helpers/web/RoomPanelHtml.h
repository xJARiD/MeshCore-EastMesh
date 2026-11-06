#pragma once

// EastMesh room server web panel pages. Served by WebPanelServer instead of the
// repeater observer pages when built with EASTMESH_ROOM_SERVER. Every control maps to
// a room-server CLI command sent through /api/command.

#include <Arduino.h>

static const char kRoomPanelLoginHtml[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Room Server Login</title>
  <link rel="icon" type="image/png" href="/favicon.ico">
  <style>
    :root { color-scheme:light; --accent:#2f8f4e; --accent-hover:#3fae61; --background:#f4f6f9; --text:#1f2937; --text-muted:#4b5563; --border:rgba(0,0,0,.08); --surface:#ffffff; }
    @media (prefers-color-scheme: dark) {
      :root { color-scheme:dark; --accent:#36a167; --accent-hover:#49c27d; --background:#222222; --text:#e6eaf0; --text-muted:#9aa4b2; --border:rgba(255,255,255,.08); --surface:#303030; }
    }
    html { min-height:100%; background:linear-gradient(180deg,var(--background),var(--surface)); background-attachment:fixed; }
    body { min-height:100vh; margin:0; display:grid; place-items:center; color:var(--text); font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,"Liberation Mono",monospace; }
    main { width:min(100%,420px); padding:20px; box-sizing:border-box; }
    .card { background:var(--surface); border:1px solid var(--border); border-radius:16px; padding:20px; }
    h1 { margin:0 0 12px; font-size:22px; }
    p { margin:0 0 16px; color:var(--text-muted); line-height:1.45; }
    input, button { width:100%; min-height:46px; box-sizing:border-box; border-radius:10px; font:inherit; }
    input { margin-bottom:12px; padding:12px; border:1px solid var(--border); background:transparent; color:var(--text); }
    button { border:none; background:var(--accent); color:#fff; cursor:pointer; font-weight:700; }
    button:hover { background:var(--accent-hover); }
    #status { min-height:1.4em; margin-top:12px; color:#c94a4a; white-space:pre-wrap; }
  </style>
</head>
<body>
  <main>
    <section class="card">
      <h1>Room Server Config</h1>
      <p>Use the room server admin password to unlock the panel.</p>
      <input id="password" type="password" placeholder="Admin password" autocomplete="current-password">
      <button id="loginBtn">Unlock</button>
      <div id="status"></div>
    </section>
  </main>
  <script>
    const TOKEN_KEY = "room-token";
    const statusEl = document.getElementById("status");
    async function login() {
      const pwd = document.getElementById("password").value;
      const res = await fetch("/login", { method:"POST", body: pwd });
      const text = await res.text();
      if (!res.ok) { statusEl.textContent = text || "Access denied"; return; }
      try { sessionStorage.setItem(TOKEN_KEY, text.trim()); } catch (_) {}
      window.location.replace("/app");
    }
    document.getElementById("loginBtn").onclick = () => login();
    document.getElementById("password").addEventListener("keydown", (e) => {
      if (e.key === "Enter") { e.preventDefault(); login(); }
    });
  </script>
</body>
</html>
)HTML";

static const char kRoomPanelAppHtml[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Room Server Config</title>
  <link rel="icon" type="image/png" href="/favicon.ico">
  <style>
    :root {
      color-scheme:light; --accent:#2f8f4e; --accent-hover:#3fae61; --background:#f4f6f9; --text:#1f2937; --text-muted:#4b5563;
      --border:rgba(0,0,0,.08); --surface1:#ffffff; --surface2:#f0f3f7; --input-bg:#ffffff; --terminal-bg:#f0f3f7;
      --terminal-cmd:#2f8f4e; --status-red:#c94a4a; --status-amber:#b7791f; --button-secondary-text:#1f2937;
    }
    :root[data-theme="dark"] {
      color-scheme:dark; --accent:#36a167; --accent-hover:#49c27d; --background:#222222; --text:#e6eaf0; --text-muted:#9aa4b2;
      --border:rgba(255,255,255,.08); --surface1:#303030; --surface2:#343434; --input-bg:#343434; --terminal-bg:#222222;
      --terminal-cmd:#8fd3ff; --status-red:#d45a5a; --status-amber:#e0a84a; --button-secondary-text:#e6eaf0;
    }
    html { min-height:100%; background:linear-gradient(180deg,var(--background),var(--surface2)); background-attachment:fixed; }
    body { min-height:100vh; margin:0; color:var(--text); font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,"Liberation Mono",monospace; font-size:16px; line-height:1.4; }
    main { max-width:920px; margin:0 auto; padding:24px; }
    .card { background:var(--surface1); border:1px solid var(--border); border-radius:14px; padding:18px; margin-bottom:18px; }
    h1,h2,h3 { margin:0 0 12px; font-size:18px; }
    input, select, button { width:100%; min-height:44px; box-sizing:border-box; border-radius:10px; border:1px solid var(--border); background:var(--input-bg); color:var(--text); padding:12px; font:inherit; -webkit-appearance:none; appearance:none; }
    button { width:auto; cursor:pointer; background:var(--accent); color:#fff; border:none; font-weight:700; }
    button:hover { background:var(--accent-hover); }
    button:disabled { opacity:.55; cursor:default; }
    .iconbtn, .themebtn, .quick button { background:var(--surface2); color:var(--button-secondary-text); border:1px solid var(--border); }
    .iconbtn:hover, .themebtn:hover, .quick button:hover { background:var(--surface1); }
    .iconbtn { width:44px; padding:12px 0; }
    .action-advert { background:linear-gradient(135deg,#d97706,#f59e0b); color:#fff7ed; }
    .action-caution { background:linear-gradient(135deg,#b94747,#d66a5f); color:#fff5f5; }
    .action-dreamy { background:linear-gradient(135deg,#4e7ac7,#8b7cf6); color:#f7f7ff; }
    .action-dreamy:hover { background:linear-gradient(135deg,#5c89d4,#9a8cff); }
    .stack { display:grid; gap:12px; }
    .row { display:grid; grid-template-columns:1fr 1fr; gap:12px; }
    .group { background:var(--surface2); border:1px solid var(--border); border-radius:12px; padding:14px; display:grid; gap:12px; }
    .group h3 { margin:0; font-size:13px; color:var(--text-muted); text-transform:uppercase; letter-spacing:.08em; }
    .field { display:grid; gap:6px; align-content:start; }
    .label { font-size:12px; color:var(--text-muted); }
    .inline { display:grid; grid-template-columns:minmax(0,1fr) auto auto; gap:8px; align-items:center; }
    .inline.one { grid-template-columns:minmax(0,1fr) auto; }
    .note { color:var(--text-muted); font-size:13px; margin:0; line-height:1.45; }
    .actions { display:flex; flex-wrap:wrap; gap:10px; justify-content:space-between; }
    .actions div { display:flex; gap:10px; flex-wrap:wrap; }
    .tiles { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:10px; }
    .tile { background:var(--surface2); border:1px solid var(--border); border-radius:12px; padding:12px; display:grid; gap:4px; min-width:0; }
    .tile-title { font-size:11px; color:var(--text-muted); text-transform:uppercase; letter-spacing:.06em; }
    .tile-state { font-size:18px; font-weight:700; }
    .tile-state.ok { color:var(--accent); }
    .tile-state.warn { color:var(--status-amber); }
    .tile-state.bad { color:var(--status-red); }
    .tile-detail { font-size:12px; color:var(--text-muted); overflow-wrap:anywhere; }
    .terminal { background:var(--terminal-bg); border:1px solid var(--border); border-radius:12px; padding:14px; min-height:140px; max-height:300px; overflow:auto; font-size:14px; }
    .term-entry { margin:0 0 10px; }
    .term-cmd { color:var(--terminal-cmd); }
    .term-out { white-space:pre-wrap; overflow-wrap:anywhere; }
    .term-out.err { color:var(--status-red); }
    #status { min-height:1.4em; color:var(--text-muted); white-space:pre-wrap; }
    @media (max-width:760px) {
      main { padding:16px; }
      .card { padding:16px; margin-bottom:14px; }
      .row, .tiles { grid-template-columns:1fr; }
      .actions div { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); width:100%; }
      .actions button { width:100%; }
    }
  </style>
</head>
<body>
  <main>
    <section class="card">
      <div class="actions">
        <div>
          <button id="advertBtn" class="action-advert">Advert</button>
          <button id="otaBtn" class="action-dreamy">Start OTA</button>
          <button id="refreshBtn" class="themebtn">Refresh</button>
        </div>
        <div>
          <button id="rebootBtn" class="action-caution">Reboot</button>
          <button id="themeToggle" class="themebtn" aria-label="Toggle theme">&#9790;</button>
          <button id="logoutBtn" class="themebtn">Logout</button>
        </div>
      </div>
      <div id="status" style="margin-top:12px"></div>
    </section>

    <section class="card">
      <h2 id="pageTitle">Room Server</h2>
      <div class="tiles">
        <div class="tile"><div class="tile-title">Wi-Fi</div><div id="wifiState" class="tile-state">--</div><div id="wifiDetail" class="tile-detail"></div></div>
        <div class="tile"><div class="tile-title">MQTT Bridge</div><div id="bridgeState" class="tile-state">--</div><div id="bridgeDetail" class="tile-detail"></div></div>
        <div class="tile"><div class="tile-title">Room Sync</div><div id="syncState" class="tile-state">--</div><div id="syncDetail" class="tile-detail"></div></div>
      </div>
    </section>

    <section class="card">
      <h2>Room Settings</h2>
      <div class="stack">
        <div class="group">
          <h3>Identity</h3>
          <div class="field" data-get="get name" data-set="set name ">
            <label class="label" for="f_name">Room Name</label>
            <div class="inline"><input id="f_name" maxlength="31"><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
          </div>
          <div class="row">
            <div class="field" data-get="get lat" data-set="set lat ">
              <label class="label" for="f_lat">Latitude</label>
              <div class="inline"><input id="f_lat" placeholder="0.0"><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
            </div>
            <div class="field" data-get="get lon" data-set="set lon ">
              <label class="label" for="f_lon">Longitude</label>
              <div class="inline"><input id="f_lon" placeholder="0.0"><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
            </div>
          </div>
          <div class="field">
            <label class="label" for="f_pubkey">Public Key</label>
            <input id="f_pubkey" readonly>
          </div>
          <div class="field">
            <label class="label" for="f_clock">Clock (UTC)</label>
            <div class="inline one"><input id="f_clock" readonly><button id="syncClockBtn">Sync from browser</button></div>
          </div>
        </div>
        <div class="group">
          <h3>Access</h3>
          <div class="row">
            <div class="field" data-get="get guest.password" data-set="set guest.password ">
              <label class="label" for="f_roompwd">Room Password</label>
              <div class="inline"><input id="f_roompwd" maxlength="15" placeholder="password clients log in with"><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
            </div>
            <div class="field" data-get="get allow.read.only" data-set="set allow.read.only ">
              <label class="label" for="f_readonly">Allow Read-Only Logins</label>
              <div class="inline"><select id="f_readonly"><option value="off">off</option><option value="on">on</option></select><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
            </div>
          </div>
          <div class="field" data-set="password " data-secret="1">
            <label class="label" for="f_adminpwd">Admin Password</label>
            <div class="inline one"><input id="f_adminpwd" type="password" maxlength="15" placeholder="new admin password"><button data-act="save">Save</button></div>
          </div>
        </div>
      </div>
    </section>

    <section class="card">
      <h2>Wi-Fi</h2>
      <div class="row">
        <div class="field" data-get="get wifi.ssid" data-set="set wifi.ssid ">
          <label class="label" for="f_ssid">SSID</label>
          <div class="inline"><input id="f_ssid" maxlength="32"><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
        </div>
        <div class="field" data-set="set wifi.pwd " data-secret="1">
          <label class="label" for="f_wifipwd">Password</label>
          <div class="inline one"><input id="f_wifipwd" type="password" maxlength="64" placeholder="new Wi-Fi password"><button data-act="save">Save</button></div>
        </div>
      </div>
      <p class="note" style="margin-top:10px">Changing Wi-Fi drops this page until the room server rejoins the network, possibly with a new IP address.</p>
    </section>

    <section class="card">
      <h2>MQTT Bridge &amp; Room Sync</h2>
      <div class="stack">
        <div class="group">
          <h3>Broker</h3>
          <div class="row">
            <div class="field" data-get="get bridge.enabled" data-set="set bridge.enabled ">
              <label class="label" for="f_bridge_en">Bridge</label>
              <div class="inline"><select id="f_bridge_en"><option value="on">on</option><option value="off">off</option></select><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
            </div>
            <div class="field">
              <label class="label" for="f_endpoint">Server host:port</label>
              <div class="inline"><input id="f_endpoint" placeholder="mqtt.example.com:1883" maxlength="70"><button id="loadEndpointBtn" class="iconbtn" title="Refresh">&#8635;</button><button id="saveEndpointBtn">Save</button></div>
            </div>
          </div>
          <div class="row">
            <div class="field" data-get="get bridge.peer.username" data-set="set bridge.peer.username " data-blank="-">
              <label class="label" for="f_user">Username</label>
              <div class="inline"><input id="f_user" maxlength="64" placeholder="optional"><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
            </div>
            <div class="field" data-set="set bridge.peer.password " data-secret="1">
              <label class="label" for="f_pass">Password</label>
              <div class="inline one"><input id="f_pass" type="password" maxlength="95" placeholder="optional"><button data-act="save">Save</button></div>
            </div>
          </div>
          <p class="note">Plain MQTT over TCP (no TLS). Every room server in a group must use the same broker.</p>
        </div>
        <div class="group">
          <h3>Room Sync</h3>
          <div class="row">
            <div class="field" data-get="get bridge.topic" data-set="set bridge.topic " data-blank="-" data-clear="off">
              <label class="label" for="f_topic">Topic</label>
              <div class="inline"><input id="f_topic" maxlength="32" placeholder="e.g. EastMesh (blank = off)"><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
            </div>
            <div class="field" data-get="get repeat" data-set="set repeat ">
              <label class="label" for="f_repeat">Repeat packets (incl. bridged)</label>
              <div class="inline"><select id="f_repeat"><option value="on">on</option><option value="off">off</option></select><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
            </div>
          </div>
          <p class="note">Posts made to this room are shared with every room server that uses the same broker, topic and bridge secret (set with <code>set bridge.secret</code> in the CLI).</p>
        </div>
      </div>
    </section>

    <section class="card">
      <h2>Radio</h2>
      <div class="stack">
        <div class="row">
          <div class="field">
            <label class="label" for="radioCurrent">Current Radio</label>
            <div class="inline one"><input id="radioCurrent" placeholder="915.800 / BW250.000 / SF10 / CR5" readonly><button id="refreshRadioBtn" class="iconbtn" title="Refresh current radio">&#8635;</button></div>
          </div>
          <div class="field">
            <label class="label" for="radioPreset">Preset</label>
            <div class="inline"><select id="radioPreset"><option value="">Loading presets...</option></select><button id="reloadRadioPresetsBtn" class="iconbtn" title="Reload presets">&#8635;</button><button id="applyRadioPresetBtn">Save</button></div>
          </div>
        </div>
        <div id="radioPresetStatus" class="note"></div>
        <div class="row">
          <div class="field">
            <label class="label" for="regionState">Region (Australia)</label>
            <div class="inline one"><select id="regionState">
              <option value="">Select state</option>
              <option value="au-act">ACT (au-act)</option>
              <option value="au-nsw">New South Wales (au-nsw)</option>
              <option value="au-qld">Queensland (au-qld)</option>
              <option value="au-sa">South Australia (au-sa)</option>
              <option value="au-tas">Tasmania (au-tas)</option>
              <option value="au-vic">Victoria (au-vic)</option>
            </select><button id="saveRegionBtn">Save</button></div>
            <div id="regionStatus" class="note"></div>
          </div>
          <div class="field" data-get="get tx" data-set="set tx ">
            <label class="label" for="f_tx">TX Power (dBm)</label>
            <div class="inline"><input id="f_tx" inputmode="numeric"><button class="iconbtn" data-act="load" title="Refresh">&#8635;</button><button data-act="save">Save</button></div>
          </div>
        </div>
        <p class="note">Radio presets come from the MeshCore community list and apply after a reboot.</p>
      </div>
    </section>

    <section class="card">
      <h2>Firmware</h2>
      <div class="field">
        <label class="label" for="fwFile">Upload firmware (.bin)</label>
        <div class="inline one"><input id="fwFile" type="file" accept=".bin,application/octet-stream"><button id="fwUploadBtn" disabled>Upload</button></div>
        <div id="fwStatus" class="note"></div>
      </div>
      <p class="note" style="margin-top:10px">Use the non-merged <code>.bin</code> built for this board's <code>*_room_server_mqtt_bridge</code> env. Release files named for a different board, and <code>-merged.bin</code> USB images, are refused. The image is checked with MD5 and the room server reboots once it is written.</p>
    </section>

    <section class="card">
      <h2>CLI</h2>
      <div class="quick" style="display:flex;flex-wrap:wrap;gap:8px;margin-bottom:12px">
        <button data-cmd="get wifi.status">wifi.status</button>
        <button data-cmd="get bridge.status">bridge.status</button>
        <button data-cmd="get bridge.sync">bridge.sync</button>
        <button data-cmd="get web.status">web.status</button>
        <button data-cmd="ver">ver</button>
      </div>
      <div class="inline one"><input id="command" placeholder="room.post hello from the panel"><button id="runBtn">Run</button></div>
      <div id="reply" class="terminal" style="margin-top:12px"></div>
    </section>
  </main>
  <script>
    const TOKEN_KEY = "room-token";
    const THEME_KEY = "room-theme";
    let token = "";
    try { token = sessionStorage.getItem(TOKEN_KEY) || ""; } catch (_) {}
    let queue = Promise.resolve();
    const statusEl = document.getElementById("status");
    const replyEl = document.getElementById("reply");
    const $ = (id) => document.getElementById(id);

    function toLogin() {
      try { sessionStorage.removeItem(TOKEN_KEY); } catch (_) {}
      token = "";
      window.location.replace("/");
    }
    function applyTheme(theme) {
      document.documentElement.dataset.theme = theme;
      $("themeToggle").innerHTML = theme === "dark" ? "&#9728;" : "&#9790;";
    }
    function initTheme() {
      let saved = "";
      try { saved = localStorage.getItem(THEME_KEY) || ""; } catch (_) {}
      if (saved !== "light" && saved !== "dark") {
        saved = window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
      }
      applyTheme(saved);
    }
    function valueOf(text) {
      return String(text || "").replace(/^>\s*/, "").trim();
    }
    function isError(text) {
      const v = valueOf(text).toLowerCase();
      return v.startsWith("err") || v.startsWith("unknown");
    }
    function logEntry(cmd, text, ok) {
      const entry = document.createElement("div");
      entry.className = "term-entry";
      const c = document.createElement("div");
      c.className = "term-cmd";
      c.textContent = "> " + cmd;
      const o = document.createElement("div");
      o.className = "term-out" + (ok ? "" : " err");
      o.textContent = text && text.length ? text : "OK";
      entry.appendChild(c);
      entry.appendChild(o);
      replyEl.appendChild(entry);
      replyEl.scrollTop = replyEl.scrollHeight;
    }
    function run(cmd, opts = {}) {
      if (!token) { toLogin(); return Promise.resolve({ ok:false, text:"" }); }
      const task = async () => {
        const res = await fetch("/api/command", { method:"POST", headers:{ "X-Auth-Token": token }, body: cmd });
        const text = await res.text();
        if (res.status === 401) { toLogin(); return { ok:false, text }; }
        const ok = res.ok && !isError(text);
        if (opts.log !== false) logEntry(opts.label || cmd, text, ok);
        return { ok, text };
      };
      const next = queue.then(task, task);
      queue = next.catch(() => {});
      return next;
    }

    // Generic get/set fields: <div class="field" data-get=".." data-set=".."> with one input/select.
    function fieldInput(field) { return field.querySelector("input,select"); }
    async function loadFieldEl(field, opts) {
      const cmd = field.dataset.get;
      if (!cmd) return;
      const r = await run(cmd, opts);
      if (!r.ok) return;
      let v = valueOf(r.text);
      if (field.dataset.blank && v === field.dataset.blank) v = "";
      fieldInput(field).value = v;
      if (fieldInput(field).id === "f_name") $("pageTitle").textContent = v || "Room Server";
    }
    async function saveFieldEl(field) {
      const input = fieldInput(field);
      let v = input.value.trim();
      if (field.dataset.clear && v === "") v = field.dataset.clear;
      const secret = field.dataset.secret === "1";
      const r = await run(field.dataset.set + v, { label: secret ? field.dataset.set + "********" : null });
      if (!r.ok) return;
      if (secret) {
        input.value = "";
      } else if (field.dataset.get) {
        await loadFieldEl(field, { log:false });
      }
      refreshStatus();
    }
    document.querySelectorAll(".field[data-set]").forEach((field) => {
      field.querySelectorAll("[data-act]").forEach((btn) => {
        btn.onclick = () => (btn.dataset.act === "load" ? loadFieldEl(field) : saveFieldEl(field));
      });
    });

    async function loadEndpoint(opts) {
      const h = await run("get bridge.peer.host", opts);
      const p = await run("get bridge.peer.port", opts);
      if (!h.ok || !p.ok) return;
      const host = valueOf(h.text);
      $("f_endpoint").value = host ? host + ":" + valueOf(p.text) : "";
    }
    async function saveEndpoint() {
      const raw = $("f_endpoint").value.trim().replace(/^(mqtts?|tcp):\/\//i, "");
      const m = raw.match(/^([^:\s\/]+)(?::(\d{1,5}))?$/);
      if (!m) { statusEl.textContent = "Enter host or host:port"; return; }
      const r = await run("set bridge.peer.host " + m[1]);
      if (!r.ok) return;
      await run("set bridge.peer.port " + (m[2] || "1883"));
      await loadEndpoint({ log:false });
      refreshStatus();
    }

    function tokens(text) {
      const out = {};
      valueOf(text).split(/\s+/).forEach((part) => {
        const i = part.indexOf(":");
        if (i > 0) out[part.slice(0, i)] = part.slice(i + 1);
      });
      return out;
    }
    function setTile(prefix, state, tone, detail) {
      const el = $(prefix + "State");
      el.textContent = state;
      el.className = "tile-state " + tone;
      $(prefix + "Detail").textContent = detail;
    }
    async function refreshStatus() {
      const quiet = { log:false };
      const w = await run("get wifi.status", quiet);
      if (w.ok) {
        const t = tokens(w.text);
        const st = t.status || "--";
        const detail = st === "connected"
          ? (t.ssid || "-") + " / " + (t.ip || "-") + " / " + (t.rssi || "-") + " dBm"
          : (t.ssid || "-") + " / " + (t.state || "-");
        setTile("wifi", st, st === "connected" ? "ok" : (st === "connecting" ? "warn" : "bad"), detail);
      }
      const b = await run("get bridge.status", quiet);
      if (b.ok) {
        const t = tokens(b.text);
        const s = t.bridge || "--";
        setTile("bridge", s, s === "online" ? "ok" : (s === "connecting" ? "warn" : "bad"),
          "peer " + (t.peer || "-") + " / repeat " + (t.repeat || "-"));
      }
      const s = await run("get bridge.sync", quiet);
      if (s.ok) {
        const t = tokens(s.text);
        const st = t.sync || "--";
        setTile("sync", st, st === "online" ? "ok" : (st === "connecting" ? "warn" : "bad"),
          "topic " + (t.topic || "-") + " / sent " + (t.tx || 0) + " / received " + (t.rx || 0));
      }
    }

    // Radio presets (same community source and behaviour as the observer panel)
    const RADIO_PRESETS_URL = "https://api.meshcore.nz/api/v1/config";
    let radioPresetEntries = [];
    let currentRadioConfig = null;
    function escapeHtml(value) {
      return String(value == null ? "" : value).replace(/&/g, "&amp;").replace(/</g, "&lt;")
        .replace(/>/g, "&gt;").replace(/"/g, "&quot;").replace(/'/g, "&#39;");
    }
    function formatDecimal(value, digits) {
      const num = Number.parseFloat(value);
      return Number.isFinite(num) ? num.toFixed(digits) : "";
    }
    function normalizeRadioConfig(c) {
      if (!c) return null;
      const frequency = Number.parseFloat(c.frequency);
      const bandwidth = Number.parseFloat(c.bandwidth);
      const spreadingFactor = Number.parseInt(c.spreadingFactor, 10);
      const codingRate = Number.parseInt(c.codingRate, 10);
      if (![frequency, bandwidth, spreadingFactor, codingRate].every(Number.isFinite)) return null;
      return { frequency, bandwidth, spreadingFactor, codingRate };
    }
    function radioSignature(c) {
      const n = normalizeRadioConfig(c);
      return n ? [n.frequency.toFixed(3), n.bandwidth.toFixed(3), n.spreadingFactor, n.codingRate].join("|") : "";
    }
    function formatRadioConfig(c) {
      const n = normalizeRadioConfig(c);
      return n ? `${n.frequency.toFixed(3)} / BW${n.bandwidth.toFixed(3)} / SF${n.spreadingFactor} / CR${n.codingRate}` : "";
    }
    function parseRadioValue(value) {
      const parts = String(value || "").split(",");
      if (parts.length !== 4) return null;
      return normalizeRadioConfig({ frequency:parts[0], bandwidth:parts[1], spreadingFactor:parts[2], codingRate:parts[3] });
    }
    function setNote(id, message, isError) {
      const el = $(id);
      el.textContent = message || "";
      el.style.color = isError ? "var(--status-red)" : "var(--text-muted)";
    }
    function syncRadioPresetUi() {
      $("radioCurrent").value = currentRadioConfig ? formatRadioConfig(currentRadioConfig) : "";
      const sel = $("radioPreset");
      const btn = $("applyRadioPresetBtn");
      if (!radioPresetEntries.length) { btn.disabled = true; return; }
      const sig = radioSignature(currentRadioConfig);
      const idx = radioPresetEntries.findIndex((p) => radioSignature(p) === sig);
      sel.value = idx >= 0 ? String(idx) : "";
      btn.disabled = idx < 0;
    }
    async function loadRadioConfig(opts) {
      const r = await run("get radio", opts);
      if (!r.ok) { setNote("radioPresetStatus", "Unable to load current radio config.", true); return; }
      const parsed = parseRadioValue(valueOf(r.text));
      if (!parsed) { setNote("radioPresetStatus", "Current radio config was not recognised.", true); return; }
      currentRadioConfig = parsed;
      setNote("radioPresetStatus", "");
      syncRadioPresetUi();
    }
    async function loadRadioPresets() {
      const sel = $("radioPreset");
      sel.innerHTML = '<option value="">Loading presets...</option>';
      $("applyRadioPresetBtn").disabled = true;
      try {
        const res = await fetch(RADIO_PRESETS_URL, { cache:"no-store" });
        if (!res.ok) throw new Error("Preset service returned " + res.status);
        const payload = await res.json();
        const config = payload && payload.config ? payload.config : {};
        const suggested = config.suggested_radio_settings || {};
        const entries = Array.isArray(suggested.entries) ? suggested.entries : [];
        radioPresetEntries = entries.map((e) => ({
          title: String(e.title || "Unnamed preset"),
          frequency: formatDecimal(e.frequency, 3),
          bandwidth: formatDecimal(e.bandwidth, 3),
          spreadingFactor: Number.parseInt(e.spreading_factor, 10),
          codingRate: Number.parseInt(e.coding_rate, 10)
        })).filter((e) => radioSignature(e));
        const options = ['<option value="">Custom / current</option>'];
        radioPresetEntries.forEach((p, i) => options.push(`<option value="${i}">${escapeHtml(p.title)}</option>`));
        sel.innerHTML = options.join("");
        syncRadioPresetUi();
      } catch (e) {
        radioPresetEntries = [];
        sel.innerHTML = '<option value="">Community presets unavailable</option>';
        $("applyRadioPresetBtn").disabled = true;
        setNote("radioPresetStatus", e && e.message ? e.message : "Unable to load community presets.", true);
      }
    }
    async function applyRadioPreset() {
      const preset = radioPresetEntries[Number.parseInt($("radioPreset").value, 10)];
      if (!preset) { setNote("radioPresetStatus", "Select a community preset first.", true); return; }
      const r = await run(`set radio ${preset.frequency},${preset.bandwidth},${preset.spreadingFactor},${preset.codingRate}`);
      if (!r.ok) { setNote("radioPresetStatus", valueOf(r.text) || "Unable to apply radio preset.", true); return; }
      currentRadioConfig = normalizeRadioConfig(preset);
      syncRadioPresetUi();
      setNote("radioPresetStatus", "Preset saved. Reboot to apply the new radio settings.");
    }

    // Region selector (same commands as the observer panel)
    async function loadRegionSelection(opts) {
      const sel = $("regionState");
      const r = await run("region list allowed", opts);
      sel.value = "";
      if (!r.ok) return;
      const allowed = valueOf(r.text).toLowerCase().split(",").map((v) => v.trim());
      const match = allowed.find((v) => v.startsWith("au-") && [...sel.options].some((o) => o.value === v));
      if (match) sel.value = match;
      setNote("regionStatus", "");
    }
    async function saveRegionSelection() {
      const region = $("regionState").value;
      if (!region) { setNote("regionStatus", "Select an AU state region first.", true); return; }
      setNote("regionStatus", "Saving regions...");
      for (const cmd of ["region def au " + region, "region save"]) {
        const r = await run(cmd);
        if (!r.ok) { setNote("regionStatus", valueOf(r.text) || ("Unable to apply: " + cmd), true); return; }
      }
      setNote("regionStatus", "Saved au and " + region + ".");
    }

    // Direct firmware upload through the authenticated /api/firmware-update endpoint
    function md5ArrayBuffer(buffer) {
      const input = new Uint8Array(buffer);
      const originalLength = input.length;
      const totalLength = (((originalLength + 8) >> 6) + 1) * 64;
      const data = new Uint8Array(totalLength);
      data.set(input);
      data[originalLength] = 0x80;
      const bitLength = originalLength * 8;
      for (let i = 0; i < 8; i++) {
        data[totalLength - 8 + i] = Math.floor(bitLength / Math.pow(256, i)) & 0xff;
      }
      const shifts = [7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21];
      const constants = Array.from({ length:64 }, (_, i) => Math.floor(Math.abs(Math.sin(i + 1)) * 4294967296) >>> 0);
      let a = 0x67452301;
      let b = 0xefcdab89;
      let c = 0x98badcfe;
      let d = 0x10325476;
      const rotl = (value, shift) => ((value << shift) | (value >>> (32 - shift))) >>> 0;
      for (let offset = 0; offset < data.length; offset += 64) {
        const words = new Array(16);
        for (let i = 0; i < 16; i++) {
          const j = offset + i * 4;
          words[i] = (data[j] | (data[j + 1] << 8) | (data[j + 2] << 16) | (data[j + 3] << 24)) >>> 0;
        }
        let aa = a, bb = b, cc = c, dd = d;
        for (let i = 0; i < 64; i++) {
          let f, g;
          if (i < 16) {
            f = (bb & cc) | (~bb & dd);
            g = i;
          } else if (i < 32) {
            f = (dd & bb) | (~dd & cc);
            g = (5 * i + 1) % 16;
          } else if (i < 48) {
            f = bb ^ cc ^ dd;
            g = (3 * i + 5) % 16;
          } else {
            f = cc ^ (bb | ~dd);
            g = (7 * i) % 16;
          }
          const temp = dd;
          dd = cc;
          cc = bb;
          bb = (bb + rotl((aa + f + constants[i] + words[g]) >>> 0, shifts[i])) >>> 0;
          aa = temp;
        }
        a = (a + aa) >>> 0;
        b = (b + bb) >>> 0;
        c = (c + cc) >>> 0;
        d = (d + dd) >>> 0;
      }
      const hexWord = (word) => [0,8,16,24].map((shift) => ((word >>> shift) & 0xff).toString(16).padStart(2, "0")).join("");
      return hexWord(a) + hexWord(b) + hexWord(c) + hexWord(d);
    }
    function fwStatus(message, isError) { setNote("fwStatus", message, isError); }
    let buildEnv = "";
    // Release files are named <env>-<version>.bin; a local build is just firmware.bin.
    function checkFirmwareName(name) {
      const lower = String(name || "").toLowerCase();
      if (/-merged\.bin$/.test(lower)) {
        return { ok:false, msg:"This is the merged USB image. Choose the non-merged .bin instead." };
      }
      const env = buildEnv.toLowerCase();
      if (env && (lower.startsWith(env + "-") || lower === env + ".bin")) {
        return { ok:true, msg:"Matches this board (" + buildEnv + ")." };
      }
      if (/(_room_server|_repeater|_companion|_sensor|_terminal)/.test(lower)) {
        return { ok:false, msg:"Built for a different board or firmware. This room server is " + (buildEnv || "unknown") + "." };
      }
      return { ok:true, msg:"Board can't be confirmed from the file name. Make sure it was built for " + (buildEnv || "this board") + "." };
    }
    function fwSelected() {
      const file = $("fwFile").files[0];
      if (!file) { $("fwUploadBtn").disabled = true; fwStatus(""); return; }
      const check = checkFirmwareName(file.name);
      $("fwUploadBtn").disabled = !check.ok;
      fwStatus(file.name + " (" + Math.round(file.size / 1024) + " KB) - " + check.msg, !check.ok);
    }
    async function uploadFirmware() {
      const file = $("fwFile").files[0];
      if (!file || !checkFirmwareName(file.name).ok) return;
      if (!confirm("Flash " + file.name + " to this room server now?")) return;
      const btn = $("fwUploadBtn");
      btn.disabled = true;
      try {
        const buffer = await file.arrayBuffer();
        if (buffer.byteLength < 1024 || new Uint8Array(buffer)[0] !== 0xE9) {
          throw new Error("Not an ESP32 app image (expected firmware.bin, not a merged or bootloader image).");
        }
        fwStatus("Checking firmware...");
        const md5 = md5ArrayBuffer(buffer);
        await new Promise((resolve, reject) => {
          const xhr = new XMLHttpRequest();
          xhr.open("POST", "/api/firmware-update");
          xhr.setRequestHeader("X-Auth-Token", token);
          xhr.setRequestHeader("X-Firmware-MD5", md5);
          xhr.setRequestHeader("Content-Type", "application/octet-stream");
          xhr.upload.onprogress = (e) => {
            if (e.lengthComputable) fwStatus("Uploading... " + Math.round((e.loaded * 100) / e.total) + "%");
          };
          xhr.onload = () => (xhr.status === 200 ? resolve() : reject(new Error(xhr.responseText || ("Upload failed (" + xhr.status + ")"))));
          xhr.onerror = () => reject(new Error("Upload failed - connection lost"));
          xhr.send(new Blob([buffer], { type:"application/octet-stream" }));
        });
        let remaining = 20;
        const tick = () => {
          fwStatus("Firmware written. Rebooting - reloading in " + remaining + "s...");
          if (remaining-- <= 0) { window.location.reload(); return; }
          window.setTimeout(tick, 1000);
        };
        tick();
      } catch (e) {
        fwStatus(e && e.message ? e.message : "Firmware update failed.", true);
        btn.disabled = false;
      }
    }

    async function loadAll() {
      const quiet = { log:false };
      statusEl.textContent = "Loading...";
      const ver = await run("ver", quiet);
      const env = await run("get mqtt.client_env", quiet);
      buildEnv = env.ok && valueOf(env.text) !== "unknown" ? valueOf(env.text) : "";
      const pk = await run("get public.key", quiet);
      if (pk.ok) $("f_pubkey").value = valueOf(pk.text).toUpperCase();
      const clk = await run("clock", quiet);
      if (clk.ok) $("f_clock").value = valueOf(clk.text);
      for (const field of document.querySelectorAll(".field[data-get]")) {
        await loadFieldEl(field, quiet);
      }
      await loadEndpoint(quiet);
      await loadRadioConfig(quiet);
      await loadRegionSelection(quiet);
      await refreshStatus();
      statusEl.textContent = ver.ok ? "Ready - " + valueOf(ver.text) : "Ready";
    }

    async function syncClock() {
      const epoch = Math.floor(Date.now() / 1000);
      let r = await run("time " + epoch);
      if (!r.ok) r = await run("time.force " + epoch);
      const clk = await run("clock", { log:false });
      if (clk.ok) $("f_clock").value = valueOf(clk.text);
    }

    async function init() {
      initTheme();
      if (!token) { toLogin(); return; }
      const res = await fetch("/api/session", { headers:{ "X-Auth-Token": token } }).catch(() => null);
      if (!res || res.status === 401) { toLogin(); return; }
      try {
        await loadAll();
        loadRadioPresets();
      } catch (e) {
        statusEl.textContent = e && e.message ? e.message : "Unable to load settings";
      }
    }

    $("themeToggle").onclick = () => {
      const next = document.documentElement.dataset.theme === "dark" ? "light" : "dark";
      try { localStorage.setItem(THEME_KEY, next); } catch (_) {}
      applyTheme(next);
    };
    $("logoutBtn").onclick = toLogin;
    $("refreshBtn").onclick = () => loadAll();
    $("advertBtn").onclick = () => { if (confirm("Send an advert now?")) run("advert"); };
    $("otaBtn").onclick = async () => {
      if (!confirm("Start OTA mode now? The page will open the firmware upload form.")) return;
      const btn = $("otaBtn");
      btn.disabled = true;
      const r = await run("start ota");
      const match = r.text && r.text.match(/https?:\/\/\S+/);
      if (r.ok && match) {
        statusEl.textContent = "OTA started - opening " + match[0] + " in 5s...";
        window.setTimeout(() => { window.location.href = match[0]; }, 5000);
      } else {
        statusEl.textContent = valueOf(r.text) || "Unable to start OTA";
        btn.disabled = false;
      }
    };
    $("fwFile").addEventListener("change", fwSelected);
    $("fwUploadBtn").onclick = uploadFirmware;
    $("rebootBtn").onclick = () => { if (confirm("Reboot the room server now?")) run("reboot"); };
    $("syncClockBtn").onclick = syncClock;
    $("loadEndpointBtn").onclick = () => loadEndpoint();
    $("saveEndpointBtn").onclick = saveEndpoint;
    $("refreshRadioBtn").onclick = () => loadRadioConfig();
    $("reloadRadioPresetsBtn").onclick = () => loadRadioPresets();
    $("radioPreset").addEventListener("change", (e) => {
      if (e.target.value === "" || !radioPresetEntries[Number.parseInt(e.target.value, 10)]) { syncRadioPresetUi(); return; }
      $("applyRadioPresetBtn").disabled = false;
    });
    $("applyRadioPresetBtn").onclick = applyRadioPreset;
    $("saveRegionBtn").onclick = saveRegionSelection;
    document.querySelectorAll("[data-cmd]").forEach((btn) => { btn.onclick = () => run(btn.dataset.cmd); });
    const runInput = () => {
      const cmd = $("command").value.trim();
      if (cmd) run(cmd);
    };
    $("runBtn").onclick = runInput;
    $("command").addEventListener("keydown", (e) => { if (e.key === "Enter") { e.preventDefault(); runInput(); } });
    setInterval(() => { if (token && document.visibilityState === "visible") refreshStatus(); }, 30000);
    init();
  </script>
</body>
</html>
)HTML";
