// ======================================================================
/*!
 * \brief Implementation of Config
 */
// ======================================================================

#include "Config.h"
#include <spine/Exception.h>
#include <stdexcept>

using namespace std;

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

Config::Config(const string& configfile)
    : itsConfig(),
      itsDefaultUrl("/csection"),
      itsDefaultTemplate("svgjson"),
      itsDefaultCustomer("fmi"),
      itsDefaultTimeZone("UTC"),
      itsTemplateDirectory("/etc/brainstorm/plugins/csection/templates"),
      itsRootDirectory("/smartmet/share/brainstorm/csection")
{
  try
  {
    if (!configfile.empty())
    {
      itsConfig.readFile(configfile.c_str());
      itsConfig.lookupValue("url", itsDefaultUrl);

      itsConfig.lookupValue("template", itsDefaultTemplate);
      itsConfig.lookupValue("templatedir", itsTemplateDirectory);
      itsConfig.lookupValue("customer", itsDefaultCustomer);
      itsConfig.lookupValue("timezone", itsDefaultTimeZone);
      itsConfig.lookupValue("root", itsRootDirectory);
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*
 * Accessors
 */
// ----------------------------------------------------------------------

const std::string& Config::defaultUrl() const
{
  return itsDefaultUrl;
}
const std::string& Config::defaultTemplate() const
{
  return itsDefaultTemplate;
}
const std::string& Config::defaultCustomer() const
{
  return itsDefaultCustomer;
}
const std::string& Config::defaultTimeZone() const
{
  return itsDefaultTimeZone;
}
const std::string& Config::templateDirectory() const
{
  return itsTemplateDirectory;
}
const std::string& Config::rootDirectory() const
{
  return itsRootDirectory;
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
