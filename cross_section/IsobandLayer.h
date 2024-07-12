// ======================================================================
/*!
 * \brief Isoband layer
 */
// ======================================================================

#pragma once

#include "Isoband.h"
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

class IsobandLayer : public Layer
{
 public:
  IsobandLayer();

  void init(const Json::Value& theJson, const Config& theConfig) override;

  void generate(CTPP::CDT& theGlobals, State& theState) override;

  std::optional<std::string> parameter;
  std::optional<std::string> zparameter;
  std::vector<Isoband> isobands;
  std::string interpolation;

  std::optional<double> multiplier;
  std::optional<double> offset;

 private:
  void generate_qEngine(CTPP::CDT& theGlobals, State& theState);
  void generate_gridEngine(CTPP::CDT& theGlobals, State& theState);
};  // class IsobandLayer

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
