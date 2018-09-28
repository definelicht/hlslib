/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      April 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include <cstddef>
#include <limits>
#ifdef HLSLIB_SYNTHESIS
#include <hls_stream.h>
#else
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#endif

// Time in seconds until a blocking call on a stream should timeout and emit a
// warning before going back to sleep
constexpr int kSecondsToTimeout = 3;

// This macro must be defined when synthesizing. Synthesis will fail otherwise.
// #ifdef HLSLIB_SYNTHESIS

// If the macro HLSLIB_STREAM_SYNCHRONIZE is set, every stream object will only
// allow reads and writes ATTEMPTS in lockstep. This way, no single dataflow
// function will run ahead of others.
// This is left optional, as it will not work for all kernels, and can result in
// deadlocks. If such a situation occurs, the synchronization method will print
// a deadlock warning to stderr after a few seconds.

#ifdef HLSLIB_DEBUG_STREAM
constexpr bool kStreamVerbose = true;
#else
constexpr bool kStreamVerbose = false;
#endif

namespace hlslib {

template <typename T, unsigned capacity = 1>
class Stream;

/// Attempt to read from a stream, blocking until a value can be read. Useful
/// for inter-process communication and generally asynchronous behavior.
template <typename T>
T ReadBlocking(Stream<T> &stream) {
  #pragma HLS INLINE
  return stream.ReadBlocking();
}

/// Attempt to read from a stream, throwing an exception if no value is
/// available. Useful for internal buffers and some synchronized dataflow
/// applications.
template <typename T>
T ReadOptimistic(Stream<T> &stream) {
  #pragma HLS INLINE
  return stream.ReadOptimistic();
}

/// Attempts to read from a stream, returning whether a value was read or not. 
/// Useful when a different action should be performed when the stream is empty.
template <typename T>
bool ReadNonBlocking(Stream<T> &stream, T &output) {
  #pragma HLS INLINE
  return stream.ReadNonBlocking(output);
}

/// Attempt to write to a stream, blocking until a value can be written. Useful
/// for inter-process communication and generally asynchronous behavior.
template <typename T>
void WriteBlocking(Stream<T> &stream, T const &val, int size) {
  #pragma HLS INLINE
  stream.WriteBlocking(val, size);
}

/// Attempt to write to a stream, blocking until a value can be written. Useful
/// for inter-process communication and generally asynchronous behavior.
template <typename T>
void WriteBlocking(Stream<T> &stream, T const &val) {
  #pragma HLS INLINE
  stream.WriteBlocking(val);
}

/// Attempt to write to a stream, throwing an exception if the stream is full.
/// Useful for internal buffers and some synchronized dataflow applications.
template <typename T>
void WriteOptimistic(Stream<T> &stream, T const &val, int size) {
  #pragma HLS INLINE
  stream.WriteOptimistic(val, size);
}

/// Attempts to write to a stream, returning whether a value was written or not. 
/// Useful when a different action should be performed when the stream is full.
template <typename T>
bool WriteNonBlocking(Stream<T> &stream, T const &val, int size) {
  #pragma HLS INLINE
  return stream.WriteNonBlocking(val, size);
}

/// Check if the stream is empty. Using this in kernel code essentially
/// implements non-blocking behavior, and should be used with caution.
template <typename T>
bool IsEmpty(Stream<T> &stream) {
  #pragma HLS INLINE
  return stream.IsEmpty();
}

/// If running simulation, returns whether the stream is empty. If running in
/// hardware, this always returns false.
/// This is useful when simulating blocking behavior of non loop-based codes
/// where the entire function is invoked every cycle.
template <typename T>
bool IsEmptySimulationOnly(Stream<T> &stream) {
  #pragma HLS INLINE
  return stream.IsEmpty();
}

/// Check if the stream is full. Using this in kernel code essentially
/// implements non-blocking behavior, and should be used with caution.
template <typename T>
bool IsFull(Stream<T> &stream, int size) {
  #pragma HLS INLINE
  return stream.IsFull(size);
}

/// If running simulation, returns whether the stream is full. If running in
/// hardware, this always returns false.
/// This is useful when simulating blocking behavior of non loop-based codes
/// where the entire function is invoked every cycle.
template <typename T>
bool IsFullSimulationOnly(Stream<T> &stream, int size) {
  #pragma HLS INLINE
  return stream.IsFull(size);
}

#ifndef HLSLIB_SYNTHESIS
/// Set the name of a stream. Useful when initializing arrays of streams that
/// cannot be constructed individually. Currently only raw strings are supported.
template <typename T>
void SetName(Stream<T> &stream, const char *name) {
  stream.set_name(name);
}
#else
template <typename T> void SetName(Stream<T> &stream, const char *) {}
#endif

/// For internal use. Used to store arrays of streams of different types
class _StreamBase {};

/// Custom stream implementation, implementing thread-safe and blocking Push
/// and Pop, as well as read/write conforming to the hls::stream interface.
/// The capacityDefault template parameter is a convenience override used
/// when instantiating arrays of streams, as the capacity cannot be passed
/// to the constructor then.
/// The constructor argument is always favored over the template argument, if
/// specified.
template <typename T, unsigned capacityDefault>
class Stream : public _StreamBase {

public:

