/**
 * @file   book_keeping.h
 * @author Stavros Papadopoulos <stavrosp@csail.mit.edu>
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2014 Stavros Papadopoulos <stavrosp@csail.mit.edu>
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
 * This file defines class BookKeeping. 
 */

#ifndef __BOOK_KEEPING_H__
#define __BOOK_KEEPING_H__

#include "fragment.h"
#include <vector>
#include <zlib.h>

/* ********************************* */
/*             CONSTANTS             */
/* ********************************* */

#define TILEDB_BK_OK     0
#define TILEDB_BK_ERR   -1

#define TILEDB_BK_BUFFER_SIZE 10000

class Fragment;

/** Stores the book-keeping structures of a fragment. */
class BookKeeping {
 public:
  // CONSTRUCTORS & DESTRUCTORS

  /** 
   * Constructor. 
   *
   * @param fragment The fragment the book-keeping structure belongs to.
   */
  BookKeeping(const Fragment* fragment);

  /** Destructor. */
  ~BookKeeping();

  // ACCESSORS

  /** Returns the range in which the fragment is constrained. */
  const void* range() const;

  // MUTATORS 
 
  /** Appends a tile offset for the input attribute. */
  void append_tile_offset(int attribute_id, size_t offset);
  /*
   * Initializes the book-keeping structure.
   * 
   * @param range The range in which the array read/write will be constrained.
   * @return TILEDB_BK_OK for success, and TILEDB_OK_ERR for error.
   */
  int init(const void* range);

  // TODO
  int load();

  // MISC

  // TODO
  int finalize();

 private:
  // PRIVATE ATTRIBUTES

  /** The fragment the book-keeping belongs to. */
  const Fragment* fragment_;
  /** The offsets of the next tile to be appended for each attribute. */
  std::vector<size_t> next_tile_offsets_;
  /**
   * The range in which the fragment is constrained. Note that the type of the
   * range must be the same as the type of the array coordinates.
   */
  void* range_;
  /** The tile offsets in their corresponding attribute files. */
  std::vector<std::vector<size_t> > tile_offsets_;

  // PRIVATE METHODS

  // TODO
  int flush_range(gzFile fd) const;

  // TODO
  int flush_tile_offsets(gzFile fd) const;

  // TODO
  int load_range(gzFile fd);

  // TODO
  int load_tile_offsets(gzFile fd);
};

#endif
