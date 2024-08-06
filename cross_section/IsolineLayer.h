#pragma once

#include "Isoline.h"
#include "Layer.h"
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

  void init(const Json::Value& theJson, const Config& theConfig) override;

  void generate(CTPP::CDT& theGlobals, State& theState) override;

  std::optional<std::string> parameter;
  std::optional<std::string> zparameter;
  std::vector<Isoline> isolines;
  std::string interpolation;

  std::optional<double> multiplier;
  std::optional<double> offset;

 private:
  void generate_qEngine(CTPP::CDT& theGlobals, State& theState);
  void generate_gridEngine(CTPP::CDT& theGlobals, State& theState);
};  // class IsolineHandler

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
