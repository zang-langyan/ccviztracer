(function() {
  "use strict";
  const LOG_TAG = `ServiceWorker: `;
  const CACHE_NAME = "ui-perfetto-dev";
  const OPEN_TRACE_PREFIX = "/_open_trace";
  const INDEX_TIMEOUT_MS = 3e3;
  const INSTALL_TIMEOUT_MS = 3e4;
  let postedFiles = /* @__PURE__ */ new Map();
  const ALLOWLISTED_DOMAINS = [
    /\.googleapis\.com$/,
    // For Google Cloud Storage fetches.
    /\.google-analytics\.com$/,
    // For analytics.
    /\.googletagmanager\.com$/
    // For analytics.
  ];
  function isAllowlistedDomain(hostname) {
    return ALLOWLISTED_DOMAINS.some((pattern) => pattern.test(hostname));
  }
  function isLocalhost(hostname) {
    return hostname === "localhost" || hostname === "127.0.0.1" || hostname === "::1";
  }
  function checkFirewall(req) {
    const url = new URL(req.url);
    if (isLocalhost(url.hostname)) {
      return { allowed: true };
    }
    if (url.origin === self.location.origin) {
      return { allowed: true };
    }
    if (isAllowlistedDomain(url.hostname)) {
      return { allowed: true };
    }
    if (req.method !== "GET") {
      return { allowed: false, reason: `Method ${req.method} not allowed` };
    }
    if (url.hostname === "api.github.com" && url.pathname.includes("/contents/") && url.searchParams.has("ref") && url.search.indexOf("&") === -1) {
      return { allowed: true };
    }
    if (url.search !== "") {
      return { allowed: false, reason: "Query strings not allowed" };
    }
    return { allowed: true };
  }
  self.addEventListener("install", (event) => {
    const doInstall = async () => {
      let bypass = true;
      try {
        bypass = await caches.has("BYPASS_SERVICE_WORKER");
      } catch (_) {
      }
      if (bypass) {
        throw new Error(LOG_TAG + "skipping installation, bypass enabled");
      }
      try {
        for (const key of await caches.keys()) {
          if (key.startsWith("dist-")) {
            await caches.delete(key);
          }
        }
      } catch (_) {
      }
      const match = /\bv=([\w.-]*)/.exec(location.search);
      if (!match) {
        throw new Error(
          `Failed to install. Was epecting a query string like ?v=v1.2-sha query string, got "${location.search}" instead`
        );
      }
      await installAppVersionIntoCache(match[1]);
      self.skipWaiting();
    };
    event.waitUntil(doInstall());
  });
  self.addEventListener("activate", (event) => {
    console.info(LOG_TAG + "activated");
    const doActivate = async () => {
      await self.clients.claim();
    };
    event.waitUntil(doActivate());
  });
  self.addEventListener("fetch", (event) => {
    const firewall = checkFirewall(event.request);
    if (!firewall.allowed) {
      console.warn(
        LOG_TAG + `Blocked: ${event.request.url} - ${firewall.reason}`
      );
      event.respondWith(
        new Response(`Blocked by firewall: ${firewall.reason}`, {
          status: 403,
          statusText: "Forbidden"
        })
      );
      return;
    }
    if (!shouldHandleHttpRequest(event.request)) {
      console.debug(LOG_TAG + `serving ${event.request.url} from network`);
      return;
    }
    event.respondWith(handleHttpRequest(event.request));
  });
  function shouldHandleHttpRequest(req) {
    if (req.cache === "only-if-cached" && req.mode !== "same-origin") {
      return false;
    }
    const url = new URL(req.url);
    if (url.pathname === "/live_reload") return false;
    if (url.pathname.startsWith(OPEN_TRACE_PREFIX)) return true;
    return req.method === "GET" && url.origin === self.location.origin;
  }
  async function handleHttpRequest(req) {
    if (!shouldHandleHttpRequest(req)) {
      throw new Error(LOG_TAG + `${req.url} shouldn't have been handled`);
    }
    const cacheOps = { cacheName: CACHE_NAME };
    const url = new URL(req.url);
    if (url.pathname === "/") {
      try {
        console.debug(LOG_TAG + `Fetching live ${req.url}`);
        return await fetchWithTimeout(req, INDEX_TIMEOUT_MS);
      } catch (err) {
        console.warn(LOG_TAG + `Failed to fetch ${req.url}, using cache.`, err);
      }
    } else if (url.pathname === "/offline") {
      const cachedRes2 = await caches.match(new Request("/"), cacheOps);
      if (cachedRes2) return cachedRes2;
    } else if (url.pathname.startsWith(OPEN_TRACE_PREFIX)) {
      return await handleOpenTraceRequest(req);
    }
    const cachedRes = await caches.match(req, cacheOps);
    if (cachedRes) {
      console.debug(LOG_TAG + `serving ${req.url} from cache`);
      return cachedRes;
    }
    console.warn(LOG_TAG + `cache miss on ${req.url}, using live network`);
    return fetch(req);
  }
  async function handleOpenTraceRequest(req) {
    const url = new URL(req.url);
    console.assert(url.pathname.startsWith(OPEN_TRACE_PREFIX));
    const fileKey = url.pathname.substring(OPEN_TRACE_PREFIX.length);
    if (req.method === "POST") {
      const formData = await req.formData();
      const qsParams = new URLSearchParams();
      formData.forEach((value, key) => {
        if (key === "trace") {
          if (value instanceof File) {
            postedFiles.set(fileKey, value);
            qsParams.set("url", req.url);
          }
          return;
        }
        qsParams.set(key, `${value}`);
      });
      return Response.redirect(`${url.protocol}//${url.host}/#!/?${qsParams}`);
    }
    const file = postedFiles.get(fileKey);
    if (file !== void 0) {
      postedFiles.delete(fileKey);
      return new Response(file);
    }
    return Response.error();
  }
  async function installAppVersionIntoCache(version) {
    const manifestUrl = `${version}/manifest.json`;
    try {
      console.log(LOG_TAG + `Starting installation of ${manifestUrl}`);
      await caches.delete(CACHE_NAME);
      const resp = await fetchWithTimeout(manifestUrl, INSTALL_TIMEOUT_MS);
      const manifest = await resp.json();
      const manifestResources = manifest["resources"];
      if (!manifestResources || !(manifestResources instanceof Object)) {
        throw new Error(`Invalid manifest ${manifestUrl} : ${manifest}`);
      }
      const cache = await caches.open(CACHE_NAME);
      const urlsToCache = [];
      urlsToCache.push(new Request("/", { cache: "reload", mode: "same-origin" }));
      for (const [resource, integrity] of Object.entries(manifestResources)) {
        const reqOpts = {
          cache: "no-cache",
          mode: "same-origin",
          integrity: `${integrity}`
        };
        urlsToCache.push(new Request(`${version}/${resource}`, reqOpts));
      }
      await cache.addAll(urlsToCache);
      console.log(LOG_TAG + "installation completed for " + version);
    } catch (err) {
      console.error(LOG_TAG + `Installation failed for ${manifestUrl}`, err);
      await caches.delete(CACHE_NAME);
      throw err;
    }
  }
  class TimeoutError extends Error {
    constructor(url) {
      super(`Timed out while fetching ${url}`);
    }
  }
  class NetworkError extends Error {
    constructor(url, cause) {
      super(`Network error while fetching ${url}: ${cause}`);
    }
  }
  function fetchWithTimeout(req, timeoutMs) {
    const url = req.url || `${req}`;
    return new Promise((resolve, reject) => {
      const timerId = setTimeout(() => {
        reject(new TimeoutError(url));
      }, timeoutMs);
      fetch(req).then((resp) => {
        clearTimeout(timerId);
        if (resp.ok) {
          resolve(resp);
        } else {
          reject(new Error(
            `Fetch failed for ${url}: ${resp.status} ${resp.statusText}`
          ));
        }
      }, (e) => {
        clearTimeout(timerId);
        reject(new NetworkError(url, e));
      });
    });
  }
})();
//# sourceMappingURL=service_worker.js.map

