#include "scripting/js-bindings/manual/jsb_conversions.hpp"
#include "scripting/js-bindings/manual/jsb_global.h"

#include "Event.h"
#include "FacebookGameSDK.h"
#include "SystemDefaultBrowser.h"

#include "jsb_facebook_games_sdk.hpp"

#include <locale>
#include <codecvt>
#include <strstream>

using namespace cocos2d;

// Delegate The Facebook PC Game SDK
class FacebookPCGameSDK
{
public:
    static FacebookPCGameSDK* getInstance()
    {
        static FacebookPCGameSDK* pInstance = nullptr;
        if (pInstance == nullptr)
        {
            pInstance = new (std::nothrow) FacebookPCGameSDK();
        }
        return pInstance;
    };

    // init, parameter is Facebook Developer Account ID
    bool initialize(std::string strAppId)
    {
        appId = std::stoull(strAppId);
        const auto browser = std::make_shared<facebook_games_sdk::SystemDefaultBrowser>(appId);
        facebook_games_sdk::FacebookGameSDK::initialize(appId, browser);
        return true;
    }

    // do login to get Facebook User Information
    std::shared_ptr<facebook_games_sdk::User> login()
    {
        auto user = facebook_games_sdk::FacebookGameSDK::getInstance().doLogin().then([=](std::shared_ptr<facebook_games_sdk::User> usr) {
            return usr;
        });
        // Calling future::get on a valid future blocks the thread until the provider makes the shared state ready
        loginUser = user.get();

        return loginUser;
    }

    // do PermissionRequest, just like do login()
    std::shared_ptr<facebook_games_sdk::User> permissionRequest()
    {
        auto userTask = facebook_games_sdk::FacebookGameSDK::getInstance().doPermissionRequest().then([=](std::shared_ptr<facebook_games_sdk::User> usr) {
            return usr;
        });
        // task, a call to get will wait for the task to finish
        // https://docs.microsoft.com/en-us/cpp/parallel/concrt/reference/task-class?view=vs-2017#get
        loginUser = userTask.get();

        return loginUser;
    }

    bool hasAccessToken()
    {
        return facebook_games_sdk::FacebookGameSDK::getInstance().hasAccessToken();
    }

    utility::string_t getAccessToken()
    {
        return facebook_games_sdk::FacebookGameSDK::getInstance().getAccessToken();
    }

    void setExternalInfo(
        const utility::string_t& extern_id,
        const utility::string_t& extern_namespace)
    {
        _isEventPrepared ? nullptr : _prepareEvent();

        // Add additional information about the user
        facebook_games_sdk::Event::setExternalInfo(extern_id, extern_namespace);
    }

    // log an event.
    void logEvent(const utility::string_t& event_name,
        const std::map<utility::string_t, utility::string_t>& details = {},
        const double value_to_sum = 0.0)
    {
        loginUser != nullptr ? nullptr : login();
        _isEventPrepared ? nullptr : _prepareEvent();

        facebook_games_sdk::Event::logEvent(event_name, details, value_to_sum);
    }

    std::vector<facebook_games_sdk::UserFriend> getFriends()
    {
        loginUser != nullptr ? nullptr : login();
        loginUser->getFriends().get();

        return loginUser->friends();
    }

    std::set<utility::string_t> getPermissions()
    {
        loginUser != nullptr ? nullptr : login();
        loginUser->getPermissions().get();

        return loginUser->permissions();
    }

    void deauthorizeApp()
    {
        loginUser->deauthorizeApp().get();
    }

    void logout()
    {
        loginUser != nullptr ? loginUser->logout() : nullptr;
    }

    // Graph Base Infos
    utility::string_t getGraphVersion()
    {
        return facebook_games_sdk::GraphAPI::getVersion();
    }
    utility::string_t getGraphBaseURL()
    {
        return facebook_games_sdk::GraphAPI::getBaseURL();
    }
    // GraphAPI (post/get/del), return json string
    utility::string_t graphPOST(
        const utility::string_t& path,
        const std::vector<utility::string_t>& args = {}
    )
    {
        loginUser != nullptr ? nullptr : login();

        auto responseTask = facebook_games_sdk::GraphAPI::post(loginUser->accessToken(), path, args).then(
            [=](json::value json_response) -> utility::string_t {
            return json_response.to_string();
        });
        return responseTask.get();
    }

