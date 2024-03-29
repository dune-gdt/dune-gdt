// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Andreas Buhr    (2014)
//   Felix Schindler (2013 - 2018, 2020)
//   René Fritze     (2012 - 2013, 2015 - 2016, 2018 - 2020)
//   Tobias Leibner  (2014, 2016, 2018, 2020)

#ifndef DUNE_XT_GRID_OUTPUT_PGF_HH
#define DUNE_XT_GRID_OUTPUT_PGF_HH

#include <array>
#include <cstdio>
#include <limits>
#include <ostream>

#include <dune/common/fvector.hh>

#include <dune/xt/grid/functors/interfaces.hh>
#include <dune/xt/grid/functors/bounding-box.hh>
#include <dune/xt/grid/walker.hh>
#include <utility>

namespace Dune::XT::Grid {


//! A means to ensure we have exactly 2 coords for tikz to draw, even in 1D
struct PgfCoordWrapper : Dune::FieldVector<double, 2>
{
  template <int wdim>
  PgfCoordWrapper(const Dune::FieldVector<double, wdim>& vector)
    : Dune::FieldVector<double, 2>(0.0)
  {
    for (size_t i = 0; i < size_t(std::min(wdim, 2)); ++i)
      (*this)[i] = vector[i];
  }
};

/** \brief print a tex(tikz) representation of an entity to stream
 *  \ingroup Latex
 *  \param output the ostream used for output
 *
 *  A functor to be used in a \ref GridWalk
 **/
class PgfEntityFunctor
{
public:
  PgfEntityFunctor(std::ostream& output)
    : file_(output)
  {
  }

  template <class Entity>
  void operator()(const Entity& ent, const int ent_idx)
  {
    const auto& geo = ent.geometry();
    for (std::size_t i = 0; i < geo.corners(); ++i) {
      const PgfCoordWrapper corner(geo.corner(i));
      std::array<char, 150> buffer = {'\0'};
#ifndef NDEBUG
      const int c =
#endif
          std::snprintf(buffer.data(), 150, "\\coordinate(C_%d_%lu) at (%f,%f);\n", ent_idx, i, corner[0], corner[1]);
      assert(c > 0);
      file_ << buffer;
    }
    // \draw (A)--(B)--(C)--cycle;
    file_ << "\\draw ";
    for (std::size_t i = 0; i < geo.corners(); ++i) {
      std::array<char, 50> buffer = {'\0'};
      std::snprintf(buffer.data(), 50, "(C_%d_%lu)--", ent_idx, i);
      file_ << buffer;
    }
    file_ << "cycle;\n ";
  }

private:
  std::ostream& file_;
};

/** \brief print a tex(tikz) representation of a intersection to stream
 *  \ingroup Latex
 *  \param file the ostream used for output
 *  \param color a latex color definiton used for lines
 *  \param print_entitiy_num whether to draw the Entity's Id number
 *
 *  A functor to be used in a \ref GridWalk
 *  \see Pgf
 **/
template <class GridViewType>
class PgfEntityFunctorIntersections : public ElementAndIntersectionFunctor<GridViewType>
{
  using BaseType = ElementAndIntersectionFunctor<GridViewType>;

public:
  PgfEntityFunctorIntersections(const GridViewType& grid_view,
                                std::ostream& file,
                                std::string color = "black",
                                const bool print_entityIndex = false)
    : file_(file)
    , color_(std::move(color))
    , print_entityIndex_(print_entityIndex)
    , grid_view_(grid_view)
  {
  }

  void apply_local(const typename BaseType::ElementType& entity) override
  {
    const auto ent_idx = grid_view_.indexSet().index(entity);
    maybePrintEntityIndex(entity, ent_idx);
  }

  void apply_local(const typename BaseType::IntersectionType& intersection,
                   const typename BaseType::ElementType& /*inside_entity*/,
                   const typename BaseType::ElementType& /*outside_entity*/) override
  {
    PgfCoordWrapper a(intersection.geometry().corner(0));
    PgfCoordWrapper b(intersection.geometry().corner(1));
    std::array<char, 250> buffer = {'\0'};
    std::snprintf(buffer.data(),
                  250,
                  "\\draw[draw=%s,line width=\\gridlinewidth pt,line cap=round] (%f,%f)--(%f,%f);\n",
                  color_.c_str(),
                  a[0],
                  a[1],
                  b[0],
                  b[1]);
    file_ << buffer;
    file_.flush();
  }

  BaseType* copy() override
  {
    return new PgfEntityFunctorIntersections<GridViewType>(*this);
  }

