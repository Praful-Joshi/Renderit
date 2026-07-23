# VS Code Extension Marketplace publishing (research)

Research date: 2026-07-22. Findings below were verified directly against
`code.visualstudio.com/api` (publishing-extension and extension-manifest
pages), the `microsoft/vscode-vsce` GitHub repo (the `@vscode/vsce` CLI's
source of truth), Microsoft Learn's Azure DevOps docs, and — for the two
areas where official docs lag a live 2026 change (Entra ID auth for CI,
shields.io badge retirement) — the `microsoft/vscode-vsce` issue tracker and
a direct fetch of a live `img.shields.io` badge URL, clearly labeled as such.
Re-verify version-sensitive claims (the Dec 2026 PAT retirement, the
Entra ID/GitHub Actions gap) close to your actual publish date — this area is
mid-migration as of this research.

Purpose: ground a real publish of `vscode-extension/` (package
`renderit-vscode`) to the official Marketplace, plus a professional
README/CHANGELOG rewrite.

Legend: **[HARD PREREQUISITE]** = publishing cannot happen at all without
this. **[REQUIRED]** = documented requirement for `vsce publish` to succeed.
**[RECOMMENDED]** = quality/discoverability polish, not a blocker. **[GOTCHA]**
= not obviously a requirement but will bite you in practice.

---

## 1. Publisher identity: still Microsoft account + Azure DevOps PAT — with a hard deadline attached

