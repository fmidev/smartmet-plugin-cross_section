// ======================================================================
/*!
 * \brief Configuration file API
 */
// ======================================================================

#pragma once

#include <libconfig.h++>
#include <boost/utility.hpp>
#include <set>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class Config : private boost::noncopyable
{
 public:
  Config() = delete;
  Config(const std::string& configfile);

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
