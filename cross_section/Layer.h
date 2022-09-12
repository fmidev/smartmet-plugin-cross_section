// ======================================================================
/*!
 * \brief An individual layer in a view
 *
 * Characteristics:
 *
 *  - unique projection with specific bounding box forced by view
 *  - may override layer/global producer and time
 */
// ======================================================================

#pragma once

#include "Attributes.h"
#include <json/json.h>
#include <string>
#include <vector>

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

class Layer
{
 public:
  virtual ~Layer() = default;
  bool init(const std::string& theName, const Json::Value& theJson, const Config& theConfig);

  virtual void init(const Json::Value& theJson, const Config& theConfig) = 0;

  virtual void generate(CTPP::CDT& theGlobals, State& theState) = 0;

  Attributes attributes;

 private:
};  // class Layer

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
