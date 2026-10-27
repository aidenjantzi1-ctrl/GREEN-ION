const CACHE="green-ion-v11-offline-2";const FILES=["./","./index.html","./manifest.webmanifest","./icon.svg","./advanced.mjs","./analysis/rkt4.mjs","./analysis-worker.mjs"];
self.addEventListener("install",e=>e.waitUntil(caches.open(CACHE).then(c=>c.addAll(FILES))));self.addEventListener("fetch",e=>e.respondWith(caches.match(e.request).then(r=>r||fetch(e.request))));

