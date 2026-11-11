/* GREEN ION V13 unified dashboard overlay.
   Same Feather controls and charts; receiver never takes flight-control actions. */
(() => {
  "use strict";
  const panel = document.createElement("section");
  panel.id = "v13LinkPanel";
  panel.setAttribute("aria-live", "polite");
  panel.style.cssText = "margin:12px 0 18px;padding:15px 18px;background:#172b40;border:1px solid #35536e;border-radius:12px";
  panel.innerHTML = '<h2 style="margin:0 0 5px">GREEN ION V13 · Unified Ground Station</h2>' +
    '<p style="margin:0 0 10px">One dashboard on this computer. Wi-Fi controls the Feather; the USB Uno receives LoRa telemetry independently.</p>' +
    '<div class="row" style="gap:14px;flex-wrap:wrap"><strong id="v13Wifi">Wi-Fi: checking...</strong>' +
    '<strong id="v13Lora">LoRa: checking...</strong><strong id="v13Source">Source: checking...</strong></div>' +
    '<p id="v13Notice" style="margin:8px 0 3px" role="status"></p>' +
    '<div class="row" style="gap:12px;flex-wrap:wrap"><span id="v13RadioDetails">Receiver status pending</span>' +
    '<span id="v13Position">Last known GPS: --</span><a href="/api/export">Download received LoRa CSV</a></div>';
  const nav = document.querySelector("nav.tabs");
  (nav || document.querySelector("header")).insertAdjacentElement("afterend", panel);
  const $ = id => document.getElementById(id);
  const wifiOnlyTabs = ["flight", "field", "testing", "utilities", "storage", "system"];
  const wifiOnlyCards = ["controlAccessCard", "prepareRecordCard", "recorderHealthCard"];
  const allButtons = new Set();
  for (const id of wifiOnlyTabs) {
    const page = $(id);
    if (page) for (const btn of page.querySelectorAll("button")) allButtons.add(btn);
  }
  for (const id of wifiOnlyCards) {
    const page = $(id);
    if (page) for (const btn of page.querySelectorAll("button")) allButtons.add(btn);
  }
  const state = {wifi: false, source: "none"};
  function setVisibility(wifi) {
    state.wifi = wifi;
    document.body.classList.toggle("v13-wifi-unavailable", !wifi);
    for (const id of wifiOnlyTabs) {
      const button = document.querySelector('nav.tabs [data-tab="' + id + '"]');
      if (button) {
        button.disabled = !wifi;
        button.title = wifi ? "" : "Requires a live Feather Wi-Fi connection";
      }
      const page = $(id);
      if (page && !wifi && page.classList.contains("active")) {
        const status = document.querySelector('nav.tabs [data-tab="status"]');
        if (status) status.click();
      }
    }
    for (const id of wifiOnlyCards) {
      const card = $(id);
      if (card) card.style.display = wifi ? "" : "none";
    }
    for (const button of allButtons) {
      if (!wifi) {
        if (!button.hasAttribute("data-v13-lock")) {
          button.setAttribute("data-v13-lock", button.disabled ? "disabled" : "enabled");
        }
        button.disabled = true;
      } else if (button.hasAttribute("data-v13-lock")) {
        if (button.getAttribute("data-v13-lock") === "enabled") button.disabled = false;
        button.removeAttribute("data-v13-lock");
      }
    }
  }
  // Even if legacy polling briefly re-enables a control, reject its click.
  document.addEventListener("click", event => {
    if (!state.wifi && [...allButtons].some(btn => btn === event.target || btn.contains(event.target))) {
      event.preventDefault();
      event.stopImmediatePropagation();
      $("v13Notice").textContent = "Feather controls require live Wi-Fi. LoRa is telemetry only.";
    }
  }, true);
  const format = n => typeof n === "number" && Number.isFinite(n) ? n.toFixed(1) : "--";
  let lastMode = "";
  async function update() {
    try {
      const response = await fetch("/api/ground-link", {cache:"no-store"});
      if (!response.ok) throw Error("Local bridge unavailable");
      const data = await response.json();
      const wifi = data.wifi.status === "live";
      setVisibility(wifi);
      const lora = data.lora.status;
      state.source = data.source;
      $("v13Wifi").textContent = "Feather Wi-Fi: " + data.wifi.status.toUpperCase();
      $("v13Lora").textContent = "Uno LoRa: " + lora.toUpperCase();
      $("v13Source").textContent = "ACTIVE: " + data.source.toUpperCase();
      $("v13Wifi").style.color = wifi ? "#72e5ab" : "#ffdd88";
      $("v13Lora").style.color = lora === "live" ? "#72e5ab" : "#ffdd88";
      $("v13RadioDetails").textContent = "Uno: " + data.lora.receiver_state +
        " · Last packet " + (data.lora.packet_age_ms ?? "--") + " ms" +
        " · RSSI " + format(data.lora.rssi_dbm) + " dBm" +
        " · Bench received " + data.lora.bench_packets;
      const position = data.last_position;
      $("v13Position").textContent = position && position.length === 2 ?
        "Last LoRa GPS: " + Number(position[0]).toFixed(6) + ", " +
          Number(position[1]).toFixed(6) + " (" + (data.last_position_age_ms ?? "?") + " ms old)" :
        "Last LoRa GPS: no valid fix yet";
      let notice;
      if (data.source === "wifi") notice = "FULL FEATHER DASHBOARD · All features use Wi-Fi. LoRa receiver is monitored separately.";
      else if (data.source === "lora") notice = "TRACKING MODE · Wi-Fi-only readings, SD files, testing, firmware updates and controls are unavailable. LoRa packets are live.";
      else if (data.source === "demo") notice = "DEMO ONLY · Simulated telemetry, NOT proof of radio reception or hardware operation.";
      else notice = "NO LIVE DATA · Values last shown are stale. The app remains open and keeps last known GPS; check antennas and connections.";
      $("v13Notice").textContent = notice;
      if (data.source !== lastMode) {
        lastMode = data.source;
        if (data.source === "lora" || data.source === "demo") {
          const connection = $("connection");
          if (connection) connection.textContent = data.source === "demo" ? "● SIMULATION" : "● LORA TRACKING";
        }
      }
    } catch (error) {
      setVisibility(false);
      $("v13Notice").textContent = "LOCAL GROUND STATION ERROR · " + error.message;
    }
  }
  setInterval(update, 800);
  update();
})();
