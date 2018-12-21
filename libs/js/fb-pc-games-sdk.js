window.fb = window.fb || {};

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
        if (!this.appId) {
            cc.warn('please call init first');
            return;
        }

        return this.sdkInstance.login();
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
}

fb.pcGameSDK = fb.pcGameSDK || new FBPCGameSDK();
