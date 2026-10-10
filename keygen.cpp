// keygen.cpp - the answer key for easy-crackme-x64.
//
// This is just DeriveKey() from the crackme, lifted out on its own, plus a bit
// of printf to format the 64-bit result the way the login box wants to see it.
// That's the whole trick: the password is a pure function of the username, so
// once you know the function you can mint a password for anybody.
//
// Build: cl /nologo /O2 /EHsc keygen.cpp
// Usage: keygen "Your Name"

#include <cstdint>
#include <cstdio>

static uint64_t rotl64(uint64_t v, int s) { return (v << s) | (v >> (64 - s)); }

static uint64_t DeriveKey(const char* username)
{
    uint64_t state = 0xC0FFEE1234567890ULL;
    for (const char* p = username; *p; ++p)
    {
        state ^= (uint8_t)(*p);
        state *= 0x100000001B3ULL;
        state = rotl64(state, 11);
        state += 0x9E3779B97F4A7C15ULL;
    }
    return state;
}

int main(int argc, char** argv)
{
    // No name on the command line? Fall back to something so the demo still runs.
    const char* username = (argc > 1) ? argv[1] : "guest";

    uint64_t k = DeriveKey(username);

    // Chop the 64-bit value into four 16-bit groups, high group first, and print
    // them as upper-case hex - exactly the shape the login box parses back.
    printf("Username: %s\n", username);
    printf("Password: %04llX-%04llX-%04llX-%04llX\n",
           (unsigned long long)((k >> 48) & 0xFFFF),
           (unsigned long long)((k >> 32) & 0xFFFF),
           (unsigned long long)((k >> 16) & 0xFFFF),
           (unsigned long long)( k        & 0xFFFF));
    return 0;
}
