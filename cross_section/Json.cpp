#include "Json.h"
#include "FileCache.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <spine/Exception.h>

namespace SmartMet
{
namespace Plugin
{
namespace CrossSection
{
// ----------------------------------------------------------------------
/*!
 * \brief Expand include statements in the JSON.
 *
 * For example:
 *
 * {
 *    "isobands":   "json:isobands/temperature.json",
 *    ...
 * }
 *
 * will be expanded so that the value of the isobands variable
 * is changed to the Json contents of the referenced file.
 * If the path begins with "/", expansion is done with respect
 * to the root path instead of the normal path.
 */
// ----------------------------------------------------------------------

void JSON::expand(Json::Value& theJson,
                  const std::string& theRootPath,
                  const std::string& thePath,
                  const FileCache& theFileCache)
{
  try
  {
    if (theJson.isString())
    {
      std::string tmp = theJson.asString();
      if (boost::algorithm::starts_with(tmp, "json:"))
      {
        std::string json_file;
        if (tmp.substr(5, 1) != "/")
          json_file = thePath + "/" + tmp.substr(5, std::string::npos);
        else
          json_file = theRootPath + "/" + tmp.substr(6, std::string::npos);

        Json::Reader reader;
        std::string json_text = theFileCache.get(json_file);
        // parse directly over old contents
        bool json_ok = reader.parse(json_text, theJson);
        if (!json_ok)
          throw SmartMet::Spine::Exception(
              BCP, "Failed to parse '" + json_file + "': " + reader.getFormattedErrorMessages());
        // TODO: should we prevent infinite recursion?
        expand(theJson, theRootPath, thePath, theFileCache);
      }
    }

    // Seek deeper in arrays
    else if (theJson.isArray())
    {
      for (unsigned int i = 0; i < theJson.size(); i++)
        expand(theJson[i], theRootPath, thePath, theFileCache);
    }
    // Seek deeper in objects
    else if (theJson.isObject())
    {
      const auto members = theJson.getMemberNames();
      BOOST_FOREACH (auto& name, members)
      {
        expand(theJson[name], theRootPath, thePath, theFileCache);
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Expand references in the JSON.
 *
 * For example:
 *
 * {
 *    "projection":   "path:name1.name2.name3[0].projection",
 *    ...
 * }
 *
 * will be expanded so that the value of the referenced variable
 * replaces the value of projection
 */
// ----------------------------------------------------------------------

void deref(Json::Value& theJson, Json::Value& theRoot)
{
  try
  {
    if (theJson.isString())
    {
      std::string tmp = theJson.asString();
      if (boost::algorithm::starts_with(tmp, "ref:"))
      {
        std::string path = "." + tmp.substr(4, std::string::npos);
        Json::Path json_path(path);
        const Json::Value& value = json_path.resolve(theRoot);
        if (value.isNull())
          throw SmartMet::Spine::Exception(BCP, "Failed to dereference '" + tmp + "'");
        // We will not dereference the dereferenced value!
        theJson = value;
      }
    }

    // Seek deeper in arrays
    else if (theJson.isArray())
    {
      for (unsigned int i = 0; i < theJson.size(); i++)
        deref(theJson[i], theRoot);
    }
    // Seek deeper in objects
    else if (theJson.isObject())
    {
      const auto members = theJson.getMemberNames();
      BOOST_FOREACH (auto& name, members)
      {
        deref(theJson[name], theRoot);
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

void JSON::dereference(Json::Value& theJson)
{
  try
  {
    deref(theJson, theJson);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace CrossSection
}  // namespace Plugin
}  // namespace SmartMet
