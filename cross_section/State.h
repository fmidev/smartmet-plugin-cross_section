// ======================================================================
/*!
 * \brief State variables for the current request
 *
 * For example we cache requested producers so that the data will
 * remain the same for each layer in the product. The cache cannot
 * be in the plugin since then it would be shared by all requests.
 * Instead, we pass the state object along to all components that
 * generate actual content.
 *
 * Since the State object is always passed along, we also store
 * in it all engines that may be needed by the layers. This way
 * we do not have to decide on whether to call the plugin or
 * the state object to get access to "global stuff".
 */
// ======================================================================

#pragma once

#include "Attributes.h"
#include "Plugin.h"
#include "Query.h"
#include <engines/contour/Engine.h>
#include <engines/querydata/Q.h>
#include <map>
#include <ogr_geometry.h>

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
class Plugin;

class State
{
 public:
  State() = delete;

  // Set the state of the plugin
  State(const Plugin& thePlugin);

  // Give access to configuration variables
  const Config& getConfig() const;

  // Add attributes
  void addAttributes(CTPP::CDT& theGlobals, const Attributes& theAttributes);

  void addAttributes(CTPP::CDT& theGlobals, CTPP::CDT& theLocals, const Attributes& theAttributes);

  // Data

  void query(const Query& theQuery) { itsQuery = theQuery; }
  const Query& query() const { return itsQuery; }
  SmartMet::Engine::Querydata::Q producer();

  // Contourer
  const SmartMet::Engine::Contour::Engine& getContourEngine() const
  {
    return itsPlugin.getContourEngine();
  }
  const SmartMet::Engine::Grid::Engine& getGridEngine() const
  {
    return itsPlugin.getGridEngine();
  }
  // Valid time
  void time(const boost::local_time::local_date_time& theTime) { itsLocalTime = theTime; }
  const boost::local_time::local_date_time& time() const { return itsLocalTime; }
  const OGREnvelope& envelope() const { return itsEnvelope; }
  void updateEnvelope(const OGRGeometryPtr& theGeom);

 private:
  const Plugin& itsPlugin;
  Query itsQuery;

  // current state:
  SmartMet::Engine::Querydata::Q itsQ;
  boost::local_time::local_date_time itsLocalTime;

  OGREnvelope itsEnvelope;
};

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
