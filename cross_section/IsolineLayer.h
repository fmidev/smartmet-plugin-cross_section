#pragma once

#include "Layer.h"
#include "Isoline.h"
#include <vector>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Config;
class Plugin;
class State;

class IsolineLayer : public Layer
{
 public:
  IsolineLayer();
  ~IsolineLayer();

  virtual void init(const Json::Value& theJson, const Config& theConfig);

  virtual void generate(CTPP::CDT& theGlobals, State& theState);

  boost::optional<std::string> parameter;
  boost::optional<std::string> zparameter;
  std::vector<Isoline> isolines;
  std::string interpolation;

  boost::optional<double> multiplier;
  boost::optional<double> offset;

 private:
};  // class IsolineHandler

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
