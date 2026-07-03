#pragma once
#include "WitnessRNG/StdLib.h"
#include <filesystem>
#include <fstream>
#include <vector>

template<typename T>
class LayerCache {
  static constexpr size_t STATE_BUFFER_SIZE = 1'000'000; // ~32 MB

public:
  class iterator {
  public:
    explicit iterator(LayerCache* layerCache) : _layerCache(layerCache) {}

    const T& operator*() { return _layerCache->_buffer[_offset]; }

    iterator& operator++() {
      _offset++;
      if (_offset >= _layerCache->_buffer.size()) {
        _layerCache->Read();
        _offset = 0;
        if (_offset >= _layerCache->_buffer.size()) {
          _layerCache->_buffer.clear(); // to avoid writing stale buffer
          _layerCache = nullptr;
        }
      }

      return *this;
    }

    bool operator!=(const iterator& other) const { return other._layerCache == nullptr && _layerCache == nullptr; }

  private:
    LayerCache* _layerCache = nullptr;
    u32 _offset = 0;
  };

  iterator begin() {
    Write();
    _stream.seekg(0);
    return iterator(this);
  }
  iterator end() { return iterator(nullptr); }


  LayerCache() = default;
  LayerCache(u32 depth) {
    std::filesystem::create_directories("layer_cache");
    _stream = std::fstream("layer_cache/layer_" + std::to_string(depth) + ".bin", std::ios::trunc | std::ios::in | std::ios::out | std::ios::binary);
    _buffer.reserve(STATE_BUFFER_SIZE);
  }
  LayerCache& operator=(LayerCache&& other) = default;

  ~LayerCache() {
    Write();
  }

  void Add(const T& state) {
    _buffer.push_back(state);
    if (_buffer.size() >= STATE_BUFFER_SIZE) {
      Write();
      _buffer.clear();
    }
  }

private:
  void Write() {
    _stream.write(reinterpret_cast<const char*>(_buffer.data()), _buffer.size() * sizeof(T));
  }

  void Read() {
    _buffer.resize(STATE_BUFFER_SIZE); // In case we did a read before this write... shouldn't happen.
    _stream.read(reinterpret_cast<char*>(_buffer.data()), _buffer.size() * sizeof(T));
    _buffer.resize(_stream.gcount() / sizeof(T));
  }

  std::fstream _stream;
  std::vector<T> _buffer;
};