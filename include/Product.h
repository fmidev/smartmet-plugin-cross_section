// ======================================================================
/*!
 * \brief Full product specification
 */
// ======================================================================

#pragma once

#include "Layers.h"
#include <spine/TimeSeriesGenerator.h>

#include <json/json.h>
#include <string>
#include <vector>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Config;
class State;

class Product
{
 public:
  void init(const Json::Value& theJson, const Config& theConfig);
  void generate(CTPP::CDT& theGlobals,
                State& theState,
                const SmartMet::Spine::TimeSeriesGenerator::LocalTimeList& theTimeList);

  Layers layers;

 private:
};  // class Product

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
