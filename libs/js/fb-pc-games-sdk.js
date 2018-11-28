window.fb = window.fb || {};

class FBPCGamesSDK {
  constructor() {
    this.appId = null;
    if (!jsb || !jsb.FacebookPCGamesSDK) {
      cc.warn('FacebookPCGamesSDK is not defined');
      return;
    }
    this.sdkInstance = new jsb.FacebookPCGamesSDK();
  }

  init(appId) {
    this.appId = appId;
    if (!this.sdkInstance) {
      cc.warn('FacebookPCGamesSDK is not found');
      return;
    }
    this.sdkInstance.init(appId);
  }

  login() {
    if (!this.appId) {
      cc.warn('please call init first');
      return;
    }

    return this.sdkInstance.login();
  }

  pushEvent(eventId, detail) {
    if (!this.appId) {
      cc.warn('please call init first');
      return;
    }
    this.sdkInstance.pushEvent(eventId);
  }
}

fb.pcGamesSDK = fb.pcGamesSDK || new FBPCGamesSDK();
