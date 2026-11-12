#include "Layer.h"
#include <macgyver/Exception.h>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{

Layer::~Layer() = default;

// ----------------------------------------------------------------------
/*!
 * \brief Initialize from JSON
 */
// ----------------------------------------------------------------------

bool Layer::init(const std::string& theName, const Json::Value& theJson, const Config& theConfig)
{
  try
  {
    if (theName == "layer_type")
      ;
    else if (theName == "attributes")
      attributes.init(theJson, theConfig);
    else
      return false;

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
