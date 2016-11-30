// ======================================================================

#include "TemplateFactory.h"
#include <spine/Exception.h>
#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Get a thread specific template formatter for given template
 */
// ----------------------------------------------------------------------

SharedFormatter TemplateFactory::get(const boost::filesystem::path& theFilename) const
{
  try
  {
    if (theFilename.empty())
      throw SmartMet::Spine::Exception(BCP, "TemplateFactory: Cannot use empty templates");

    TemplateMap* tmap = itsTemplates.get();

    // tmap is thread specific and hence safe to use without locking

    if (tmap == NULL)
    {
      tmap = new TemplateMap;
      itsTemplates.reset(tmap);
    }

    // Find out if there is a formatter which is up to date

    auto& info = (*tmap)[theFilename];

    std::time_t modtime = boost::filesystem::last_write_time(theFilename);

    // We assume there are no templates with modtime=0
    if (info.modtime == modtime)
      return info.formatter;

    // Initialize a new formatter and return it

    info.modtime = modtime;
    info.formatter = boost::make_shared<Fmi::TemplateFormatter>();

    info.formatter->load_template(theFilename.c_str());

    return info.formatter;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