    utility::string_t graphGET(
        const utility::string_t& path,
        const std::vector<utility::string_t>& args = {}
    )
    {
        loginUser != nullptr ? nullptr : login();

        auto responseTask = facebook_games_sdk::GraphAPI::get(loginUser->accessToken(), path, args).then(
            [=](json::value json_response) -> utility::string_t {
            return json_response.to_string();
        });
        return responseTask.get();
    }

    utility::string_t graphDELETE(
        const utility::string_t& path,
        const std::vector<utility::string_t>& args = {}
    )
    {
        loginUser != nullptr ? nullptr : login();

        auto responseTask = facebook_games_sdk::GraphAPI::del(loginUser->accessToken(), path, args).then(
            [=](json::value json_response) -> utility::string_t {
            return json_response.to_string();
        });
        return responseTask.get();
    }
private:
    FacebookPCGameSDK()
        : appId(0)
        , accessToken(U(""))
        , _isEventPrepared(false)
        , loginUser(nullptr)
    {
    }

    void _prepareEvent()
    {
        accessToken = loginUser ? loginUser->accessToken() : U("");
        // To send events to Facebook
        // You would be able to see the events in Analytics
        facebook_games_sdk::Event::initialize(appId, accessToken);

        _isEventPrepared = true;
    }

private:

    unsigned long long appId;

    std::wstring accessToken;

    bool _isEventPrepared;

    std::shared_ptr<facebook_games_sdk::User> loginUser;
};

// conversions
bool FB_User_to_seval(std::shared_ptr<facebook_games_sdk::User> user, se::Value* ret)
{
    assert(ret != nullptr);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    se::HandleObject obj(se::Object::createPlainObject());
    obj->setProperty("email", se::Value(convert.to_bytes(user->email())));
    obj->setProperty("name", se::Value(convert.to_bytes(user->name())));
    obj->setProperty("accessToken", se::Value(convert.to_bytes(user->accessToken())));
    obj->setProperty("hasPicture", se::Value(user->hasPicture()));
    if (user->hasPicture())
    {
        se::HandleObject pic(se::Object::createPlainObject());
        pic->setProperty("width", se::Value(user->picture().width));
        pic->setProperty("height", se::Value(user->picture().height));
        pic->setProperty("url", se::Value(convert.to_bytes(user->picture().url)));

        se::Value picture;
        picture.setObject(pic);
        obj->setProperty("picture", picture);
    }
    ret->setObject(obj);

    return true;
};

bool seval_to_FB_Strings(const se::Value& v, std::wstring* ret)
{
    assert(ret != nullptr);

    SE_PRECONDITION3(!v.isNullOrUndefined(), false, ret->clear());

    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    *ret = convert.from_bytes(v.toStringForce());
    return true;
};

bool FB_Strings_to_seval(const std::wstring& w, se::Value* ret)
{
    assert(ret != nullptr);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    ret->setString(convert.to_bytes(w));
    return true;
};

bool seval_to_FB_StringsMap(const se::Value& v, std::map<utility::string_t, utility::string_t>* ret)
{
    assert(ret != nullptr);

    SE_PRECONDITION3(!v.isNullOrUndefined(), false, ret->clear());

    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    std::map<std::string, std::string> tmp;
    seval_to_std_map_string_string(v, &tmp);
    for (auto iter : tmp)
    {
        (*ret)[convert.from_bytes(iter.first)] = convert.from_bytes(iter.second);
    }

    return true;
};

bool seval_to_FB_StringsVector(const se::Value& v, std::vector<utility::string_t>* ret)
{
    assert(ret != nullptr);

    SE_PRECONDITION3(!v.isNullOrUndefined(), false, ret->clear());

    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    std::vector<std::string> tmp;
    seval_to_std_vector_string(v, &tmp);
    for (auto item : tmp)
    {
        ret->push_back(convert.from_bytes(item));
    }
    return true;
};

