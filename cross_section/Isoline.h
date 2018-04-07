// ======================================================================
/*!
 * \brief Isoline details
 */
// ======================================================================

#pragma once

#include "Attributes.h"
#include <boost/optional.hpp>
#include <json/json.h>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Config;

class Isoline
{
 public:
  Isoline() : value(0) {}
  void init(const Json::Value& theJson, const Config& theConfig);

  // Must be present:
  double value;

  // SVG attributes (id, class, style, ...)
  Attributes attributes;

 private:
};  // class Isoline

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
