# Responsive Blue Website — Phone, iPad and Windows

| Screen width | V14 layout |
|---|---|
| 320–420 px | Single column, vertical action buttons, scrollable blue tabs |
| 421–767 px | Touch-friendly single column |
| 768–899 px | Tablet-width single column |
| 900–1099 px | Single column for dense controls and responsive charts |
| 1100+ px | Independent two-column blue cards with full-width sensor and chart sections |

On all sizes: the original **Status, Flight, Field, Testing, Utilities, Storage and System** tabs remain, plus Connections, History, Cloud and Updates. Optional compact cards, screen-safe padding and reduced-motion styles are supported.

**Windows:** open [V14](https://green-ion-beta.vercel.app/) in Chrome/Edge. Connect Arduino Uno USB in **Connections** for receive-only LoRa telemetry at 115200 baud. Join Feather Wi-Fi for direct hardware access.

**iPhone/iPad:** install the website from Safari if desired. Offline history, graphs, reports, cloud UI and Sensor Lab remain available. Safari **cannot read ordinary Arduino Uno USB serial**. Use Feather Wi-Fi, and if hosted HTTPS cannot contact its local HTTP API, open the Feather's own independent site at **http://192.168.4.1**.

**Offline:** the PWA caches application files but not Supabase account data or sensor API results. Backup to Supabase resumes with internet; local BIN/SEN and the Feather SD recorder continue independently. Save local backups to Files or the computer because browsers can evict data.

**Troubleshooting:** no readings means no verified source; no phone USB-LoRa is expected; failed cloud sync never blocks recording. Disable advanced Sensor Lab or SEN explorer in System if it slows an older phone.

Sources: [MDN Web Serial](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API), [Responsive CSS container queries](https://developer.mozilla.org/en-US/docs/Web/CSS/Guides/Containment/Container_queries), [PWA offline caching](https://developer.mozilla.org/en-US/docs/Web/Progressive_web_apps/Guides/Offline_and_background_operation).
