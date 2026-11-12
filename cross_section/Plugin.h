// ======================================================================
/*!
 * \brief SmartMet CrossSection plugin
 */
// ======================================================================

#pragma once

#include "Config.h"
#include "FileCache.h"
#include "Product.h"
#include "TemplateFactory.h"
#include <spine/SmartMetPlugin.h>
#include <spine/Thread.h>
#include <map>
#include <memory>
#include <string>

namespace SmartMet::Engine::Contour
{
class Engine;
}  // namespace SmartMet::Engine::Contour
namespace SmartMet::Engine::Geonames
{
class Engine;
}  // namespace SmartMet::Engine::Geonames
namespace SmartMet::Engine::Grid
{
class Engine;
}  // namespace SmartMet::Engine::Grid
namespace SmartMet::Engine::Querydata
{
class Engine;
}  // namespace SmartMet::Engine::Querydata
namespace SmartMet::Spine::HTTP
{
class Request;
class Response;
}  // namespace SmartMet::Spine::HTTP
namespace SmartMet::Spine
{
class Reactor;
}  // namespace SmartMet::Spine

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Plugin : public SmartMetPlugin
{
 public:
  Plugin(SmartMet::Spine::Reactor* theReactor, const char* theConfig);
  ~Plugin() override = default;
  Plugin() = delete;
  Plugin(const Plugin& other) = delete;
  Plugin& operator=(const Plugin& other) = delete;
  Plugin(Plugin&& other) = delete;
  Plugin& operator=(Plugin&& other) = delete;

  const std::string& getPluginName() const override;
  int getRequiredAPIVersion() const override;

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
  void init() override;
  void shutdown() override;
  void requestHandler(SmartMet::Spine::Reactor& theReactor,
                      const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse) override;

 private:
  std::string query(const SmartMet::Spine::Reactor& theReactor,
                    const SmartMet::Spine::HTTP::Request& theRequest,
                    const SmartMet::Spine::HTTP::Response& theResponse);
  // Plugin configuration
  const std::string itsModuleName;
  SmartMet::Plugin::CrossSection::Config itsConfig;

  // Cache server and engine instances
  SmartMet::Spine::Reactor* itsReactor = nullptr;
  std::shared_ptr<SmartMet::Engine::Querydata::Engine> itsQEngine;
  std::shared_ptr<SmartMet::Engine::Grid::Engine> itsGridEngine;
  std::shared_ptr<SmartMet::Engine::Contour::Engine> itsContourEngine;
  std::shared_ptr<SmartMet::Engine::Geonames::Engine> itsGeoEngine;

  // Cache templates
  TemplateFactory itsTemplateFactory;

  // Cache products
  using ProductCache = std::map<std::string, Product>;
  mutable SmartMet::Spine::MutexType itsProductCacheMutex;
  mutable ProductCache itsProductCache;

  // Cache files
  mutable FileCache itsFileCache;

};  // class Plugin

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
