/*
 * Copyright (C) 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GAPIR_RESOURCE_IN_MEMORY_CACHE_H
#define GAPIR_RESOURCE_IN_MEMORY_CACHE_H

#include "replay_connection.h"
#include "resource_cache.h"

#include "core/cc/assert.h"

#include <functional>
#include <memory>
#include <unordered_map>

namespace gapir {

// Fixed size in-memory resource cache. It uses a ring buffer to store the cache
// and starts invalidating cache entries from the oldest to the newest when more
// space is required.
class ResourceInMemoryCache : public ResourceCache {
 public:
  // Creates a new in-memory cache with the given fallback provider and base
  // address. The initial cache size is 0 byte.
  static std::unique_ptr<ResourceInMemoryCache> create(
      std::unique_ptr<ResourceProvider> fallbackProvider, void* buffer);

  // destructor
  ~ResourceInMemoryCache();

  // Prefetches the specified resources, caching as many that fit in memory as
  // possible.
  void prefetch(const Resource* resources, size_t count, ReplayConnection* conn,
                void* temp, size_t tempSize) override;

  // clears the cache.
  void clear();

  // resets the size of the buffer used for caching.
  void resize(size_t newSize);

  // debug print the internal state.
  void dump(FILE*);

 protected:
  // A doubly-linked list data structure representing a chunk of memory in the
  // cache.
  struct Block {
    inline Block();
    inline Block(size_t offset, size_t size);
    inline Block(size_t offset, size_t size, const ResourceId& id);

    inline ~Block();

    inline void linkAfter(Block* other);
    inline void linkBefore(Block* other);
    inline void unlink();
    inline bool isFree() const;
    inline size_t end() const;

    size_t offset;  // offset in bytes from mBuffer.
    size_t size;    // size in bytes. May wrap-around the cache buffer.
    ResourceId id;
    Block* next;
    Block* prev;
  };

  void putCache(const Resource& resource, const void* data) override;
  bool getCache(const Resource& resource, void* data) override;

  // free evicts the cache entry for block, transforming it into a free block.
  void free(Block* block);

  // foreach_block calls cb for each block, starting with first.
  void foreach_block(Block* first, const std::function<void(Block*)>& cb);

  // destroy frees, unlinks and deletes the block, returning the next block.
  Block* destroy(Block* block);

  // first returns the block with the lowest offset.
  Block* first();

  // last returns the block with the highest offset.
  Block* last();

 private:
  // constructor
  ResourceInMemoryCache(std::unique_ptr<ResourceProvider> fallbackProvider,
                        void* buffer);

  // put adds the the resource to the cache.
  // size must be less or equal to mBufferSize.
  void put(const ResourceId& id, size_t size, const uint8_t* data);

  // A pointer to the next block to be used for a resource allocation.
  // While filling the cache, mHead will point to the first free block. Once
  // the cache is full it will point to an existing cache entry that will be
  // next to be evicted.
  Block* mHead;

  // A map of cached resource identifiers to offsets on mBuffer.
  std::unordered_map<ResourceId, size_t> mCache;

  // The base address and the size of the memory used for caching.
  // This memory region is owned by the memory manager class, not by the cache
  // itself.
  uint8_t* mBuffer;
  size_t mBufferSize;
};

inline ResourceInMemoryCache::Block::Block()
    : offset(0), size(0), next(this), prev(this) {}
inline ResourceInMemoryCache::Block::Block(size_t offset_, size_t size_)
    : offset(offset_), size(size_), next(this), prev(this) {}
inline ResourceInMemoryCache::Block::Block(size_t offset_, size_t size_,
                                           const ResourceId& id_)
    : offset(offset_), size(size_), id(id_), next(this), prev(this) {}

inline ResourceInMemoryCache::Block::~Block() {
  GAPID_ASSERT(next == this && prev == this);
}

inline void ResourceInMemoryCache::Block::linkAfter(Block* other) {
  GAPID_ASSERT(next == this && prev == this);
  next = other->next;
  prev = other;
  next->prev = this;
  prev->next = this;
}

inline void ResourceInMemoryCache::Block::linkBefore(Block* other) {
  GAPID_ASSERT(next == this && prev == this);
  next = other;
  prev = other->prev;
  next->prev = this;
  prev->next = this;
}

inline void ResourceInMemoryCache::Block::unlink() {
  GAPID_ASSERT(next != this && prev != this);
  next->prev = prev;
  prev->next = next;
  next = this;
  prev = this;
}

inline bool ResourceInMemoryCache::Block::isFree() const {
  return id.size() == 0;
}

inline size_t ResourceInMemoryCache::Block::end() const {
  return offset + size;
}

inline void ResourceInMemoryCache::free(Block* block) {
  mCache.erase(block->id);
  block->id = ResourceId();
}

inline void ResourceInMemoryCache::foreach_block(
    Block* first, const std::function<void(Block*)>& cb) {
  std::vector<Block*> blocks;
  blocks.push_back(first);
  for (Block* block = first->next; block != first; block = block->next) {
    blocks.push_back(block);
  }
  for (Block* block : blocks) {
    cb(block);
  }
}

inline ResourceInMemoryCache::Block* ResourceInMemoryCache::destroy(
    Block* block) {
  Block* next = block->next;
  if (mHead == block) {
    mHead = next;
  }
  free(block);
  block->unlink();
  delete block;
  return next;
}

inline ResourceInMemoryCache::Block* ResourceInMemoryCache::first() {
  return last()->next;
}

inline ResourceInMemoryCache::Block* ResourceInMemoryCache::last() {
  Block* block = mHead;
  for (; block->next->offset > block->offset; block = block->next) {
  }
  return block;
}

}  // namespace gapir

#endif  // GAPIR_RESOURCE_IN_MEMORY_CACHE_H
