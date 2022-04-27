/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
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

namespace hlslib {

// Time in seconds until a blocking call on a stream should timeout and emit a
// warning before going back to sleep
#ifdef HLSLIB_STREAM_TIMEOUT
constexpr int kSecondsToTimeout = HLSLIB_STREAM_TIMEOUT;
#else
constexpr int kSecondsToTimeout = 3;
#endif

// This macro must be defined when synthesizing:
// #ifdef HLSLIB_SYNTHESIS
// Synthesis will fail otherwise.

// If the macro HLSLIB_STREAM_SYNCHRONIZE is set, every stream object will only
// allow read and write ATTEMPTS in lockstep. This way, no single dataflow
// function will run ahead of others.
// This is left optional, as it will not work for all kernels, and can result in
// deadlocks. If such a situation occurs, the synchronization method will print
// a deadlock warning to stderr after a few seconds.

#ifdef HLSLIB_DEBUG_STREAM
constexpr bool kStreamVerbose = true;
#else
constexpr bool kStreamVerbose = false;
#endif

/// Instruct the HLS tool to implement the FIFO using a specific resource.
enum class Storage {
  Unspecified,  // Let the tool decide
  BRAM,
  LUTRAM,
  SRL
};

// Forward declaration.
template <typename T, size_t depth = 0, Storage = Storage::Unspecified>
class Stream;

#ifndef HLSLIB_SYNTHESIS
/// Set the name of a stream. Useful when initializing arrays of streams that
/// cannot be constructed individually. Currently only raw strings are
/// supported.
template <typename T>
void SetName(Stream<T> &stream, const char *name) {
  stream.set_name(name);
}
#else
template <typename T>
void SetName(Stream<T> &, const char *) {}
#endif

/// For internal use. Allows containers of streams of different types and
/// depths.
class _StreamBase {};

/// Custom stream implementation, implementing thread-safe and blocking Push
/// and Pop, as well as read/write conforming to the hls::stream interface.
/// The depth argument specifies the maximum depth of the stream, which will be
/// reflected both in hardware and in simulation.
template <typename T>
class Stream<T, 0, Storage::Unspecified> {
 public:

  Stream() : Stream("(unnamed)") {
    #pragma HLS INLINE
  }

  // Setting a depth of 0 will be interpreted as a depth of 2 in simulation,
  // but the underlying hls::stream object will receive 0, indicating that it
  // was undefined. The generic implementation below can override this with the
  // specified value.
  Stream(char const *const name) : Stream(name, 2, Storage::Unspecified) {
    #pragma HLS INLINE
  }

 protected:
#ifdef HLSLIB_SYNTHESIS
#if defined(__VIVADO_HLS__) && !defined(__VITIS_HLS)
  Stream(char const *const name, const size_t depth, const Storage storage)
      : stream_(name) {
    #pragma HLS INLINE
    #pragma HLS STREAM variable=stream_ depth=depth
    if (storage == Storage::BRAM) {
      #pragma HLS RESOURCE variable=stream_ core=FIFO_BRAM
    } else if (storage == Storage::LUTRAM) {
      #pragma HLS RESOURCE variable=stream_ core=FIFO_LUTRAM
    } else if (storage == Storage::SRL) {
      #pragma HLS RESOURCE variable=stream_ core=FIFO_SRL
    }
  }
#else  // Assume we're using vitis_hls
  // The name constructor is broken in Vitis 2020.1
  Stream(char const *const, size_t, Storage) : stream_() {
    // Setting depth and storage are doned from the derived class
    #pragma HLS INLINE
  }
#endif
#else
  Stream(char const *const name, size_t depth, Storage)
      : name_(name), depth_(depth) {}
#endif  // !HLSLIB_SYNTHESIS

  // Streams represent hardware entities. Don't allow copy or assignment.
  Stream(Stream<T> const &) = delete;
  Stream(Stream<T> &&) = delete;
  Stream<T> &operator=(Stream<T> const &) = delete;
  Stream<T> &operator=(Stream<T> &&) = delete;

