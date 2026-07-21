# VS Code Webview API constraints for porting the Renderit viewer

Research date: 2026-07-21. Findings below were verified directly against
`code.visualstudio.com/api` (and its underlying markdown source in
`microsoft/vscode-docs`), the `vscode.d.ts` type definitions in
`microsoft/vscode` (branch `main`, fetched 2026-07-21), the official
`microsoft/vscode-extension-samples` repo, `microsoft/vscode` GitHub issues,
and `vitejs/vite` docs, as of the fetch date above. VS Code's extension API
evolves; re-verify version-sensitive claims (e.g. the `1.57+` ArrayBuffer
behavior in §4) against the current docs before relying on them long after
this date.

Purpose: ground the GitHub issue/ticket specs for the VS Code extension port
of the Renderit Three.js viewer (`web/src/viewer/*.ts`, `web/src/main.ts`).

Legend used throughout: **[BLOCKER]** = hard constraint, the port cannot work
another way. **[REQUIRED PATTERN]** = documented, must be followed but has a
known recipe. **[GRAY AREA]** = not officially guaranteed; empirically
observed or community-reported only — flag as a risk/spike in ticket, not an
acceptance criterion.

---

## 1. Content Security Policy

**Source:** [Webview API guide → Security → Content security policy](https://code.visualstudio.com/api/extension-guides/webview#content-security-policy)
(markdown source: `microsoft/vscode-docs/api/extension-guides/webview.md`,
lines 945–982, DateApproved 2026-07-15 per the page's own front matter).

- **[REQUIRED PATTERN]** CSP is applied via a `<meta>` tag at the top of
  `<head>`: `<meta http-equiv="Content-Security-Policy" content="...">`. Exact
  quote: "To add a content security policy, put a
  `<meta http-equiv="Content-Security-Policy">` directive at the top of the
  webview's `<head>`."
- The doc's minimal example: `content="default-src 'none';"` — quote: "The
  policy `default-src 'none';` disallows all content."
- The doc's working example for a scripted webview with images:
  ```html
  <meta
    http-equiv="Content-Security-Policy"
    content="default-src 'none'; img-src ${webview.cspSource} https:; script-src ${webview.cspSource}; style-src ${webview.cspSource};"
  />
  ```
  `${webview.cspSource}` is a placeholder for the `Webview.cspSource` runtime
  value (see below) — "The `${webview.cspSource}` value is a placeholder for
  a value that comes from the webview object itself."
- **[BLOCKER-adjacent]** Quote: "This content security policy also implicitly
  disables inline scripts and styles." `default-src 'none'` (or any
  `script-src` allow-list) blocks inline `<script>` and inline event handlers
  by default — Three.js/Vite bundle output must be an external `<script src>`
  file, not inlined.
- **`cspSource`** — exact doc comment from `vscode.d.ts` (`Webview.cspSource`,
  line ~10038): "Content security policy source for webview resources. This
  is the origin that should be used in a content security policy rule:
  ```` `img-src https: ${webview.cspSource} ...;` ````" This resolves to the
  `vscode-webview://<uuid>` origin at runtime (the actual scheme/origin value
  isn't spelled out as a literal string in the docs — extensions are expected
  to always use the `cspSource` property rather than hardcode the scheme).
- **Nonce pattern (not in the prose guide, but in the official sample):**
  the guide's own CSP example above does **not** use a nonce — it CSP-allows
  scripts by matching `script-src` to `webview.cspSource`. The official
  `webview-sample` extension (`microsoft/vscode-extension-samples`,
  `webview-sample/src/extension.ts`) instead uses a **nonce**, generated as:
  ```ts
  function getNonce() {
    let text = '';
    const possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    for (let i = 0; i < 32; i++) {
      text += possible.charAt(Math.floor(Math.random() * possible.length));
    }
    return text;
  }
  ```
  used in CSP as `script-src 'nonce-${nonce}';` and on the tag itself as
  `<script nonce="${nonce}" src="${scriptUri}"></script>`. Both the
  `cspSource`-based allow-list and the nonce pattern are legitimate; the
  nonce pattern is what the maintained sample actually ships, so it is the
  safer one to copy for a new extension. **No hash-based (`sha256-...`)
  CSP example appears in either the guide or the sample** — nonce is the
  documented/demonstrated mechanism for VS Code webviews, not hashes.
- **WebGL/Three.js specifically:** neither the webview guide nor the sample
  mentions WebGL, `<canvas>`, or a `worker-src`/`connect-src` directive by
  name — **[GRAY AREA, but low-risk]**. WebGL context creation and rendering
  to a `<canvas>` is not itself network-gated by CSP (it's not a
  `script-src`/`img-src`-controlled resource type), so no special CSP
  directive is documented as required for WebGL to run once scripts are
  allowed to execute. However, Three.js loaders (`fetch()`/`XMLHttpRequest`
  calls made by `GLTFLoader`, `RGBELoader`, etc. to load the `.glb`/HDRI
  files) are governed by the CSP `connect-src` (or `default-src` if
  `connect-src` is unset) directive — **this is not explicitly called out in
  VS Code's docs**, it follows from general CSP semantics, but it means the
  CSP meta tag will need `connect-src ${webview.cspSource}` (or an
  equivalent) in addition to `img-src`/`script-src`/`style-src`, or fetches
  of `asWebviewUri`-mapped resources will be silently blocked. Flag this as
  a **ticket acceptance-criterion item to test explicitly** since it's
  inferred, not documented.
- **Web Workers inside a webview** — relevant if any Three.js loader used by
  the viewer employs worker-based decoding (e.g. `DRACOLoader`/`KTX2Loader`
  style loaders spin up worker pools from a script URL). Per the guide's
  ["Using Web Workers"](https://code.visualstudio.com/api/extension-guides/webview#using-web-workers)
  section (webview.md lines 895–914): **[BLOCKER for naive ports]** "workers
  can only be loaded using either a `data:` or `blob:` URI. You cannot
  directly load a worker from your extension's folder." The documented
  workaround is to `fetch()` the worker script and construct a `blob:` URL:
  ```js
  fetch(workerSource).then(r => r.blob()).then(blob => {
    const blobUrl = URL.createObjectURL(blob);
    new Worker(blobUrl);
  });
  ```
  Also: "Worker scripts also do not support importing source code using
  `importScripts` or `import(...)`" — a worker script must be bundled as a
  single self-contained file (the guide recommends webpack's
  `LimitChunkCountPlugin`; the Vite/rollup equivalent would be an
  `output.inlineDynamicImports`-style single-file worker build). If Renderit's
  showcase model or any supported import path exercises a Draco/KTX2/worker
  code path in Three.js, this needs its own ticket line item.

---

## 2. `asWebviewUri()` and `localResourceRoots`

**Sources:** [Webview API guide → Loading local content](https://code.visualstudio.com/api/extension-guides/webview#loading-local-content)
(webview.md lines 433–528) and `vscode.d.ts` (`Webview.asWebviewUri`,
`WebviewOptions.localResourceRoots`, lines 9922–9929 and 10023–10036).

- **[BLOCKER]** Quote (webview.md:435): "Webviews run in isolated contexts
  that cannot directly access local resources. This is done for security
  reasons. This means that in order to load images, stylesheets, and other
  resources from your extension, or to load any content from the user's
  current workspace, you must use the `Webview.asWebviewUri` function to
  convert a local `file:` URI into a special URI that VS Code can use to
  load a subset of local resources." This applies to **every** local asset
  the built Vite bundle references — the JS/CSS entry files, the bundled
  `showcase.glb`, and the bundled HDRI environment maps.
- **API shape** (`vscode.d.ts`):
  ```ts
  asWebviewUri(localResource: Uri): Uri;
  ```
  Doc comment: "Convert a uri for the local file system to one that can be
  used inside webviews. Webviews cannot directly load resources from the
  workspace or local file system using `file:` uris." Usage pattern from the
  guide:
  ```ts
  const onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'media', 'cat.gif');
  const catGifSrc = panel.webview.asWebviewUri(onDiskPath);
  ```
  The guide notes the resulting URI historically looked like
  `vscode-resource:/…` in its worked example text, but current VS Code
  produces `https://<uuid>.vscode-webview.net/...`-shaped
  (or `vscode-webview://` scheme, depending on platform/version) URIs at
  runtime — the doc does not commit to one literal string, which is exactly
  why code must always call `asWebviewUri()` rather than construct/guess the
  scheme by hand.
- **`localResourceRoots`** (`WebviewOptions`, `vscode.d.ts` line ~9922): "Root
  paths from which the webview can load local (filesystem) resources using
  uris from `asWebviewUri` Default to the root folders of the current
  workspace plus the extension's install directory. Pass in an empty array to
  disallow access to any local resources." The guide reiterates the default
  ("By default, webviews can only access resources in the following
  locations: Within your extension's install directory. Within the user's
  currently active workspace.") and gives the restriction pattern:
  ```ts
  localResourceRoots: [vscode.Uri.joinPath(context.extensionUri, 'media')]
  ```
  **[REQUIRED PATTERN]** For the Renderit port, `localResourceRoots` should
  be scoped to the extension's bundled `dist`/`media` output directory (for
  the shipped bundle, `showcase.glb`, and HDRIs) **and**, separately, to
  whatever the file-import feature needs to read from the user's
  workspace/filesystem (see §4) — the guide explicitly warns
  "`localResourceRoots` does not offer complete security protection on its
  own," i.e. this is a defense-in-depth control layered under CSP, not a
  substitute for it.
- Data URIs are always allowed regardless of `localResourceRoots`: "You can
  also always use data URIs to embed resources directly within the webview,"
  which is a possible (if wasteful for a multi-MB `.glb`) fallback for very
  small assets.

---

## 3. `postMessage` bridging and sandboxing

**Sources:** [Webview API guide → Scripts and message passing](https://code.visualstudio.com/api/extension-guides/webview#scripts-and-message-passing)
(webview.md lines 669–895) and `vscode.d.ts` (`Webview` interface, lines
9950–10048).

- **[BLOCKER, architectural]** `vscode.d.ts` doc comment on `Webview.html`
  (line ~9961, this is the single most load-bearing sentence in the whole
  API): "Webviews are sandboxed from normal extension process, so all
  communication with the webview must use message passing." The webview
  content is (per the guide's own framing, webview.md:14) "an `iframe`
  within VS Code that your extension controls" — it runs in a separate,
  sandboxed renderer context with **no Node.js APIs, no `fs` access, and no
  direct access to the `vscode` extension API**. Every filesystem read (model
  files, textures) must happen extension-host-side and be relayed in.
- **Host → webview:** `webview.postMessage(message)` — quote from the guide:
  "An extension can send data to its webviews using `webview.postMessage()`.
  This method sends any JSON serializable data to the webview. The message
  is received inside the webview through the standard `message` event"
  (`window.addEventListener('message', event => { const message = event.data; ... })`).
  `vscode.d.ts` signature: `postMessage(message: any): Thenable<boolean>` —
  doc comment clarifies delivery is best-effort: "Messages are only delivered
  if the webview is live (either visible or in the background with
  `retainContextWhenHidden`)" and "A response of `true` does not mean that
  the message was actually received by the webview" (no listener may be
  attached yet).
- **Webview → host:** `acquireVsCodeApi()` — quote: "This is accomplished
  using a `postMessage` function on a special VS Code API object inside the
  webview. To access the VS Code API object, call `acquireVsCodeApi` inside
  the webview. This function can only be invoked once per session. You must
  hang onto the instance ... and hand it out to any other functions that need
  to use it." **[BLOCKER if violated]** — calling `acquireVsCodeApi()` more
  than once throws; the Vite app's bootstrap code must call it exactly once
  at startup and pass the handle down (this matters for how `main.ts` gets
  restructured — it can no longer just be a flat script that pokes globals
  freely if multiple modules independently want the API handle).
- **What can be sent:** `Webview.onDidReceiveMessage` doc comment (line
  ~9986): "Webview content can post strings or json serializable objects back
  to an extension. They cannot post `Blob`, `File`, `ImageData` and other DOM
  specific objects since the extension that receives the message does not
  run in a browser environment." This directly affects the drag-and-drop /
  file-import port: browser `File`/`Blob` objects (which `DragDropFiles.ts`
  and `ImportFileSet.ts` presumably traffic in today) **cannot** cross the
  postMessage boundary as-is; only their raw bytes (as a serializable
  array/ArrayBuffer, base64 string, etc.) can.

---

## 4. File/folder import equivalent, and payload size for large binaries

**Sources:** `vscode.d.ts` (`OpenDialogOptions`, `window.showOpenDialog`,
`FileSystem.readFile`), and `microsoft/vscode` GitHub issues #115411,
#115807, #79257 (fetched directly via `gh issue view`).

**Editorial note (post-research scope decision):** this section was written
assuming an in-webview "browse file/folder" button, mirroring the website's
welcome-page UI. The user later clarified the extension drops that UI
entirely — opening is Explorer-native (single/double-click a model file,
right-click "Open with Renderit" for zip/folder) — so `showOpenDialog` is
**not used** in the actual design; see §8 for the mechanism that replaces it.
Everything below about `workspace.fs.readFile`'s `Uint8Array` fast-path,
`postMessage` payload behavior, and the `File`/`Blob` sandboxing restriction
still applies identically to reading a file/folder/zip triggered from the
Explorer instead of from a dialog — only the *trigger* changes, not the
host↔webview relay mechanics.

- **File/folder picker — extension-host side.** `vscode.d.ts` (line ~11466):
  ```ts
  export function showOpenDialog(options?: OpenDialogOptions): Thenable<Uri[] | undefined>;
  ```
  `OpenDialogOptions` (line ~2063) fields, quoted directly:
  - `canSelectFiles?: boolean` — "Allow to select files, defaults to `true`."
  - `canSelectFolders?: boolean` — "Allow to select folders, defaults to
    `false`."
  - `canSelectMany?: boolean` — "Allow to select many files or folders."
  - `filters?: { [name: string]: string[] }` — e.g.
    `{ 'GLB/GLTF': ['glb', 'gltf'], 'Zip': ['zip'] }`.
  - `defaultUri?: Uri`, `openLabel?: string`, `title?: string` (title "might
    be ignored ... macOS").
  **[REQUIRED PATTERN]** This is the direct extension-host replacement for
  the web app's `<input type="file">`/folder picker: `canSelectFolders: true`
  plus `canSelectMany: true` reproduces "flat or nested folder" import; a
  separate `filters: { Zip: ['zip'] }` call (or reusing the same dialog with
  both filters) reproduces zip-file import.
- **Reading bytes — extension-host side.** `vscode.workspace.fs` (the
  `FileSystem` interface) `readFile`:
  ```ts
  readFile(uri: Uri): Thenable<Uint8Array>;
  ```
  Doc comment: "Read the entire contents of a file. @returns An array of
  bytes or a thenable that resolves to such." For folder imports, the host
  would need to enumerate directory entries via `workspace.fs.readDirectory`
  (recursively, for "nested folder" support) and `readFile` each one, since
  there is no bulk/streaming read API documented on this interface.
- **Relaying bytes into the webview — payload size.** **There is no
  documented hard payload-size ceiling for `webview.postMessage`** in the
  VS Code API docs or `vscode.d.ts` — this is a **[GRAY AREA]** confirmed
  empirically via VS Code's own issue tracker rather than a published limit:
  - `vscode.d.ts` `Webview.postMessage` doc comment (line ~9999) itself flags
    a real, version-gated performance cliff: "For older versions of vscode,
    if an `ArrayBuffer` is included in `message`, it will not be serialized
    properly and will not be received by the webview. Similarly any
    TypedArrays, such as a `Uint8Array`, will be very inefficiently
    serialized and will also not be recreated as a typed array inside the
    webview. However if your extension targets vscode 1.57+ in the `engines`
    field of its `package.json`, any `ArrayBuffer` values that appear in
    `message` will be more efficiently transferred to the webview and will
    also be correctly recreated inside of the webview." **[REQUIRED
    PATTERN]** the `package.json` `engines.vscode` field must be `^1.57.0`
    or later, and binary payloads (texture/`.glb` bytes) must be sent as
    `ArrayBuffer`, not as a plain `Uint8Array`/array-of-numbers, to get
    efficient transfer.
  - GitHub issue [microsoft/vscode#115411](https://github.com/microsoft/vscode/issues/115411)
    ("Support transferables in webview postMessage", closed as duplicate of
    #115807) documents the pre-fix behavior directly from the reporter: "a
    `Uint8Array` ... ends up being encoded as an object like `{0: 0, 1: 1}`"
    and that workarounds (`Array.from`, ASCII string encode/decode) are
    "slow, duplicates memory and causes unnecessary GC." VS Code maintainer
    **@mjbvz** (Matt Bierner) closed it with: "Supporting SharedArrayBuffers
    (or anything that cannot be serialized) is not possible due to our
    extension architecture. This is not likely to change in the short term"
    — i.e. **[BLOCKER]** `SharedArrayBuffer`-based zero-copy sharing between
    host and webview is confirmed architecturally impossible, not just
    unimplemented.
  - The actual fix, [microsoft/vscode#115807](https://github.com/microsoft/vscode/issues/115807)
    ("Improve transfer of ArrayBuffers to and from webviews", closed,
    shipped 1.57), contains real before/after perf numbers from a VS Code
    core contributor's (@Tyriar) own testing, which is the closest thing to
    an authoritative performance data point available: "Using 1.57.0 ...
    the transport time on save (webview → exthost) drops from ~4s to ~400ms
    ... For opening (exthost → webview), it went from ~10s to ~500ms." Same
    comment flags a sharp gotcha for exactly this port's use case: **`await
    workspace.fs.readFile(uri)` returns a `Buffer` in practice (despite the
    `Uint8Array` typing), and a `Buffer` does *not* get the fast-path
    ArrayBuffer transfer** — the fix required explicitly rewrapping:
    ```ts
    // Slow (Buffer doesn't get fast-path treatment):
    const data = await workspace.fs.readFile(uri);
    // Fast:
    const data = new Uint8Array(await workspace.fs.readFile(uri));
    ```
    **[REQUIRED PATTERN]** — any file-import code path that calls
    `workspace.fs.readFile` and forwards the result into `postMessage` must
    explicitly wrap it in `new Uint8Array(...)` (and pass `.buffer`, or the
    typed array itself per current API, as the transferable) or it will
    silently fall back to the slow serialization path with no error.
  - No source found (docs, `vscode.d.ts`, or these issues) states a maximum
    message size in bytes/MB. Given the ~4s→400ms/~10s→500ms figures were for
    unspecified-but-presumably-moderate payloads, and no ceiling is
    documented, **treat this as an open risk to spike early in the sprint**:
    a folder import containing many large binary textures should be
    prototyped and measured directly (chunking per-file, or streaming
    multiple smaller `postMessage` calls per asset instead of one giant
    message, is a reasonable defensive pattern even though it's not
    something VS Code's docs prescribe).
  - `onDidReceiveMessage`/webview→host direction has the same "cannot post
    `Blob`, `File`, `ImageData`" restriction noted in §3 — the file
    picker/zip/folder-import flow is necessarily host-initiated
    (`showOpenDialog` + `readFile`), not webview-initiated the way browser
    drag-and-drop is today.

---

## 5. Native OS drag-and-drop into a webview

**Sources:** `microsoft/vscode` issues #182449, #139111 (fetched via
`gh issue view`), and the [Custom Editor API guide](https://code.visualstudio.com/api/extension-guides/custom-editors).

- **[CONFIRMED WORKING, but with a specific caveat]** GitHub issue
  [microsoft/vscode#182449](https://github.com/microsoft/vscode/issues/182449)
  ("Drop files from Explorer to CustomEditor (webview) doesn't work") is the
  most directly relevant primary source. The reporter built a minimal test
  `CustomEditor` and explicitly enumerated three drop sources, quoted
  verbatim from the issue body:
  - "Drop a file from Explorer tree view: ❌ **does not work**"
  - "Drop an item with `dataTransfer.setData('text/uri-list', value)`
    manually set in another webview: ✅ OK"
  - "Drop a file from OS: ✅ OK"

  So **dragging a file from the native OS file system (Finder/Windows
  Explorer/desktop) directly onto a webview's/custom editor's content area
  is confirmed to work** — this directly supports porting `DragDropFiles.ts`'s
  drop-target behavior largely as-is at the HTML5 `dragover`/`drop` event
  level inside the webview's own document. What does **not** work by default
  is dragging from VS Code's own **Explorer tree view** (the sidebar file
  tree) onto a webview — that is a separate, VS Code-internal drag source
  that historically wasn't wired to deliver into webviews.
  - This issue is **closed**, labeled `bug`, `verified`, `insiders-released`,
    milestone **"June 2024"** — i.e. Microsoft explicitly fixed the
    Explorer-tree-view-to-webview gap as a confirmed bug fix, meaning
    current VS Code versions (well past June 2024) should support dropping
    from the Explorer sidebar too, not just from the OS. Recommend spiking
    this on the target VS Code version rather than assuming either way.
- **[GRAY AREA / narrower caveat]** [microsoft/vscode#139111](https://github.com/microsoft/vscode/issues/139111)
  ("iframe within a webview does not allow you to use drag and drop") is
  about a **nested `<iframe>` embedded inside a webview's HTML** (e.g. a
  webview that itself embeds an external site via `asExternalUri` inside an
  `<iframe src="...">`, per the [Remote Extensions "Option 1" pattern](https://code.visualstudio.com/api/advanced-topics/remote-extensions#option-1-use-asexternaluri)
  referenced in the issue body) — not the webview's own top-level document.
  Reporter quote: "On the webview, it all works fine, but inside the iframe
  its contents it's not working. It seems that events are just ignored or
  not correctly bubble down to the iframe." **This does not apply to the
  Renderit port as planned** — the Vite-built viewer would be loaded as the
  webview's own root HTML document (via `panel.webview.html`), not as a
  nested iframe-within-the-webview, so drag-and-drop events should reach it
  directly per issue #182449's confirmation. Only becomes relevant if a
  future architecture choice nests the viewer inside an iframe within the
  webview shell.
- **Custom Editor API as the architecturally "correct" home for this
  feature:** the [Custom Editor guide](https://code.visualstudio.com/api/extension-guides/custom-editors)
  (`microsoft/vscode-docs/api/extension-guides/custom-editors.md`, line 14)
  explicitly lists as a use case: **"Previewing assets, such as shaders or
  3D models, directly in VS Code."** — this is a direct, named endorsement of
  the Renderit use case from Microsoft's own docs, and suggests the sprint
  should scope the extension as a **`CustomEditorProvider`/`CustomReadonlyEditorProvider`**
  registered against `.glb`/`.gltf` (opening a dropped/opened model file
  directly, VS Code-native-editor-style) rather than (or in addition to) a
  plain command-triggered `WebviewPanel`. The guide (line 52) frames the
  choice plainly: "if you are working with a text based file format use
  `CustomTextEditorProvider`, for binary file formats use
  `CustomEditorProvider`" — `.glb`/`.gltf`-with-binary-buffers is squarely
  the binary case.

---

## 6. State persistence: `retainContextWhenHidden` and `WebviewPanelSerializer`

**Sources:** [Webview API guide → Persistence](https://code.visualstudio.com/api/extension-guides/webview#persistence)
(webview.md lines 1002–1129) and `vscode.d.ts` (`WebviewPanelOptions`,
`WebviewPanelSerializer`, lines 10053–10217).

- **Default behavior without any persistence mechanism:** quote
  (webview.md:1004): "The contents of webviews ... are created when the
  webview becomes visible and destroyed when the webview is moved into the
  background. Any state inside the webview will be lost when the webview is
  moved to a background tab." **[BLOCKER-relevant]** — this is exactly the
  "viewer resets when panel loses focus" problem the ticket needs to solve;
  it is the documented default, not a bug.
- **Preferred fix — `getState`/`setState`:** inside the webview,
  `acquireVsCodeApi()` also exposes `getState()`/`setState(jsonValue)` for a
  JSON-serializable blob, persisted across the webview's content being
  torn down/recreated (but destroyed once the panel itself is closed). Quote:
  "`getState` and `setState` are the preferred way to persist state, as they
  have much lower performance overhead than `retainContextWhenHidden`."
  For a Three.js viewer this would mean serializing camera position, loaded
  model reference/metadata, lighting preset, etc. — not the live WebGL
  context or loaded geometry buffers themselves (those aren't
  JSON-serializable and would need to be reloaded/rebuilt from the
  persisted state on restore).
- **`retainContextWhenHidden`** (`WebviewPanelOptions`, `vscode.d.ts` line
  ~10062): "Controls if the webview panel's content (iframe) is kept around
  even when the panel is no longer visible. Normally the webview panel's
  html context is created when the panel becomes visible and destroyed when
  it is hidden. Extensions that have complex state or UI can set the
  `retainContextWhenHidden` to make the editor keep the webview context
  around, even when the webview moves to a background tab. When a webview
  using `retainContextWhenHidden` becomes hidden, its scripts and other
  dynamic content are suspended. When the panel becomes visible again, the
  context is automatically restored in the exact same state it was in
  originally. You cannot send messages to a hidden webview, even with
  `retainContextWhenHidden` enabled." — note the last sentence: even with
  this flag on, `postMessage` from the host to a currently-hidden webview
  is a no-op (confirmed again in the `postMessage` doc comment: "Messages
  are only delivered if the webview is live (either visible or in the
  background with `retainContextWhenHidden`)" — so `retainContextWhenHidden`
  is specifically what makes a *hidden* webview still count as "live" for
  message delivery).
- **[DOCUMENTED COST — explicit warning]** Quote (webview.md:1128, verbatim):
  "Although `retainContextWhenHidden` may be appealing, keep in mind that
  this has high memory overhead and should only be used when other
  persistence techniques will not work." The `WebviewPanelOptions` doc
  comment doesn't repeat the word "memory" but the guide is unambiguous and
  is the citable source for ticket acceptance criteria: **default to
  `getState`/`setState`-driven state reconstruction (reload model URI +
  camera pose + lighting preset from persisted JSON) and only reach for
  `retainContextWhenHidden: true` if reload-on-show proves too slow/jarring
  for large `.glb` files** — this is a real trade-off to call out explicitly
  in the ticket rather than defaulting to `retainContextWhenHidden: true`
  for convenience.
- **`WebviewPanelSerializer`** — for surviving a full VS Code
  restart/reload (not just a panel losing focus). `vscode.d.ts` (line
  ~10204):
  ```ts
  export interface WebviewPanelSerializer<T = unknown> {
    deserializeWebviewPanel(webviewPanel: WebviewPanel, state: T): Thenable<void>;
  }
  ```
  Doc comment on the method: "Restore a webview panel from its serialized
  `state`. Called when a serialized webview first becomes visible." Guide
  usage pattern: register via
  `vscode.window.registerWebviewPanelSerializer(viewType, serializerInstance)`,
  and declare an `onWebviewPanel:<viewType>` activation event in
  `package.json` so the extension is reactivated to restore the panel: "This
  activation event ensures that our extension will be activated whenever VS
  Code needs to restore a webview with the viewType." The `state` argument is
  exactly the JSON blob the webview previously passed to `setState` — i.e.
  serialization is explicitly built on top of `getState`/`setState`, not a
  separate mechanism ("Serialization builds on `getState` and `setState`,
  and is only enabled if your extension registers a
  `WebviewPanelSerializer`").

---

## 7. Vite `dist/` output and `asWebviewUri` interaction

**Sources:** [`asWebviewUri` requirement, §2 above] and Vite's own docs —
`vitejs/vite` repo, `docs/config/shared-options.md` (the `base` option) and
`docs/guide/build.md` ("Public Base Path" section), fetched directly from
`raw.githubusercontent.com/vitejs/vite/main/...`.

- **Vite's `base` option** (`vite-shared-options.md`, verbatim):
  ```
  ## base
  - Type: string
  - Default: /
  Base public path when served in development or production. Valid values include:
  - Absolute URL pathname, e.g. /foo/
  - Full URL, e.g. https://bar.com/foo/ (The origin part won't be used in development so the value is the same as /foo/)
  - Empty string or ./ (for embedded deployment)
  ```
  By default (`base: '/'`), Vite emits **absolute, domain-root-relative**
  asset paths in the built `dist/index.html` and in JS-imported asset URLs —
  e.g. `<script src="/assets/index-abc123.js">` and
  `import(/* ... */ "/assets/chunk-xyz.js")`. This is the exact problem
  flagged in the task brief: an absolute `/assets/...` path, if left
  untouched, would resolve against whatever origin the webview's document is
  served from and will not go through VS Code's URI-rewriting layer at all —
  it will 404 or (worse) attempt to load from an unintended origin, because
  it never becomes an `asWebviewUri()`-minted URI.
- **The fix — relative base:** Vite's `build.md` ("Public Base Path" /
  "Relative base" sections, verbatim): "If you don't know the base path in
  advance, you may set a relative base path with `"base": "./"` or
  `"base": ""`. This will make all generated URLs to be relative to each
  file." **[REQUIRED PATTERN]** Renderit's `vite.config.ts` (or equivalent)
  needs `base: './'` (or `''`) for the extension-targeted build (possibly a
  separate build target/mode from the existing web-hosted build, since the
  open-web deployment presumably wants to keep `base: '/'` or its current
  setting) — this makes `dist/index.html`'s emitted `<script src="./assets/...">`
  and CSS/asset references **relative to the HTML file's own location**
  rather than domain-root-absolute.
- **Why relative alone isn't sufficient — interaction with `asWebviewUri`:**
  relative paths solve *where the browser resolves the URL from*, but the
  webview still enforces (per §2) that "Webviews cannot directly load
  resources from the workspace or local file system using `file:` uris" —
  a relative path resolved against the webview document's base URI would
  still resolve to a `file:`/`vscode-webview://`-relative path that VS Code
  has not explicitly allow-listed via `localResourceRoots`, and would not
  carry whatever token/origin-rewriting `asWebviewUri()` performs. **In
  practice this means the extension's activation code cannot just point
  `panel.webview.html` at the raw `dist/index.html` file and let relative
  paths do the work** — the standard, documented pattern (used throughout
  the webview guide's own examples) is for the extension host to read/
  template the HTML and rewrite each asset reference through
  `webview.asWebviewUri(vscode.Uri.joinPath(extensionUri, 'dist', 'assets', 'index-abc123.js'))`
  explicitly, i.e.:
  1. Build with `base: './'` so Vite's manifest/HTML uses relative,
     single-directory-relative paths (avoids Vite baking in an unrelated
     absolute prefix that would need stripping).
  2. At activation time, either (a) parse `dist/index.html` and replace each
     `src="./assets/..."`/`href="./assets/..."` with the corresponding
     `panel.webview.asWebviewUri(...)` result, or (b) skip Vite's own
     `index.html` entirely and hand-author the webview's HTML shell
     (following the CSP-meta-tag + nonce pattern in §1), injecting
     `asWebviewUri()`-derived `<script>`/`<link>` tags that point at the
     hashed filenames Vite's build manifest (`dist/.vite/manifest.json` /
     `manifest.json` if `build.manifest: true` is set) reports. Option (b)
     is what the official `webview-sample` and most Vite-in-a-webview
     community setups do in practice, since it avoids fragile HTML
     string-rewriting of Vite's own output.
  - This overall interaction (Vite `base` config + mandatory
    `asWebviewUri` rewriting) is **not documented anywhere on
    code.visualstudio.com** — VS Code's docs don't know Vite exists. The
    finding here is a synthesis of two independently-documented, individually
    non-negotiable constraints (Vite's `base` behavior + VS Code's
    `asWebviewUri` requirement), not a single citable "how-to" — treat the
    exact rewriting mechanism (manifest-driven HTML generation vs.
    post-build string replace) as an implementation decision for the sprint,
    not something VS Code or Vite prescribes.
- **Existing `import.meta.env.BASE_URL` usage is directly implicated:** the
  brief notes `import.meta.env.BASE_URL` is already used in
  `web/src/main.ts:207` and `web/src/viewer/LightingPresets.ts:67` to build
  fetch URLs for the showcase `.glb` and bundled HDRIs. Per Vite's own docs
  (`build.md`): "when dynamically building URLs at runtime, developers must
  use the globally injected `import.meta.env.BASE_URL` variable ... Note
  this variable is statically replaced during build so it must appear
  exactly as-is (i.e. `import.meta.env['BASE_URL']` won't work)." Because
  `BASE_URL` reflects whatever `base` is configured (`./` for the extension
  build), these two call sites will need their resulting relative path
  additionally routed through `asWebviewUri()` (or otherwise reconciled with
  wherever the two options above land) rather than fetched directly — a
  plain `fetch(import.meta.env.BASE_URL + 'showcase.glb')` will not
  transparently work inside a webview even with a correct relative `base`,
  because "relative to the HTML document" and "an address the webview
  sandbox will actually serve" are two different things per §2's sandboxing
  rule.

---

## 8. Entry points: `customEditors` priority, and folder/zip via `explorer/context`

**Sources:** [Contribution points reference → `customEditors`](https://code.visualstudio.com/api/references/contribution-points)
and [When clause contexts reference](https://code.visualstudio.com/api/references/when-clause-contexts)
(markdown source: `microsoft/vscode-docs/api/references/when-clause-contexts.md`),
fetched directly.

- **`customEditors` contribution, `priority` field** — two documented values:
  `"default"` ("Try to use the custom editor for every file that matches the
  custom editor's `selector`") and `"option"` ("Do not use the custom editor
  by default but allow users to switch to it or configure it as their
  default" — surfaces in the "Open With..." picker instead of taking over the
  file type). `selector.filenamePattern` matches by glob, e.g. `"*.glb"` or
  `"*.zip"`.
- **[BLOCKER]** Custom editors are **file-resource-only** — the contribution
  points doc's examples and prose only ever describe matching individual
  files by filename pattern; there is no mechanism for registering a custom
  editor against a **folder**. A folder cannot be opened via `customEditors`
  at any priority.
- **The folder (and, for a consistent single right-click affordance, zip)
  path is a `menus`→`explorer/context` command instead**, gated by a `when`
  clause. Exact quotes from `when-clause-contexts.md`:
  - `explorerResourceIsFolder`: "True if a folder is selected in the
    Explorer."
  - `resourceExtname`: "True when the Explorer or editor filename extension
    matches. Example: `resourceExtname == .js`"
  **[REQUIRED PATTERN]** contribute a command (e.g. `renderit.openFolder`)
  to `explorer/context` with
  `"when": "explorerResourceIsFolder || resourceExtname == .zip"`, labeled
  "Open with Renderit", which reads the folder (recursive
  `workspace.fs.readDirectory` + `readFile` per entry) or the zip file
  (`readFile` + extract, reusing `ImportFileSet.ts`'s existing
  `extractZip`/`fflate` logic almost verbatim since it already operates on
  bytes, not on `File`/browser APIs specifically) and opens a `WebviewPanel`
  fed via the same `postMessage`-relay pattern from §3–4. Registering `.zip`
  as a `customEditors` `priority: "option"` entry *in addition* to this
  command is possible (it would additionally surface Renderit in the native
  "Open With..." submenu for zips specifically, since zips are real file
  resources) but isn't required — a single unified `explorer/context` command
  covering both is simpler and is what the ticket should default to unless
  parity with the native "Open With..." picker for zip specifically turns
  out to matter.

---

## Sprint-relevant flags

Hard blockers / must-follow architectural approaches to bake into ticket
acceptance criteria:

- **All local asset references (bundled JS/CSS, `showcase.glb`, HDRIs, and
  any user-imported model/texture files) must be resolved through
  `webview.asWebviewUri()` — raw/relative paths from Vite's default build
  will not load.** This includes the two existing `import.meta.env.BASE_URL`
  call sites (`web/src/main.ts:207`, `web/src/viewer/LightingPresets.ts:67`),
  which need rework, not just a `base` config tweak.
- **Vite must build with `base: './'` (or `''`) for the extension target**,
  likely as a separate build mode/config from the open-web build — plus a
  manifest- or template-driven step at extension-activation time that
  rewrites each asset URL through `asWebviewUri()`. No off-the-shelf VS
  Code or Vite feature does this rewrite automatically.
- **CSP is mandatory and must explicitly allow the webview's own origin**
  for scripts/styles/images, following the nonce pattern from the official
  `webview-sample` (not just the guide's simpler `cspSource`-only example).
  **Verify `connect-src` (or `default-src`) also covers `webview.cspSource`**
  — this isn't spelled out in VS Code's docs but is required for Three.js
  loaders' `fetch()` calls to succeed; treat as a concrete test case, not an
  assumption.
- **No direct `fs`/Node access inside the webview at all** — every file read
  (bundled assets, user-picked files/folders, zip contents) happens
  extension-host-side (`vscode.workspace.fs`, `AdmZip`/similar for zip
  extraction) and is relayed via `postMessage`. `File`/`Blob`/`ImageData`
  cannot cross the boundary — only serializable bytes.
- **File/folder import must be re-architected around
  `vscode.window.showOpenDialog`** (`canSelectFolders`/`canSelectMany`/
  `filters`) rather than an `<input type="file">`/File System Access API
  port — those browser APIs are not meaningfully usable the same way inside
  the webview sandbox for this purpose.
- **Binary payload transfer needs two specific correctness fixes, not just
  "send the bytes":** (1) `package.json` must declare `engines.vscode` at
  `^1.57.0` or later; (2) any `workspace.fs.readFile()` result must be
  explicitly rewrapped as `new Uint8Array(...)` before `postMessage`, or it
  silently falls back to a ~10x-slower, non-typed-array serialization path
  (per VS Code contributor @Tyriar's own measurements: ~10s→~500ms,
  ~4s→~400ms swings observed from this exact fix). **No documented maximum
  payload size exists** — budget a spike to empirically measure
  multi-file/large-texture folder imports on the target VS Code version
  before committing to a no-chunking design.
- **`SharedArrayBuffer`-based zero-copy sharing between host and webview is
  confirmed impossible** ("not possible due to our extension architecture,"
  per VS Code maintainer @mjbvz) — don't scope a ticket around it.
- **Native OS drag-and-drop (dragging a file from Finder/Explorer straight
  onto the webview content) is confirmed to work** and can largely carry
  over the existing `DragDropFiles.ts` HTML5 drop-handler logic as-is, as
  long as the viewer is the webview's own top-level document (not nested in
  a second iframe inside it — that nested case is a separate, still-flaky
  scenario). Dragging from VS Code's **Explorer sidebar** into the webview
  was a distinct, separately-fixed (~June 2024) gap — confirm on the actual
  target VS Code version rather than assuming parity with OS-level drop.
- **Consider scoping this as a `CustomEditorProvider` (registered for
  `.glb`/`.gltf`), not just a command-triggered `WebviewPanel`** — Microsoft's
  own Custom Editor guide names "previewing ... 3D models" as a first-class
  use case, and it gets "open this file" UX (double-click a `.glb` in the
  Explorer) for free instead of requiring a separate import flow for the
  primary open-a-model case.
- **Default to `getState`/`setState` for surviving panel-hidden/backgrounded
  state (camera pose, loaded-model reference, lighting preset), not
  `retainContextWhenHidden`.** The latter is explicitly documented as having
  "high memory overhead" and should be a fallback only if reload-from-state
  proves too slow for large models — don't default to it for convenience.
- **Add a `WebviewPanelSerializer` (with an `onWebviewPanel:<viewType>`
  activation event) if the viewer should survive a full VS Code
  restart**, layered on the same `getState`/`setState` JSON blob — this is
  a separate mechanism from `retainContextWhenHidden`/panel-visibility
  persistence and should likely be its own ticket.
