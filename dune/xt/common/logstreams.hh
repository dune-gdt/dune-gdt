// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012 - 2014, 2016 - 2017)
//   René Fritze     (2012 - 2016, 2018 - 2020)
//   Sven Kaulmann   (2014)
//   Tobias Leibner  (2018, 2020)

#ifndef DUNE_XT_COMMON_LOGSTREAMS_HH
#define DUNE_XT_COMMON_LOGSTREAMS_HH

#include <ostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <type_traits>
#include <mutex>
#include <list>

#include <dune/common/timer.hh>

#include <dune/xt/common/memory.hh>

namespace Dune::XT::Common {


enum LogFlags
{
  LOG_NONE = 1,
  LOG_ERROR = 2,
  LOG_INFO = 4,
  LOG_DEBUG = 8,
  LOG_CONSOLE = 16,
  LOG_FILE = 32,
  LOG_NEXT = 64
};
static constexpr auto LogMax = LOG_INFO | LOG_ERROR | LOG_DEBUG | LOG_CONSOLE | LOG_FILE;
static constexpr auto LogDefault = LOG_INFO | LOG_ERROR | LOG_CONSOLE;

class CombinedBuffer;

class SuspendableStrBuffer : public std::basic_stringbuf<char, std::char_traits<char>>
{
  using BaseType = std::basic_stringbuf<char, std::char_traits<char>>;

public:
  using PriorityType = int;
  static constexpr PriorityType default_suspend_priority = 0;

  SuspendableStrBuffer(int loglevel, int& logflags);
  SuspendableStrBuffer(const SuspendableStrBuffer&) = delete;

  /** \brief stop accepting input into the buffer
   * the suspend_priority_ mechanism provides a way to silence streams from 'higher' modules
   * no-op if already suspended
   ***/
  void suspend(PriorityType priority);

  /** \brief start accepting input into the buffer again
   * no-op if not suspended
   ***/
  void resume(PriorityType priority);

  int pubsync();

protected:
  friend class CombinedBuffer;
  std::streamsize xsputn(const char_type* s, std::streamsize count) override;
  int_type overflow(int_type ch = traits_type::eof()) override;

private:
  inline bool enabled() const
  {
    return (!is_suspended_) && ((logflags_ & loglevel_) != 0);
  }

  int& logflags_;
  int loglevel_;
  int suspended_logflags_;
  bool is_suspended_;
  PriorityType suspend_priority_;
  std::mutex mutex_;
}; // class SuspendableStrBuffer


class OstreamBuffer : public SuspendableStrBuffer
{
public:
  OstreamBuffer(int loglevel, int& logflags, std::ostream& out)
    : SuspendableStrBuffer(loglevel, logflags)
    , out_(out)
  {
  }

private:
  std::ostream& out_;
  std::mutex sync_mutex_;

protected:
  int sync() override;
}; // class FileBuffer

class CombinedBuffer : public SuspendableStrBuffer
{
public:
  CombinedBuffer(int loglevel, int& logflags, std::initializer_list<SuspendableStrBuffer*> buffer_input)
    : SuspendableStrBuffer(loglevel, logflags)
  {
    for (auto&& buffer_ptr : buffer_input) {
      buffer_.emplace_back(buffer_ptr);
    }
  }

  int pubsync();

protected:
  std::streamsize xsputn(const char_type* s, std::streamsize count) override;
  int_type overflow(int_type ch = traits_type::eof()) override;
  int sync() override;

private:
  std::list<std::unique_ptr<SuspendableStrBuffer>> buffer_;
}; // class FileBuffer

using FileBuffer = OstreamBuffer;

class EmptyBuffer : public SuspendableStrBuffer
{
public:
  EmptyBuffer(int loglevel, int& logflags)
    : SuspendableStrBuffer(loglevel, logflags)
  {
  }

protected:
  int sync() override;
}; // class EmptyBuffer

/**
 * \brief A stream buffer to be used in TimedPrefixedLogStream.
 *
 * \note Most likely you do not want to use this class directly, but TimedPrefixedLogStream instead.
 */
class TimedPrefixedStreamBuffer : public std::basic_stringbuf<char, std::char_traits<char>>
{
  using BaseType = std::basic_stringbuf<char, std::char_traits<char>>;

public:
  TimedPrefixedStreamBuffer(const Timer& timer, std::string prefix, std::ostream& out = std::cout);
  TimedPrefixedStreamBuffer(const TimedPrefixedStreamBuffer&) = delete;

