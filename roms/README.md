# User-supplied Wii U game files

Put your legally dumped Skylanders Wii U game files here if you want the
launcher stubs to find them automatically.

Supported layouts and file types:

- `.wud`
- `.wux`
- `.wua`
- `.iso`
- extracted title folders containing `code/`, `content/`, and `meta/`
- `code/*.rpx` inside an extracted title folder

Only this README is tracked by git. Game files placed in this folder are
ignored intentionally and should not be committed.

If a matching game file is not found here, Skylanders Engine asks for a game
file on first launch and remembers that external path under `runtime/config/`.
