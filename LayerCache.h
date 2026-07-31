#pragma once
#include "Common.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#ifdef LAYERCACHE_ZSTD
#include <zstd.h>
#include <stdexcept>
#ifndef LAYERCACHE_ZSTD_LEVEL
#define LAYERCACHE_ZSTD_LEVEL 3
#endif
#endif

#define MAX_BUFFER_SIZE 1'000'000

template<typename T>
class LayerCache {
public:
  LayerCache() = default;
  LayerCache(const auto&... args) {
    _name = "cache/";
    ((_name += std::format("_{}", args)), ...); // magical C++20 "unwrap these varargs using this function"
    _name += ".bin";

    _in = std::ifstream(_name, std::ios::binary);
  }

  void Add(const T& item) {
    if (_writeBuffer.size() == 0) _writeBuffer.reserve(MAX_BUFFER_SIZE);
    _writeBuffer.push_back(item);
    if (_writeBuffer.size() >= MAX_BUFFER_SIZE) WriteToDisk();
  }

  void FinishWriteAndResetRead() {
    WriteToDisk();
    if (!_out.is_open()) return; // We may not have written anything *this time*, so just check if the output file was ever opened.
    _in.close();
    _out.close();
    std::filesystem::rename(_name + ".tmp", _name);
    _in = std::ifstream(_name, std::ios::binary);
  }

  struct iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    iterator() = default; // end sentinel (_in == nullptr)
    iterator(std::istream* in) : _in(in) {
      _readBuffer.reserve(MAX_BUFFER_SIZE);
      ReadFromDisk();
    }

    const T& operator*() const { return _readBuffer[_current]; }
    const T* operator->() const { return &_readBuffer[_current]; }

    iterator& operator++() {
      if (++_current >= _readBuffer.size()) ReadFromDisk();
      return *this;
    }

    bool operator==(const iterator& other) const { return _in == other._in; }
    bool operator!=(const iterator& other) const { return _in != other._in; }

  private:
    void ReadFromDisk() {
      if (!_in) return; // Dead iterator

#ifdef LAYERCACHE_ZSTD
      u64 header[2]; // { uncompressed bytes, compressed bytes }
      _in->read(reinterpret_cast<char*>(header), sizeof(header));
      if (_in->gcount() < static_cast<std::streamsize>(sizeof(header))) {
        _readBuffer.clear();
        _in = nullptr; // stream exhausted -> end
        return;
      }
      const u64 rawBytes = header[0];
      const u64 compBytes = header[1];
      _compressBuffer.resize(compBytes);
      _in->read(_compressBuffer.data(), compBytes);
      _readBuffer.resize(rawBytes / sizeof(T));
      size_t decompressed = ZSTD_decompress(_readBuffer.data(), rawBytes, _compressBuffer.data(), compBytes);
      if (ZSTD_isError(decompressed)) throw std::runtime_error(ZSTD_getErrorName(decompressed));
      _current = 0;
#else
      _readBuffer.resize(MAX_BUFFER_SIZE);
      _in->read(reinterpret_cast<char*>(_readBuffer.data()), _readBuffer.size() * sizeof(T));
      _readBuffer.resize(static_cast<size_t>(_in->gcount()) / sizeof(T));
      _current = 0;
#endif
      if (_readBuffer.empty()) _in = nullptr; // stream exhausted -> end
    }

    std::istream* _in = nullptr;
    std::vector<T> _readBuffer;
    u32 _current = 0;
#ifdef LAYERCACHE_ZSTD
    std::vector<char> _compressBuffer;
#endif
  };
  iterator begin() { return iterator(&_in); }
  iterator end() { return iterator(); }

private:
  void WriteToDisk() {
    if (_writeBuffer.size() == 0) return; // Nothing to write

    if (!_out.is_open()) _out = std::ofstream(_name + ".tmp", std::ios::binary);
#ifdef LAYERCACHE_ZSTD
    const size_t rawBytes = _writeBuffer.size() * sizeof(T);
    const size_t bound = ZSTD_compressBound(rawBytes);
    _compressBuffer.resize(bound);
    const size_t compBytes = ZSTD_compress(_compressBuffer.data(), bound, _writeBuffer.data(), rawBytes, LAYERCACHE_ZSTD_LEVEL);
    if (ZSTD_isError(compBytes)) throw std::runtime_error(ZSTD_getErrorName(compBytes));

    const u64 header[2] = { rawBytes, compBytes }; // { uncompressed bytes, compressed bytes }
    _out.write(reinterpret_cast<const char*>(header), sizeof(header));
    _out.write(_compressBuffer.data(), compBytes);
#else
    _out.write(reinterpret_cast<const char*>(_writeBuffer.data()), _writeBuffer.size() * sizeof(T));
#endif
    _writeBuffer.clear();
  }

  std::string _name;
  std::ifstream _in;
  std::ofstream _out;
  std::vector<T> _writeBuffer;
#ifdef LAYERCACHE_ZSTD
  std::vector<char> _compressBuffer;
#endif
};