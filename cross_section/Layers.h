// ======================================================================
/*!
 * \brief Layers container
 */
// ======================================================================

#pragma once

#include <json/json.h>
#include <list>
#include <memory>

namespace CTPP
{
class CDT;
}

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Config;
class Layer;
class State;

class Layers
{
 public:
  void init(const Json::Value& theJson, const Config& theConfig);

  void generate(CTPP::CDT& theGlobals, State& theState);

  bool empty() const { return layers.empty(); }

 private:
  std::list<std::shared_ptr<Layer> > layers;
};

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
