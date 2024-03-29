// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012 - 2014, 2016 - 2017, 2019 - 2020)
//   René Fritze     (2012 - 2016, 2018 - 2020)
//   Tobias Leibner  (2016, 2020)

#include "config.h"

#include <ostream>

#include <boost/format.hpp>
#include <utility>

#include "memory.hh"
#include "exceptions.hh"
#include "filesystem.hh"
#include "timedlogging.hh"

namespace Dune::XT::Common {


DefaultLogger::DefaultLogger(const std::string& prfx,
                             const std::array<bool, 3>& initial_state,
                             std::array<std::string, 3> colors,
                             bool global_timer)
  : prefix(prfx)
  , state(initial_state)
  , colors_(std::move(colors))
  , global_timer_(global_timer)
  , info_(std::make_shared<TimedPrefixedLogStream>(global_timer_ ? SecondsSinceStartup() : timer_,
                                                   build_prefix(prfx.empty() ? "info" : prfx, copy_count, colors_[0]),
                                                   std::cout))
  , debug_(std::make_shared<TimedPrefixedLogStream>(global_timer_ ? SecondsSinceStartup() : timer_,
                                                    build_prefix(prfx.empty() ? "debug" : prfx, copy_count, colors_[1]),
                                                    std::cout))
  , warn_(std::make_shared<TimedPrefixedLogStream>(global_timer_ ? SecondsSinceStartup() : timer_,
                                                   build_prefix(prfx.empty() ? "warn" : prfx, copy_count, colors_[2]),
                                                   std::cerr))
{
}

DefaultLogger::DefaultLogger(const DefaultLogger& other)
  : prefix(other.prefix)
  , state(other.state)
  , copy_count(other.copy_count + 1)
  , colors_(other.colors_)
  , global_timer_(other.global_timer_)
  , info_(
        std::make_shared<TimedPrefixedLogStream>(global_timer_ ? SecondsSinceStartup() : timer_,
                                                 build_prefix(prefix.empty() ? "info" : prefix, copy_count, colors_[0]),
                                                 std::cout))
  , debug_(std::make_shared<TimedPrefixedLogStream>(
        global_timer_ ? SecondsSinceStartup() : timer_,
        build_prefix(prefix.empty() ? "debug" : prefix, copy_count, colors_[1]),
        std::cout))
  , warn_(
        std::make_shared<TimedPrefixedLogStream>(global_timer_ ? SecondsSinceStartup() : timer_,
                                                 build_prefix(prefix.empty() ? "warn" : prefix, copy_count, colors_[2]),
                                                 std::cerr))
{
}

DefaultLogger& DefaultLogger::operator=(const DefaultLogger& other)
{
  if (&other != this) {
    prefix = other.prefix;
    state = other.state;
    copy_count = other.copy_count;
    timer_ = other.timer_;
    colors_ = other.colors_;
    global_timer_ = other.global_timer_;
    info_ =
        std::make_shared<TimedPrefixedLogStream>(global_timer_ ? SecondsSinceStartup() : timer_,
                                                 build_prefix(prefix.empty() ? "info" : prefix, copy_count, colors_[0]),
                                                 std::cout);
    debug_ = std::make_shared<TimedPrefixedLogStream>(
        global_timer_ ? SecondsSinceStartup() : timer_,
        build_prefix(prefix.empty() ? "debug" : prefix, copy_count, colors_[1]),
        std::cout);
    warn_ =
        std::make_shared<TimedPrefixedLogStream>(global_timer_ ? SecondsSinceStartup() : timer_,
                                                 build_prefix(prefix.empty() ? "warn" : prefix, copy_count, colors_[2]),
                                                 std::cerr);
  }
  return *this;
} // ... operator=(...)

void DefaultLogger::enable(const std::string& prfx)
{
  state = default_logger_state();
  if (!prfx.empty()) {
    prefix = prfx;
    copy_count = 0;
    info_ = std::make_shared<TimedPrefixedLogStream>(
        global_timer_ ? SecondsSinceStartup() : timer_, build_prefix(prfx, copy_count, colors_[0]), std::cout);
    debug_ = std::make_shared<TimedPrefixedLogStream>(
        global_timer_ ? SecondsSinceStartup() : timer_, build_prefix(prfx, copy_count, colors_[1]), std::cout);
    warn_ = std::make_shared<TimedPrefixedLogStream>(
        global_timer_ ? SecondsSinceStartup() : timer_, build_prefix(prfx, copy_count, colors_[2]), std::cerr);
  }
} // ... enable(...)


void DefaultLogger::enable_like(const DefaultLogger& other)
{
  this->state[0] = other.state[0];
  this->state[1] = other.state[1];
  this->state[2] = other.state[2];
}

std::array<bool, 3> DefaultLogger::get_state_or(const std::array<bool, 3>& other_state) const
{
  std::array<bool, 3> ret;
  ret[0] = this->state[0] || other_state[0];
  ret[1] = this->state[1] || other_state[1];
  ret[2] = this->state[2] || other_state[2];
  return ret;
}

std::array<bool, 3> DefaultLogger::get_state_and(const std::array<bool, 3>& other_state) const
{
  std::array<bool, 3> ret;
  ret[0] = this->state[0] && other_state[0];
  ret[1] = this->state[1] && other_state[1];
  ret[2] = this->state[2] && other_state[2];
  return ret;
}

void DefaultLogger::state_or(const std::array<bool, 3>& other_state)
{
  state = get_state_or(other_state);
}

void DefaultLogger::state_and(const std::array<bool, 3>& other_state)
{
  state = get_state_and(other_state);
}

void DefaultLogger::disable()
{
  state = {{false, false, false}};
}

std::ostream& DefaultLogger::info()
{
  if (state[0])
    return *info_;
  return dev_null;
}

std::ostream& DefaultLogger::debug()
{
  if (state[1])
    return *debug_;
  return dev_null;
}

std::ostream& DefaultLogger::warn()
{
  if (state[2])
    return *warn_;
  return dev_null;
}


TimedLogManager::TimedLogManager(const Timer& timer,
                                 const std::string& info_prefix,
                                 const std::string& debug_prefix,
                                 const std::string& warning_prefix,
                                 const ssize_t max_info_level,
                                 const ssize_t max_debug_level,
                                 const bool enable_warnings,
                                 std::atomic<ssize_t>& current_level,
                                 std::ostream& disabled_out,
                                 std::ostream& enabled_out,
                                 std::ostream& warn_out)
  : timer_(timer)
  , current_level_(current_level)
  , info_(std::make_shared<TimedPrefixedLogStream>(
        timer_, info_prefix, current_level_ <= max_info_level ? enabled_out : disabled_out))
  , debug_(std::make_shared<TimedPrefixedLogStream>(timer_,
                                                    debug_prefix,
#if DUNE_XT_COMMON_TIMEDLOGGING_ENABLE_DEBUG
                                                    current_level_ <= max_debug_level ? enabled_out : disabled_out))
#else
                                                    current_level_ <= max_debug_level ? enabled_out : dev_null))
