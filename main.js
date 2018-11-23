'use strict';
const Path = require('fire-path');
const Fs = require('fire-fs');

/**
 * 拷贝 pc sdk 的相关文件到proj.win32目录
 * @param options
 * @param cb
 * @returns {Promise.<void>}
 */
async function handleEvent(options, cb) {
    let config = Editor._projectProfile.data['facebook'];

    if (!config || !config.enable || !config['pc-sdk'].enable) {
        cb && cb();
        return;
    }

    if (options.actualPlatform.toLowerCase() === "win32") {
        cb && cb();
        return;
    }

    let libPath = Editor.url('packages://fb-pc-sdk/facebooksdk');
    if (!Fs.existsSync(libPath)) {
        cb('facebook sdk library not found');
        return;
    }

    let destPath = Path.join(options.dest, 'frameworks/runtime-src/proj.win32');
    if (!Fs.existsSync(destPath)) {
        cb(`${destPath} not found`);
        return;
    }

    Fs.copySync(libPath, destPath);
    cb && cb();
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
