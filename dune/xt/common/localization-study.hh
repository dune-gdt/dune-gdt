// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2014, 2016 - 2017)
//   René Fritze     (2015 - 2016, 2018 - 2020)
//   Tobias Leibner  (2020)

/// \file
/// \brief Provides the LocalizationStudy base class for comparing localization indicators against a reference.

#ifndef DUNE_XT_COMMON_LOCALIZATION_STUDY_HH
#define DUNE_XT_COMMON_LOCALIZATION_STUDY_HH

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <dune/common/dynvector.hh>

#include <dune/xt/common/exceptions.hh>

namespace Dune::XT::Common {

/// \brief Base class to compare provided localization indicators against reference indicators.
class LocalizationStudy
{
public:
  /// \brief Constructs the study, optionally restricting it to the given indicators.
  LocalizationStudy(std::vector<std::string> only_these_indicators = {});

  virtual ~LocalizationStudy();

  /// \brief Returns a name identifying this study.
  virtual std::string identifier() const = 0;

  /// \brief Computes the reference indicators the provided indicators are compared against.
  virtual DynamicVector<double> compute_reference_indicators() const = 0;

  /// \brief Returns the names of all indicators provided by this study.
  virtual std::vector<std::string> provided_indicators() const = 0;

  /// \brief Computes the indicator of the given type.
  virtual DynamicVector<double> compute_indicators(const std::string type) const = 0;

  /// \brief Returns the provided indicators actually used by this study.
  std::vector<std::string> used_indicators() const;

  /// \brief Runs the study and prints the comparison of each indicator against the reference to out.
  /*std::map< std::string, std::vector< double > >*/ void run(std::ostream& out = std::cout) const;

private:
  const std::vector<std::string> only_these_indicators_;
}; // class LocalizationStudy

} // namespace Dune::XT::Common

#endif // DUNE_XT_COMMON_LOCALIZATION_STUDY_HH
