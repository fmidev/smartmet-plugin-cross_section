// ======================================================================
/*!
 * \brief JSON tools
 */
//======================================================================

#pragma once
#include <json/json.h>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
class FileCache;

namespace JSON
{
// expand includes in the Json ("json:file/name.json")
void expand(Json::Value& theJson,
            const std::string& theRootPath,
            const std::string& thePath,
            const FileCache& theFileCache);

// expand references in the Json ("path:name1.name2[0].parameter")
void dereference(Json::Value& theJson);
}  // namespace JSON

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
