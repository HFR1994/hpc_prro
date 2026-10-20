/* Cutting Planner front end: editable stock/pieces tables, priority sliders, solver run + polling. */
(() => {
  const $ = (id) => document.getElementById(id);
  const state = { stock: [], pieces: [], job: null, poll: null, defaults: null };

  // ------------------------------------------------------------------ priorities
  const W = { cut: $("w-cut"), board: $("w-board"), waste: $("w-waste") };
  function weights() {
    const raw = { cut: +W.cut.value, board: +W.board.value, waste: +W.waste.value };
    const sum = raw.cut + raw.board + raw.waste;
    if (sum <= 0) return { cut: 0, board: 0, waste: 0 };
    return { cut: raw.cut / sum, board: raw.board / sum, waste: raw.waste / sum };
  }
  function renderWeights() {
    const w = weights();
    for (const k of ["cut", "board", "waste"]) {
      $(`w-${k}-val`).textContent = `${(w[k] * 100).toFixed(0)} %`;
      $("weights-bar").querySelector(`.wb-${k}`).style.width = `${w[k] * 100}%`;
    }
    $("weights-text").textContent =
      `Weights sent to the solver: cut ${w.cut.toFixed(3)}, board ${w.board.toFixed(3)}, waste ${w.waste.toFixed(3)} per m².`;
    document.querySelectorAll(".chip").forEach((c) => {
      const [a, b, d] = c.dataset.preset.split(",").map(Number);
      c.classList.toggle("active", a === +W.cut.value && b === +W.board.value && d === +W.waste.value);
    });
  }
  Object.values(W).forEach((el) => el.addEventListener("input", renderWeights));
  document.querySelectorAll(".chip").forEach((c) =>
    c.addEventListener("click", () => {
      const [a, b, d] = c.dataset.preset.split(",");
      W.cut.value = a; W.board.value = b; W.waste.value = d;
      renderWeights();
    }));

  // ------------------------------------------------------------------ CRUD tables
  const COLS = {
    stock: [
      { key: "name", type: "text", placeholder: "board_2400x400" },
      { key: "length", type: "number", min: 1 },
      { key: "width", type: "number", min: 1 },
      { key: "price", type: "number", min: 0, step: 0.01 },
    ],
    pieces: [
      { key: "name", type: "text", placeholder: "shelf" },
      { key: "length", type: "number", min: 1 },
      { key: "width", type: "number", min: 1 },
      { key: "qty", type: "number", min: 1, step: 1 },
      { key: "rotatable", type: "checkbox" },
    ],
  };
  const blank = {
    stock: () => ({ name: "", length: 2400, width: 400, price: 0 }),
    pieces: () => ({ name: "", length: 300, width: 200, qty: 1, rotatable: true }),
  };

  function renderTable(kind) {
    const tbody = $(`${kind}-table`).querySelector("tbody");
    tbody.innerHTML = "";
    state[kind].forEach((row, idx) => {
      const tr = document.createElement("tr");
      for (const col of COLS[kind]) {
        const td = document.createElement("td");
        const input = document.createElement("input");
        input.type = col.type;
        if (col.type === "checkbox") {
          td.className = "check";
          input.checked = !!row[col.key];
          input.addEventListener("change", () => { row[col.key] = input.checked; });
        } else {
          input.value = row[col.key] ?? "";
          if (col.placeholder) input.placeholder = col.placeholder;
          if (col.min !== undefined) input.min = col.min;
          if (col.step !== undefined) input.step = col.step; else if (col.type === "number") input.step = "any";
          input.addEventListener("input", () => {
            row[col.key] = col.type === "number" ? (input.value === "" ? "" : +input.value) : input.value;
            input.classList.toggle("invalid", !fieldValid(col, row[col.key]));
            if (kind === "pieces") renderPiecesSummary();
          });
          input.classList.toggle("invalid", !fieldValid(col, row[col.key]));
        }
        td.appendChild(input);
        tr.appendChild(td);
      }
      const del = document.createElement("td");
      del.className = "del";
      const btn = document.createElement("button");
      btn.type = "button"; btn.className = "del"; btn.title = "Remove row"; btn.textContent = "✕";
      btn.addEventListener("click", () => { state[kind].splice(idx, 1); renderTable(kind); });
      del.appendChild(btn);
      tr.appendChild(del);
      tbody.appendChild(tr);
    });
    $(`${kind}-empty`).hidden = state[kind].length > 0;
    if (kind === "pieces") renderPiecesSummary();
  }
  function fieldValid(col, v) {
    if (col.type === "text") return String(v ?? "").trim().length > 0;
    if (col.type === "number") return v !== "" && v !== null && !Number.isNaN(+v) && +v >= (col.min ?? -Infinity);
    return true;
  }
  function renderPiecesSummary() {
    let n = 0, area = 0;
    for (const p of state.pieces) {
      const q = Math.max(0, +p.qty || 0);
      n += q; area += q * (+p.length || 0) * (+p.width || 0);
    }
    $("pieces-summary").textContent = `${n} pieces in total, ${(area / 1e6).toFixed(3)} m² of material.`;
  }
  function addRow(kind) {
    state[kind].push(blank[kind]());
    renderTable(kind);
    const inputs = $(`${kind}-table`).querySelectorAll("tbody tr:last-child input");
    if (inputs.length) inputs[0].focus();
  }
  $("stock-add").addEventListener("click", () => addRow("stock"));
  $("pieces-add").addEventListener("click", () => addRow("pieces"));

  // CSV import/export (same column layout as dataset/*.csv)
  function toCSV(kind) {
    const header = kind === "stock" ? "# name,length,width,price" : "# name,length,width,qty,rotatable";
    const lines = state[kind].map((r) => kind === "stock"
      ? [r.name, r.length, r.width, r.price || 0].join(",")
      : [r.name, r.length, r.width, r.qty || 1, r.rotatable ? 1 : 0].join(","));
    return header + "\n" + lines.join("\n") + "\n";
  }
  function fromCSV(kind, text) {
    const rows = [];
    for (const raw of text.split(/\r?\n/)) {
      const line = raw.trim();
      if (!line || line.startsWith("#")) continue;
      const f = line.split(",").map((s) => s.trim());
      if (f.length < 3 || Number.isNaN(+f[1]) || Number.isNaN(+f[2])) continue; // skips a header row too
      rows.push(kind === "stock"
        ? { name: f[0], length: +f[1], width: +f[2], price: f[3] !== undefined ? +f[3] || 0 : 0 }
        : { name: f[0], length: +f[1], width: +f[2], qty: f[3] !== undefined ? +f[3] || 1 : 1, rotatable: f[4] === undefined ? true : +f[4] !== 0 });
    }
    return rows;
  }
  function download(name, text, type = "text/csv") {
    const url = URL.createObjectURL(new Blob([text], { type }));
    const a = document.createElement("a");
    a.href = url; a.download = name; a.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  }
  for (const kind of ["stock", "pieces"]) {
    $(`${kind}-export`).addEventListener("click", () => download(`${kind}.csv`, toCSV(kind)));
    $(`${kind}-import`).addEventListener("click", () => {
      const fi = $("file-input");
      fi.value = "";
      fi.onchange = () => {
        const file = fi.files[0];
        if (!file) return;
        file.text().then((t) => {
          const rows = fromCSV(kind, t);
          if (!rows.length) { setStatus(`No usable rows found in ${file.name}.`, "error"); return; }
          state[kind] = rows;
          renderTable(kind);
          setStatus(`Imported ${rows.length} ${kind} rows from ${file.name}.`, "ok");
        });
      };
      fi.click();
    });
  }

  // ------------------------------------------------------------------ solver settings
  function spec() {
    const seed = $("seed").value.trim();
    return {
      weights: weights(),
      kerf: +$("kerf").value,
      decoder: $("decoder").value,
      pop_size: +$("pop-size").value,
      iterations: +$("iterations").value,
      np: +$("np").value,
      flight_steps: +$("flight-steps").value,
      lookout_steps: +$("lookout-steps").value,
      radius: +$("radius").value,
      seed: seed === "" ? null : +seed,
      stock: state.stock,
      pieces: state.pieces,
    };
  }
  function renderSolverHint() {
    const n = state.pieces.reduce((a, p) => a + Math.max(0, +p.qty || 0), 0);
    const keys = $("decoder").value === "shelf" ? 3 : 4;
    const feat = n * keys;
    $("solver-hint").textContent =
      `Search space: ${feat} keys per raven (${keys} per piece × ${n} pieces). ` +
      `Population file random-${+$("pop-size").value}-${feat}.csv is generated on the server for every run.`;
  }
  ["pop-size", "decoder"].forEach((id) => $(id).addEventListener("input", renderSolverHint));
  $("pieces-table").addEventListener("input", renderSolverHint);

  // ------------------------------------------------------------------ run / poll
  function setStatus(msg, cls = "idle") {
    const el = $("status");
    el.className = `status ${cls}`;
    el.textContent = msg;
  }
  function clientValidate(s) {
    if (!s.stock.length) return "Add at least one stock board.";
    if (!s.pieces.length) return "Add at least one piece.";
    for (const kind of ["stock", "pieces"]) {
      for (const [i, r] of s[kind].entries()) {
        for (const col of COLS[kind]) {
          if (!fieldValid(col, r[col.key])) return `${kind === "stock" ? "Stock" : "Piece"} row ${i + 1}: invalid ${col.key}.`;
        }
      }
    }
    if (s.np > s.pop_size) return "MPI processes cannot exceed the population size.";
    return null;
  }
  async function run() {
    const s = spec();
    const err = clientValidate(s);
    if (err) { setStatus(err, "error"); return; }
    $("btn-run").disabled = true;
    $("result").hidden = true;
    $("result-actions").hidden = true;
    $("progress").hidden = false;
    $("progress-text").textContent = "Submitting job…";
    setStatus("Running the solver. This can take from a few seconds to several minutes depending on population, iterations and pieces.");
    try {
      const res = await fetch("/api/run", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(s) });
      const job = await res.json();
      if (!res.ok) throw new Error(job.error || res.statusText);
      state.job = job;
      state.poll = setInterval(poll, 1500);
      poll();
    } catch (e) {
      finish(); setStatus(`Could not start: ${e.message}`, "error");
    }
  }
  async function poll() {
    if (!state.job) return;
    let job;
    try {
      const res = await fetch(`/api/jobs/${state.job.id}`);
      job = await res.json();
      if (!res.ok) throw new Error(job.error || res.statusText);
    } catch (e) {
      finish(); setStatus(`Lost contact with the server: ${e.message}`, "error"); return;
    }
    $("log").textContent = job.log_tail || "";
    if (job.status === "queued") $("progress-text").textContent = "Queued, waiting for a free slot…";
    else if (job.status === "running") {
      $("progress-text").textContent = `Running on ${job.spec.np} MPI process(es), ${job.spec.iterations} iterations… ${Math.round(job.running_for || 0)} s`;
    }
    if (job.status === "done") { finish(); showResult(job); }
    else if (job.status === "failed") { finish(); setStatus(`Solver failed: ${job.error}. See the solver log below.`, "error"); $("log-details").open = true; }
    else if (job.status === "cancelled") { finish(); setStatus("Run cancelled.", "idle"); }
  }
  function finish() {
    clearInterval(state.poll); state.poll = null;
    $("btn-run").disabled = false;
    $("progress").hidden = true;
  }
  $("btn-run").addEventListener("click", run);
  $("btn-cancel").addEventListener("click", async () => {
    if (!state.job) return;
    await fetch(`/api/jobs/${state.job.id}/cancel`, { method: "POST" });
  });

  // ------------------------------------------------------------------ results
  function tile(k, v, small) {
    return `<div class="tile"><div class="k">${k}</div><div class="v">${v}${small ? ` <small>${small}</small>` : ""}</div></div>`;
  }
  function showResult(job) {
    const r = job.result, s = r.summary;
    const unplaced = s && s.placed < s.total;
    setStatus(unplaced
      ? `Finished in ${job.elapsed.toFixed(1)} s, but ${s.total - s.placed} piece(s) fit on no stock board.`
      : `Finished in ${job.elapsed.toFixed(1)} s.`, unplaced ? "error" : "ok");
    $("tiles").innerHTML = s ? [
      tile("Saw cuts", s.cuts),
      tile("Boards", s.boards),
      tile("Waste", s.waste_m2.toFixed(3), `m² (${s.waste_pct.toFixed(1)} %)`),
      tile("Stock used", s.stock_m2.toFixed(3), "m²"),
      tile("Pieces placed", `${s.placed}/${s.total}`),
      tile("Fitness", s.fitness.toFixed(4)),
    ].join("") : "";
    $("buy").innerHTML = r.buy.length
      ? `<b>Shopping list</b><ul>${r.buy.map((b) => `<li>${b.qty} × ${b.name}</li>`).join("")}</ul>` : "";
    $("svg-wrap").innerHTML = r.svg;
    renderCSV("plan-table", r.plan);
    renderCSV("cuts-table", r.cuts);
    $("dl-plan").href = `/api/jobs/${job.id}/plan.csv`;
    $("dl-cuts").href = `/api/jobs/${job.id}/cuts.csv`;
    $("dl-svg").href = `/api/jobs/${job.id}/plan.svg`;
    $("result-actions").hidden = false;
    $("result").hidden = false;
    $("log").textContent = job.log_tail || "";
  }
  function renderCSV(id, text) {
    const rows = text.trim().split(/\r?\n/).map((l) => l.split(","));
    const head = rows.shift() || [];
    const esc = (v) => String(v).replace(/&/g, "&amp;").replace(/</g, "&lt;");
    $(id).innerHTML =
      `<thead><tr>${head.map((h) => `<th>${esc(h)}</th>`).join("")}</tr></thead>` +
      `<tbody>${rows.map((r) => `<tr>${r.map((v) => `<td class="${Number.isNaN(+v) || v === "" ? "" : "num"}">${esc(v)}</td>`).join("")}</tr>`).join("")}</tbody>`;
  }

  // ------------------------------------------------------------------ defaults
  async function loadDefaults() {
    try {
      const res = await fetch("/api/defaults");
      const d = await res.json();
      state.defaults = d;
      state.stock = d.stock.map((x) => ({ ...x }));
      state.pieces = d.pieces.map((x) => ({ ...x }));
      $("np").max = d.max_np;
      if (+$("np").value > d.max_np) $("np").value = d.max_np;
    } catch (e) {
      state.stock = []; state.pieces = [];
      setStatus(`Could not load defaults from the server: ${e.message}`, "error");
    }
    renderTable("stock"); renderTable("pieces"); renderSolverHint();
  }
  $("btn-reset").addEventListener("click", () => {
    if (!state.defaults) return loadDefaults();
    state.stock = state.defaults.stock.map((x) => ({ ...x }));
    state.pieces = state.defaults.pieces.map((x) => ({ ...x }));
    renderTable("stock"); renderTable("pieces"); renderSolverHint();
    setStatus("Defaults reloaded.", "idle");
  });

  renderWeights();
  loadDefaults();
})();
