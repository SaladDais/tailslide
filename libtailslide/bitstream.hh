#pragma once

/*
Based on https://github.com/kaimast/bitstream/blob/master/include/bitstream.h

BSD 3-Clause License

Copyright (c) 2018, Kai Mast
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <type_traits>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <fstream>
#include <stdexcept>

#include "portable_endian.hh"


namespace Tailslide {

enum Endianness {
  ENDIAN_LITTLE,
  ENDIAN_BIG,
};

template <typename T>
using SameSizedUInt =
typename std::conditional<sizeof(T) == 1, std::uint8_t,
    typename std::conditional<sizeof(T) == 2, std::uint16_t,
        typename std::conditional<sizeof(T) == 4, std::uint32_t,
            std::uint64_t
        >::type
    >::type
>::type;

class BitStream {
  private:
    static constexpr uint32_t MIN_REALLOC = 10;

  public:
    explicit BitStream(Endianness endian=ENDIAN_BIG) : m_pos(0), m_size(0), m_alloc_size(0), m_data(nullptr),
        m_read_only(false), m_endianness(endian) {
    }

    /**
     * @brief BitStream
     * @param data
     * @param length
     *
     * NOTE: Use assign() if you want to prevent memcpy
     */
    BitStream(const uint8_t *data, const uint32_t length, Endianness endian=ENDIAN_BIG)
        : m_pos(0), m_size(0), m_alloc_size(MIN_REALLOC), m_data(nullptr), m_read_only(false), m_endianness(endian) {
      m_data = reinterpret_cast<uint8_t *>(malloc(m_alloc_size));
      if (m_data == nullptr) {
        throw std::runtime_error("Failed to allocated data");
      }
      memset(m_data, 0, m_alloc_size);

      if (length > 0) {
        write_raw_data(data, length, false);
      }
    }

    BitStream(BitStream &&other) noexcept
        : m_pos(other.m_pos), m_size(other.m_size), m_alloc_size(other.m_alloc_size), m_data(other.m_data),
          m_read_only(other.m_read_only), m_endianness(other.m_endianness) {
      other.m_data = nullptr;
    }

    BitStream(const BitStream &other) = delete;

    ~BitStream() {
      clear();
    }

    bool is_read_only() const {
      return m_read_only;
    }

    /// Create read-only BitStream that accesses this BitStream's data
    BitStream make_view() const {
      BitStream view;
      view.assign(data(), size(), true);
      return view;
    }

    /**
     * Allocate initial buffer space so we reduce the number of mallocs
     */
    void pre_alloc(uint32_t size) {
      if (size < m_alloc_size) {
        throw std::runtime_error("Can only increase alloc size");
      }

      size_t old_size = m_alloc_size;
      m_alloc_size = size;
      m_data = reinterpret_cast<uint8_t *>(realloc(m_data, m_alloc_size));

      if (m_data == nullptr) {
        throw std::runtime_error("Failed to allocated data");
      }
      // zero out any new data added.
      memset(m_data + old_size, 0, size - old_size);
    }

    /**
     * Create an identical copy of this BitStream
     *
     * \param force_copy [optional]
     *      Ensure the copy has it's own dedicated copy of the data
     */
    BitStream duplicate(bool force_copy = false) const {
      BitStream result;

      if (m_read_only && !force_copy) {
        result.assign(m_data, m_size, true);
      } else {
        result = BitStream(m_data, m_size);
      }

      result.move_to(pos());
      return result;
    }

    void clear() {
      // NOLINTNEXTLINE
      if (!m_read_only && m_data != nullptr) {
        // NOLINTNEXTLINE
        free(m_data);
      }

      m_read_only = false;
      m_data = nullptr;
      m_size = m_alloc_size = 0;
    }

    BitStream& operator=(BitStream &&other)  noexcept {
      clear();

      m_data = other.m_data;
      m_read_only = other.m_read_only;
      m_pos = other.m_pos;
      m_size = other.m_size;
      m_alloc_size = other.m_alloc_size;

      other.m_data = nullptr;
      other.m_pos = other.m_size = other.m_alloc_size = 0;
      return *this;
    }

    void resize(uint32_t new_size) {
      if (m_read_only) {
        throw std::runtime_error("Cannot resize: is read-only!");
      }

      if (m_data == nullptr) {
        m_size = m_alloc_size = new_size;
        if (new_size > 0) {
          m_data = reinterpret_cast<uint8_t *>(malloc(new_size));
          memset(m_data, 0, new_size);
        }
        return;
      }

      if (new_size < m_size) {
        m_size = new_size;
        //no reason to change alloc size?
        return;
      }

      if (new_size <= m_alloc_size) {
        m_size = new_size;
        return;
      }

      uint32_t step = std::max(MIN_REALLOC, new_size - m_size);
      pre_alloc(m_alloc_size + step);

      m_size = new_size;
    }

    /**
     * Write a raw buffer to the BitStream
     *
     * @note This will raise an exception if data is a nullpointer or length is of size 0
     */
    BitStream &write_raw_data(const uint8_t *data, const uint32_t length, bool advance = true) {
      if (data == nullptr || length == 0) {
        throw std::runtime_error("Invalid parameters");
      }

      if (m_pos + length > size()) {
        resize(m_pos + length);
      }

      memcpy(&m_data[m_pos], data, length);

      if (advance) {
        m_pos += length;
      }

      return *this;
    }

    void make_space(uint32_t increase) {
      if (m_read_only) {
        throw std::runtime_error("Cannot make space in read only mode");
      }

      auto remain = remaining_size();
      resize(size() + increase);

      // had data after the cursor and we're increasing in size
      if (increase > 0 && remain) {
        // shift all data after the cursor up to account for the new space
        memmove(current() + increase, current(), remain);
        // zero out the space we just created
        memset(current(), 0, increase);
      }
    }

    void remove_space(uint32_t decrease) {
      if (decrease > remaining_size()) {
        throw std::runtime_error("Not enough space left");
      }

      memmove(current(), current() + decrease, remaining_size() - decrease);
      resize(size() - decrease);
    }

    void read_raw_data(uint8_t **data, uint32_t length) {
      if (m_pos + length > size()) {
        throw std::runtime_error("Cannot read from BitStream. Already at the end.");
      }

      *data = &m_data[m_pos];
      m_pos += length;
    }

    /**
     * Write virtually any kind of data to the stream
     *
     * @note data is expected not to be a pointer. you might have to dereference it
     */
    template<typename T>
    BitStream &operator<<(const T &data) {
      static_assert(std::is_trivially_copyable<T>(), "Need a specialized serialized function for non-POD types");

      if (m_pos + sizeof(T) > size()) {
        uint32_t newSize = m_pos + sizeof(T);
        resize(newSize);
      }

      // need special endian swapping logic if this is an integral larger than a byte
      if constexpr ((std::is_arithmetic_v<T> || std::is_enum_v<T>) && sizeof(T) > 1) {
        // Store data reinterpreted as an integral type of the appropriate size and byte-swap,
        // should optimize to a bswap instruction for the given integer width.
        SameSizedUInt<T> val = reinterpret_cast<const SameSizedUInt<T> &>(data);
        if (m_endianness == ENDIAN_BIG)
          val = tail_htobe(val);
        else
          val = tail_htole(val);
        memcpy(&m_data[m_pos], &val, sizeof(T));
      } else {
        memcpy(&m_data[m_pos], &data, sizeof(T));
      }
      m_pos += sizeof(T);

      return *this;
    }

    /**
     * Read virtually any kind of data from the stream
     *
     * @note data is expected not to be a pointer. you might have to dereference it
     */
    template<typename T>
    BitStream &operator>>(T &data) {
      if (m_pos + sizeof(T) > size()) {
        throw std::runtime_error("Cannot read from BitStream: Already at the end.");
      }

      // need special endian-swapping logic if this is an integral larger than a byte
      if constexpr ((std::is_arithmetic_v<T> || std::is_enum_v<T>) && sizeof(T) > 1) {
        // reinterpret as an integral type of the appropriate size and byte-swap
        // should optimize to a bswap instruction for the given integer width
        auto *val = reinterpret_cast<SameSizedUInt<T> *>(&m_data[m_pos]);
        auto &sized_data = reinterpret_cast<SameSizedUInt<T> &>(data);
        if (m_endianness == ENDIAN_BIG)
          sized_data = tail_betoh(*val);
        else
          sized_data = tail_letoh(*val);
      } else {
        memcpy(&data, &m_data[m_pos], sizeof(T));
      }
      m_pos += sizeof(T);

      return *this;
    }

    /**
     * @brief Access raw content of the stream
     * @note This should only be used internally to write to the socket
     */
    const uint8_t *data() const {
      return m_data;
    }

    uint8_t *data() {
      return m_data;
    }

    void detach(uint8_t *&out, uint32_t &len) {
      if (m_size == 0) {
        out = nullptr;
        // there might be pre-allocated buffer
        free(m_data);
      } else if (m_size < m_alloc_size) {
        // no need to zero out because array is shrinking
        out = reinterpret_cast<uint8_t *>(realloc(m_data, m_size));
      } else if (m_size == m_alloc_size) {
        out = m_data;
      } else {
        throw std::runtime_error("invalid state: m_size > m_alloc_size");
      }

      len = m_size;

      m_data = nullptr;
      m_size = 0;
      m_alloc_size = 0;
    }

    int64_t hash() const {
      int64_t result = 0;
      auto *ptr = reinterpret_cast<uint8_t *>(&result);

      for (uint32_t i = 0; i < this->size(); ++i) {
        ptr[i % 8] = ptr[i % 8] ^ m_data[i];
      }

      return result;
    }

    void assign(const uint8_t *buffer, uint32_t len, bool read_only) {
      if (read_only) {
        assign(const_cast<uint8_t *>(buffer), len, true);
      } else {
        throw std::runtime_error("Cannot use const reference and writable buffer");
      }
    }

    void assign(uint8_t *buffer, uint32_t len, bool read_only = false) {
      clear();

      m_data = buffer;
      m_size = m_alloc_size = len;
      m_pos = 0;
      m_read_only = read_only;
    }

    /**
     * @brief Return the current size of the stream
     */
    size_t size() const {
      return m_size;
    }

    bool empty() const {
      return m_size == 0;
    }

    size_t remaining_size() const {
      return size() - pos();
    }

    size_t allocated_size() const {
      return m_alloc_size;
    }

    /**
     * @brief Did we read until the end of the stream
     * @note This only makes sense when reading. Writing dynamically expands the stream
     */
    bool at_end() const {
      return m_pos == m_size;
    }

    /**
     * @brief Get the currenty position
     * @return Returns the position (in Bytes)
     */
    uint32_t pos() const {
      return m_pos;
    }

    bool move_to(uint32_t pos, bool allocate = false) {
      if (pos > m_size) {
        if (allocate) {
          resize(pos + 1);
        } else {
          return false;
        }
      }

      m_pos = pos;
      return true;
    }

    bool move_by(int32_t offset, bool allocate = false) {
      // make sure we don't overflow
      if (offset < 0 && static_cast<uint32_t>((-1) * offset) > m_pos) {
        throw std::runtime_error("Can't move. Would overflow buffer!");
      }

      return move_to(m_pos + offset, allocate);
    }

    /**
     * @brief Get raw pointer to the current position in the stream
     * @return
     */
    uint8_t *current() {
      return &m_data[m_pos];
    }

    const uint8_t *current() const {
      return &m_data[m_pos];
    }

  private:
    uint32_t m_pos;
    uint32_t m_size, m_alloc_size;

    uint8_t *m_data;

    Endianness m_endianness;

    // a read only BitStream cannot modify the underlying data
    // useful to avoid copying and delete
    bool m_read_only;
};

