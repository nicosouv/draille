#pragma once

#include <XPowersLib.h>

class PmuManager {
public:
    bool begin();
    bool isOk() const { return _ok; }

private:
    XPowersAXP2101 _pmu;
    bool _ok = false;
};
