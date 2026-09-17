#pragma once

namespace rapio {
enum class Boundary {
  None,
  Wrap,
  Clamp
};

struct BoundNone {
  static inline bool resolve(int& idx, int max){ return (idx >= 0 && idx < max); }
};

struct BoundWrap {
  static inline bool
  resolve(int& idx, int max)
  {
    idx = idx % max;
    if (idx < 0) { idx += max; }
    return true;
  }
};

struct BoundClamp {
  static inline bool
  resolve(int& idx, int max)
  {
    if (idx < 0) { idx = 0; } else if (idx >= max) { idx = max - 1; }
    return true;
  }
};
}
