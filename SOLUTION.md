# Solution (spoilers!)

Turn back now if you want to crack it yourself.

## What the login actually checks

`CheckPassword(username, password)`:

1. The username can't be empty.
2. The password has to be exactly 16 hex digits, shown as `XXXX-XXXX-XXXX-XXXX`
   (dashes and spaces are ignored). Those 16 digits are read back into one
   64-bit number.
3. That number has to equal `DeriveKey(username)`.

There's no stored password and nothing random, so the right password is simply a
function of the username. That's what makes it keygennable.

## The algorithm

`DeriveKey` walks the username one byte at a time:

```
state = 0xC0FFEE1234567890
for each byte c of the username:
    state ^= c
    state *= 0x100000001B3          // FNV prime
    state  = rotl64(state, 11)      // rotate left 11 bits
    state += 0x9E3779B97F4A7C15     // golden-ratio constant
password = state, printed as four 16-bit groups, high group first
```

## Keygen

See `keygen.cpp` - it's the loop above plus a `printf`. Build and run it:

```
cl /nologo /O2 /EHsc keygen.cpp
keygen "Ada Lovelace"
```

and paste the password it prints into the login box.

## Finding it by reversing

- Load `CrackMe.exe` in Ghidra / IDA / x64dbg.
- Follow the two input buffers to the one place they're compared, or set a
  breakpoint on the hex-parsing loop that skips `-`.
- The constants `0x100000001B3` and `0x9E3779B97F4A7C15` are very recognisable,
  and the rotate shows up as a pair of shifts or a `rol`.
- Re-implement the loop in whatever language you like. Done.
