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
  Isoline() = default;
  void init(const Json::Value& theJson, const Config& theConfig);

  // Must be present:
  double value = 0.0;

  // SVG attributes (id, class, style, ...)
  Attributes attributes;

 private:
};  // class Isoline

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
