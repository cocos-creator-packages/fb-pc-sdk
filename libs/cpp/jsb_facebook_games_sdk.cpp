#include "scripting/js-bindings/manual/jsb_conversions.hpp"
#include "scripting/js-bindings/manual/jsb_global.h"

#include "Event.h"
#include "FacebookGameSDK.h"
#include "SystemDefaultBrowser.h"

#include "jsb_facebook_games_sdk.hpp"

#include <locale>
#include <codecvt>

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
			if (usr)
			{
				usr->getPermissions().get();
			}
			return usr;
		});
		// Calling future::get on a valid future blocks the thread until the provider makes the shared state ready
		loginUser = user.get();

		return loginUser;
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
		_isEventPrepared ? nullptr : _prepareEvent();

		facebook_games_sdk::Event::logEvent(event_name, details, value_to_sum);
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
	ret->setObject(obj);

	return true;
};

bool seval_to_FB_Strings(const se::Value& v, std::wstring* ret)
{
	assert(ret != nullptr);
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

	*ret = convert.from_bytes(v.toStringForce());
	return true;
};

bool seval_to_FB_StringsMap(const se::Value& v, std::map<utility::string_t, utility::string_t>* ret)
{
	assert(ret != nullptr);
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

	std::map<std::string, std::string> tmp;
	seval_to_std_map_string_string(v, &tmp);
	for (auto iter : tmp)
	{
		(*ret)[convert.from_bytes(iter.first)] = convert.from_bytes(iter.second);
	}

	return true;
};

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
	SE_PRECONDITION2(cobj, false, "js_FacebookPCGameSDK_init : Invalid Native Object");
	const auto& args = s.args();
	size_t argc = args.size();
	CC_UNUSED bool ok = true;
	if (argc == 1)
	{
		std::string arg0;
		ok &= seval_to_std_string(args[0], &arg0);
		SE_PRECONDITION2(ok, false, "js_FacebookPCGameSDK_init : Error processing arguments");
		cobj->initialize(arg0);
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 1);
	return false;
}
SE_BIND_FUNC(js_FacebookPCGameSDK_initialize)

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

bool js_register_FacebookPCGameSDK(se::Object* obj)
{
	auto cls = se::Class::create("FacebookPCGameSDK", obj, nullptr, nullptr);

	cls->defineStaticFunction("getInstance", _SE(js_FacebookPCGameSDK_getInstance));
	cls->defineFunction("initialize", _SE(js_FacebookPCGameSDK_initialize));
	cls->defineFunction("login", _SE(js_FacebookPCGameSDK_login));
	cls->defineFunction("setExternalInfo", _SE(js_FacebookPCGameSDK_setExternalInfo));
	cls->defineFunction("logEvent", _SE(js_FacebookPCGameSDK_logEvent));
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