  void maybePrintEntityIndex(const typename BaseType::ElementType& entity, const int idx)
  {
    if (!print_entityIndex_)
      return;
    PgfCoordWrapper center(entity.geometry().center());
    std::array<char, 50> buffer = {'\0'};
    std::snprintf(buffer.data(), 50, "\\node[circle] at (%f,%f) {%d};\n", center[0], center[1], idx);
    file_ << buffer;
  }

protected:
  std::ostream& file_;
  const std::string color_;
  const bool print_entityIndex_;
  const GridViewType& grid_view_;
};

/** \brief print a tex(tikz) representation of each entity to strean
 * basically the same as \ref PgfEntityFunctorIntersections, but shifting the drawn line "inwards"
 *  \ingroup Latex
 *  \param file the ostream used for output
 *  \param color a latex color definiton used for lines
 *  \param print_entitiy_num whether to draw the Entity's Id number
 *
 *  A functor to be used in a \ref GridWalk
 **/
template <class GridViewType>
class PgfEntityFunctorIntersectionsWithShift : public PgfEntityFunctorIntersections<GridViewType>
{
  using BaseType = PgfEntityFunctorIntersections<GridViewType>;

public:
  PgfEntityFunctorIntersectionsWithShift(const GridViewType& grid_view,
                                         std::ostream& file,
                                         const std::string& color = "black",
                                         const int level = 0,
                                         bool print_entityIndex = false)
    : BaseType(grid_view, file, color, print_entityIndex)
    , level_(level)
  {
  }

  void apply_local(const typename BaseType::IntersectionType& intersection,
                   const typename BaseType::ElementType& inside_entity,
                   const typename BaseType::ElementType& /*outside_entity*/) override
  {
    const PgfCoordWrapper center(inside_entity.geometry().center());
    const auto fac = 0.16 * level_;
    PgfCoordWrapper a(intersection.geometry().corner(0));
    PgfCoordWrapper b(intersection.geometry().corner(1));
    PgfCoordWrapper a_c = center - a;
    PgfCoordWrapper b_c = center - b;
    a_c *= fac;
    b_c *= fac;
    a += a_c;
    b += b_c;
    std::array<char, 150> buffer = {'\0'};
    std::snprintf(buffer.data(),
                  150,
                  "\\draw[draw=%s,line width=\\gridlinewidth pt,line cap=round] (%f,%f)--(%f,%f);\n",
                  this->color_.c_str(),
                  a[0],
                  a[1],
                  b[0],
                  b[1]);
    this->file_ << buffer;
    this->file_.flush();
  }

  ElementAndIntersectionFunctor<GridViewType>* copy() override
  {
    return new PgfEntityFunctorIntersectionsWithShift<GridViewType>(*this);
  }


private:
  const int level_;
};

/** \brief Provider of three types of latex drawings for \ref Grid
 *  \tparam GridType a \ref Grid implementation
 **/
template <class GridType>
class PgfOutput
{
public:
  PgfOutput(GridType& grid)
    : grid_(grid)
  {
  }

  //! print a tex representation of any given 2D grid leaf level to file
  void leaf(std::ostream& file, const bool includable = true) const
  {
    if (!includable) {
      file << "\\documentclass{article}\n"
              "\\usepackage{tikz}\n"
              "\\usetikzlibrary{calc,intersections, arrows,shapes.misc,shapes.arrows}\n"
              "\\pagestyle{empty}\n\\newcommand{\\gridplotscale}{14.8}\n"
              "\\newcommand{\\gridcoordscale}{0.8}\n"
              "\\newcommand{\\gridlinewidth}{0.8}\n"
              "\\begin{document}\n"
              "\\begin{tikzpicture}[scale=\\gridplotscale]\n";
    } else
      file << "\\begin{tikzpicture}\n";
    const auto grid_view = grid_.leafGridView();
    Walker<typename GridType::LeafGridView> gridWalk(grid_view);
    PgfEntityFunctorIntersections<typename GridType::LeafGridView> pgf(grid_view, file);
    gridWalk.append(pgf);
    gridWalk.walk();

    file << "\\end{tikzpicture}\n";
    if (!includable)
      file << "\\end{document}\n";
  }

