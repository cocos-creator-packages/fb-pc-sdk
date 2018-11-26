'use strict';
const Path = require('fire-path');
const Fs = require('fire-fs');
const {base} = Editor.require('app://editor/core/native-packer');

/**
 * 拷贝 pc sdk 的相关文件到proj.win32目录
 * @param options
 * @param cb
 * @returns {Promise.<void>}
 */
async function handleEvent(options, cb) {
    let config = Editor._projectProfile.data['facebook'];
    let error_tips = null;
    do {
        if (!config || !config.enable || !config.pcsdk.enable) {
            break;
        }

        if (!options.actualPlatform.toLowerCase() === "win32") {
            break;
        }

        let libPath = Editor.url('packages://fb-pc-sdk/libs/facebooksdk');
        if (!Fs.existsSync(libPath)) {
            error_tips = 'facebook sdk library not found';
            break;
        }
        let nativePacker = new base(options);

        //拷贝js文件
        let jsLibPath = Editor.url('packages://fb-pc-sdk/libs/js');
        let destJsPath = Path.join(options.dest, 'src');
        Fs.copySync(jsLibPath, destJsPath);

        nativePacker.addRequireToMainJs('src/fb-pc-games-sdk.js');

        //拷贝facebook SDK
        let srcFbPath = Editor.url('packages://fb-pc-sdk/libs/facebooksdk');
        let destFbPath = Path.join(options.dest, 'frameworks/runtime-src/proj.win32/facebooksdk');
        nativePacker.ensureFile(srcFbPath, destFbPath);

        //拷贝facebook sdk的依赖文件
        let srcSDKPath = Editor.url('packages://fb-pc-sdk/libs/facebooksdk/cpprestsdk');
        let destSDKPath = Path.join(options.dest, 'frameworks/runtime-src/proj.win32/facebooksdk/cpprestsdk');
        nativePacker.ensureFile(srcSDKPath, destSDKPath);
    } while (false);
    cb && cb(error_tips);
}

module.exports = {
    load() {
        Editor.Builder.on('before-change-files', handleEvent);
    },

    unload() {
        Editor.Builder.removeListener('before-change-files', handleEvent);
    },

    messages: {}
};
