# easy-crackme-x64

A tiny, beginner-friendly **crackme** for 64-bit Windows: a little login window
built with [Dear ImGui](https://github.com/ocornut/imgui) on top of DirectX 11.

It asks for a username and a password. There's no server and no stored password
database behind it - the "correct" password is computed from the username right
there in the code. Your job is to figure out how, and then write a keygen.

![it's just a login box](https://img.shields.io/badge/difficulty-easy-brightgreen)

## How to play

1. Grab `CrackMe.exe` from the [Releases](../../releases) page (or build it, see
   below) and run it.
2. Type any username and guess a password. You'll get **Access denied.**
3. Open `CrackMe.exe` in a disassembler or debugger (Ghidra, IDA, x64dbg, ...),
   find how the password is checked, and work out a valid one.
4. Bonus round: write a keygen that produces a valid password for any username.

Nothing is obfuscated and there's no anti-debugging - this one is meant to be a
friendly first crackme.

## Spoilers

- `SOLUTION.md` explains the algorithm step by step.
- `keygen.cpp` is a working reference keygen. `keygen "Ada Lovelace"` prints a
  password you can paste straight into the login box.

Don't peek until you've given it a go.

## Building it yourself

You'll need Visual Studio 2022 with the C++ toolchain (for `cl`). Then from a
clone of this repo:

```bat
build.bat
```

That compiles `CrackMe.exe` and `keygen.exe`. Dear ImGui is vendored under
`imgui/`, so there's nothing else to download.

## Layout

```
crackme.cpp     the login window + the password check (start reading here)
keygen.cpp      reference keygen / the answer
SOLUTION.md     full walkthrough (spoilers)
build.bat       builds both exes with MSVC
imgui/          vendored Dear ImGui (see imgui/LICENSE.txt)
winapi-demo/    unrelated little ImGui + WinAPI playground (not part of the crackme)
```

## License

The crackme code is MIT licensed - see [LICENSE](LICENSE). Dear ImGui keeps its
own MIT license in `imgui/LICENSE.txt`.
