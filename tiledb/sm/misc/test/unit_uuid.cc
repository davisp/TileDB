/**
 * @file   unit_uuid.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2018-2022 TileDB, Inc.
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
 * Tests the UUID utility functions.
 */

#include <test/support/tdb_catch.h>
#include <thread>
#include <vector>

#include "tiledb/sm/misc/tdb_time.h"
#include "tiledb/sm/misc/uuid.h"

using namespace tiledb::sm;

size_t generate_uuids(std::vector<std::string>& uuids) {
  size_t uuids_size = uuids.size();
  auto now = utils::time::timestamp_now_ms();
  size_t idx = 0;
  while ((utils::time::timestamp_now_ms()) < now + 100 && idx < uuids_size) {
    uuids[idx++] = uuid::generate_uuid();
  }
  return idx;
}

void validate_uuids(std::vector<std::string>& uuids, size_t num_uuids) {
  // The contents of the UUIDs vector will be a bunch of UUIDs where anything
  // generated in the same millisecond shares a prefix that is being
  // incremented. Seriously, try throwing a log statement in this loop. Its
  // fancy.
  //
  // The logic for asserting this can't be overly prescriptive given the exact
  // contents of the UUIDs vector will have an unknown number of entries given
  // we're just racing the processor and entropy pools. Best I can think of is
  // to just assert that we've got more than 5 groups that have more than
  // 10 members each. Groups are detected as having the same first four bytes.
  uint64_t num_groups = 0;
  uint64_t this_group = 0;
  for (size_t i = 1; i < num_uuids; i++) {
    bool match = true;
    for (size_t j = 0; j < 4; j++) {
      if (uuids[i - 1][j] != uuids[i][j]) {
        match = false;
        break;
      }
    }
    if (!match) {
      if (this_group > 10) {
        num_groups += 1;
      }
      this_group = 0;
      continue;
    }

    // We share a prefix so assert that they're ordered.
    REQUIRE(uuids[i] > uuids[i - 1]);
    this_group += 1;
  }

  REQUIRE(num_groups > 10);
}

TEST_CASE("Seriali UUID Generation", "[uuid][serial]") {
  // Generate a UUID to make sure we've primed all the initialization.
  auto uuid = uuid::generate_uuid();
  REQUIRE(uuid.size() == 32);

  // A million strings should be enough for anyone.
  std::vector<std::string> uuids{1000000};

  auto num_uuids = generate_uuids(uuids);
  validate_uuids(uuids, num_uuids);
}

TEST_CASE("Parallel UUID Generation", "[uuid][parallel]") {
  const unsigned nthreads = 20;
  std::vector<std::thread> threads;
  std::vector<std::vector<std::string>> uuids{nthreads};
  size_t num_uuids[nthreads];

  // Pre-allocate our buffers so we're getting as much contention as possible
  for (size_t i = 0; i < nthreads; i++) {
    uuids[i].resize(1000000);
  }

  // Generate UUIDs simultaneously in multiple threads.
  for (size_t i = 0; i < nthreads; i++) {
    auto num_ptr = &num_uuids[i];
    auto vec_ptr = &uuids[i];
    threads.emplace_back([num_ptr, vec_ptr]() {
      auto num = generate_uuids(*vec_ptr);
      *num_ptr = num;
    });
  }

  // Wait for all of our threads to finish.
  for (auto& t : threads) {
    t.join();
  }

  // Check that we've generated the correct number of unique UUIDs.
  std::unordered_set<std::string> uuid_set;
  size_t total_uuids = 0;
  for (size_t i = 0; i < nthreads; i++) {
    total_uuids += num_uuids[i];
    for (size_t j = 0; j < num_uuids[i]; j++) {
      uuid_set.insert(uuids[i][j]);
    }
  }
  REQUIRE(uuid_set.size() == total_uuids);

  // Threads fighting over who gets which UUID in the sequence means we can't
  // really make many guarantees on what each individual thread generated.
  // However, we can make an assertion about the combined group same as for the
  // serial case. The sort just combines all thread generated UUIDs as if they
  // were generated in a single thread.
  std::vector<std::string> all_uuids{total_uuids};
  size_t idx = 0;
  for (auto uuid : uuid_set) {
    all_uuids[idx++] = uuid;
  }
  std::sort(all_uuids.begin(), all_uuids.end());
  validate_uuids(all_uuids, total_uuids);
}