  /** \brief refine given grid refineLevel times and draw all entities into the same figure
   * \param includable true: only output tikzpicture environment
   *                   false: wrap tikzpicture environment in complete document
   * \param refineLevel passed to \ref Grid::globalRefine
   * \param file the output stream
   **/
  void stacked(std::ostream& file, const int refineLevel, const bool includable = true)
  {
    if (!includable) {
      file << "\\documentclass{article}\n"
              "\\usepackage{tikz}\n"
              "\\usetikzlibrary{calc,intersections, arrows,shapes.misc,shapes.arrows}\n"
              "\\pagestyle{empty}\n\\newcommand{\\gridplotscale}{14.8}\n"
              "\\newcommand{\\gridcoordscale}{0.8}\n"
              "\\newcommand{\\gridlinewidth}{0.8}\n"
              "\\begin{document}\n"
              "\\begin{tikzpicture}[scale=\\gridplotscale]\n";
    } else
      file << "\\begin{tikzpicture}\n";
    grid_.globalRefine(refineLevel);
    for (int i = 0; i < refineLevel; ++i) {
      using ViewType = typename GridType::LevelGridView;
      const ViewType& view = grid_.levelGridView(i);
      Walker<ViewType> gridWalk(view);
      static const std::array<std::string, 7> texcolors = {
          {"black", "red", "blue", "green", "yellow", "cyan", "magenta"}};
      PgfEntityFunctorIntersectionsWithShift<ViewType> pgf(
          view, file, texcolors[std::min(i, int(texcolors.size()))], i, true);
      gridWalk.append(pgf);
      gridWalk.walk();
      file << "%%%%%%%%%%%%%%%" << view.size(0) << "%%%%%%%%%%%%%%%%\n";
    }

    file << "\\end{tikzpicture}\n";
    if (!includable)
      file << "\\end{document}\n";
  }

  /** \brief refine given grid refineLevel times and create a subfigure for each level
   *  \param includable true: only output figure environment
   *                   false: wrap figure environment in complete document
   *  \param refineLevel passed to \ref Grid::globalRefine
   *  \param file the output stream
   *
   *  The written latex code allows tweaking by changing values of gridplotscale,
   *  gridcoordscale and gridlinewidth
   **/
  void refineseries(std::ostream& file, const int refineLevel, const bool includable = true)
  {
    if (!includable) {
      file << "\\documentclass{article}\n"
              "\\usepackage{tikz}\n\\usepackage{subfig}\n"
              "\\usetikzlibrary{calc,intersections, arrows,shapes.misc,shapes.arrows}\n"
              "\\pagestyle{empty}\n\\newcommand{\\gridplotscale}{0.8}\n"
              "\\newcommand{\\gridcoordscale}{0.8}\n"
              "\\newcommand{\\gridlinewidth}{0.8}\n"
              "\\begin{document}\n";
    }
    file << "\\begin{figure}\n";
    grid_.globalRefine(refineLevel);
    for (int i = 0; i < refineLevel; ++i) {
      using ViewType = typename GridType::LevelGridView;
      {
        const ViewType& view = grid_.levelGridView(i);
        std::array<char, 80> buffer = {'\0'};
        std::snprintf(buffer.data(), 80, "\\subfloat[Level %d]{\n\\begin{tikzpicture}[scale=\\gridplotscale]\n", i);
        file << buffer;
        Walker<ViewType> gridWalk(view);
        PgfEntityFunctorIntersections<ViewType> thisLevel(view, file, "black", true);
        gridWalk.append(thisLevel);
        gridWalk.walk();
      }
      using LeafView = typename GridType::LeafGridView;
      Walker<LeafView> leafWalk(grid_.leafGridView());
      MinMaxCoordinateFunctor<LeafView> minMaxCoord;
      leafWalk.append(minMaxCoord);
      leafWalk.walk();
      const auto minima = minMaxCoord.result().first;
      const auto maxima = minMaxCoord.result().second;

      switch (int(GridType::dimensionworld)) {
        case 1: {
          std::array<char, 80> buffer = {'\0'};
          const double offset = 0.2;
          const char* format = "\\node[scale=\\gridcoordscale] at (%f,0) {(%d)};\n";
          std::snprintf(buffer.data(), 80, format, minima[0] - offset, minima[0]);
          file << buffer;
          std::snprintf(buffer.data(), 80, format, maxima[0] - offset, maxima[0]);
          file << buffer;
          break;
        }
        case 2:
        case 3: {
          const double offset = 0.2;
          const char* format = "\\node[scale=\\gridcoordscale] at (%f,%f) {(%d,%d)};\n";
          std::array<char, 100> buffer = {'\0'};
          std::snprintf(buffer.data(), 100, format, minima[0] - offset, minima[1] - offset, minima[0], minima[1]);
          file << buffer;
          std::snprintf(buffer.data(), 100, format, maxima[0] - offset, maxima[1] - offset, maxima[0], maxima[1]);
          file << buffer;
          break;
        }
      }
      if ((i + 1) % 3 == 0) // three subfigs per row
        file << "\\end{tikzpicture}}\\\\\n";
      else
        file << "\\end{tikzpicture}}\n";
    }
    file << "\\end{figure}\n";
    if (!includable)
      file << "\\end{document}\n";
  }

private:
  GridType& grid_;
};

} // namespace Dune::XT::Grid

#endif // DUNE_XT_GRID_OUTPUT_PGF_HH
