// ======================================================================
/*!
 * \brief Configuration file API
 */
// ======================================================================

#pragma once

#include <libconfig.h++>
#include <set>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Config
{
 public:
  Config(const std::string& configfile);

  Config() = delete;
  Config(const Config& other) = delete;
  Config& operator=(const Config& other) = delete;
  Config(Config&& other) = delete;
  Config& operator=(Config&& other) = delete;

  const std::string& defaultUrl() const;
  const std::string& defaultTemplate() const;
  const std::string& defaultCustomer() const;
  const std::string& defaultTimeZone() const;

  const std::string& templateDirectory() const;
  const std::string& rootDirectory() const;

 private:
  libconfig::Config itsConfig;
  std::string itsDefaultUrl;
  std::string itsDefaultTemplate;
  std::string itsDefaultCustomer;
  std::string itsDefaultTimeZone;

  std::string itsTemplateDirectory;
  std::string itsRootDirectory;

};  // class Config

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
