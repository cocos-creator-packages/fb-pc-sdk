/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

 http://www.cocos.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated engine source code (the "Software"), a limited,
 worldwide, royalty-free, non-assignable, revocable and non-exclusive license
 to use Cocos Creator solely to develop games on your target platforms. You shall
 not use Cocos Creator software for developing other software or tools that's
 used for developing games. You are not granted to publish, distribute,
 sublicense, and/or sell copies of Cocos Creator.

 The software or tools in this License Agreement are licensed, not sold.
 Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

window.fb = window.fb || {};
fb.CONST = window.fb.CONST || {};
fb.CONST.kGraphAPIVersion = "v3.20";
fb.CONST.kFacebookDomain = "facebook.com";
fb.CONST.kScopes = "email";
fb.CONST.kLoginURL = "https://www.facebook.com/dialog/oauth";
fb.CONST.kRedirectURL = "http://localhost:8890";
fb.CONST.kParamEventName = "_eventName";
fb.CONST.kParamLogTime = "_logTime";
fb.CONST.kParamImplicitlyLogged = "_implicitlyLogged";
fb.CONST.kParamValueToSum = "_valueToSum";
fb.CONST.kParamCurrency = "fb_currency";
fb.CONST.kParamRegistrationMethod = "fb_registeration_method";
fb.CONST.kParamContentType = "fb_content_type";
fb.CONST.kParamContent = "fb_content";
fb.CONST.kParamContentId = "fb_content_id";
fb.CONST.kParamSearchString = "fb_search_string";
fb.CONST.kParamSuccess = "fb_success";
fb.CONST.kParamMaxRatingValue = "fb_max_rating_value";
fb.CONST.kParamPaymentInfoAvailable = "fb_payment_info_available";
fb.CONST.kParamNumItems = "fb_num_items";
fb.CONST.kParamLevel = "fb_level";
fb.CONST.kParamDescription = "fb_description";
fb.CONST.kEventActivateApp = "fb_mobile_activate_app";
fb.CONST.kEventCompletedRegistration = "fb_mobile_complete_registeration";
fb.CONST.kEventViewedContent = "fb_mobile_content_view";
fb.CONST.kEventSearch = "fb_mobile_search";
fb.CONST.kEventRate = "fb_mobile_rate";
fb.CONST.kEventCompletedTutorial = "fb_mobile_tutorial_completion";
fb.CONST.kEventAddedToCart = "fb_mobile_add_to_cart";
fb.CONST.kEventAddedToWishList = "fb_mobile_add_to_wishlist";
fb.CONST.kEventInitiatedCheckout = "fb_mobile_initiated_checkout";
fb.CONST.kEventAddedPaymentInfo = "fb_mobile_add_payment_info";
fb.CONST.kEventPurchased = "fb_mobile_purchase";
fb.CONST.kEventAchievedLevel = "fb_mobile_level_achieved";
fb.CONST.kEventUnlockedAchievement = "fb_mobile_achievement_unlocked";
fb.CONST.kEventSpentCredits = "fb_mobile_spent_credits";
fb.CONST.kEventTypeCustomAppEvents = "CUSTOM_APP_EVENTS";
fb.CONST.kParamAdvertiserID = "advertiser_id";
fb.CONST.kParamAdvertiserTracking = "advertiser_tracking_enabled";
fb.CONST.kParamApplicationTracking = "application_tracking_enabled";
fb.CONST.kParamValueInfoVer = "pcg1";
fb.CONST.kParamExternID = "extern_id";
fb.CONST.kParamExternNamespace = "extern_namespace";
fb.CONST.kParamExtInfo = "ext_info";
fb.CONST.kParamExtInfoVersion = "0";
fb.CONST.kParamExtInfoAppPkgName = "1";
fb.CONST.kParamExtInfoPkgVerCode = "2";
fb.CONST.kParamExtInfoPkgVerInfoName = "3";
fb.CONST.kParamExtInfoOsVersion = "4";
fb.CONST.kParamExtInfoDevModel = "5";
fb.CONST.kParamExtInfoLocale = "6";
fb.CONST.kParamExtInfoCpuCores = "12";
fb.CONST.kParamUserIdentity = "ud";

class FBPCGameSDK {
    constructor() {
        this.appId = null;
        if (!jsb || !jsb.FacebookPCGameSDK) {
            cc.warn('FacebookPCGamesSDK is not defined');
            return;
        }
        this.sdkInstance = jsb.FacebookPCGameSDK.getInstance();
    }

    init(appId) {
        this.appId = appId;
        if (!this.sdkInstance) {
            cc.warn('FacebookPCGamesSDK is not found');
            return;
        }
        this.sdkInstance.initialize(appId);
    }

    login() {
        return this._invokeFb('login');
    }

    permissionRequest() {
        return this._invokeFb('permissionRequest');
    }

    hasAccessToken() {
        return this._invokeFb('hasAccessToken');
    }

    getAccessToken() {
        return this._invokeFb('getAccessToken');
    }

    setExternalInfo(extendId, extendNamespace) {
        if (!this.appId) {
            cc.warn('please call init first');
            return;
        }
        if (!extendId || !extendNamespace) {
            cc.warn('extendId and extendNameSpace can not be null');
            return;
        }

        this.sdkInstance.setExternalInfo(extendId, extendNamespace);
    }

    logEvent(eventId, detail, valueToSum) {
        if (!this.appId) {
            cc.warn('please call init first');
            return;
        }
        if (arguments.length === 1) {
            this.sdkInstance.logEvent(eventId);
        } else if (arguments.length === 2) {
            this.sdkInstance.logEvent(eventId, detail);
        } else {
            this.sdkInstance.logEvent(eventId, detail, valueToSum);
        }
    }

    getFriends() {
        return this._invokeFb('getFriends');
    }

    getPermissions() {
        return this._invokeFb('getPermissions');
    }

    deauthorizeApp() {
        return this._invokeFb('deauthorizeApp');
    }

    logout() {
        this._invokeFb('logout');
    }

    getGraphVersion() {
        return this._invokeFb('getGraphVersion');
    }

    getGraphBaseURL() {
        return this._invokeFb('getGraphBaseURL');
    }

    _invokeFb(methodName) {
        if (!this.appId) {
            cc.warn('please call init first');
            return;
        }

        let ret = {};
        let result = this.sdkInstance[methodName]();
        console.log('_invokeFb result is ', result);
        if (result && typeof result === 'string') {
            try {
                ret = JSON.parse(result);
            } catch (e) {
                ret = result;
            }
        } else {
            ret = result;
        }
        return ret;
    }

    graphPost(path, params) {
        return this._graphAPI('POST', path, params);
    }

    graphGet(path, params) {
        return this._graphAPI('GET', path, params);
    }

    graphDelete(path, params) {
        return this._graphAPI('DELETE', path, params);
    }

    _graphAPI(method, path, params) {
        let ret = {};
        let result;
        if (!params) {
            result = this.sdkInstance[`graph${method.toUpperCase()}`](path);
        } else {
            result = this.sdkInstance[`graph${method.toUpperCase()}`](path, [params]);
        }
        if (result && typeof result === 'string') {
            try {
                ret = JSON.parse(result);
            } catch (e) {
                cc.error(`parse ${methodName} error`, e);
            }
        } else {
            ret = result;
        }
        return ret;
    }
}

fb.pcGameSDK = fb.pcGameSDK || new FBPCGameSDK();
