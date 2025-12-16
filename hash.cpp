#include <utility>
#include <list>
#include "hash.hpp"
#include <algorithm>
#include <cmath>


HashSet::Iterator HashSet::begin() {
  return elements.begin();
}


HashSet::Iterator HashSet::end() {
  return elements.end();
}


HashSet::HashSet() : size_(0), max_load_factor_(0.75f) {
  buckets.resize(sizes[0], elements.end());
}

// The copy constructor creates a new HashSet that's a deep copy of the original
// The idea is generally not only to copy the elements but also preserve the bucket-to-element mapping
HashSet::HashSet(const HashSet& other) : buckets(other.bucketCount()), size_(other.size_) {

    elements = other.elements;
    buckets.resize(other.bucketCount(), elements.end());

// For each bucket in the original HashSet, the pointer to the corresponding position in the new
// elements list needs to be recreated.
    for (std::size_t i = 0; i < bucketCount(); i++) {
      if (other.buckets[i] != other.elements.end()) {

        auto pos_in_other = other.buckets[i];

        size_t c = 0;
        auto it = other.elements.begin();
        while (it != pos_in_other && it != other.elements.end()) {
          ++c;
          ++it;
        }

        auto our_it = elements.begin();
        for (size_t j = 0; j < c && our_it != elements.end(); ++j) {
          ++our_it;
        }
        buckets[i] = our_it;
      }
    }
  }


HashSet& HashSet::operator=(HashSet other) {
  std::swap(elements, other.elements);
  std::swap(buckets, other.buckets);
  std::swap(size_, other.size_);
  std::swap(max_load_factor_, other.max_load_factor_);

  return *this;
}


HashSet::~HashSet() {
}

void HashSet::insert(int key) {

// Checks if rehashing is needed before insertion.
  if ((size_ + 1) > bucketCount() * maxLoadFactor()) {
    rehash(bucketCount() * 2);
  }

  std::size_t idx = bucket(key);

// Don't insert duplicates.
  if (contains(key)) {
    return;
  }

  Iterator insertPosition;

  if (buckets[idx] == elements.end()) {

// Finds the next non-empty bucket to maintain element ordering. Elements with
// the same hash value must be contiguous in the list (if 1st element in bucket).
    std::size_t next_idx = idx + 1;
    while ((next_idx < bucketCount()) && buckets[next_idx] == elements.end()) {
      next_idx++;
    }

    if (next_idx == bucketCount()) {
      insertPosition = elements.end();
    }
    else {
      insertPosition = buckets[next_idx];
    }
  }
  else {
    // If the bucket already has elements, then the end of the chain is found
    // to maintain contiguity of elements with the same hash.
    Iterator position = buckets[idx];
    Iterator nextPosition = position;
    ++nextPosition;

    while (nextPosition != elements.end() && bucket(*nextPosition) == idx) {
      position = nextPosition;
      ++nextPosition;
    }

    insertPosition = ++position;
  }

// Actual insertion is performed here. Yep, neat right?
  Iterator new_elem = elements.insert(insertPosition, key);

  if (buckets[idx] == elements.end()) {
    buckets[idx] = new_elem;
  }

  size_++;
}


// The main concept here is to return true if the key exists in the HashSet. It uses
// the hash to locate the corresponding bucket and search through it.
bool HashSet::contains(int key) const {
  std::size_t idx = bucket(key);

  if (buckets[idx] == elements.end()) {
    return false;
  }

  auto it = buckets[idx];
  while (it != elements.end() && bucket(*it) == idx) {
    if (*it == key) {
      return true;
    }
    ++it;
  }
  return false;
}

// The key idea here is to return an iterator to the key if found, otherwise just to return
// elements.end(). It efficiently searches only within the relevant bucket using hashing.
HashSet::Iterator HashSet::find(int key) {
  std::size_t idx = bucket(key);

  if (buckets[idx] ==elements.end()) {
    return elements.end();
  }

  Iterator it = buckets[idx];

  while (it != elements.end() && bucket(*it) == idx) {
    if (*it == key) {
      return it;
    }
    ++it;
  }
  return elements.end();
}