;(self.__SOURCEMAPS=self.__SOURCEMAPS||{})['service_worker.js']={"version":3,"sources":["../../src/service_worker/service_worker.ts"],"mappings":";;AA6CA;AACA;AACA;AAIA;AAIA;AAGA;AAGA;AAA4B;AAC1B;AAAA;AACA;AAAA;AACA;AAAA;AAGF;AACE;AAAmE;AAGrE;AACE;AAEe;AAQjB;AACE;AAGA;AACE;AAAiB;AAInB;AACE;AAAiB;AAInB;AACE;AAAiB;AAInB;AACE;AAAoD;AAMtD;AAIE;AAAiB;AAGnB;AACE;AAAgC;AAGlC;AAAiB;AAcnB;AACE;AAGE;AACA;AACE;AAAiD;AACvC;AAGZ;AAEE;AAAiE;AAInE;AACE;AACE;AACE;AAAuB;AACzB;AACF;AACU;AASZ;AACA;AACE;AAAU;AAE2C;AAAA;AAEvD;AASA;AAAK;AAEP;AAA2B;AAG7B;AACE;AACA;AAIE;AAAmB;AAErB;AAA4B;AAG9B;AAEE;AACA;AACE;AAAQ;AACsD;AAE9D;AAAM;AACoD;AAC9C;AACI;AACb;AAEH;AAAA;AAKF;AACE;AACA;AAAA;AAGF;AAAkD;AAIpD;AAIE;AACE;AAAO;AAGT;AACA;AACA;AAEA;AAA4D;AAG9D;AACE;AACE;AAAkE;AAcpE;AACA;AACA;AACE;AACE;AAEA;AAAmD;AAEnD;AAAsE;AAExE;AAIA;AACA;AAAsB;AAEtB;AAAuC;AAGzC;AACA;AACE;AACA;AAAO;AAKT;AACA;AAAgB;AAuBlB;AACE;AACA;AACA;AACA;AACE;AACA;AAIA;AACE;AACE;AACE;AACA;AAA2B;AAE7B;AAAA;AAEF;AAA4B;AAE9B;AAAuE;AAIzE;AACA;AACE;AACA;AAAwB;AAI1B;AAAgB;AAGlB;AACE;AACA;AACE;AACA;AACA;AACA;AACA;AACA;AACE;AAA+D;AAGjE;AACA;AAKA;AAEA;AAIE;AAA6B;AACpB;AACD;AACiB;AAEzB;AAA+D;AAEjE;AACA;AAA6D;AAE7D;AACA;AACA;AAAM;AACR;AACF;AAEiC;AAE7B;AAAuC;AACzC;AACF;AAEiC;AAE7B;AAAqD;AACvD;AAGF;AACE;AACA;AACE;AACE;AAA4B;AAE9B;AACE;AACA;AACE;AAAY;AAEZ;AAAW;AACmD;AAAG;AACnE;AACU;AAAuB;AAA+B;AAAI;AACvE","file":"service_worker.js"};