// only FBID_t info in UserFriend struct
bool FB_Friends_to_seval(std::vector<facebook_games_sdk::UserFriend> &friends, se::Value* ret)
{
    assert(ret != nullptr);

    // FBID_t type is unsigned long long, we can't put it into se::Value directly, so convert to std::string
    std::vector<std::string> friendsInfo;
    for (const auto& fri : friends)
    {
        std::string item;
        std::strstream strS;
        strS << fri.getId();
        strS >> item;
        friendsInfo.push_back(item);
    }
    return std_vector_string_to_seval(friendsInfo, ret);
};

bool FB_Permissions_to_seval(std::set<utility::string_t> &permissions, se::Value* ret)
{
    assert(ret != nullptr);

    std::vector<std::string> permissionsInfo;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    for (const auto& perm : permissions)
    {
        permissionsInfo.push_back(convert.to_bytes(perm));
    }
    return std_vector_string_to_seval(permissionsInfo, ret);
}

se::Object* __jsb_FacebookPCGameSDK_proto = nullptr;
se::Class* __jsb_FacebookPCGameSDK_class = nullptr;

static bool js_FacebookPCGameSDK_getInstance(se::State& s)
{
    const auto& args = s.args();
    size_t argc = args.size();
    CC_UNUSED bool ok = true;
    if (argc == 0)
    {
        FacebookPCGameSDK* fbSDK = FacebookPCGameSDK::getInstance();
        // refine, check whether need to add a destoryInstance to free rooted object
        ok &= native_ptr_to_rooted_seval<FacebookPCGameSDK>(fbSDK, __jsb_FacebookPCGameSDK_class, &s.rval());
        SE_PRECONDITION2(ok, false, "js_FacebookPCGameSDK_getInstance : Error processing arguments");

        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_getInstance)

bool js_FacebookPCGameSDK_initialize(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_initialize : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    CC_UNUSED bool ok = true;
    if (argc == 1)
    {
        std::string arg0;
        ok &= seval_to_std_string(args[0], &arg0);
        SE_PRECONDITION2(ok, false, "js_FacebookPCGameSDK_initialize : Error processing arguments");
        cobj->initialize(arg0);
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 1);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_initialize)

bool js_FacebookPCGameSDK_login(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_login : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0)
    {
        std::shared_ptr<facebook_games_sdk::User> user = cobj->login();
        bool ok = FB_User_to_seval(user, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_User_to_seval failed");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_login)

bool js_FacebookPCGameSDK_permissionRequest(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_permissionRequest : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0)
    {
        std::shared_ptr<facebook_games_sdk::User> user = cobj->permissionRequest();
        bool ok = FB_User_to_seval(user, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_User_to_seval failed");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_permissionRequest)

bool js_FacebookPCGameSDK_hasAccessToken(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_hasAccessToken : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0) {
        auto has = cobj->hasAccessToken();
        s.rval().setBoolean(has);
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_hasAccessToken)

bool js_FacebookPCGameSDK_getAccessToken(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_getAccessToken : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0) {
        auto token = cobj->getAccessToken();
        bool ok = FB_Strings_to_seval(token, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval Error");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_getAccessToken)

bool js_FacebookPCGameSDK_setExternalInfo(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_setExternalInfo : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    CC_UNUSED bool ok = true;
    if (argc == 2)
    {
        std::wstring arg0, arg1;
        ok &= seval_to_FB_Strings(args[0], &arg0);
        ok &= seval_to_FB_Strings(args[1], &arg1);
        SE_PRECONDITION2(ok, false, "Error processing arguments");
        cobj->setExternalInfo(arg0, arg1);
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d", (int)argc);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_setExternalInfo)

bool js_FacebookPCGameSDK_logEvent(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_logEvent : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    CC_UNUSED bool ok = true;
    if (argc == 1)
    {
        std::wstring event_name;
        ok &= seval_to_FB_Strings(args[0], &event_name);
        SE_PRECONDITION2(ok, false, "Error processing arguments");
        cobj->logEvent(event_name);
        return true;
    }
    else if (argc == 2)
    {
        std::wstring event_name;
        ok &= seval_to_FB_Strings(args[0], &event_name);
        std::map<utility::string_t, utility::string_t> stringsMap;
        ok &= seval_to_FB_StringsMap(args[1], &stringsMap);
        SE_PRECONDITION2(ok, false, "Error processing arguments");
        cobj->logEvent(event_name, stringsMap);
        return true;
    }
    else if (argc == 3)
    {
        std::wstring event_name;
        ok &= seval_to_FB_Strings(args[0], &event_name);
        std::map<utility::string_t, utility::string_t> stringsMap;
        ok &= seval_to_FB_StringsMap(args[1], &stringsMap);
        double value_to_sum;
        ok &= seval_to_double(args[2], &value_to_sum);
        SE_PRECONDITION2(ok, false, "Error processing arguments");
        cobj->logEvent(event_name, stringsMap, value_to_sum);
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d", (int)argc);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_logEvent)



bool js_FacebookPCGameSDK_getFriends(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_getFriends : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0) {
        bool ok = FB_Friends_to_seval(cobj->getFriends(), &s.rval());
        SE_PRECONDITION2(ok, false, "FB_User_to_seval failed");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_getFriends)

bool js_FacebookPCGameSDK_getPermissions(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_getPermissions : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0) {
        bool ok = FB_Permissions_to_seval(cobj->getPermissions(), &s.rval());
        SE_PRECONDITION2(ok, false, "FB_User_to_seval failed");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_getPermissions)

bool js_FacebookPCGameSDK_deauthorizeApp(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_deauthorizeApp : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0) {
        cobj->deauthorizeApp();
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_deauthorizeApp)

bool js_FacebookPCGameSDK_logout(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_logout : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0) {
        cobj->logout();
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_logout)


bool js_FacebookPCGameSDK_getGraphVersion(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_getAccessToken : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0) {
        auto version = cobj->getGraphVersion();
        bool ok = FB_Strings_to_seval(version, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval Error");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_getGraphVersion)

bool js_FacebookPCGameSDK_getGraphBaseURL(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_getAccessToken : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc == 0) {
        auto baseURL = cobj->getGraphBaseURL();
        bool ok = FB_Strings_to_seval(baseURL, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval Error");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_getGraphBaseURL)

bool js_FacebookPCGameSDK_graphPOST(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_getAccessToken : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    bool ok = true;
    if (argc == 1)
    {
        std::wstring arg0;
        ok &= seval_to_FB_Strings(args[0], &arg0);
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        auto response = cobj->graphPOST(arg0);
        ok = FB_Strings_to_seval(response, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        return true;
    }
    else if (argc == 2)
    {
        std::wstring arg0;
        std::vector<std::wstring> arg1;
        ok &= seval_to_FB_Strings(args[0], &arg0);
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        ok &= seval_to_FB_StringsVector(args[1], &arg1);
        SE_PRECONDITION2(ok, false, "seval_to_FB_StringsVector failed");
        // call Graph API
        auto response = cobj->graphPOST(arg0, arg1);
        ok = FB_Strings_to_seval(response, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting >= %d", (int)argc, 1);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_graphPOST)

bool js_FacebookPCGameSDK_graphGET(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_getAccessToken : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    bool ok = true;
    if (argc == 1)
    {
        std::wstring arg0;
        ok &= seval_to_FB_Strings(args[0], &arg0);
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        auto response = cobj->graphGET(arg0);
        ok = FB_Strings_to_seval(response, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        return true;
    }
    else if (argc == 2)
    {
        std::wstring arg0;
        std::vector<std::wstring> arg1;
        ok &= seval_to_FB_Strings(args[0], &arg0);
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        ok &= seval_to_FB_StringsVector(args[1], &arg1);
        SE_PRECONDITION2(ok, false, "seval_to_FB_StringsVector failed");
        // call Graph API
        auto response = cobj->graphGET(arg0, arg1);
        ok = FB_Strings_to_seval(response, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting >= %d", (int)argc, 1);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_graphGET)

bool js_FacebookPCGameSDK_graphDELETE(se::State& s)
{
    FacebookPCGameSDK* cobj = (FacebookPCGameSDK*)s.nativeThisObject();
    SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_getAccessToken : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    bool ok = true;
    if (argc == 1)
    {
        std::wstring arg0;
        ok &= seval_to_FB_Strings(args[0], &arg0);
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        // call Graph API
        auto response = cobj->graphDELETE(arg0);
        ok = FB_Strings_to_seval(response, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        return true;
    }
    else if (argc == 2)
    {
        std::wstring arg0;
        std::vector<std::wstring> arg1;
        ok &= seval_to_FB_Strings(args[0], &arg0);
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        ok &= seval_to_FB_StringsVector(args[1], &arg1);
        SE_PRECONDITION2(ok, false, "seval_to_FB_StringsVector failed");
        // call Graph API
        auto response = cobj->graphDELETE(arg0, arg1);
        ok = FB_Strings_to_seval(response, &s.rval());
        SE_PRECONDITION2(ok, false, "FB_Strings_to_seval failed");
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting >= %d", (int)argc, 1);
    return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_graphDELETE)

bool js_register_FacebookPCGameSDK(se::Object* obj)
{
    auto cls = se::Class::create("FacebookPCGameSDK", obj, nullptr, nullptr);
    // sdk manager
    cls->defineStaticFunction("getInstance", _SE(js_FacebookPCGameSDK_getInstance));
    cls->defineFunction("initialize", _SE(js_FacebookPCGameSDK_initialize));
    cls->defineFunction("login", _SE(js_FacebookPCGameSDK_login));
    cls->defineFunction("permissionRequest", _SE(js_FacebookPCGameSDK_permissionRequest));
    cls->defineFunction("hasAccessToken", _SE(js_FacebookPCGameSDK_hasAccessToken));
    cls->defineFunction("getAccessToken", _SE(js_FacebookPCGameSDK_getAccessToken));
    // event interface, for facebook analysis
    cls->defineFunction("setExternalInfo", _SE(js_FacebookPCGameSDK_setExternalInfo));
    cls->defineFunction("logEvent", _SE(js_FacebookPCGameSDK_logEvent));
    // user interface
    cls->defineFunction("getFriends", _SE(js_FacebookPCGameSDK_getFriends));
    cls->defineFunction("getPermissions", _SE(js_FacebookPCGameSDK_getPermissions));
    cls->defineFunction("deauthorizeApp", _SE(js_FacebookPCGameSDK_deauthorizeApp));
    cls->defineFunction("logout", _SE(js_FacebookPCGameSDK_logout));
    // Graph Base Infos
    cls->defineFunction("getGraphVersion", _SE(js_FacebookPCGameSDK_getGraphVersion));
    cls->defineFunction("getGraphBaseURL", _SE(js_FacebookPCGameSDK_getGraphBaseURL));
    // GraphAPI kernal API, return JSON string
    cls->defineFunction("graphPOST", _SE(js_FacebookPCGameSDK_graphPOST));
    cls->defineFunction("graphGET", _SE(js_FacebookPCGameSDK_graphGET));
    cls->defineFunction("graphDELETE", _SE(js_FacebookPCGameSDK_graphDELETE));

    cls->install();
    JSBClassType::registerClass<FacebookPCGameSDK>(cls);

    __jsb_FacebookPCGameSDK_proto = cls->getProto();
    __jsb_FacebookPCGameSDK_class = cls;

    se::ScriptEngine::getInstance()->clearException();
    return true;
}

bool register_all_facebook_pc_games_sdk(se::Object* obj)
{
    // Get the ns
    se::Value nsVal;
    if (!obj->getProperty("jsb", &nsVal))
    {
        se::HandleObject jsobj(se::Object::createPlainObject());
        nsVal.setObject(jsobj);
        obj->setProperty("jsb", nsVal);
    }
    se::Object* ns = nsVal.toObject();

    js_register_FacebookPCGameSDK(ns);
    return true;
}