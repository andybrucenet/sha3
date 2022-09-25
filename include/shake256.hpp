#pragma once
#include "sponge.hpp"

// SHAKE256 Extendable Output Function : Keccak[512](M || 1111, d)
namespace shake256 {

// Capacity of sponge, in bits
constexpr size_t capacity = 512;
// Rate of sponge, in bits
constexpr size_t rate = 1600 - capacity;

// SHAKE256 Extendable Output Function
//
// See SHA3 extendable output function definition in section 6.2 of SHA3
// specification https://dx.doi.org/10.6028/NIST.FIPS.202
struct shake256
{
private:
  uint64_t state[25];
  bool absorbed = false;
  size_t readable = 0;

public:
  // Given N -bytes input message, this routine consumes it into keccak[512]
  // sponge state
  //
  // Once you call this function on some object, calling it again doesn't do
  // anything !
  void hash(const uint8_t* const __restrict msg, const size_t mlen)
  {
    if (!absorbed) {
      sponge::absorb<0b00001111, 4, rate>(state, msg, mlen);

      absorbed = true;
      readable = rate >> 3;
    }
  }

  // Given that N -bytes input message is already absorbed into sponge state
  // using `hash( ... )` routine, this routine is used for squeezing M -bytes
  // out of consumable part of state ( i.e. rate portion of state )
  //
  // This routine can be used for squeezing arbitrary number of bytes from
  // sponge keccak[512]
  //
  // Make sure you absorb ( see hash(...) routine ) message bytes first, then
  // only call this function, otherwise it can't squeeze out anything.
  void read(uint8_t* const __restrict dig, const size_t dlen)
  {
    if (!absorbed) {
      return;
    }

    constexpr size_t rbytes = rate >> 3;

    size_t doff = 0;
    while (doff < dlen) {
      const size_t read = std::min(readable, dlen - doff);
      const size_t soff = rbytes - readable;

      if constexpr (std::endian::native == std::endian::little) {
        std::memcpy(dig + doff, reinterpret_cast<uint8_t*>(state) + soff, read);
      } else {
        const size_t till = soff + read;

        for (size_t i = soff; i < till; i++) {
          const size_t woff = i >> 3;
          const size_t boff = (i & 7ul) << 3;

          dig[doff + i] = static_cast<uint8_t>(state[woff] >> boff);
        }
      }

      readable -= read;
      doff += read;

      if (readable == 0) {
        keccak::permute(state);
        readable = rbytes;
      }
    }
  }
};

}
