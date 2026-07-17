// jsdom doesn't implement the Pointer Events capture API; OrbitControls calls
// these on real pointerdown/up. Polyfill as no-ops so tests can dispatch
// synthetic pointer events without throwing.
if (!Element.prototype.setPointerCapture) {
  Element.prototype.setPointerCapture = () => {};
}
if (!Element.prototype.releasePointerCapture) {
  Element.prototype.releasePointerCapture = () => {};
}
if (!Element.prototype.hasPointerCapture) {
  Element.prototype.hasPointerCapture = () => false;
}

// Node provides a native URL.createObjectURL/fetch — but jsdom's Blob/File
// (used throughout these tests, e.g. `new File(...)`) isn't the same class
// Node's native Blob-URL registry and undici's Response recognize, so the
// native pair silently mis-registers/corrupts jsdom Blobs rather than
// throwing (confirmed: an 840-byte jsdom Blob round-tripped through native
// createObjectURL + fetch came back truncated). So this is NOT a
// `!URL.createObjectURL` presence check — the native one exists and must be
// unconditionally replaced with our own registry that reads jsdom Blobs via
// their own .arrayBuffer() (which works correctly) before handing bytes to
// Response. This does NOT help image/texture loading — jsdom's <img> never
// fires onload/onerror for a blob: URL regardless (no real image codec), so
// it hangs forever rather than failing gracefully. Keep texture-bearing
// fixtures out of jsdom-run tests for that reason; verify those in a real
// browser instead.
const blobRegistry = new Map<string, Blob>();
let blobUrlCounter = 0;

URL.createObjectURL = (blob: Blob) => {
  const url = `blob:mock-${blobUrlCounter++}`;
  blobRegistry.set(url, blob);
  return url;
};
URL.revokeObjectURL = (url: string) => {
  blobRegistry.delete(url);
};

const originalFetch = globalThis.fetch;
globalThis.fetch = (async (input: RequestInfo | URL, init?: RequestInit) => {
  const url = typeof input === "string" ? input : input instanceof URL ? input.href : input.url;
  const blob = blobRegistry.get(url);
  if (blob) {
    return new Response(await blob.arrayBuffer());
  }
  return originalFetch(input, init);
}) as typeof fetch;
