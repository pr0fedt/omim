#include "UserMarkHelper.hpp"

#include "map/place_page_info.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/banner.hpp"

#include "base/string_utils.hpp"

namespace usermark_helper
{
using search::AddressInfo;
using feature::Metadata;

void InjectMetadata(JNIEnv * env, jclass const clazz, jobject const mapObject, feature::Metadata const & metadata)
{
  static jmethodID const addId = env->GetMethodID(clazz, "addMetadata", "(ILjava/lang/String;)V");
  ASSERT(addId, ());

  for (auto const t : metadata.GetPresentTypes())
  {
    // TODO: It is not a good idea to pass raw strings to UI. Calling separate getters should be a better way.
    jni::TScopedLocalRef metaString(env, t == feature::Metadata::FMD_WIKIPEDIA ?
                                              jni::ToJavaString(env, metadata.GetWikiURL()) :
                                              jni::ToJavaString(env, metadata.Get(t)));
    env->CallVoidMethod(mapObject, addId, t, metaString.get());
  }
}

jobject CreateBanner(JNIEnv * env, string const & id, jint type)
{
  static jmethodID const bannerCtorId =
      jni::GetConstructorID(env, g_bannerClazz, "(Ljava/lang/String;I)V");

  return env->NewObject(g_bannerClazz, bannerCtorId, jni::ToJavaString(env, id), type);
}

jobject CreateMapObject(JNIEnv * env, int mapObjectType, string const & title,
                        string const & subtitle, double lat, double lon, string const & address,
                        Metadata const & metadata, string const & apiId, jobjectArray jbanners,
                        bool isReachableByTaxi)
{
  // public MapObject(@MapObjectType int mapObjectType, String title, String subtitle, double lat,
  // double lon, String address, String apiId, @NonNull Banner banner, boolean reachableByTaxi)
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_mapObjectClazz,
                            "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;DDLjava/lang/"
                            "String;[Lcom/mapswithme/maps/ads/Banner;Z)V");

  jobject mapObject =
      env->NewObject(g_mapObjectClazz, ctorId, mapObjectType, jni::ToJavaString(env, title),
                     jni::ToJavaString(env, subtitle), jni::ToJavaString(env, address), lat, lon,
                     jni::ToJavaString(env, apiId), jbanners, isReachableByTaxi);

  InjectMetadata(env, g_mapObjectClazz, mapObject, metadata);
  return mapObject;
}

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  jobjectArray jbanners = nullptr;
  if (info.HasBanner())
    jbanners = ToBannersArray(env, info.GetBanners());

  if (info.IsBookmark())
  {
    // public Bookmark(@IntRange(from = 0) int categoryId, @IntRange(from = 0) int bookmarkId,
    // String name, @Nullable String objectTitle, @NonNull Banner banner, boolean reachableByTaxi)
    static jmethodID const ctorId = jni::GetConstructorID(
        env, g_bookmarkClazz,
        "(IILjava/lang/String;Ljava/lang/String;[Lcom/mapswithme/maps/ads/Banner;Z)V");

    auto const & bac = info.GetBookmarkAndCategory();
    BookmarkCategory * cat = g_framework->NativeFramework()->GetBmCategory(bac.m_categoryIndex);
    BookmarkData const & data =
        static_cast<Bookmark const *>(cat->GetUserMark(bac.m_bookmarkIndex))->GetData();

    jni::TScopedLocalRef jName(env, jni::ToJavaString(env, data.GetName()));
    jni::TScopedLocalRef jTitle(env, jni::ToJavaString(env, info.GetTitle()));
    jobject mapObject =
        env->NewObject(g_bookmarkClazz, ctorId, static_cast<jint>(info.m_bac.m_categoryIndex),
                       static_cast<jint>(info.m_bac.m_bookmarkIndex), jName.get(), jTitle.get(),
                       jbanners, info.IsReachableByTaxi());
    if (info.IsFeature())
      InjectMetadata(env, g_mapObjectClazz, mapObject, info.GetMetadata());
    return mapObject;
  }

  ms::LatLon const ll = info.GetLatLon();
  search::AddressInfo const address =
      g_framework->NativeFramework()->GetAddressInfoAtPoint(info.GetMercator());

  // TODO(yunikkk): object can be POI + API + search result + bookmark simultaneously.
  // TODO(yunikkk): Should we pass localized strings here and in other methods as byte arrays?
  if (info.IsMyPosition())
    return CreateMapObject(env, kMyPosition, info.GetTitle(), info.GetSubtitle(), ll.lat, ll.lon,
                           address.FormatAddress(), {}, "", jbanners, info.IsReachableByTaxi());

  if (info.HasApiUrl())
    return CreateMapObject(env, kApiPoint, info.GetTitle(), info.GetSubtitle(), ll.lat, ll.lon,
                           address.FormatAddress(), info.GetMetadata(), info.GetApiUrl(),
                           jbanners, info.IsReachableByTaxi());

  return CreateMapObject(env, kPoi, info.GetTitle(), info.GetSubtitle(), ll.lat, ll.lon,
                         address.FormatAddress(),
                         info.IsFeature() ? info.GetMetadata() : Metadata(), "", jbanners,
                         info.IsReachableByTaxi());
}

jobjectArray ToBannersArray(JNIEnv * env, vector<ads::Banner> const & banners)
{
  return jni::ToJavaArray(env, g_bannerClazz, banners,
                          [](JNIEnv * env, ads::Banner const & item) {
                            return CreateBanner(env, item.m_bannerId, static_cast<jint>(item.m_type));
                          });
}
}  // namespace usermark_helper