#endif
  , warn_(std::make_shared<TimedPrefixedLogStream>(timer_, warning_prefix, enable_warnings ? warn_out : disabled_out))
{
}

TimedLogManager::~TimedLogManager()
{
  --current_level_;
}

std::ostream& TimedLogManager::info()
{
  return *info_;
}

std::ostream& TimedLogManager::debug()
{
  return *debug_;
}

std::ostream& TimedLogManager::warn()
{
  return *warn_;
}


TimedLogging::TimedLogging()
  : max_info_level_(default_max_info_level)
  , max_debug_level_(default_max_debug_level)
  , enable_warnings_(default_enable_warnings)
  , enable_colors_(default_enable_colors && terminal_supports_color())
  , info_prefix_(enable_colors_ ? default_info_color() : "")
  , debug_prefix_(enable_colors_ ? default_debug_color() : "")
  , warning_prefix_(enable_colors_ ? default_warning_color() : "")
  , info_suffix_(enable_colors_ ? StreamModifiers::normal : "")
  , debug_suffix_(enable_colors_ ? StreamModifiers::normal : "")
  , warning_suffix_(enable_colors_ ? StreamModifiers::normal : "")
  , current_level_(-1)
{
  update_colors();
}

void TimedLogging::create(const ssize_t max_info_level,
                          const ssize_t max_debug_level,
                          const bool enable_warnings,
                          const bool enable_colors,
                          const std::string& info_color,
                          const std::string& debug_color,
                          const std::string& warning_color)
{
  [[maybe_unused]] std::lock_guard<std::mutex> guard(mutex_);
  DUNE_THROW_IF(created_, Exceptions::logger_error, "Do not call create() more than once!");
  max_info_level_ = max_info_level;
  max_debug_level_ = max_debug_level;
  enable_warnings_ = enable_warnings;
  enable_colors_ = enable_colors && terminal_supports_color();
  info_prefix_ = enable_colors_ ? info_color : "";
  debug_prefix_ = enable_colors_ ? debug_color : "";
  warning_prefix_ = enable_colors_ ? warning_color : "";
  created_ = true;
  current_level_ = -1;
  update_colors();
} // ... create(...)

TimedLogManager TimedLogging::get(const std::string& id)
{
  [[maybe_unused]] std::lock_guard<std::mutex> guard(mutex_);
  ++current_level_;
  return TimedLogManager(timer_,
                         info_prefix_ + (id.empty() ? "info" : id) + ": " + info_suffix_,
                         debug_prefix_ + (id.empty() ? "debug" : id) + ": " + debug_suffix_,
                         warning_prefix_ + (id.empty() ? "warn" : id) + ": " + warning_suffix_,
                         max_info_level_,
                         max_debug_level_,
                         enable_warnings_,
                         current_level_);
}

void TimedLogging::update_colors()
{
  if (enable_colors_) {
    info_prefix_ = color(info_prefix_);
    debug_prefix_ = color(debug_prefix_);
    warning_prefix_ = color(warning_prefix_);
    if (info_prefix_.empty())
      info_suffix_ = "";
    else {
      info_prefix_ += StreamModifiers::bold;
      info_suffix_ = StreamModifiers::normal;
    }
    if (debug_prefix_.empty())
      debug_suffix_ = "";
    else {
      debug_prefix_ += StreamModifiers::bold;
      debug_suffix_ = StreamModifiers::normal;
    }
    if (warning_prefix_.empty())
      warning_suffix_ = "";
    else {
      warning_prefix_ += StreamModifiers::bold;
      warning_suffix_ = StreamModifiers::normal;
    }
  }
} // ... update_colors(...)


} // namespace Dune::XT::Common
