// ======================================================================

#include "FileCache.h"
#include <spine/Exception.h>
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <stdexcept>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Get file contents
 */
// ----------------------------------------------------------------------

std::string FileCache::get(const boost::filesystem::path& thePath) const
{
  try
  {
    std::time_t mtime = boost::filesystem::last_write_time(thePath);

    // Try using the cache with a lock first
    {
      SmartMet::Spine::ReadLock lock(itsMutex);
      auto iter = itsCache.find(thePath);
      if (iter != itsCache.end())
      {
        if (mtime == iter->second.modification_time)
          return iter->second.content;
      }
    }

    // No active lock while we read the file contents

    std::string content;
    std::ifstream in(thePath.c_str());
    if (!in)
      throw SmartMet::Spine::Exception(
          BCP, "Failed to open '" + std::string(thePath.c_str()) + "' for reading!");
    content.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());

    // Now insert the value into the cache and return it

    SmartMet::Spine::WriteLock lock(itsMutex);
    FileContents contents(mtime, content);
    itsCache[thePath] = contents;
    return content;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
