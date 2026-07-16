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