**Source:** [Publishing Extensions → "Create a publisher"](https://code.visualstudio.com/api/working-with-extensions/publishing-extension)

- **[HARD PREREQUISITE]** Create a publisher at
  [marketplace.visualstudio.com/manage](https://marketplace.visualstudio.com/manage),
  signed in with a Microsoft account. Two fields, both permanent-ish: **ID**
  ("the unique identifier for your publisher in Marketplace that will be used
  in your extension URLs. ID cannot be changed once created") and **Name**
  ("the unique name of your publisher that will be displayed in Marketplace").
  This `id` is the value that goes in `package.json`'s `publisher` field.
- **[HARD PREREQUISITE]** To actually push (`vsce publish`), you authenticate
  via `vsce login <publisher-id>` using a Personal Access Token minted in
  Azure DevOps (`dev.azure.com`), **not** a GitHub token or the Microsoft
  account password directly.
- **PAT scope — confirmed, still accurate today:** Organization must be set
  to **"All accessible organizations"** (not a specific org), and under
  Scopes you must click **"Show all scopes"** and select **Marketplace →
  Manage**.
- **[GOTCHA] Creating the Azure DevOps organization itself now needs an Azure
  subscription.** Per Microsoft Learn (Azure DevOps docs, page dated
  2026-03-03, updated 2026-05-08):
  > "**Azure subscription** | You need an active Azure subscription to create
  > new organizations. Existing organizations and free tier limits aren't
  > affected."
  (Source: <https://learn.microsoft.com/en-us/azure/devops/organizations/accounts/create-organization?view=azure-devops>)
  A practitioner account of hitting this in mid-2026 (dev.classmethod.jp,
  "When I tried to publish a VS Code extension from CI...", 2026) reports
  free-trial Azure subscriptions being rejected specifically, forcing a
  Pay-As-You-Go subscription — Microsoft's own Q&A/docs corroborate that
  **Free Trial subscriptions currently aren't supported** for this, but a
  personal Microsoft account with **no** Azure subscription attached at all
  (never touched Azure) is the simpler path that still works. If you already
  have an Azure DevOps org from years ago, this doesn't apply — it's only a
  new-organization gate. **If you don't already have an Azure DevOps org,
  budget time for this step; it is not just "sign up and go" anymore.**
- **[HARD DEADLINE — plan around this now, not later]** Both the official
  docs and Azure DevOps's own blog confirm: **global PATs (the "All
  accessible organizations" scope kind, above) are retired on December 1,
  2026.** Quote from the publishing guide: "On December 1, 2026, global
  Personal Access Tokens (PATs) in Azure DevOps are retired. To keep
  publishing extensions, use secure automated publishing with Microsoft
  Entra ID instead of PATs." That's about 4 months from this research date.
  **For a one-off manual `vsce publish` today, a PAT is still fully correct
  and is what the docs currently walk you through** — but don't wire a
  long-lived CI secret around it without a plan to revisit before December.
- The recommended replacement is **Microsoft Entra ID with workload identity
  federation / managed identity**, eliminating long-lived secrets. See §8 for
  why this is *not yet practical* for a personal-account GitHub Actions setup
  specifically (as opposed to an Azure Pipelines + org-owned-identity setup).

---

## 2. The publishing tool: `@vscode/vsce`, confirmed

**Source:** [Publishing Extensions](https://code.visualstudio.com/api/working-with-extensions/publishing-extension);
[`microsoft/vscode-vsce`](https://github.com/microsoft/vscode-vsce) README.

- **[REQUIRED]** Package name today is **`@vscode/vsce`** (the bare `vsce`
  name on npm is the old, deprecated package — don't install that one).
  Install: `npm install -g @vscode/vsce`, or run ad hoc via
  `npx @vscode/vsce --version`.
- Core commands, all confirmed directly in the docs/README:
  - `vsce login <publisher-id>` — stores your PAT (via `keytar`, a
    `VSCE_STORE=file` file, or the `VSCE_PAT` env var — the vsce README
    explicitly notes "Using the `VSCE_PAT` environment variable will also
    avoid using `keytar`", which is the right pattern for CI since `keytar`
    needs `libsecret` on Linux).
  - `vsce package` — builds a local `.vsix` without publishing (what you'd
    hand to someone for manual sideload/testing, or attach to a GitHub
    Release). Confirmed working directly against this repo — see the
    "Renderit-specific" note in §7.
  - `vsce publish` — packages **and** uploads to the Marketplace in one step.
  - `vsce unpublish` — removes a published extension.
- No separate "submit for review" step exists in the tool — see §10.

---

## 3. Required vs. recommended `package.json` fields

**Source:** [Extension Manifest reference](https://code.visualstudio.com/api/references/extension-manifest)

**[REQUIRED] — `vsce publish` hard-fails without these:**
- `publisher` — must exactly match a publisher `id` you created (§1). Must be
  the identifier itself, not a human-friendly display name — confirmed
  empirically: `vsce package` rejected a placeholder value with "Expected the
  identifier of a publisher, not its human-friendly name."
- `name` — "should be all lowercase with no spaces. The name must be unique
  to the Marketplace" (uniqueness is scoped to `publisher.name`, not global).
- `version` — SemVer-compatible.
- `engines.vscode` — "An object containing at least the `vscode` key
  matching the versions of VS Code that the extension is compatible with.
  **Cannot be `*`**." Renderit's manifest sets `"vscode": "^1.57.0"`, which
  satisfies this.
- **[GOTCHA, confirmed empirically against this repo]** `@types/vscode`'s
  declared range must not imply a version newer than `engines.vscode`'s
  floor — `vsce package` hard-failed with "`@types/vscode ^1.125.0` greater
  than `engines.vscode ^1.57.0`. Either upgrade `engines.vscode` or use an
  older `@types/vscode` version" until this was fixed by pinning
  `@types/vscode` to `^1.57.1` (matching the intentional 1.57 floor chosen
  for the `postMessage` `ArrayBuffer` fast-path, rather than raising the
  floor just to match whatever `@types/vscode` happened to be installed).

**Renderit's original `vscode-extension/package.json` was missing `publisher`
entirely and had `"private": true`** — both are hard blocks, not
marketplace-quality nice-to-haves; `vsce` refuses to package a `private:
true` manifest at all.

**[RECOMMENDED] — quality/discoverability, not hard blockers:**
- `displayName`, `description` — already set, good.
- `icon` — "The path to the icon of **at least 128x128 pixels (256x256 for
  Retina screens)**." **PNG only — vsce will not publish extensions
  containing user-provided SVG images** for the icon or README images (the
  docs' "trusted badge providers" security carve-out explicitly calls this
  out). Renderit now ships a 256×256 PNG.
- `categories` — `["Visualization"]` is a real category from the doc's exact
  allowed list: "Programming Languages, Snippets, Linters, Themes,
  Debuggers, Formatters, Keymaps, SCM Providers, Other, Extension Packs,
  Language Packs, Data Science, Machine Learning, Visualization, Notebooks,
  Education, Testing." No change needed.
- `keywords` — array is "currently limited to 30 keywords" per the doc; pure
  SEO, not a requirement.
- `repository` — **[RECOMMENDED but functionally important]**: see §6 — this
  field is what lets `vsce` resolve relative README links/images
  automatically, and needs a `directory` sub-field for a monorepo extension
  (Renderit's lives at `vscode-extension/`, not the repo root).
- `license` — docs: "If you do have a `LICENSE` file in the root of your
  extension, the value for `license` should be `SEE LICENSE IN <filename>`."
  A plain SPDX identifier like `"MIT"` is valid on its own without a
  matching file, but `vsce package` still emits `WARNING LICENSE,
  LICENSE.md, or LICENSE.txt not found` if none is physically present in the
  extension's own root — confirmed empirically. Renderit resolved this by
  keeping the canonical `LICENSE` at the repo root (governing the whole
  monorepo) and placing a copy inside `vscode-extension/` specifically,
  since that's the directory `vsce` actually packages from.
- `galleryBanner` — optional; `color` + `theme` (`"dark"`/`"light"`) tint the
  header behind your icon on the listing page. Cosmetic only.
- `qna` — optional; `"marketplace"` (default), a custom URL string, or
  `false` to disable the Q&A tab.
- `badges` — array of image badges shown in the listing sidebar; see §11 —
  constrained to a **trusted-provider allowlist** (SVGs from anywhere else
  are rejected at publish time): "api.travis-ci.com, badge.fury.io,
  badgen.net, circleci.com, codacy.com, codecov.io, dev.azure.com,
  github.com (from Workflows only), img.shields.io, marketplace.visualstudio.com,
  snyk.io" and a few others, per the extension-manifest doc's
  "approved badges" list.

---

## 4. README conventions

**Source:** [Publishing Extensions → "Marketplace integration"](https://code.visualstudio.com/api/working-with-extensions/publishing-extension);
[Extension Manifest reference](https://code.visualstudio.com/api/references/extension-manifest)

- **[REQUIRED for a listing page at all]** "Add a `README.md` file to the
  root of your extension with the content you want to show on the
  extension's Marketplace page" — the whole file becomes the listing body.
- No official VS Code docs page was found prescribing a specific section
  order ("demo gif → Features → Requirements → Extension Settings → Known
  Issues → Release Notes"). That structure is real and extremely well-known
  among extension authors, but its actual source is the `README.md`
  template scaffolded by the `yo code` / `generator-code` Yeoman generator
  (a first-party Microsoft tool, but a code template, not a prose guidance
  page) — **flag as a strong, widely-followed convention rather than a
  documented requirement.**
- What **is** confirmed from the official guide: keep `README.md`,
  `CHANGELOG.md`, `LICENSE`, and (optionally) `SUPPORT.md` at the extension
  root; `vsce` looks for all of them there.

---

## 5. CHANGELOG.md: surfaced by the Marketplace UI, format is a community convention (not VS Code's own spec)

**Source:** [Publishing Extensions → "Marketplace integration"](https://code.visualstudio.com/api/working-with-extensions/publishing-extension);
[keepachangelog.com](https://keepachangelog.com/en/1.1.0/) (community
convention site, not a Microsoft source — labeled accordingly).

- **[CONFIRMED]** A `CHANGELOG.md` at the extension root is picked up and
  shown in its own tab/section on the Marketplace listing page.
- **Format is not mandated by VS Code.** "Keep a Changelog" (keepachangelog.com)
  is the de facto community standard: entries grouped under a version
  heading with an ISO-8601 date, bucketed into `Added`/`Changed`/
  `Deprecated`/`Removed`/`Fixed`/`Security`, newest-first, with an
  `## [Unreleased]` section at the top for in-flight work. Renderit's
  `vscode-extension/CHANGELOG.md` follows this convention.

---

## 6. Icon/assets and README image links: absolute URLs are the safe default; `repository` is a shortcut, not a requirement

**Source:** [Extension Manifest reference](https://code.visualstudio.com/api/references/extension-manifest);
[Publishing Extensions](https://code.visualstudio.com/api/working-with-extensions/publishing-extension)

- **Icon:** `icon` field, PNG, **minimum 128×128, recommended 256×256 for
  Retina** — no SVG (vsce blocks user-provided SVGs for security reasons,
  confirmed in the publishing guide's security section).
- **README images:** the manifest doc's literal wording is: "You can provide
  relative path image links in the `README.md`." So relative links *are*
  supported — **conditionally**. The mechanism: "If you have a `repository`
  property in `package.json` pointing to a public GitHub repo, `vsce` will
  automatically detect it and adjust relative links accordingly, using the
  `main` branch by default" — i.e. `vsce` rewrites `./docs/demo.gif` into a
  `raw.githubusercontent.com` URL at package time, using the `repository`
  field to know which repo/branch to point at. **Without a `repository`
  field, skip relative links and use absolute HTTPS URLs directly** — the
  Marketplace renders the README standalone, outside any git context, so a
  relative link with no `repository` field to resolve against will just
  404 on the listing page even though it renders fine in VS Code's own
  README preview. Renderit added `repository` (with a `directory` sub-field
  pointing at `vscode-extension`, since the extension lives in a
  subdirectory of the monorepo, not the repo root) specifically to make this
  safe.
- **Badges:** SVG badges are allowed only from the trusted-provider allowlist
  (§3/§11) — this is specifically to prevent hosting arbitrary/malicious SVG
  content on Microsoft's CDN under someone else's extension listing.

---

## 7. `.vscodeignore`

**Source:** [Bundling Extensions](https://code.visualstudio.com/api/working-with-extensions/bundling-extension);
verified empirically by running `vsce package` against this repo and
inspecting the resulting `.vsix` contents.

- Purpose: gitignore-syntax exclude list applied when `vsce package`/
  `publish` builds the `.vsix` — controls what actually ships to users,
  independent of what's tracked in git.
- For a bundled (webpack/esbuild/**Vite**, Renderit's case) extension, the
  docs' rationale is direct: "Everything that's now bundled into the
  `dist/extension.js` file can be excluded, usually the `out` folder (in
  case you didn't delete it yet) and most importantly, the `node_modules`
  folder." **This generic advice assumes a single-bundle setup and doesn't
  apply as-is to Renderit** — verify which output directories are actually
  load-bearing before excluding either.
- **Renderit-specific pipeline, confirmed by reading `package.json`'s
  scripts and `main` field:** `compile` emits the **extension-host** entry
  point to `out/` (what `package.json`'s `"main": "./out/src/extension.js"`
  actually loads at runtime), while `build:webview` emits the
  **Vite-bundled webview** to `dist/`, and `copy-assets` copies the bundled
  HDRI environment maps into `dist/environments/`. **Both `out/` and
  `dist/` are real runtime dependencies and must ship** — only the `.ts`
  sources those are compiled/bundled from get excluded.
- **[GOTCHA, confirmed empirically]** A bare `*.map` pattern in
  `.vscodeignore` did **not** exclude `.js.map` files from the packaged
  `.vsix` — unlike plain `.gitignore` semantics (where a pattern with no
  slash matches at any depth), `vsce`'s ignore-file globbing needed the
  explicit `**/*.map` form to actually take effect. Verified by packaging
  before and after the fix and diffing the file listing `vsce package`
  prints.
- **[GOTCHA, confirmed empirically]** `tsconfig.json`'s `"include": ["src"]`
  sweeps up `src/*.test.ts` alongside the real extension-host code (an
  intentional, low-cost tradeoff made when the test file was first added —
  see the commit history), so the *compiled* `out/src/*.test.js` (plus its
  sourcemap) ends up in the package unless explicitly excluded too — the
  `.ts`-source exclusion alone isn't enough, since it's the *compiled
  output* of that file, not the source, that lands in `out/`. Fixed with an
  explicit `**/*.test.js` rule.
- Negation escape hatch confirmed: `!node_modules/mySpecialModule` to
  re-include a specific runtime dependency that can't be bundled — not
  needed here, since Renderit's extension-host code (`src/*.ts`) imports
  only `vscode` (provided by the runtime) and its own local modules; the
  webview's `three`/`fflate` dependencies are already inlined into
  `dist/main.js` by Vite. Confirmed by grepping every `import` statement
  under `src/`.
- **End-to-end verification:** running `npx @vscode/vsce package` against
  this repo (with a temporary, validly-formatted dummy publisher id, since
  the real `package.json` intentionally ships a placeholder) produced a
  clean 1.76 MB `.vsix` containing exactly `LICENSE.txt`, `changelog.md`,
  `icon.png`, `package.json`, `dist/` (bundle + CSS + both HDRIs), and
  `out/` (5 clean `.js` files, no maps, no test files) — no warnings.

---

## 8. Versioning/publishing workflow and CI automation

**Source:** [Publishing Extensions → "Auto-increment the extension version" / pre-release / Entra ID sections](https://code.visualstudio.com/api/working-with-extensions/publishing-extension);
`microsoft/vscode-vsce` issues [#976](https://github.com/microsoft/vscode-vsce/issues/976)
and [#1023](https://github.com/microsoft/vscode-vsce/issues/1023) (primary —
the tool's own issue tracker); secondary/practitioner sources labeled below.

- **[CONFIRMED] Version bump handling:** `vsce publish patch|minor|major`
  bumps `package.json`'s `version` (e.g. `vsce publish minor` takes
  `1.0.0`→`1.1.0`) and publishes in one step; `vsce publish 1.2.3` sets an
  explicit version. In a git repo this also creates the matching commit +
  tag via the same mechanism as `npm version`.
- **[NOT FOUND] an official Microsoft-authored GitHub Action.** Neither the
  publishing guide nor the `vscode-vsce` repo names one. The de facto
  standard third-party action remains **`HaaLeo/publish-vscode-extension`**
  (GitHub Marketplace listing confirmed, `@v2` is current major version,
  last tagged release Feb 23 2025 per its GitHub Releases page — still the
  reference implementation people use, just not an "official Microsoft"
  action). Basic pattern (from its own README):
  ```yaml
  on:
    push:
      tags: ["*"]
  jobs:
    deploy:
      runs-on: ubuntu-latest
      steps:
        - uses: actions/checkout@v4
        - uses: actions/setup-node@v4
        - run: npm ci
        - uses: HaaLeo/publish-vscode-extension@v2
          with:
            pat: ${{ secrets.VSCE_PAT }}
            registryUrl: https://marketplace.visualstudio.com
  ```
  This is PAT-based, which is fine for now (§1) but will need revisiting
  before December 2026.
- **[GOTCHA, primary-sourced from the vsce issue tracker itself]** The
  documented Entra ID replacement (`vsce publish --azure-credential`,
  requires `@vscode/vsce` ≥ v2.26.1 per the official guide) is **not
  currently workable for a GitHub-Actions-plus-personally-owned-publisher**
  setup like Renderit's. Two open/closed issues on `microsoft/vscode-vsce`
  document this directly: [#976](https://github.com/microsoft/vscode-vsce/issues/976)
  ("The requested operation is not allowed" when using `--azure-credential`)
  and [#1023](https://github.com/microsoft/vscode-vsce/issues/1023) (using
  `--azure-credential` with a federated service principal raises "You need
  to be logged in with your corporate credentials to perform this action");
  a practitioner writeup (dev.classmethod.jp, 2026, secondary source, cited
  only for the practical framing) explains the root cause as an identity
  mismatch — a workload-identity-federated Entra token belongs to an *Azure
  tenant*, but Renderit's publisher is (presumably) a personal Microsoft
  account with no tenant behind it, and the official Entra ID walkthrough
  assumes an Azure-Pipelines-plus-managed-identity setup, not GitHub Actions.
  **Recommendation for Renderit specifically: use a PAT (§1) for now,
  store it as a GitHub Actions secret (`VSCE_PAT`), and re-evaluate before
  the December 2026 deadline** rather than spending setup time on Entra ID
  today.

---

## 9. Pre-release versions

**Source:** [Publishing Extensions → "Prerelease Extensions"](https://code.visualstudio.com/api/working-with-extensions/publishing-extension)

- **[CONFIRMED]** The Marketplace supports a pre-release channel:
  `vsce publish --pre-release` (also works with `vsce package
  --pre-release` for local testing). Users opt in via a **"Switch to
  Pre-Release Version"** control on the extension's page/Extensions view in
  VS Code (or automatically if they're on VS Code Insiders, depending on
  settings) to get "the latest extension version before the official
  extension release."
- **[CONFIRMED constraint]** "We only support `major.minor.patch` for
  extension versions; `semver` pre-release tags are **not supported**" —
  i.e. you cannot publish `1.2.0-beta.1`; pre-release-ness is a flag on the
  *publish action*, not encoded in the version string itself, and a
  pre-release version number must differ from any regular-release version
  number you've used.
- **Recommended convention** straight from the docs: use an **even** middle
  (minor) version number for release builds and an **odd** one for
  pre-release builds — e.g. `0.2.x` = stable, `0.3.x` = pre-release — so the
  two channels never collide on a version number.
- Not relevant to Renderit's *first* publish (nothing to pre-release yet),
  but worth adopting once there's an ongoing release cadence.

---

## 10. Review/approval process: no manual gate — `vsce publish` is live immediately, subject to after-the-fact policy enforcement

**Source:** [Publishing Extensions](https://code.visualstudio.com/api/working-with-extensions/publishing-extension);
Microsoft's Marketplace legal docs (Visual Studio Marketplace Publisher
Agreement / Marketplace Publication Guidelines, linked from
[learn.microsoft.com/en-us/legal/marketplace/](https://learn.microsoft.com/en-us/legal/marketplace/)).

- **[CONFIRMED]** There is **no Chrome-Web-Store-style manual pre-publish
  review queue.** `vsce publish` uploads and the extension becomes publicly
  installable essentially immediately — no waiting period is documented
  anywhere in the publishing guide.
- What exists instead is **after-the-fact policy enforcement**: the
  Marketplace Publisher Agreement and Marketplace Publication Guidelines
  require offerings to "extend Microsoft Visual Studio and related
  products," accurate listing metadata, and compliance with technical
  onboarding rules; Microsoft "reserves the right to suspend or remove
  offerings that do not comply with policies" post-hoc, rather than gating
  publication up front.
- **Verified Publisher badge is optional and separate from publishing
  itself** — it does not block or gate `vsce publish` in any way. Per the
  publishing guide: eligibility requires "one or more extensions on the VS
  Marketplace for a minimum of 6 months, and the registration of the domain
  must also be at least 6 months old," verified via a DNS TXT record; once
  submitted, "the Marketplace team will review your request and let you
  know the result within 5 business days." **Not relevant to a first
  publish** — Renderit won't qualify for 6 months regardless, and doesn't
  need to for the extension to go live.

---

## 11. README badges

**Source:** [shields.io Visual Studio Marketplace badge pages](https://shields.io/badges/visual-studio-marketplace-version);
direct fetch of a live badge URL; `badges/shields` GitHub issue
[#11796](https://github.com/badges/shields/issues/11796).

- **[IMPORTANT CURRENT-STATE FINDING — do not copy old README examples
  blindly]** shields.io's entire `visual-studio-marketplace/*` badge family
  (version, installs, rating, downloads, last-updated) **is retired as of
  ~April 2026**, not merely flaky. A live badge URL was fetched directly —
  `https://img.shields.io/visual-studio-marketplace/v/ms-python.python` (a
  real, actively-published, extremely popular extension) — and it rendered
  literally as a **"visual-studio-marketplace: retired badge"** SVG instead
  of a version number. `badges/shields` issue #11796 (opened April 11 2026,
  closed without a working fix) and the shields.io project's own retirement
  PR confirm the reason: **there is no publicly documented Marketplace API
  for third-party metadata**, and undocumented-endpoint scraping at
  shields.io's scale caused rate-limiting/instability, so the maintainers
  pulled the whole badge family rather than keep patching it. A request for
  an official, documented API (`microsoft/vsmarketplace` issue #1776) is
  open but unresolved as of this research.
  - **Practical implication for Renderit's README:** don't add
    `img.shields.io/visual-studio-marketplace/...` badges expecting them to
    render live install/rating counts — they currently show as "retired
    badge," which looks broken/unprofessional, i.e. worse than no badge at
    all. `vsmarketplacebadges.dev` is a thin wrapper around the same
    shields.io backend, so it inherits the same breakage.
  - Alternatives that still work today and were used in Renderit's README:
    a CI-status badge (GitHub Actions workflow badge, `github.com`-hosted,
    which **is** on the vsce trusted-badge allowlist per §3/§6) and a
    license badge from `img.shields.io/github/license/...` (a *GitHub*-data
    badge, not a Marketplace-data one, so unaffected by this retirement).

---

## Concrete checklist for Renderit's first publish

**[HARD PREREQUISITES — nothing else works until these are done]**
1. Create the Marketplace publisher (§1) — pick and lock in a publisher
   `id` now, since it's permanent and goes straight into every install URL.
2. Get (or reuse) an Azure DevOps organization + PAT with "All accessible
   organizations" + Marketplace→Manage scope (§1) — budget extra time if you
   don't already have an Azure DevOps org, per the subscription gotcha.
3. Replace `"REPLACE_WITH_YOUR_MARKETPLACE_PUBLISHER_ID"` in
   `vscode-extension/package.json`'s `publisher` field with your real
   publisher id.

**[DONE — verified in this repo already]**
4. ~~Add an `icon` field + a real 128×128 (256×256 recommended) PNG icon~~ —
   done (`vscode-extension/icon.png`, 256×256).
5. ~~Add a `repository` field~~ — done, with a `directory` sub-field since
   the extension lives in `vscode-extension/`, not the repo root.
6. ~~Write/rewrite `README.md` and add a `CHANGELOG.md`~~ — done.
7. ~~Add/trim a `.vscodeignore` tailored to this project's actual
   `compile`→`out/`, `build:webview`→`dist/`, `copy-assets` pipeline~~ —
   done and verified end-to-end via a real `vsce package` run (§7).
8. ~~Add `keywords`, `license` (+ a `LICENSE` file)~~ — done (MIT, with a
   copy inside `vscode-extension/` specifically for `vsce`'s benefit).

**[RECOMMENDED polish, not blockers, still open]**
9. Set up `HaaLeo/publish-vscode-extension@v2` in GitHub Actions with a
   `VSCE_PAT` secret for tag-triggered publishes (§8) — plan to revisit auth
   before December 2026, don't attempt Entra ID/`--azure-credential` today.
   Not wired up yet — worth doing only after the first manual publish.
10. Skip shields.io Marketplace badges (currently rendering as "retired
    badge" — see §11); a CI-status and GitHub-license badge are used in the
    README instead.
11. Not relevant yet: pre-release channel (§9) and Verified Publisher badge
    (§10) — both are for an established extension with release history, not
    a first publish.
