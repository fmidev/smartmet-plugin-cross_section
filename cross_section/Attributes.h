// ======================================================================
/*!
 * \brief Generic SVG element attribute container
 */
// ======================================================================

#pragma once

#include <json/json.h>
#include <map>
#include <string>

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
class State;

class Attributes
{
 public:
  void init(const Json::Value& theJson, const Config& theConfig);

  void generate(CTPP::CDT& theLocals, State& theState) const;

 private:
  std::map<std::string, std::string> attributes;
};

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