  Stream() : Stream("(unnamed)", capacityDefault) {
    #pragma HLS INLINE
  }

  Stream(char const *const name) : Stream(name, capacityDefault) {
    #pragma HLS INLINE
  }

  Stream(size_t capacity) : Stream("(unnamed)", capacity) {
    #pragma HLS INLINE
  }

  Stream(char const *const name, size_t capacity)
#ifdef HLSLIB_SYNTHESIS
      : stream_(name) {
    #pragma HLS INLINE
    #pragma HLS STREAM variable=stream_ depth=capacity 
  }
#else
      : name_(name), capacity_(capacity) {}
#endif // !HLSLIB_SYNTHESIS

  // Streams represent hardware entities. Don't allow copy or assignment.
  Stream(Stream<T> const &) = delete;
  Stream(Stream<T> &&) = delete;
  Stream<T> &operator=(Stream<T> const &) = delete;
  Stream<T> &operator=(Stream<T> &&) = delete;

  ~Stream() {
#ifndef HLSLIB_SYNTHESIS
    if (queue_.size() > 0) {
      std::cerr << name_ << " contained " << queue_.size()
                << " elements at destruction.\n";
    }
#endif
  }

#ifndef HLSLIB_SYNTHESIS
  void ReadSynchronize(std::unique_lock<std::mutex> &lock) {
    (void)lock; // Silence unused warning
#ifdef HLSLIB_STREAM_SYNCHRONIZE
    while (!readNext_) {
      if (cvSync_.wait_for(lock, std::chrono::seconds(kSecondsToTimeout)) ==
          std::cv_status::timeout) {
        std::stringstream ss;
        ss << "Stream synchronization stuck on reading \"" << name_
           << "\". Possibly a deadlock?" << std::endl;
        std::cerr << ss.str();
      }
    }
    readNext_ = false;
    cvSync_.notify_all();
#endif // !HLSLIB_SYNTHESIS
  }
#endif

  T ReadBlocking() {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return stream_.read();
#else
    std::unique_lock<std::mutex> lock(mutex_);
    ReadSynchronize(lock);
    bool slept = false;
    while (queue_.empty()) {
      if (kStreamVerbose && !slept) {
        std::stringstream ss;
        ss << name_ << " empty [sleeping].\n";
        std::cout << ss.str();
      }
      slept = true;
      if (cvRead_.wait_for(lock, std::chrono::seconds(kSecondsToTimeout)) ==
          std::cv_status::timeout) {
        std::stringstream ss;
        ss << "Stream \"" << name_
           << "\" is stuck as being EMPTY. Possibly a deadlock?" << std::endl;
        std::cerr << ss.str();
      }
    }
    if (kStreamVerbose && slept) {
      std::stringstream ss;
      ss << name_ << " empty [woke up].\n";
      std::cout << ss.str();
    }
    auto front = queue_.front();
    queue_.pop();
    cvWrite_.notify_all();
    return front;
#endif // !HLSLIB_SYNTHESIS
  }

  // Compatibility with Vivado HLS interface
  T read() {
    #pragma HLS INLINE
    return ReadOptimistic();
  }

  T Pop() {
    #pragma HLS INLINE
    return ReadBlocking();
  }

  T ReadOptimistic() {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return stream_.read();
#else
    std::unique_lock<std::mutex> lock(mutex_);
    ReadSynchronize(lock);
    if (queue_.empty()) {
      throw std::runtime_error(name_ + ": read while empty.");
    }
    auto front = queue_.front();
    queue_.pop();
    cvWrite_.notify_all();
    return front;
#endif // !HLSLIB_SYNTHESIS
  }

  bool ReadNonBlocking(T &output) {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return stream_.read_nb(output);
#else
    std::unique_lock<std::mutex> lock(mutex_);
    ReadSynchronize(lock);
    if (queue_.empty()) {
      return false;
    }
    output = queue_.front();
    queue_.pop();
    cvWrite_.notify_all();
    return true;
#endif // !HLSLIB_SYNTHESIS
  }

#ifndef HLSLIB_SYNTHESIS
  void WriteSynchronize(std::unique_lock<std::mutex> &lock) {
    (void)lock; // Silence unused warning
#ifdef HLSLIB_STREAM_SYNCHRONIZE
    while (readNext_) {
      if (cvSync_.wait_for(lock, std::chrono::seconds(kSecondsToTimeout)) ==
          std::cv_status::timeout) {
        std::stringstream ss;
        ss << "Stream synchronization stuck on writing \"" << name_
           << "\". Possibly a deadlock?" << std::endl;
        std::cerr << ss.str();
      }
    }
    readNext_ = true;
    cvSync_.notify_all();
#endif
  }
#endif
  