 public:

  ~Stream() {
#ifndef HLSLIB_SYNTHESIS
    // Don't throw exceptions during destruction. Resort to printing to stderr
    if (queue_.size() > 0) {
      std::cerr << name_ << " contained " << queue_.size()
                << " elements at destruction.\n";
    }
#endif
  }

  /////////////////////////////////////////////////////////////////////////////

  /// Primary interface to pushing elements to the stream. Blocks if the stream
  /// is full, both in hardware and in simulation.
  void Push(T const &val) {
    #pragma HLS INLINE
    WriteBlocking(val);
  }

  /// Primary interface to popping elements from the stream. Blocks if the
  /// stream is empty, both in hardware and in simulation.
  T Pop() {
    #pragma HLS INLINE
    return ReadBlocking();
  }

  /////////////////////////////////////////////////////////////////////////////
  // Compatibility functions to comply to the hls::stream interface.
  /////////////////////////////////////////////////////////////////////////////

  /// Compatibility with hls::stream interface.
  T read() {
    #pragma HLS INLINE
    return ReadBlocking();
  }

  /// Compatibility with hls::stream interface.
  void read(T &out) {
    #pragma HLS INLINE
    out = read();
  }

  /// Compatibility with hls::stream interface.
  void write(T const &val) {
    #pragma HLS INLINE
    WriteBlocking(val);
  }

  /// Compatibility with hls::stream interface.
  bool empty() const {
    #pragma HLS INLINE
    return IsEmpty();
  }

  /// Compatibility with hls::stream interface.
  bool full() const {
    #pragma HLS INLINE
    return IsFull();
  }

  /////////////////////////////////////////////////////////////////////////////
  // Below are more exotic interfaces to hlslib::Stream. You probably don't
  // need these, but they can be useful in specific scenarios.
  /////////////////////////////////////////////////////////////////////////////

  /// Equivalent to Pop().
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
#endif  // !HLSLIB_SYNTHESIS
  }

  /// Attempts to read from a stream, returning whether a value was read or not.
  /// Useful when a different action should be performed when the stream is
  /// empty.
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
#endif  // !HLSLIB_SYNTHESIS
  }

  /// Attempt to read from a stream, throwing an exception if no value is
  /// available when run in simulation. Useful for sanity checking internal
  /// buffers and some synchronized dataflow applications.
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
#endif  // !HLSLIB_SYNTHESIS
  }

  /// Equivalent to Push().
  void WriteBlocking(T const &val) {
#ifndef HLSLIB_SYNTHESIS
    WriteBlocking(val, depth_);
#else
    #pragma HLS INLINE
    WriteBlocking(val, std::numeric_limits<int>::max());
#endif
  }

  /// Non-blocking interface.
  bool WriteNonBlocking(T const &val) {
#ifndef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return WriteNonBlocking(val, depth_);
#else
    return WriteNonBlocking(val, std::numeric_limits<int>::max());
#endif
  }

  void WriteOptimistic(T const &val) {
    #pragma HLS INLINE
#ifndef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return WriteOptimistic(val, depth_);
#else
    return WriteOptimistic(val, std::numeric_limits<int>::max());
#endif
  }

#ifdef HLSLIB_SYNTHESIS
  void WriteOptimistic(T const &val, size_t) {
    #pragma HLS INLINE
    stream_.write(val);
  }
#else
  /// Attempt to write to a stream, throwing an exception if the stream is full
  /// in simulation. Useful for sanity checking internal buffers and some
  /// synchronized dataflow applications.
  void WriteOptimistic(T const &val, size_t depth) {
    std::unique_lock<std::mutex> lock(mutex_);
    WriteSynchronize(lock);
    if (queue_.size() == depth) {
      throw std::runtime_error(std::string(name_) + ": written while full.");
    }
    queue_.emplace(val);
    cvRead_.notify_all();
  }
