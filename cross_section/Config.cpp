// ======================================================================
/*!
 * \brief Implementation of Config
 */
// ======================================================================

#include "Config.h"
#include <spine/Exception.h>
#include <stdexcept>

using std::string;

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
    : itsDefaultUrl("/csection"),
      itsDefaultTemplate("svgjson"),
      itsDefaultCustomer("fmi"),
      itsDefaultTimeZone("UTC"),
      itsTemplateDirectory("/etc/brainstorm/plugins/csection/templates")
{
  try
  {
    if (!configfile.empty())
    {
      itsConfig.readFile(configfile.c_str());

      // required parameters
      std::string root = itsConfig.lookup("root");
      itsRootDirectory = root;

      // optional parameters
      itsConfig.lookupValue("url", itsDefaultUrl);
      itsConfig.lookupValue("template", itsDefaultTemplate);
      itsConfig.lookupValue("templatedir", itsTemplateDirectory);
      itsConfig.lookupValue("customer", itsDefaultCustomer);
      itsConfig.lookupValue("timezone", itsDefaultTimeZone);
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
