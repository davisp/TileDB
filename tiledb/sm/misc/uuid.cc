/**
 * @file   uuid.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2018-2023 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file defines a platform-independent UUID generator.
 */

#include <mutex>

#include "tiledb/common/exception/exception.h"
#include "tiledb/sm/misc/tdb_time.h"
#include "tiledb/sm/misc/uuid.h"

#ifdef _WIN32
#include <Rpc.h>
#else
#include <openssl/err.h>
#include <openssl/rand.h>
#include <cstdio>
#endif

using namespace tiledb::common;

namespace tiledb::sm::uuid {

class UUIDException : public StatusException {
 public:
  explicit UUIDException(const std::string& message)
      : StatusException("UUID", message) {
  }
};

using BinaryUUID = std::array<uint8_t, 16>;

void fill_random_bytes(BinaryUUID& buf);

class UUIDGenerator {
 public:
  DISABLE_COPY_AND_COPY_ASSIGN(UUIDGenerator);
  DISABLE_MOVE_AND_MOVE_ASSIGN(UUIDGenerator);

  static std::string generate_uuid() {
    static UUIDGenerator generator;
    return generator.generate();
  }

 protected:
  UUIDGenerator()
      : prev_gen_time_(tiledb::sm::utils::time::timestamp_now_ms()) {
    fill_random_bytes(prev_uuid_);
  }

  std::string generate() {
    BinaryUUID uuid;
    get_uuid_bytes(uuid);

    // Convert our generated UUID into a string without hyphens because
    // hyphenated UUID are a waste of time and money. It costs money to store
    // and transmit those hyphens.

    // Start with a nulled string.
    char ret[33];
    memset(ret, 0, 33);

    // For each byte, set the appropriate hexadecimal digit.
    char hex_lut[17] = "0123456789abcdef";
    for (size_t bin_idx = 0; bin_idx < 16; bin_idx++) {
      size_t hex_idx = bin_idx * 2;

      // For the non-bit twiddlers out there, this might look a bit intimidating
      // but its rather simple. Converting binary to hexadecimal just means
      // we take the top four bits of a byte and use that as an index into the
      // hex lookup table defined a few lines above. Then again with the lower
      // 4 bits.
      ret[hex_idx] = hex_lut[(uuid[bin_idx] >> 4) & 0x0F];
      ret[hex_idx + 1] = hex_lut[uuid[bin_idx] & 0x0F];
    }

    return std::string(ret);
  }