#endif  // !HLSLIB_SYNTHESIS

  bool IsEmpty() const {
#ifdef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return stream_.empty();
#else
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() == 0;
#endif
  }

  bool IsFull() const {
#ifndef HLSLIB_SYNTHESIS
    #pragma HLS INLINE
    return IsFull(depth_);
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
  std::string const &name() const { return name_; }
#endif

#ifndef HLSLIB_SYNTHESIS
  void set_name(char const *const name) {
    name_ = name;
#else
  void set_name(char const *const) {
#endif
  }

  /////////////////////////////////////////////////////////////////////////////

 private:
#ifndef HLSLIB_SYNTHESIS
  void ReadSynchronize(std::unique_lock<std::mutex> &lock) {
    (void)lock;  // Silence unused warning
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
#endif  // !HLSLIB_SYNTHESIS
  }
#endif

#ifndef HLSLIB_SYNTHESIS
  void WriteSynchronize(std::unique_lock<std::mutex> &lock) {
    (void)lock;  // Silence unused warning
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

#ifdef HLSLIB_SYNTHESIS
  void WriteBlocking(T const &val, size_t) {
    #pragma HLS INLINE
    stream_.write(val);
  }
#else
  void WriteBlocking(T const &val, size_t depth) {
    std::unique_lock<std::mutex> lock(mutex_);
    WriteSynchronize(lock);
    bool slept = false;
    while (queue_.size() == depth) {
      if (kStreamVerbose && !slept) {
        std::stringstream ss;
        ss << name_ << " full [" << queue_.size() << "/" << depth
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
      ss << name_ << " full [" << queue_.size() << "/" << depth
         << " elements, woke up].\n";
      std::cout << ss.str();
    }
    queue_.emplace(val);
    cvRead_.notify_all();
  }
#endif  // !HLSLIB_SYNTHESIS

#ifdef HLSLIB_SYNTHESIS
  bool WriteNonBlocking(T const &val, size_t) {
    #pragma HLS INLINE
    return stream_.write_nb(val);
  }
#else
  bool WriteNonBlocking(T const &val, size_t depth) {
    std::unique_lock<std::mutex> lock(mutex_);
    WriteSynchronize(lock);
    if (queue_.size() == depth) {
      return false;
    }
    queue_.emplace(val);
    cvRead_.notify_all();
    return true;
  }
#endif

#ifdef HLSLIB_SYNTHESIS
  bool IsFull(size_t) const {
    #pragma HLS INLINE
    return stream_.full();
  }
#else
  bool IsFull(size_t depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() == depth;
  }
#endif

  /////////////////////////////////////////////////////////////////////////////

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
  size_t depth_;
#else
 protected:
  hls::stream<T> stream_;
#endif
};

template <typename T, size_t depth, Storage storage>
class Stream : public Stream<T, 0, storage> {

public:
  Stream() : Stream("(unnamed)") {
    #pragma HLS INLINE
  }

  Stream(char const *const name) : Stream<T, 0, storage>(name, depth, storage) {
    #pragma HLS INLINE
#if !defined(__VIVADO_HLS__) || defined(__VITIS_HLS__)
    #pragma HLS STREAM variable=this->stream_ depth=depth
    if (storage == Storage::BRAM) {
      #pragma HLS bind_storage variable=this->stream_ type=FIFO impl=BRAM
    } else if (storage == Storage::LUTRAM) {
      #pragma HLS bind_storage variable=this->stream_ type=FIFO impl=LUTRAM
    } else if (storage == Storage::SRL) {
      #pragma HLS bind_storage variable=this->stream_ type=FIFO impl=SRL
    }
#endif
  }

  // Streams represent hardware entities. Don't allow copy or assignment.
  Stream(Stream<T> const &) = delete;
  Stream(Stream<T> &&) = delete;
  Stream<T> &operator=(Stream<T> const &) = delete;
  Stream<T> &operator=(Stream<T> &&) = delete;
};

///////////////////////////////////////////////////////////////////////////////

}  // End namespace hlslib
