#pragma once
#include "WitnessRNG/StdLib.h"
#include <filesystem>
#include <fstream>
#include <vector>

#define MAX_BUFFER_SIZE 1'000'000 // ~32 MB

template<typename T>
class WritableLayerCache
{
public:
  WritableLayerCache() = default;
  WritableLayerCache(u32 depth) {
    std::string fileName = "layer_cache/layer_" + std::to_string(depth) + ".bin";
    _out = std::ofstream(fileName, std::ios::binary);
    _buffer.reserve(MAX_BUFFER_SIZE);
  }
  WritableLayerCache(u32 depth, std::initializer_list<T> values) : WritableLayerCache(depth) {
    for (const T& value : values) Add(value);
  }
  ~WritableLayerCache() {
    WriteToDisk();
  }

  void Add(const T& item) {
    if (!_out) return;
    _buffer.push_back(item);
    if (_buffer.size() >= MAX_BUFFER_SIZE) WriteToDisk();
  }

  size_t Size() {
    return _out.tellp() / sizeof(T) +  _buffer.size();
  }

private:
  void WriteToDisk() {
    if (!_out) return;
    _out.write(reinterpret_cast<const char*>(_buffer.data()), _buffer.size() * sizeof(T));
    _buffer.clear();
  }

  std::ofstream _out;
  std::vector<T> _buffer;
};

template<typename T>
class ReadableLayerCache
{
public:
  ReadableLayerCache() = default;
  ReadableLayerCache(u32 depth) {
    std::string fileName = "layer_cache/layer_" + std::to_string(depth) + ".bin";
    _in = std::ifstream(fileName, std::ios::binary);
    _buffer.reserve(MAX_BUFFER_SIZE);
    ReadFromDisk();
  }

  bool MoveNext() {
    if (++_current < _buffer.size()) return true;
    ReadFromDisk();
    return !_buffer.empty();
  }

  const T& Current() {
    return _buffer[_current];
  }

private:
  void ReadFromDisk() {
    if (!_in) {
      _buffer.clear();
      return;
    }
    _buffer.resize(MAX_BUFFER_SIZE);
    _in.read(reinterpret_cast<char*>(_buffer.data()), _buffer.size() * sizeof(T));
    _buffer.resize(_in.gcount() / sizeof(T));
    _current = 0;
  }

  std::ifstream _in;
  std::vector<T> _buffer;
  u32 _current = 0;
};
