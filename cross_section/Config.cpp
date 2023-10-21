// ======================================================================
/*!
 * \brief Implementation of Config
 */
// ======================================================================

#include "Config.h"
#include <boost/filesystem/path.hpp>
#include <macgyver/Exception.h>
#include <spine/ConfigTools.h>
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
      itsTemplateDirectory("/usr/share/smartmet/cross_section")
{
  try
  {
    if (!configfile.empty())
    {
      // Enable sensible relative include paths
      boost::filesystem::path p = configfile;
      p.remove_filename();
      itsConfig.setIncludeDir(p.c_str());

      itsConfig.readFile(configfile.c_str());
      Spine::expandVariables(itsConfig);

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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
