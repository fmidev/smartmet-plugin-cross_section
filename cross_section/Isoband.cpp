#include "Isoband.h"
#include "Config.h"
#include <boost/foreach.hpp>
#include <spine/Exception.h>
#include <stdexcept>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Initialize from JSON
 */
// ----------------------------------------------------------------------

void Isoband::init(const Json::Value& theJson, const Config& theConfig)
{
  try
  {
    if (!theJson.isObject())
      throw SmartMet::Spine::Exception(BCP, "Isoband JSON is not a JSON object");

    // Iterate through all the members

    const auto members = theJson.getMemberNames();
    BOOST_FOREACH (const auto& name, members)
    {
      const Json::Value& json = theJson[name];

      if (name == "lolimit")
        lolimit = json.asDouble();
      else if (name == "hilimit")
        hilimit = json.asDouble();
      else if (name == "attributes")
        attributes.init(json, theConfig);
      else
        throw SmartMet::Spine::Exception(BCP,
                                         "Isoband does not have a setting named '" + name + "'");
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
