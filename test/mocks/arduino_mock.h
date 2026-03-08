#pragma once
// Minimal Arduino stubs for native (desktop) compilation

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>

// ─── Arduino types ────────────────────────────────────────────────────────────
using byte = uint8_t;

// ─── Arduino functions ────────────────────────────────────────────────────────
inline uint32_t millis() { return 0; }
inline void     delay(uint32_t) {}

// ─── Serial stub ─────────────────────────────────────────────────────────────
struct FakeSerial {
    void begin(int) {}
    template<typename... A> void println(A...) {}
    template<typename... A> void printf(const char* fmt, A... args) { ::printf(fmt, args...); }
    bool available() { return false; }
    int  read()      { return -1; }
};
inline FakeSerial Serial;
