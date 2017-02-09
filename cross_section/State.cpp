// ======================================================================

#include "State.h"
#include "Plugin.h"
#include <ctpp2/CDT.hpp>
#include <boost/foreach.hpp>
#include <stdexcept>
#include <spine/Exception.h>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Set the state of the plugin
 */
// ----------------------------------------------------------------------

State::State(const Plugin& thePlugin)
    : itsPlugin(thePlugin), itsLocalTime(boost::date_time::not_a_date_time)
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Get the configuration object
 */
// ----------------------------------------------------------------------

const Config& State::getConfig() const
{
  try
  {
    return itsPlugin.getConfig();
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Add attributes to the generated CDT at the global level
 */
// ----------------------------------------------------------------------

void State::addAttributes(CTPP::CDT& theGlobals, const Attributes& theAttributes)
{
  try
  {
    addAttributes(theGlobals, theGlobals, theAttributes);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Add attributes to the generated CDT
 */
// ----------------------------------------------------------------------

void State::addAttributes(CTPP::CDT& theGlobals,
                          CTPP::CDT& theLocals,
                          const Attributes& theAttributes)
{
  try
  {
    // The locals are easily handled by inserting them to the CDT
    theAttributes.generate(theLocals, *this);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get the data to be used
 */
// ----------------------------------------------------------------------

SmartMet::Engine::Querydata::Q State::producer()
{
  try
  {
    if (itsQuery.producer.empty())
      throw SmartMet::Spine::Exception(BCP, "The producer has not been set");

    itsQ = itsPlugin.getQEngine().get(itsQuery.producer);
    return itsQ;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Update the envelope
 */
// ----------------------------------------------------------------------

void State::updateEnvelope(OGRGeometryPtr theGeom)
{
  try
  {
    if (!theGeom || theGeom->IsEmpty())
      return;

    OGREnvelope env;
    theGeom->getEnvelope(&env);
    itsEnvelope.Merge(env);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
