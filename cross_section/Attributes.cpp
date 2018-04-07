// ======================================================================

#include "Attributes.h"
#include "Config.h"
#include "State.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <ctpp2/CDT.hpp>
#include <spine/Exception.h>
#include <spine/HTTP.h>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Initialize attributes from JSON
 *
 * Note: We allow JSON values to be integers and doubles, but
 *       convert them directly to strings since that is how
 *       we are going to represent them when printing SVG
 *       attributes. This feature allows the user to put
 *       attributes in a natural form into the attributes without
 *       bothering with unnecessary quotes.
 */
// ----------------------------------------------------------------------

void Attributes::init(const Json::Value& theJson, const Config& theConfig)
{
  try
  {
    // Allowed since it makes JSON self documenting
    if (theJson.isNull())
      return;

    if (!theJson.isObject())
      throw SmartMet::Spine::Exception(BCP,
                                       "Attributes JSON is not a JSON object (name-value pairs)");

    // Iterate trhough all the members

    const auto members = theJson.getMemberNames();
    BOOST_FOREACH (const auto& name, members)
    {
      const Json::Value& json = theJson[name];

      switch (json.type())
      {
        case Json::nullValue:
          break;
        case Json::intValue:
        {
          attributes[name] = boost::lexical_cast<std::string>(json.asInt());
          break;
        }
        case Json::uintValue:
        {
          attributes[name] = boost::lexical_cast<std::string>(json.asUInt64());
          break;
        }
        case Json::realValue:
        {
          attributes[name] = boost::lexical_cast<std::string>(json.asDouble());
          break;
        }
        case Json::stringValue:
        case Json::booleanValue:
        {
          attributes[name] = json.asString();
          break;
        }
        case Json::arrayValue:
        {
          throw SmartMet::Spine::Exception(BCP, "Arrays are not allowed as an Attribute value");
        }
        case Json::objectValue:
        {
          throw SmartMet::Spine::Exception(BCP, "Maps are not allowed as an Attribute value");
        }
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Add the attributes to the CDT
 */
// ----------------------------------------------------------------------

void Attributes::generate(CTPP::CDT& theLocals, State& theState) const
{
  try
  {
    CTPP::CDT attrs = CTPP::CDT(CTPP::CDT::HASH_VAL);

    BOOST_FOREACH (const auto& attribute, attributes)
    {
      const auto& name = attribute.first;
      const auto& value = attribute.second;
      attrs[name] = value;
    }

    // There may be pre-existing attributes, so we merge instead of assigning
    theLocals["attributes"].MergeCDT(attrs);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