  void WriteBlocking(T const &val, size_t capacity) {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    stream_.write(val);
#else
    std::unique_lock<std::mutex> lock(mutex_);
    WriteSynchronize(lock);
    bool slept = false;
    while (queue_.size() == capacity) {
      if (kStreamVerbose && !slept) {
        std::stringstream ss;
        ss << name_ << " full [" << queue_.size() << "/" << capacity 
           << " elements, sleeping].\n";
        std::cout << ss.str();
      }
      slept = true;
      if (cvWrite_.wait_for(lock, std::chrono::seconds(kSecondsToTimeout)) ==
          std::cv_status::timeout) {
        std::stringstream ss;
        ss << "Stream \"" << name_
           << "\" is stuck as being FULL. Possibly a deadlock?" << std::endl;
        std::cerr << ss.str();
      }
    }
    if (kStreamVerbose && slept) {
      std::stringstream ss;
      ss << name_ << " full [" << queue_.size() << "/" << capacity 
         << " elements, woke up].\n";
      std::cout << ss.str();
    }
    queue_.emplace(val);
    cvRead_.notify_all();
#endif // !HLSLIB_SYNTHESIS
  }

  void WriteBlocking(T const &val) {
#ifndef HLSLIB_SYNTHESIS
    WriteBlocking(val, capacity_);
#else
    #pragma HLS INLINE
    WriteBlocking(val, std::numeric_limits<int>::max());
#endif
  }

  // Compatibility with Vivado HLS interface
  void write(T const &val) {
    #pragma HLS INLINE
    WriteOptimistic(val, std::numeric_limits<int>::max());
  }

  void Push(T const &val) {
    #pragma HLS INLINE
    WriteBlocking(val);
  }
  
  void WriteOptimistic(T const &val, size_t capacity) {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    stream_.write(val);
#else
    std::unique_lock<std::mutex> lock(mutex_);
    WriteSynchronize(lock);
    if (queue_.size() == capacity) {
      throw std::runtime_error(std::string(name_) + ": written while full.");
    }
    queue_.emplace(val);
    cvRead_.notify_all();
#endif // !HLSLIB_SYNTHESIS
  }

  void WriteOptimistic(T const &val) {
#ifndef HLSLIB_SYNTHESIS
    WriteOptimistic(val, capacity_);
#else
    #pragma HLS INLINE
    WriteOptimistic(val, std::numeric_limits<int>::max());
#endif
  }

  bool WriteNonBlocking(T const &val, size_t capacity) {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return stream_.write_nb(val);
#else
    std::unique_lock<std::mutex> lock(mutex_);
    WriteSynchronize(lock);
    if (queue_.size() == capacity) {
      return false;
    }
    queue_.emplace(val);
    cvRead_.notify_all();
    return true; 
#endif
  }

  bool WriteNonBlocking(T const &val) {
#ifndef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return WriteNonBlocking(val, capacity_); 
#else
    return WriteNonBlocking(val, std::numeric_limits<int>::max());
#endif
  }

  bool IsEmpty() const {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return stream_.empty();
#else
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() == 0;
#endif
  }

  bool IsFull(size_t capacity) const {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return stream_.full();
#else
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() == capacity;
#endif
  }

  bool IsFull() const {
#ifndef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return IsFull(capacity_);
#else
    return IsFull(std::numeric_limits<int>::max());
#endif
  }

  size_t Size() const {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return stream_.size();
#else
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
#endif
  }

#ifndef HLSLIB_SYNTHESIS
  std::string const &name() const {
    return name_;
  }
#endif

  void set_name(char const *const name) {
#ifndef HLSLIB_SYNTHESIS
    name_ = name;
#endif
  }

private:
#ifndef HLSLIB_SYNTHESIS
  mutable std::mutex mutex_{};
  mutable std::condition_variable cvRead_{};
  mutable std::condition_variable cvWrite_{};
#ifdef HLSLIB_STREAM_SYNCHRONIZE
  mutable std::condition_variable cvSync_{};
  bool readNext_{false};
#endif
  std::queue<T> queue_;
  std::string name_;
  size_t capacity_;
#else
  hls::stream<T> stream_;
#endif
};

} // End namespace hlslib