/***
 * Read a length-prefixed BitStream and place its contents in @bstream
 * @param bstream
 */
template<>
inline
BitStream &BitStream::operator>><BitStream>(BitStream &bstream) {
  if (bstream.size() > 0) {
    throw std::runtime_error("Target bstream already contains data");
  }

  uint32_t length = 0;
  *this >> length;

  if (m_pos + length > size()) {
    throw std::runtime_error("length is longer than BitStream");
  }

  if (length > 0) {
    bstream.write_raw_data(current(), length);
    move_by((int32_t)length);
  }

  bstream.move_to(0);
  return *this;
}

/***
 * Write a length-prefixed BitStream into another BitStream
 */
template<>
inline BitStream &BitStream::operator<< <BitStream>(const BitStream &bstream) {
  uint32_t length = bstream.size();
  *this << length;

  if (length > 0) {
    write_raw_data(bstream.data(), length);
  }

  return *this;
}

template<>
inline
BitStream &BitStream::operator>><char>(char &data) {
  if (at_end()) {
    throw std::runtime_error("Cannot read more: Already at end");
  }

  data = (char)m_data[m_pos];
  m_pos++;

  return *this;
}


inline bool operator==(const BitStream &first, const BitStream &second) {
  if (first.size() != second.size()) {
    return false;
  } else {
    return memcmp(first.data(), second.data(), first.size()) == 0;
  }
}

inline bool operator!=(const BitStream &first, const BitStream &second) {
  return !(first == second);
}

class ScopedBitStreamSeek {
  public:
    ScopedBitStreamSeek(BitStream &bstream, uint32_t pos, bool allocate=false) {
      _m_old_pos = bstream.pos();
      _m_bstream = &bstream;
      _m_bstream->move_to(pos, allocate);
    };
    ~ScopedBitStreamSeek() {
      _m_bstream->move_to(_m_old_pos);
    }
  protected:
    uint32_t _m_old_pos;
    BitStream *_m_bstream;
};

}