  int sync() override;

private:
  std::string elapsed_time_str() const;

  const Timer& timer_;
  const std::string prefix_;
  std::ostream& out_;
  bool prefix_needed_;
  std::mutex mutex_;
}; // class TimedPrefixedStreamBuffer

class LogStream
  : StorageProvider<SuspendableStrBuffer>
  , public std::basic_ostream<char, std::char_traits<char>>
{
  using StorageBaseType = StorageProvider<SuspendableStrBuffer>;
  using BaseType = std::basic_ostream<char, std::char_traits<char>>;

public:
  using PriorityType = SuspendableStrBuffer::PriorityType;
  static constexpr PriorityType default_suspend_priority = SuspendableStrBuffer::default_suspend_priority;

  explicit LogStream(SuspendableStrBuffer*&& buffer)
    : StorageBaseType(std::move(buffer))
    , BaseType(&this->access())
  {
  }

  ~LogStream() override
  {
    flush();
  }

  //! dump buffer into file/stream and clear it
  LogStream& flush();

  /** \brief forwards suspend to buffer
   * the suspend_priority_ mechanism provides a way to silence streams from 'higher' modules
   * no-op if already suspended
   ***/
  void suspend(PriorityType priority = default_suspend_priority)
  {
    this->access().suspend(priority);
  }

  /** \brief start accepting input into the buffer again
   * no-op if not suspended
   ***/
  void resume(PriorityType priority = default_suspend_priority)
  {
    this->access().resume(priority);
  } // Resume
}; // LogStream

/**
 * \brief A std::ostream compatible stream that begins every line by printing elapsed time and prefix.
 *
 *        Given a Timer, a prefix and a std::ostream compatible stream this class prints any input to the given
 *        stream prepending each line by the elapsed time and the given prefix:
\code
// the following code
Timer timer;
TimedPrefixedLogStream out(timer, "prefix: ", std::cout);
out << "sample\nline" << std::flush;
// .. do something that takes 2 seconds
out << "\n" << 3 << "\n\nend" << std::endl;
// should give the following output:
00:00|prefix: sample
00:00|prefix: line
00:02|prefix: 3
00:02|prefix:
00:02|prefix: end

\endcode
 *
 * \note This class is intended to be used by TimedLogManager.
 */
class TimedPrefixedLogStream
  : StorageProvider<TimedPrefixedStreamBuffer>
  , public std::basic_ostream<char, std::char_traits<char>>
{
  using StorageBaseType = StorageProvider<TimedPrefixedStreamBuffer>;
  using OstreamBaseType = std::basic_ostream<char, std::char_traits<char>>;

public:
  TimedPrefixedLogStream(const Timer& timer, const std::string& prefix, std::ostream& outstream);

  ~TimedPrefixedLogStream() override;
}; // TimedPrefixedLogStream

//! ostream compatible class wrapping file and console output
class OstreamLogStream : public LogStream
{
public:
  OstreamLogStream(int loglevel, int& logflags, std::ostream& out);
}; // class OstreamLogStream

//! ostream compatible class wrapping file and console output
class DualLogStream : public LogStream
{
public:
  DualLogStream(int loglevel, int& logflags, std::ostream& out, std::ofstream& file);
}; // class OstreamLogStream

//! /dev/null
class EmptyLogStream : public LogStream
{
public:
  explicit EmptyLogStream(int& logflags);
}; // class EmptyLogStream

namespace {
int dev_null_logflag;
EmptyLogStream dev_null(dev_null_logflag);
} // namespace


} // namespace Dune::XT::Common

#endif // LOGSTREAMS_HH
