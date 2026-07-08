#pragma once
#include "WitnessRNG\StdLib.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

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
    // Close file buffers, and copy the written data back to the primary name
    _in.close();
    if (_out) _out.close();
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

      _readBuffer.resize(MAX_BUFFER_SIZE);
      _in->read(reinterpret_cast<char*>(_readBuffer.data()), _readBuffer.size() * sizeof(T));
      _readBuffer.resize(static_cast<size_t>(_in->gcount()) / sizeof(T));
      _current = 0;
      if (_readBuffer.empty()) _in = nullptr; // stream exhausted -> end
    }

    std::istream* _in = nullptr;
    std::vector<T> _readBuffer;
    u32 _current = 0;
  };
  iterator begin() { return iterator(&_in); }
  iterator end() { return iterator(); }

private:
  void WriteToDisk() {
    if (!_out) {
      _out = std::ofstream(_name + ".tmp", std::ios::binary);
    }

    _out.write(reinterpret_cast<const char*>(_writeBuffer.data()), _writeBuffer.size() * sizeof(T));
    _writeBuffer.clear();
  }

  std::string _name;
  std::ifstream _in;
  std::ofstream _out;
  std::vector<T> _writeBuffer;
};