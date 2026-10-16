// ======================================================================
/*!
 * \brief Grid engine tools shared by the isoband and isoline layers
 */
// ======================================================================

#pragma once

#include <engines/grid/Engine.h>
#include <string>
#include <vector>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class State;

// A vertical grid fetched from the grid engine
struct VerticalGrid
{
  std::string utcTime;
  std::vector<T::Coordinate> coordinates;
  std::vector<float> gridData;
  uint gridWidth = 0;
  uint gridHeight = 0;
};

// Remove a ".raw" suffix from the parameter name, true if there was one
bool remove_raw_suffix(std::string& theParameter);

// Parameter details of the value parameter with mappings unless it was found directly
Engine::Grid::ParameterDetails_vec get_value_parameter_details(
    const Engine::Grid::Engine& theGridEngine,
    const std::string& theProducer,
    const std::string& theParameter,
    const std::string& theParameterName);

// Parameter details of the height parameter with mappings unless it was found directly
Engine::Grid::ParameterDetails_vec get_height_parameter_details(
    const Engine::Grid::Engine& theGridEngine,
    const std::string& theProducer,
    const std::string& theParameter);

// Fetch the vertical grid of the value parameter along the cross section
VerticalGrid get_vertical_grid(const Engine::Grid::Engine& theGridEngine,
                               const State& theState,
                               const Engine::Grid::ParameterDetails_vec& theParameterDetails,
                               const Engine::Grid::ParameterDetails_vec& theZParameterDetails,
                               bool theRawFlag);

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
