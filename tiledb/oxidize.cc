#include <string.h>

#include "tiledb/api/c_api_support/c_api_support.h"
#include "tiledb/oxidize.h"

#define BUFFER_SIZE 1024

// C Definitions that are provided by Rust
extern "C" {
uint32_t tiledb_rs_get_mime(
    void* data, uint64_t size, char* buffer, uint64_t buffer_len);
uint32_t tiledb_rs_get_mime_encoding(
    void* data, uint64_t size, char* buffer, uint64_t buffer_len);
uint64_t tiledb_rs_timestamp_now_ms();
}

namespace tiledb::rs {

std::string get_mime(void* data, uint64_t size) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  if (tiledb_rs_get_mime(data, size, buffer, BUFFER_SIZE) != 0) {
    throw api::CAPIStatusException(std::string("Error getting mime type."));
  }
  // Defensively ensure the buffer is zero terminated.
  buffer[BUFFER_SIZE - 1] = 0;
  return std::string{buffer, strlen(static_cast<char*>(buffer))};
}

std::string get_mime_encoding(void* data, uint64_t size) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  if (tiledb_rs_get_mime_encoding(data, size, buffer, BUFFER_SIZE) != 0) {
    throw api::CAPIStatusException(std::string("Error getting mime encoding."));
  }
  // Defensively ensure the buffer is zero terminated.
  buffer[BUFFER_SIZE - 1] = 0;
  return std::string(buffer, strlen(buffer));
}

uint64_t timestamp_now_ms() {
  return tiledb_rs_timestamp_now_ms();
}

}  // namespace tiledb::rs