  void get_uuid_bytes(BinaryUUID& data) {
    std::lock_guard<std::mutex> lg(mtx_);

    auto now = utils::time::timestamp_now_ms();
    // The use of `!=` here is subtly important. If we were to use `>` instead
    // it would lead to a broken generation algorithm any time the host
    // machine's clock is rewound.
    if (now != prev_gen_time_) {
      // The easy case. We haven't generated a UUID in this millisecond so we
      // can just generate a new one and be done.
      prev_gen_time_ = now;
      fill_random_bytes(prev_uuid_);

      // Normally, I wouldn't bother removing entropy from the generated UUID,
      // but just so folks don't think I'm crazy, I'll remove the 6 bits of
      // entropy so we can call this an "official" UUIDv4 algorithm.

      // Set the top four bits of byte 6 to 0x4 for the version indicator.
      prev_uuid_[6] = 0x40 | (0x0F & prev_uuid_[6]);

      // Set the top two bits of byte 8 to 01
      prev_uuid_[8] = 0x40 | (0x3F & prev_uuid_[8]);

      // Set the 0th bit of the return UUIDs to 0 so that our counter logic
      // below works. Yes, this technically removing some entropy and if anyone
      // ever pays attention they'll see that we rarely (but not never) return
      // a UUID that starts with a hex digit greater than 7.
      prev_uuid_[0] = 0x7F & prev_uuid_[0];

      data = prev_uuid_;
      return;
    }

    // Now the interesting part. The goal here is ensure that all UUIDs
    // generated in the same milliseconds are ordered by time. You'll notice
    // that we're not actually trying to insert time into the UUID here as
    // that would reduce entropy. Instead, we'll just accept that a nefarious
    // attacker *might* be able to deduce that two UUIDs were generated in the
    // same millisecond. Yeah, that seems silly, but some folks care about
    // things like that.

    // The way we make sure that UUIDs generated in the same millisecond are
    // ordered by time is to treat the top four bytes as a counter. Given that
    // we always set to 0th bit to 0, this gives us a space of *at least* 2^31
    // possible values to fill. So, this is basically safe until we can
    // generate over 2 billion UUIDs in a millisecond. For reference, if my
    // math is correct, that's when we get 2 *terahertz* processors that can
    // generate a UUID in a single instruction. So, no time soon.

    // Step one, add 1 to the four byte counter. Standard overflow math here.
    // if the current val at the current index is 255, set to 0, and add 1 to
    // the next index.
    size_t idx = 3;
    while (idx >= 0) {
      if (prev_uuid_[idx] < 255) {
        prev_uuid_[idx] += 1;
        break;
      } else {
        if (idx == 0) {
          // We've managed it! We finally did it, we created 2 billion UUIDs in
          // a single millisecond. Or we have a terrible bug. One of the two.
          throw UUIDException(
              "Error generating UUID: Maximum generation frequency exceeded.");
        }
        prev_uuid_[idx] = 0;
      }
      idx--;
    }

    // The last step in generating our UUIDs is to randomize the trailing
    // 12 bytes. We'll be a bit wasteful here and generate 16 bytes of data
    // and only copy the 12 we need.
    BinaryUUID new_bytes;
    fill_random_bytes(new_bytes);

    for (size_t i = 4; i < 16; i++) {
      prev_uuid_[i] = new_bytes[i];
    }

    // Copy the newly generated UUID to the client buffer and we're done.
    data = prev_uuid_;
  }

 private:
  static UUIDGenerator generator_;

  /** UUID generation is not thread safe. */
  std::mutex mtx_;

  /** The last UUID generated. */
  BinaryUUID prev_uuid_;

  /** The time in milliseconds of the last UUID creation. */
  uint64_t prev_gen_time_;
};

std::string generate_uuid() {
  return UUIDGenerator::generate_uuid();
}

#ifdef _WIN32

/**
 * Fill a buffer with random bytes using UuidCreate.
 */
void fill_random_bytes(BinaryUUID& uuid) {
  UUID uuid_bytes;
  RPC_STATUS rc = UuidCreate(&uuid);
  if (rc != RPC_S_OK) {
    return Status_UtilsError("Unable to generate Win32 UUID: creation error");
  }

  // Copy the bytes over manually so we don't have to fight with compiler
  // struct packing semantics.
  uuid[0] = static_cast<uint8_t>(uuid_bytes.Data1 >> 24 & 0xFF);
  uuid[1] = static_cast<uint8_t>(uuid_bytes.Data1 >> 16 & 0xFF);
  uuid[2] = static_cast<uint8_t>(uuid_bytes.Data1 >> 8 & 0xFF);
  uuid[3] = static_cast<uint8_t>(uuid_bytes.Data1 & 0xFF);
  uuid[4] = static_cast<uint8_t>(uuid_bytes.Data2 >> 8 & 0xFF);
  uuid[5] = static_cast<uint8_t>(uuid_bytes.Data2 & 0xFF);
  uuid[6] = static_cast<uint8_t>(uuid_bytes.Data3 >> 8 & 0xFF);
  uuid[7] = static_cast<uint8_t>(uuid_bytes.Data3 & 0xFF);
  for (size_t i = 0; i < 8; i++) {
    uuid[8 + i] = static_cast<uint8_t>(uuid_bytes.Data4[i]);
  }
}

#else

/**
 * Fill a buffer with random bytes using OpenSSL RAND_bytes.
 */
void fill_random_bytes(BinaryUUID& uuid) {
  if (RAND_bytes(uuid.data(), uuid.size()) != 1) {
    char err_msg[256];
    ERR_error_string_n(ERR_get_error(), err_msg, sizeof(err_msg));
    throw UUIDException("Error generating UUID: " + std::string(err_msg));
  }
}

#endif

}  // namespace tiledb::sm::uuid
