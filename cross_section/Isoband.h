// ======================================================================
/*!
 * \brief Isoband details
 */
// ======================================================================

#pragma once

#include "Attributes.h"
#include <json/json.h>
#include <optional>

namespace SmartMet::Plugin::CrossSection
{
class Config;
}  // namespace SmartMet::Plugin::CrossSection

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Isoband
{
 public:
  void init(const Json::Value& theJson, const Config& theConfig);

  // One or both may be missing. Interpretation:
  // null,value --> -inf,value
  // value,null --> value,inf
  // null,null  --> missing values

  std::optional<double> lolimit;
  std::optional<double> hilimit;

  // Attributes (id, class, style, ...)
  Attributes attributes;

 private:
};  // class Isoband

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
