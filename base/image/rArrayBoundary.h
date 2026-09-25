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

#define RAPIO_DISPATCH_BOUNDARIES(BX, BY, FUNC, ...) \
  if (BX == rapio::Boundary::Wrap) { \
    if (BY == rapio::Boundary::Wrap) FUNC<rapio::BoundWrap, rapio::BoundWrap>(__VA_ARGS__); \
    else if (BY == rapio::Boundary::Clamp) FUNC<rapio::BoundWrap, rapio::BoundClamp>(__VA_ARGS__); \
    else FUNC<rapio::BoundWrap, rapio::BoundNone>(__VA_ARGS__); \
  } else if (BX == rapio::Boundary::Clamp) { \
    if (BY == rapio::Boundary::Wrap) FUNC<rapio::BoundClamp, rapio::BoundWrap>(__VA_ARGS__); \
    else if (BY == rapio::Boundary::Clamp) FUNC<rapio::BoundClamp, rapio::BoundClamp>(__VA_ARGS__); \
    else FUNC<rapio::BoundClamp, rapio::BoundNone>(__VA_ARGS__); \
  } else { \
    if (BY == rapio::Boundary::Wrap) FUNC<rapio::BoundNone, rapio::BoundWrap>(__VA_ARGS__); \
    else if (BY == rapio::Boundary::Clamp) FUNC<rapio::BoundNone, rapio::BoundClamp>(__VA_ARGS__); \
    else FUNC<rapio::BoundNone, rapio::BoundNone>(__VA_ARGS__); \
  }
}
