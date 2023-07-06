/**
 * @file type_casts.h
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
 * This file contains safe type casting utilities.
 */

#ifndef TILEDB_TYPE_CASTS_H
#define TILEDB_TYPE_CASTS_H

#include <limits>
#include <stdexcept>

#include "tiledb/common/unreachable.h"
#include "tiledb/sm/enums/datatype.h"
#include "tiledb/sm/misc/types.h"

namespace tiledb::sm::utils {

/**
 * Safely convert integral values between different bit widths checking for
 * invalid conversions. The basic checks are just to make sure that the
 * conversion is roundtrip-able without an intermediate change in sign.
 *
 * This would likely be significantly faster if I were to take the time and
 * write a cross platform interface for '__builtin_clzll'. However, the current
 * needs don't need the absolute speed as this is currently only used once
 * per Enumeration attribute per query (as opposed, once per query condition
 * comparison or something crazy).
 *
 * The algorithm here might seem a bit odd here when we could instead be
 * using std::numeric_limits<T>::{min,max}() which is what I started out
 * using. However, when Target and Source don't agree on signed-ness the
 * compiler generates warnings about comparing differently signed values.
 *
 * @param src The value to convert.
 * @return The converted value.
 */
template <typename Source, typename Target>
Target safe_integral_cast(Source src) {
  using SourceLimits = std::numeric_limits<Source>;
  using TargetLimits = std::numeric_limits<Target>;

  static_assert(SourceLimits::is_integer, "Source is not an integral type");
  static_assert(TargetLimits::is_integer, "Target is not an integral type");

  Target ret = static_cast<Target>(src);

  // If it can't round trip, its an invalid cast. Note that the converse
  // is not true as a sign could have changed for types of the same bit width
  // but different signed-ness.
  if (static_cast<Source>(ret) != src) {
    throw std::invalid_argument("Invalid integral cast: Roundtrip failed");
  }

  // If the sign changed
  if ((src < 0 && ret >= 0) || (src >= 0 && ret < 0)) {
    throw std::invalid_argument("Invalid integral cast: Sign changed");
  }

  return ret;
}

/**
 * Use `safe_integral_cast` to convert an integral value into a specific
 * Datatype stored in a ByteVecValue.
 *
 * @param value The value to convert.
 * @param type The datatype to convert the value to.
 * @param dest A ByteVecValue to store the converted value in.
 */
template <typename Source>
void safe_integral_cast_to_datatype(
    Source value, Datatype type, ByteVecValue& dest) {
  if (!datatype_is_integer(type)) {
    throw std::invalid_argument("Datatype must be integral");
  }

  if (type == Datatype::BLOB) {
    throw std::invalid_argument(
        "Datatype::BLOB not supported in integral conversion");
  }

  switch (type) {
    case Datatype::BOOL:
      dest.assign_as<uint8_t>(safe_integral_cast<Source, uint8_t>(value));
      return;
    case Datatype::INT8:
      dest.assign_as<int8_t>(safe_integral_cast<Source, int8_t>(value));
      return;
    case Datatype::UINT8:
      dest.assign_as<uint8_t>(safe_integral_cast<Source, uint8_t>(value));
      return;
    case Datatype::INT16:
      dest.assign_as<int16_t>(safe_integral_cast<Source, int16_t>(value));
      return;
    case Datatype::UINT16:
      dest.assign_as<uint16_t>(safe_integral_cast<Source, uint16_t>(value));
      return;
    case Datatype::INT32:
      dest.assign_as<int32_t>(safe_integral_cast<Source, int32_t>(value));
      return;
    case Datatype::UINT32:
      dest.assign_as<uint32_t>(safe_integral_cast<Source, uint32_t>(value));
      return;
    case Datatype::INT64:
      dest.assign_as<int64_t>(safe_integral_cast<Source, int64_t>(value));
      return;
    case Datatype::UINT64:
      dest.assign_as<uint64_t>(safe_integral_cast<Source, uint64_t>(value));
      return;
    default:
      throw std::logic_error("Definitions of integral types are mismatched.");
  }

  ::stdx::unreachable();
}

}  // namespace tiledb::sm::utils

#endif  // TILEDB_TYPE_CASTS_H
