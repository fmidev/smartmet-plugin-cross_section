// ======================================================================
/*!
 * \brief SmartMet CrossSection plugin
 */
// ======================================================================

#pragma once

#include <engines/contour/Engine.h>
#include <engines/geonames/Engine.h>
#include <engines/querydata/Engine.h>
#include <engines/grid/Engine.h>
#include <spine/HTTP.h>
#include <spine/Reactor.h>
#include <spine/SmartMetPlugin.h>
#include <spine/Thread.h>

#include "Config.h"
#include "FileCache.h"
#include "Product.h"
#include "TemplateFactory.h"

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Plugin : public SmartMetPlugin, private boost::noncopyable
{
 public:
  Plugin(SmartMet::Spine::Reactor* theReactor, const char* theConfig);
  virtual ~Plugin();

  const std::string& getPluginName() const;
  int getRequiredAPIVersion() const;

  // Get the engines
  const SmartMet::Engine::Querydata::Engine& getQEngine() const { return *itsQEngine; }
  const SmartMet::Engine::Grid::Engine& getGridEngine() const { return *itsGridEngine; }
  const SmartMet::Engine::Contour::Engine& getContourEngine() const { return *itsContourEngine; }
  // Plugin specific public API:

  const Config& getConfig() const;
  SharedFormatter getTemplate(const std::string& theName) const;
  Product getProduct(const std::string& theCustomer,
                     const std::string& theName,
                     bool theDebugFlag) const;

 protected:
  void init();
  void shutdown();
  void requestHandler(SmartMet::Spine::Reactor& theReactor,
                      const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse);

 private:
  Plugin();
  std::string query(SmartMet::Spine::Reactor& theReactor,
                    const SmartMet::Spine::HTTP::Request& req,
                    SmartMet::Spine::HTTP::Response& response);
  // Plugin configuration
  const std::string itsModuleName;
  SmartMet::Plugin::CrossSection::Config itsConfig;

  // Cache server and engine instances
  SmartMet::Spine::Reactor* itsReactor;
  SmartMet::Engine::Querydata::Engine* itsQEngine;
  SmartMet::Engine::Grid::Engine* itsGridEngine;
  SmartMet::Engine::Contour::Engine* itsContourEngine;
  SmartMet::Engine::Geonames::Engine* itsGeoEngine;

  // Cache templates
  TemplateFactory itsTemplateFactory;

  // Cache products
  typedef std::map<std::string, Product> ProductCache;
  mutable SmartMet::Spine::MutexType itsProductCacheMutex;
  mutable ProductCache itsProductCache;

  // Cache files
  mutable FileCache itsFileCache;

};  // class Plugin

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