void HashSet::erase(int key) {

  Iterator it = find(key);
  if (it != elements.end()) {
    erase(it);
  }
}

HashSet::Iterator HashSet::erase(HashSet::Iterator it) {
  if (it == elements.end()) {
    return it;
  }

  int key = *it;

  std::size_t idx = bucket(key);
  Iterator next = std::next(it);

// If the element that the bucket points to is being erased, the pointer must be updated.
  if (buckets[idx] == it) {

    if (next != elements.end() && bucket(*next) == idx) {
// If there are more elements with the same hash, just point to the next one.
      buckets[idx] = next;

    }
    else {
      // Otherwise just mark the bucket as empty.
      buckets[idx] = elements.end();
    }
  }

// Actual erasure is performed and size is accordingly updated.
  Iterator result = elements.erase(it);
  size_--;

  return result;
}

void HashSet::rehash(std::size_t newSize) {
  // Appropriate new size is found from predefined sizes list.
  // This needs to be at least as large as requested and satisfies the load factor constraint.

  std::size_t new_size_ = sizes[0]; 
  for (std::size_t size : sizes) {
    if (size >= newSize && (static_cast<float>(size_) / size <= maxLoadFactor() || size == sizes.back())) {
      new_size_ = size;
      break;
    }
  }

// There is no need to reshash if the new size is not larger than the current size.
  if (new_size_ <= bucketCount()) { 
    return;
  }

// A new buckets array is created with all elements.end().
  std::vector<Iterator> newBuckets(new_size_, elements.end());
  
  // Each element is processed in place using the list splice operation - this approach
  // maintains iterator validity by rearranging the existing list instead of creating a new one.
  for (auto it = elements.begin(); it != elements.end(); ) {
    auto positionNow = it++; // Current position is saved and the iterator is advanced.
    std::size_t newHashValue = (*positionNow) % new_size_;

    if (newBuckets[newHashValue] == elements.end()) {
      // First element for this bucket, just set the bucket pointer.
      newBuckets[newHashValue] = positionNow;
    }
    else {
      // The current head of the bucket chain is received.
      auto oldPointer = newBuckets[newHashValue];

// This element is spliced to come after the current chain - thus moving the element in the list
// without invalidating its iterator.
      elements.splice(oldPointer, elements, positionNow);

// The bucket pointer is updated to point to this element now.
      newBuckets[newHashValue] = positionNow;
    }
  }

// The old buckets array is replaced with the new one.
  std::swap(buckets, newBuckets);

}

std::size_t HashSet::size() const {
  return size_;
}

bool HashSet::empty() const {
  return (size_ == 0);
}

std::size_t HashSet::bucketCount() const {
  return buckets.size();
}


std::size_t HashSet::bucketSize(std::size_t b) const {

// If the bucket index is invalid or the bucket is empty, then 0 is returned.
  if (b >= bucketCount() || buckets[b] == elements.end()) {
    return 0;
  }

// The elements in this bucket are counted by traversing the list. Elements with the 
// same hash value are stored contiguously.
  std::size_t c = 0;
  auto it = buckets[b];

  while (it != elements.end() && bucket(*it) == b) {
    c++; // Magical moment here (if you know, you know). 
    ++it;
  }

  return c;

}

std::size_t HashSet::bucket(int key) const {
  return (key % buckets.size());
}

float HashSet::loadFactor() const {
  if (bucketCount() == 0) {
    return 0.0f;
  }
  return (static_cast<float>(size()) / bucketCount());
}

float HashSet::maxLoadFactor() const {
  return max_load_factor_;
}


void HashSet::maxLoadFactor(float maxLoad) {
  max_load_factor_ = maxLoad;

// If the current load factor exceeds the new maximum, then reshash immediately. 
  if (loadFactor() > max_load_factor_) {
    std::size_t reqBuckets = std::ceil(size_ / max_load_factor_);
    rehash(reqBuckets);
  }
}
