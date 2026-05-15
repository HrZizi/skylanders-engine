# User-supplied Cemu keys

Place your own legally obtained Wii U `keys.txt` in this folder:

```text
keys/keys.txt
```

Skylanders Engine validates that the file contains at least one 128-bit hex key
and copies it to the configured Cemu directory before launching a game.

Do not commit `keys.txt`. It contains user-supplied decryption material and is
ignored by git.
