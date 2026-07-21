# Fast-follow reuse target is a VS Code extension, not a Chrome/Firefox browser extension

`web/CONTEXT.md` and ADR 0004 both describe the Viewer's eventual reuse target as "a browser extension popup," and issue #9's Out of Scope lists "The browser extension (Chrome/Firefox popup)" as a deferred fast-follow. That framing predates this decision and is superseded here — no Chrome/Firefox extension work has started or is currently planned.

**Decision:** the actual fast-follow, scoped in issue #27 and its child tickets, is a **VS Code extension** that embeds the Viewer core in a webview. Entry points are VS Code-native (Explorer single/double-click via a `CustomEditorProvider`, right-click "Open with Renderit" for zip/folder), not a popup UI with its own import controls.

**Consequence:** `web/CONTEXT.md`'s Viewer glossary entry is updated to say "extension host" rather than "browser extension popup." This ADR doesn't retroactively rewrite ADR 0004's own text (it's a historical record of its own moment), but ADR 0004's "browser-extension fast-follow" phrase should be read as referring to this VS Code work. If a genuine Chrome/Firefox popup is ever pursued later, it would need its own spec and would be additional to, not a replacement for, this work.
