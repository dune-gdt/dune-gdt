// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2013 - 2020)
//   René Fritze     (2014 - 2019)
//   Tim Keil        (2018)
//   Tobias Leibner  (2014, 2017 - 2020)

/// \file
/// \brief The colorful DUNE_THROW macro and the dune-xt exception hierarchy.

#ifndef DUNE_XT_COMMON_EXCEPTIONS_HH
#define DUNE_XT_COMMON_EXCEPTIONS_HH

#include <dune/common/exceptions.hh>
#include <dune/common/parallel/mpihelper.hh>

#include <dune/xt/common/color.hh>

#ifdef DUNE_THROW
#  undef DUNE_THROW
#endif


/**
 *  \brief Macro to throw a colorfull exception.
 *
 *  Example:
\code
#include <dune/xt/common/exceptions.hh>

if (a.size() != b.size())
  DUNE_THROW(Exceptions::shapes_do_not_match,
             "size of a (" << a.size() << ") does not match the size of b (" << b.size() << ")!");
\endcode
 *  This macro is essentially copied from dune-common with added color functionality and rank information.
 *  \param  E Exception class, derived from Dune::Exception.
 *  \param  m Message in ostream notation.
 *  \see Dune::Exception
 */
#define DUNE_THROW(E, m)                                                                                               \
  do {                                                                                                                 \
    const std::string th__red = Dune::XT::Common::terminal_supports_color() ? "\033[31m" : "";                         \
    const std::string th__brown = Dune::XT::Common::terminal_supports_color() ? "\033[33m" : "";                       \
    const std::string th__clear = Dune::XT::Common::terminal_supports_color() ? "\033[0m" : "";                        \
    E th__ex;                                                                                                          \
    std::ostringstream th__msg;                                                                                        \
    th__msg << m;                                                                                                      \
    std::ostringstream th__out;                                                                                        \
    th__out << th__red << #E << th__clear;                                                                             \
    if (Dune::MPIHelper::getCommunication().size() > 1)                                                                \
      th__out << " (on rank " << Dune::MPIHelper::getCommunication().rank() << ")";                                    \
    th__out << "\n";                                                                                                   \
    th__out << th__brown << "[" << th__clear;                                                                          \
    th__out << th__red << __func__ << th__clear;                                                                       \
    th__out << th__brown << "|" << th__clear;                                                                          \
    th__out << __FILE__ << th__brown << ":" << th__clear << th__red << __LINE__ << th__clear << th__brown << "]"       \
            << th__clear;                                                                                              \
    if (!th__msg.str().empty())                                                                                        \
      th__out << "\n" << th__brown << "=>" << th__clear << " " << th__msg.str();                                       \
    th__ex.message(th__out.str());                                                                                     \
    throw th__ex;                                                                                                      \
  } while (0)
// DUNE_THROW


#define DUNE_THROW_IF(condition, E, m)                                                                                 \
  do {                                                                                                                 \
    if (condition) {                                                                                                   \
      DUNE_THROW(E, m);                                                                                                \
    }                                                                                                                  \
  } while (0)

#ifndef NDEBUG
#  define DEBUG_THROW_IF(condition, E, m) DUNE_THROW_IF(condition, E, m)
#else
#  define DEBUG_THROW_IF(condition, E, m) (void)0
#endif


namespace Dune {
namespace XT {
namespace Common {
namespace Exceptions {


/// \brief Thrown when a CRTP interface method is not implemented by the derived class.
class CRTP_check_failed : public Dune::Exception
{};

/// \brief Thrown when the shapes (sizes/dimensions) of two containers do not match.
class shapes_do_not_match : public Dune::Exception
{};

/// \brief Thrown when an index lies outside the valid range.
class index_out_of_range : public Dune::Exception
{};

/// \brief Thrown when an interface is used in a way that violates its contract.
class you_are_using_this_wrong : public Dune::Exception
{};

/// \brief Thrown when the input provided to a function is invalid.
class wrong_input_given : public you_are_using_this_wrong
{};

/// \brief Thrown when the requirements (preconditions) of an operation are not met.
class requirements_not_met : public you_are_using_this_wrong
{};

/// \brief Thrown when a Configuration is malformed or contains invalid entries.
class configuration_error : public Dune::Exception
{};

/// \brief Thrown when a conversion between types or representations fails.
class conversion_error : public Dune::Exception
{};

/// \brief Thrown when a computed result does not match the expected one.
class results_are_not_as_expected : public Dune::Exception
{};

/// \brief Thrown on an internal (library-side) error that should not normally occur.
class internal_error : public Dune::Exception
{};

/// \brief Thrown when an external dependency or library reports an error.
class external_error : public Dune::Exception
{};

/// \brief Thrown when a method that must be implemented by the derived class is missing.
class you_have_to_implement_this : public Dune::NotImplemented
{};

/// \brief Thrown when a debug assertion (DXT_ASSERT) fails.
class debug_assertion : public Dune::Exception
{};

/// \brief Thrown when a bisection algorithm fails to converge or is given invalid input.
class bisection_error : public wrong_input_given
{};

/// \brief Thrown when a Parameter is invalid or does not match the expected ParameterType.
class parameter_error : public Dune::Exception
{};

/// \brief Thrown when a required (often optional) dependency is missing.
class dependency_missing : public Dune::Exception
{};

/// \brief Thrown when an assumed-to-be-unreachable code path is taken.
class this_should_not_happen : public Dune::InvalidStateException
{};

/// \brief Thrown when the logging facilities are misconfigured or misused.
class logger_error : public Dune::Exception
{};


} // namespace Exceptions


/// \brief Prints a Dune::Exception and returns a non-zero exit code, for use in main()-level catch blocks.
int handle_exception(const Dune::Exception& exp);

/// \brief Prints a std::exception and returns a non-zero exit code, for use in main()-level catch blocks.
int handle_exception(const std::exception& exp);


} // namespace Common
} // namespace XT
} // namespace Dune

#endif // DUNE_XT_COMMON_EXCEPTIONS_HH
