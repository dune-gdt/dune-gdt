// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2016 - 2017)
//   Rene Milk       (2016 - 2018)
//   Tobias Leibner  (2016 - 2017)

#ifndef DUNE_GDT_TIMESTEPPER_INTERFACE_HH
#define DUNE_GDT_TIMESTEPPER_INTERFACE_HH

#include <cstdio>
#include <utility>

#include <dune/xt/common/memory.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/common/tuple.hh>

#include <dune/xt/la/container.hh>

#include <dune/xt/functions/interfaces/function.hh>
#include <dune/xt/functions/interfaces/grid-function.hh>
#include <dune/xt/functions/generic/grid-function.hh>
#include <dune/xt/functions/visualization.hh>

#include <dune/gdt/operators/interfaces.hh>
#include <dune/gdt/discretefunction/default.hh>

#include "enums.hh"

namespace Dune {
namespace GDT {
namespace internal {


struct FloatCmpLt
{
  template <class A, class B>
  bool operator()(const A& a, const B& b) const
  {
    return Dune::XT::Common::FloatCmp::lt(a, b);
  }
};


} // namespace internal


template <class DiscreteFunctionImp>
class TimeStepperInterface
  : Dune::XT::Common::StorageProvider<DiscreteFunctionImp>
  , Dune::XT::Common::StorageProvider<std::map<typename DiscreteFunctionImp::RangeFieldType,
                                               std::unique_ptr<DiscreteFunctionImp>,
                                               typename internal::FloatCmpLt>>
{
public:
  using DiscreteFunctionType = DiscreteFunctionImp;
  using GridViewType = typename DiscreteFunctionType::SpaceType::GridViewType;
  using EntityType = typename GridViewType::template Codim<0>::Entity;
  using DomainFieldType = typename DiscreteFunctionType::DomainFieldType;
  using RangeFieldType = typename DiscreteFunctionType::RangeFieldType;
  static constexpr size_t dimDomain = DiscreteFunctionType::d;
  static constexpr size_t dimRange = DiscreteFunctionType::r;
  static constexpr size_t dimRangeCols = DiscreteFunctionType::rC;
  using DomainType = typename DiscreteFunctionType::LocalFunctionType::DomainType;
  using RangeType = typename DiscreteFunctionType::LocalFunctionType::RangeType;
  using DiscreteSolutionType =
      typename std::map<RangeFieldType, std::unique_ptr<DiscreteFunctionType>, typename internal::FloatCmpLt>;
  using GridFunctionType = XT::Functions::GridFunctionInterface<EntityType, dimRange, dimRangeCols, RangeFieldType>;
  using DataHandleType = DiscreteFunctionDataHandle<DiscreteFunctionType>;
  using VisualizerType = XT::Functions::VisualizerInterface<dimRange, dimRangeCols, RangeFieldType>;
  using StringifierType = std::function<std::string(const RangeType&)>;
  using ThisType = TimeStepperInterface;

private:
  using CurrentSolutionStorageProviderType = typename Dune::XT::Common::StorageProvider<DiscreteFunctionImp>;
  using SolutionStorageProviderType = typename Dune::XT::Common::StorageProvider<DiscreteSolutionType>;

protected:
  TimeStepperInterface(const RangeFieldType t_0, DiscreteFunctionType& initial_values)
    : CurrentSolutionStorageProviderType(initial_values)
    , SolutionStorageProviderType(new DiscreteSolutionType())
    , t_(t_0)
    , u_n_(&CurrentSolutionStorageProviderType::access())
    , solution_(&SolutionStorageProviderType::access())
  {
  }

public:
  TimeStepperInterface(const ThisType& other) = delete;
  TimeStepperInterface(ThisType&& source) = delete;
  ThisType& operator=(const ThisType& other) = delete;
  ThisType& operator=(ThisType&& source) = delete;

  virtual ~TimeStepperInterface() = default;

  /**
   * \brief Perform one time step and return the estimated optimal time step length for the next time step.
   * \param dt Time step length.
   * \param max_dt Maximal allowed time step length in this time step.
   * \return Estimated optimal time step length for the next time step.
   * \note For adaptive methods or if max_dt < dt, dt is just the initial time step length, the actual time step taken
   * may be shorter. To get the actual time step length you need to compare current_time() before and after calling
   * step().
   * \note This method has to increase the current time by the actual time step length taken, otherwise
   * solve will never finish execution (if current_time is not increased) or give wrong results (if current time is
   * increased by a wrong dt).
   */
  virtual RangeFieldType step(const RangeFieldType dt, const RangeFieldType max_dt) = 0;

  const RangeFieldType& current_time() const
  {
    return t_;
  }

  RangeFieldType& current_time()
  {
    return t_;
  }

  const DiscreteFunctionType& current_solution() const
  {
    return *u_n_;
  }

  DiscreteFunctionType& current_solution()
  {
    return *u_n_;
  }

  void set_current_solution_pointer(DiscreteFunctionType& discrete_function)
  {
    u_n_ = &discrete_function;
  }

  const DiscreteSolutionType& solution() const
  {
    return *solution_;
  }

  DiscreteSolutionType& solution()
  {
    return *solution_;
  }

  void set_solution_pointer(DiscreteSolutionType& solution_ref)
  {
    solution_ = &solution_ref;
  }

  static const GridFunctionType& dummy_solution()
  {
    static auto dummy_sol = XT::Functions::GenericGridFunction<EntityType, dimRange, dimRangeCols, RangeFieldType>(0);
    return dummy_sol;
  }

  /**
   * \brief solve Applies time stepping scheme up to time t_end
   * \param t_end Final time.
   * \param initial_dt Initial time step length. Non-adaptive schemes will use this dt for all time steps. Adaptive
   * schemes will use initial_dt as a first guess for the time step length.
   * \param num_save_steps Number of time points that will be stored/visualized/written Default is size_t(-1), which
   * will store all time steps.
   * \param num_output_steps Number of time points where current time and current time step length will be written to
   * std::cout. Default is size_t(-1), which will output all time steps. Set to 0 to suppress output.
   * \param save_solution If true, the discrete solution at num_save_steps equidistant time points will be stored in
   * solution.
   * \param visualize If true, solution will be written to .vtp/.vtu files at num_save_steps equidistant time points
   * \param write_discrete If true, discrete solution will be written to .txt files at num_save_steps equidistant time
   * points
   * \param write_exact If true, exact solution will be written to .txt files at num_save_steps equidistant time points
   * \param prefix Filename prefix of .vtp/.vtu/.txt files that are written
   * \param sol The discrete solution
   * \param visualizer Function object that determines how the discrete solution is visualized.
   * \param stringifier Function object that determines how RangeType is converted to string before writing to .txt
   * files.
   * \param exact_solution Exact solution function.
   * \return estimated optimal time step length for next step
   * \note If num_save_steps is specified (i.e. if it is not size_t(-1), the solution will be stored/visualized at
   * exactly num_save_steps + 1 equidistant time points (including the initial time and t_end), even if the time step
   * length has to be reduced to hit these time points.
   */
  virtual RangeFieldType solve(const RangeFieldType t_end,
                               const RangeFieldType initial_dt,
                               const size_t num_save_steps,
                               const size_t num_output_steps,
                               const bool save_solution,
                               const bool visualize,
                               const bool write_discrete,
                               const bool write_exact,
                               const bool reset_begin_time,
                               const std::string prefix,
                               DiscreteSolutionType& sol,
                               const VisualizerType& visualizer,
                               const StringifierType& stringifier,
                               const GridFunctionType& exact_solution)
  {
    if (reset_begin_time)
      begin_time_ = std::chrono::steady_clock::now();
    RangeFieldType dt = initial_dt;
    RangeFieldType t = current_time();
    assert(Dune::XT::Common::FloatCmp::ge(t_end, t));
    size_t time_step_counter = 0;

    const RangeFieldType save_interval = (t_end - t) / num_save_steps;
    const RangeFieldType output_interval =
        (num_output_steps == 0 ? std::numeric_limits<RangeFieldType>::max() : (t_end - t) / num_output_steps);
    RangeFieldType next_save_time = t + save_interval > t_end ? t_end : t + save_interval;
    RangeFieldType next_output_time = t + output_interval > t_end ? t_end : t + output_interval;
    size_t save_step_counter = 1;

    // save/visualize initial solution
    if (save_solution)
      sol.emplace(t, current_solution().copy_as_discrete_function());
    write_files(visualize,
                write_discrete,
                write_exact,
                current_solution(),
                exact_solution,
                prefix,
                0,
                t,
                stringifier,
                visualizer);

    // store initial time
    timepoints_.push_back(t);
    while (Dune::XT::Common::FloatCmp::lt(t, t_end)) {
      RangeFieldType max_dt = dt;
      // match saving times and t_end exactly
      if (Dune::XT::Common::FloatCmp::gt(t + dt, t_end))
        max_dt = t_end - t;
      if (Dune::XT::Common::FloatCmp::gt(t + dt, next_save_time) && num_save_steps != size_t(-1))
        max_dt = std::min(next_save_time - t, max_dt);

      // do a timestep
      const auto walltime_before_step = std::chrono::steady_clock::now();
      dt = step(dt, max_dt);
      const auto walltime_after_step = std::chrono::steady_clock::now();
      t = current_time();
      timepoints_.push_back(t);
      std::chrono::duration<double> step_time = walltime_after_step - walltime_before_step;
      step_walltimes_.push_back(step_time.count());

      // augment time step counter
      ++time_step_counter;

      // check if data should be written in this timestep (and write)
      if (Dune::XT::Common::FloatCmp::ge(t, next_save_time) || num_save_steps == size_t(-1)) {
        if (save_solution)
          sol.emplace_hint(sol.end(), t, current_solution().copy_as_discrete_function());
        write_files(visualize,
                    write_discrete,
                    write_exact,
                    current_solution(),
                    exact_solution,
                    prefix,
                    save_step_counter,
                    t,
                    stringifier,
                    visualizer);
        next_save_time += save_interval;
        ++save_step_counter;
      }
      if (num_output_steps && (Dune::XT::Common::FloatCmp::ge(t, next_output_time) || num_output_steps == size_t(-1))) {
        if (current_solution().space().grid_view().comm().rank() == 0)
          std::cout << "time step " << time_step_counter << " done, time =" << t << ", current dt= " << dt << std::endl;
        next_output_time += output_interval;
      }
    } // while (t < t_end)
    solve_walltime_ = std::chrono::steady_clock::now() - begin_time_;
    // for the last time point there is no actual dt and no computation time as the step is not taken anymore, so we
    // store the estimate for the next timestep and the time for the whole solution process
    dts_.push_back(dt);
    step_walltimes_.push_back(solve_walltime_.count());

    return dt;
  } // ... solve(...)

  // default solve, use internal solution
  virtual RangeFieldType solve(const RangeFieldType t_end,
                               const RangeFieldType initial_dt,
                               const size_t num_save_steps = size_t(-1),
                               const size_t num_output_steps = size_t(-1),
                               const bool save_solution = false,
                               const bool visualize = false,
                               const bool write_discrete = false,
                               const bool write_exact = false,
                               const bool reset_begin_time = true,
                               const std::string prefix = "solution",
                               const VisualizerType& visualizer = default_visualizer(),
                               const StringifierType& stringifier = vector_stringifier(),
                               const GridFunctionType& exact_solution = dummy_solution())
  {
    return solve(t_end,
                 initial_dt,
                 num_save_steps,
                 num_output_steps,
                 save_solution,
                 visualize,
                 write_discrete,
                 write_exact,
                 reset_begin_time,
                 prefix,
                 *solution_,
                 visualizer,
                 stringifier,
                 exact_solution);
  }

  // solve and store in sol, no (file) output
  virtual RangeFieldType solve(const RangeFieldType t_end,
                               const RangeFieldType initial_dt,
                               const size_t num_save_steps,
                               DiscreteSolutionType& sol,
                               const bool reset_begin_time = false)
  {
    return solve(t_end,
                 initial_dt,
                 num_save_steps,
                 0,
                 true,
                 false,
                 false,
                 false,
                 reset_begin_time,
                 "",
                 sol,
                 default_visualizer(),
                 vector_stringifier(),
                 dummy_solution());
  }

  /**
   * \brief Find discrete solution for time point that is closest to t.
   *
   * The timestepper only stores the solution at discrete time points. This function returns the discrete solution
   * for
   * the stored time point that is closest to the query time t.
   *
   * \param t Time
   * \return std::pair with pair.second the discrete solution at time pair.first
   */
  virtual const typename DiscreteSolutionType::value_type& solution_closest_to_time(const RangeFieldType t) const
  {
    if (solution().empty())
      DUNE_THROW(Dune::InvalidStateException, "Solution is empty!");
    return solution().upper_bound(t)->first - t > t - solution().lower_bound(t)->first ? *solution().lower_bound(t)
                                                                                       : *solution().upper_bound(t);
  }

  virtual const DiscreteFunctionType& solution_at_time(const RangeFieldType t) const
  {
    const auto it = solution().find(t);
    if (it == solution().end())
      DUNE_THROW(Dune::InvalidStateException,
                 "There is no solution for time " + Dune::XT::Common::to_string(t) + " stored in this object!");
    return *it->second;
  }

  virtual void visualize_solution(const std::string prefix = "",
                                  const VisualizerType& visualizer = default_visualizer()) const
  {
    size_t counter = 0;
    for (const auto& pair : solution()) {
      XT::Functions::visualize(*pair.second,
                               pair.second->space().grid_view(),
                               prefix + "_" + Dune::XT::Common::to_string(counter),
                               false,
                               VTK::appendedraw,
                               {},
                               visualizer);
      ++counter;
    }
  }

  static const VisualizerType& default_visualizer()
  {
    static auto default_vis =
        std::make_unique<XT::Functions::DefaultVisualizer<dimRange, dimRangeCols, RangeFieldType>>();
    return *default_vis;
  }

  static StringifierType vector_stringifier()
  {
    return [](const RangeType& val) {
      std::string ret = XT::Common::to_string(val[0], 15);
      for (size_t ii = 1; ii < val.size(); ++ii)
        ret += " " + XT::Common::to_string(val[ii], 15);
      return ret;
    };
  } // ... vector_stringifier()

  static std::string rankfile_name(const std::string& prefix, const int rank, const size_t step)
  {
    return prefix + "_rank_" + XT::Common::to_string(rank) + "_" + XT::Common::to_string(step) + ".txt";
  }

  static void write_to_textfile(const GridFunctionType& u_n,
                                const GridViewType& grid_view,
                                const std::string& prefix,
                                const size_t step,
                                const RangeFieldType t,
                                const StringifierType& stringifier,
                                const bool intersection_wise = false)
  {
    // write one file per MPI rank
    std::ofstream rankfile(rankfile_name(prefix, grid_view.comm().rank(), step));
    const auto local_func = u_n.local_function();
    for (const auto& entity : elements(grid_view, Dune::Partitions::interiorBorder)) {
      local_func->bind(entity);
      const auto entity_center = entity.geometry().center();
      if (intersection_wise) {
        for (const auto& intersection : Dune::intersections(grid_view, entity)) {
          auto position = intersection.geometry().center();
          assert(position.size() == dimDomain);
          // avoid ambiguity at interface
          for (size_t ii = 0; ii < dimDomain; ++ii)
            if (position[ii] < entity_center[ii])
              position[ii] += 1e-6 * (entity_center[ii] - position[ii]);
          const auto val = local_func->evaluate(entity.geometry().local(position), {"t", t});
          for (size_t ii = 0; ii < dimDomain; ++ii)
            rankfile << XT::Common::to_string(position[ii], 15) << " ";
          rankfile << stringifier(val) << std::endl;
        } // intersections
      } else {
        auto position = entity_center;
        assert(position.size() == dimDomain);
        // avoid ambiguity at interface
        const auto val = local_func->evaluate(entity.geometry().local(position), {"t", t});
        for (size_t ii = 0; ii < dimDomain; ++ii)
          rankfile << XT::Common::to_string(position[ii], 15) << " ";
        rankfile << stringifier(val) << std::endl;
      }
    }
    rankfile.close();
    // Wait till files on all ranks are written
    grid_view.comm().barrier();
    // Merge files
    if (grid_view.comm().rank() == 0) {
      const std::string mergedfile_name = prefix + "_" + XT::Common::to_string(step) + ".txt";
      std::remove(mergedfile_name.c_str());
      std::ofstream merged_file(mergedfile_name, std::ios_base::binary | std::ios_base::app);
      for (int ii = 0; ii < grid_view.comm().size(); ++ii) {
        const std::string rankfile_to_merge_name = rankfile_name(prefix, ii, step);
        std::ifstream rankfile_to_merge(rankfile_to_merge_name, std::ios_base::binary);
        merged_file << rankfile_to_merge.rdbuf();
        rankfile_to_merge.close();
        std::remove(rankfile_to_merge_name.c_str());
      } // ii
      merged_file.close();
    } // if (rank == 0)
  } // void write_to_textfile()

  static void write_files(const bool visualize,
                          const bool write_discrete,
                          const bool write_exact,
                          const DiscreteFunctionType& discrete_sol,
                          const GridFunctionType& exact_sol,
                          const std::string& prefix,
                          const size_t step,
                          const RangeFieldType t,
                          const StringifierType& stringifier,
                          const VisualizerType& visualizer)
  {
    const auto& grid_view = discrete_sol.space().grid_view();
    if (visualize)
      XT::Functions::visualize(discrete_sol,
                               discrete_sol.space().grid_view(),
                               prefix + "_" + XT::Common::to_string(step),
                               false,
                               VTK::appendedraw,
                               {},
                               visualizer);
    if (write_discrete)
      write_to_textfile(discrete_sol, grid_view, prefix, step, t, stringifier);
    if (write_exact)
      write_to_textfile(exact_sol, grid_view, prefix + "_exact", step, t, stringifier);
  }

  void write_timings(const std::string& prefix)
  {
    const std::string filename = prefix + "_timings.txt";
    std::ofstream timings_file(filename);
    timings_file << "step t dt steptime" << std::endl;
    timings_file << std::setprecision(15);
    for (size_t ii = 0; ii < dts_.size(); ++ii) {
      timings_file << ii + 1 << " " << XT::Common::to_string(timepoints_[ii], 15) << " "
                   << XT::Common::to_string(dts_[ii], 15) << " " << XT::Common::to_string(step_walltimes_[ii], 15)
                   << std::endl;
    }
    timings_file.close();
  }


protected:
  RangeFieldType t_;
  DiscreteFunctionType* u_n_;
  DiscreteSolutionType* solution_;
  std::chrono::time_point<std::chrono::steady_clock> begin_time_;
  std::vector<double> dts_;
  std::vector<double> timepoints_;
  std::vector<double> step_walltimes_;
  std::chrono::duration<double> solve_walltime_;
}; // class TimeStepperInterface


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_TIMESTEPPER_INTERFACE_HH
