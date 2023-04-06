/**
 * @file   utils.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017-2021 TileDB, Inc.
 * @copyright Copyright (c) 2016 MIT and Intel Corporation
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
 * This file contains useful (global) functions.
 */

#ifndef TILEDB_UTILS_H
#define TILEDB_UTILS_H

#include <cassert>
#include <cmath>
#include <string>
#include <type_traits>
#include <vector>

#include "constants.h"
#include "tiledb/common/logger_public.h"
#include "tiledb/common/status.h"
#include "tiledb/common/unreachable.h"
#include "tiledb/sm/enums/datatype.h"
#include "tiledb/sm/misc/types.h"

namespace tiledb {
namespace sm {

using namespace tiledb::common;

class Posix;
class URI;

enum class SerializationType : uint8_t;

namespace utils {

/* ********************************* */
/*          TYPE FUNCTIONS           */
/* ********************************* */

namespace datatype {

/**
 * Check if a given type T is quivalent to the tiledb::sm::DataType
 * @tparam T
 * @param datatype to compare T to
 * @return Status indicating Ok() on equal data types else Status:Error()
 */
template <class T>
Status check_template_type_to_datatype(Datatype datatype);

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

}  // namespace datatype

/* ********************************* */
/*        GEOMETRY FUNCTIONS         */
/* ********************************* */

namespace geometry {

/**
 * Returns the number of cells in the input rectangle. Applicable
 * only to integers.
 *
 * @tparam T The rectangle domain type.
 * @param rect The input rectangle.
 * @param dim_num The number of dimensions of the rectangle.
 * @return The number of cells in the rectangle.
 */
template <
    class T,
    typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
uint64_t cell_num(const T* rect, unsigned dim_num) {
  uint64_t ret = 1;
  for (unsigned i = 0; i < dim_num; ++i)
    ret *= rect[2 * i + 1] - rect[2 * i] + 1;
  return ret;
}

/** Non-applicable to non-integers. */
template <
    class T,
    typename std::enable_if<!std::is_integral<T>::value, T>::type* = nullptr>
uint64_t cell_num(const T* rect, unsigned dim_num) {
  assert(false);
  (void)rect;
  (void)dim_num;
  return 0;
}

/**
 * Checks if `coords` are inside `rect`.
 *
 * @tparam T The type of the cell and subarray.
 * @param coords The coordinates to be checked.
 * @param rect The hyper-rectangle to be checked, expressed as [low, high] pairs
 *     along each dimension.
 * @param dim_num The number of dimensions for the coordinates and
 *     hyper-rectangle.
 * @return `true` if `coords` are inside `rect` and `false` otherwise.
 */
template <class T>
bool coords_in_rect(const T* coords, const T* rect, unsigned int dim_num);

/**
 * Checks if `coords` are inside `rect`.
 *
 * @tparam T The type of the cell and subarray.
 * @param coords The coordinates to be checked.
 * @param rect The hyper-rectangle to be checked, expressed as [low, high] pairs
 *     along each dimension.
 * @param dim_num The number of dimensions for the coordinates and
 *     hyper-rectangle.
 * @return `true` if `coords` are inside `rect` and `false` otherwise.
 */
template <class T>
bool coords_in_rect(
    const T* coords, const std::vector<const T*>& rect, unsigned int dim_num);

/** Returns *true* if hyper-rectangle `a` overlaps with `b`. */
template <class T>
bool overlap(const T* a, const T* b, unsigned dim_num);

/**
 * Computes the overlap between two rectangles.
 *
 * @tparam T The types of the rectangles.
 * @param a The first input rectangle.
 * @param b The second input rectangle.
 * @param o The overlap area between `a` and `b`.
 * @param overlap `true` if the two rectangles overlap and `false` otherwise.
 */
template <class T>
void overlap(const T* a, const T* b, unsigned dim_num, T* o, bool* overlap);

/**
 * Returns the percentage of coverage of hyper-rectangle `a` in `b`.
 * Note that the function assumes that `a` is fully contained in `b`.
 */
template <class T>
double coverage(const T* a, const T* b, unsigned dim_num);

/**
 * Returns the intersection between r1 and r2.
 *
 * @param r1 A vector of 1D ranges, one range per dimension as a 2-element
 *     (start, end) array.
 * @param r2 A vector of 1D ranges, one range per dimension as a 2-element
 *     (start, end) array.
 * @return The intersection between r1 and r2, similarly as a vector of
 *     (start, end) 1D ranges.
 */
template <class T>
std::vector<std::array<T, 2>> intersection(
    const std::vector<std::array<T, 2>>& r1,
    const std::vector<std::array<T, 2>>& r2);

}  // namespace geometry

}  // namespace utils
}  // namespace sm
}  // namespace tiledb

#endif  // TILEDB_UTILS_H
