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
  ~IsobandLayer();

  virtual void init(const Json::Value& theJson, const Config& theConfig);

  virtual void generate(CTPP::CDT& theGlobals, State& theState);

  boost::optional<std::string> parameter;
  boost::optional<std::string> zparameter;
  std::vector<Isoband> isobands;
  std::string interpolation;

  boost::optional<double> multiplier;
  boost::optional<double> offset;

 private:
};  // class IsobandLayer

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
