#include "State.h"
#include "Plugin.h"
#include <ctpp2/CDT.hpp>
#include <macgyver/Exception.h>
#include <stdexcept>

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
    : itsPlugin(thePlugin), itsLocalTime(Fmi::LocalDateTime::NOT_A_DATE_TIME)
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Add attributes to the generated CDT
 */
// ----------------------------------------------------------------------

void State::addAttributes(CTPP::CDT& /* theGlobals */,
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
      throw Fmi::Exception(BCP, "The producer has not been set");

    itsQ = itsPlugin.getQEngine().get(itsQuery.producer);
    return itsQ;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Update the envelope
 */
// ----------------------------------------------------------------------

void State::updateEnvelope(const OGRGeometryPtr& theGeom)
{
  try
  {
    if (theGeom == nullptr || theGeom->IsEmpty() != 0)
      return;

    OGREnvelope env;
    theGeom->getEnvelope(&env);
    itsEnvelope.Merge(env);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
