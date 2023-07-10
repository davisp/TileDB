/**
 * @file unit_as_built.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2023 TileDB, Inc.
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
 * Tests the tiledb::as_built namespace.
 */

#include "tiledb/as_built/as_built.h"

#include <tdb_catch.h>
#include <iostream>

#include <string_view>

template <typename T>
constexpr auto type_name() {
  std::string_view name, prefix, suffix;
#ifdef __clang__
  name = __PRETTY_FUNCTION__;
  prefix = "auto type_name() [T = ";
  suffix = "]";
#elif defined(__GNUC__)
  name = __PRETTY_FUNCTION__;
  prefix = "constexpr auto type_name() [with T = ";
  suffix = "]";
#elif defined(_MSC_VER)
  name = __FUNCSIG__;
  prefix = "auto __cdecl type_name<";
  suffix = ">(void)";
#endif
  name.remove_prefix(prefix.size());
  name.remove_suffix(suffix.size());
  return name;
}

namespace as_built = tiledb::as_built;

const std::string dump_str() noexcept {
  try {
    return as_built::dump();
  } catch (...) {
    return "";
  }
}
static const std::string dump_str_{dump_str()};

std::optional<json> dump_json(std::string dump_str) noexcept {
  try {
    return json::parse(dump_str);
  } catch (...) {
    return std::nullopt;
  }
}
static const std::optional<json> dump_{dump_json(dump_str())};

TEST_CASE("show json version", "[as_built][version]") {
  std::cerr << "JSON VERSION: " << NLOHMANN_JSON_VERSION_MAJOR << "."
            << NLOHMANN_JSON_VERSION_MINOR << "." << NLOHMANN_JSON_VERSION_PATCH
            << std::endl;
}

TEST_CASE("as_built: Ensure dump() does not throw", "[as_built][dump]") {
  std::cerr << dump_str_ << std::endl;
  std::string x;
  CHECK_NOTHROW(x = as_built::dump());
  CHECK(x.compare(dump_str_) == 0);
}

TEST_CASE("as_built: Ensure dump is non-empty", "[as_built][dump][non-empty]") {
  REQUIRE(!dump_str_.empty());
}

TEST_CASE("as_built: Print dump", "[as_built][dump][.print_json]") {
  std::cerr << dump_str_ << std::endl;
}

/** Allow Ubuntu-only failure on all of the following tests; fix in progress. */
TEST_CASE("as_built: Ensure dump has json output", "[as_built][dump][json]") {
  json x;
  CHECK_NOTHROW(x = json::parse(dump_str_));
  CHECK(!x.is_null());
  CHECK(dump_ != std::nullopt);
  CHECK(x == dump_);
}

TEST_CASE("as_built: Validate top-level key", "[as_built][top-level]") {
  auto x{dump_.value()["as_built"]};
  std::cerr << "XKCD1: " << dump_.value()["as_built"].dump(2) << std::endl;
  std::cerr << "XKCD2: " << x.dump(2) << std::endl;
  std::cerr << type_name<decltype(x)>() << std::endl;
  CHECK(x.type() == nlohmann::detail::value_t::object);
  CHECK(!x.empty());
}

TEST_CASE("as_built: Validate parameters key", "[as_built][parameters]") {
  auto x{dump_.value()["as_built"]["parameters"]};
  CHECK(x.type() == nlohmann::detail::value_t::object);
  CHECK(!x.empty());
}

TEST_CASE(
    "as_built: Validate storage_backends key", "[as_built][storage_backends]") {
  auto x{dump_.value()["as_built"]["parameters"]["storage_backends"]};
  CHECK(x.type() == nlohmann::detail::value_t::object);
  CHECK(!x.empty());
}

TEST_CASE(
    "as_built: storage_backends attributes", "[as_built][storage_backends]") {
  auto x{dump_.value()["as_built"]["parameters"]["storage_backends"]};
  CHECK(!x.empty());

#ifdef TILEDB_AZURE
  CHECK(x["azure"]["enabled"] == true);
#else
  CHECK(x["azure"]["enabled"] == false);
#endif  // TILEDB_AZURE

#ifdef TILEDB_GCS
  CHECK(x["gcs"]["enabled"] == true);
#else
  CHECK(x["gcs"]["enabled"] == false);
#endif  // TILEDB_GCS

#ifdef TILEDB_S3
  CHECK(x["s3"]["enabled"] == true);
#else
  CHECK(x["s3"]["enabled"] == false);
#endif  // TILEDB_S3
}

TEST_CASE("as_built: Validate support key", "[as_built][support]") {
  auto x{dump_.value()["as_built"]["parameters"]["support"]};
  CHECK(x.type() == nlohmann::detail::value_t::object);
  CHECK(!x.empty());
}

TEST_CASE("as_built: support attributes", "[as_built][support]") {
  auto x{dump_.value()["as_built"]["parameters"]["support"]};
  CHECK(!x.empty());

#ifdef TILEDB_SERIALIZATION
  CHECK(x["serialization"]["enabled"] == true);
#else
  CHECK(x["serialization"]["enabled"] == false);
#endif  // TILEDB_SERIALIZATION
}
