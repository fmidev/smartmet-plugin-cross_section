// ======================================================================
/*!
 * \brief Full product specification
 */
// ======================================================================

#pragma once

#include "Layers.h"
#include <json/json.h>
#include <timeseries/TimeSeriesGenerator.h>
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
                const TimeSeries::TimeSeriesGenerator::LocalTimeList& theTimes);

  Layers layers;

 private:
};  // class Product

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
