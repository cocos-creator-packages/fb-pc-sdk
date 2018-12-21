'use strict';
const Path = require('fire-path');
const Fs = require('fire-fs');
const xml2js = require("xml2js");
const spawn = require('child_process').spawn;
const {base} = Editor.require('app://editor/core/native-packer');

/**
 * 拷贝 pc sdk 的相关文件到proj.win32目录
 * @param options
 * @param cb
 * @returns {Promise.<void>}
 */
async function handleEvent(options, cb) {
    let config = Editor._projectProfile.data['facebook'];
    let errorTips = null;
    let basePacker = new base(options);
    do {
        if (!config || !config.enable || !config.pcsdk.enable) {
            break;
        }

        if (!options.actualPlatform.toLowerCase() === "win32") {
            break;
        }

        let libPath = Editor.url('packages://fb-pc-sdk/libs/facebooksdk');
        if (!Fs.existsSync(libPath)) {
            errorTips = 'facebook sdk library not found';
            break;
        }
        let nativePacker = new base(options);

        //拷贝facebook SDK
        let srcFbPath = Editor.url('packages://fb-pc-sdk/libs/facebooksdk');
        let destFbPath = Path.join(options.dest, 'frameworks/runtime-src/proj.win32/facebooksdk');
        nativePacker.ensureFile(srcFbPath, destFbPath);

        //拷贝facebook sdk的依赖文件
        //fixme:可能要换成异步的，同步拷贝太多个，可能会引起没有响应
        let srcSDKPath = Editor.url('packages://fb-pc-sdk/libs/facebooksdk/cpprestsdk');
        let destSDKPath = Path.join(options.dest, 'frameworks/runtime-src/proj.win32/facebooksdk/cpprestsdk');
        nativePacker.ensureFile(srcSDKPath, destSDKPath);

        //拷贝 dll 文件到对应目录
        let srcDllPath = Editor.url('packages://fb-pc-sdk/libs/dlls');
        let destDllPath = Path.join(options.dest, 'frameworks/runtime-src/proj.win32/dlls');
        nativePacker.ensureFile(srcDllPath, destDllPath);

        //为sln 工程添加 game sdk 的引用
        let slnResult = addConfigToSlnFile(basePacker);
        if (slnResult) {
            errorTips = slnResult;
            break;
        }

        //为 .vcxproj 文件添加 fb-pcsdk 需要的依赖文件
        let vcxResult = await addConfigToVcxProjFile(basePacker);
        if (vcxResult) {
            errorTips = vcxResult;
            break;
        }

        //为 .vcxproj.filters 添加过滤文件
        let filterResult = await addConfigToProjFilters(basePacker);
        if (filterResult) {
            errorTips = filterResult;
            break;
        }

        //拷贝C++文件
        let cppLibPath = Editor.url('packages://fb-pc-sdk/libs/cpp');
        let destCppPath = Path.join(options.dest, 'frameworks/runtime-src/classes');
        Fs.copySync(cppLibPath, destCppPath);

        //拷贝js文件
        let jsLibPath = Editor.url('packages://fb-pc-sdk/libs/js');
        let destJsPath = Path.join(options.dest, 'src');
        Fs.copySync(jsLibPath, destJsPath);

        //把对应的 pc-sdk 的 js 文件 require 到 main.js
        nativePacker.addRequireToMainJs('src/fb-pc-games-sdk.js');

        //注册 fb-pcsdk 的绑定
        let moduleRegisterPath = Path.join(options.dest, 'frameworks/runtime-src/classes', 'jsb_module_register.cpp');
        let moduleRegisterContent = Fs.readFileSync(moduleRegisterPath, 'utf-8');

        if (!moduleRegisterContent) {
            errorTips = 'read jsb_module_register.cpp fail';
            break;
        }

        let replaceIncludeStr = `#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)\n#include "jsb_facebook_games_sdk.hpp"\n#endif\n#include "cocos2d.h"`;
        let replaceRegStr = `se->addRegisterCallback(register_all_extension);\n\n#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)\n    se->addRegisterCallback(register_all_facebook_pc_games_sdk);\n#endif`;

        if (moduleRegisterContent.indexOf('jsb_facebook_games_sdk.hpp') === -1) {
            moduleRegisterContent = moduleRegisterContent.replace(/#include "cocos2d.h"/, replaceIncludeStr);
        }
        if (moduleRegisterContent.indexOf('register_all_facebook_pc_games_sdk') === -1) {
            moduleRegisterContent = moduleRegisterContent.replace(/se->addRegisterCallback\(register_all_extension\);/, replaceRegStr);
        }
        Fs.writeFileSync(moduleRegisterPath, moduleRegisterContent);

    } while (false);
    cb && cb(errorTips);
}

/**
 * 往 .sln 里面添加 pc-sdk 的工程引用
 * @param packer
 * @returns {*}
 */
function addConfigToSlnFile(packer) {
    let error = null;
    let destSlnPath = Path.join(packer.options.dest, 'frameworks/runtime-src/proj.win32/', `${packer.options.title}.sln`);
    do {
        if (!Fs.existsSync(destSlnPath)) {
            error = '.sln file not found';
            break;
        }
        let slnContent = Fs.readFileSync(destSlnPath, 'utf-8');

        //如果已经存在那么就略过了，直接返回
        if (slnContent.indexOf('game-sdk') !== -1) {
            break;
        }
        console.log("before", slnContent);
        slnContent = slnContent.replace(/Global/, `Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "game-sdk", "facebooksdk\\game-sdk.vcxproj", "{9F659167-2AA1-440A-BB76-F54A2ABA87F5}"\nEndProject\nGlobal`);
        console.log("after", slnContent);

        Fs.writeFileSync(destSlnPath, slnContent);
    } while (false);
    return error;
}

/**
 * 在 .vcxproj 添加 pc-sdk 的相关依赖
 * @param packer
 * @returns {Promise<void>}
 */
function addConfigToVcxProjFile(packer) {
    let error = null;
    let detVcxProjPath = Path.join(packer.options.dest, 'frameworks/runtime-src/proj.win32/', `${packer.options.title}.vcxproj`);
    do {
        if (!Fs.existsSync(detVcxProjPath)) {
            error = '.vcxproj file not found';
            break;
        }

        let content = Fs.readFileSync(detVcxProjPath, 'utf-8');
        if (!content) {
            error = 'parse .vcproj file fail';
            break;
        }

        let ptGroupReplaceStr = ['$(ProjectDir)facebooksdk\\cpprestsdk\\debug\\lib;', '$(ProjectDir)facebooksdk\\cpprestsdk\\lib;'];
        let ptGroupIdx = 0;

        if (content.indexOf(ptGroupReplaceStr[0]) === -1) {
            content = content.replace(/<LibraryPath>\$\(MSBuildProgramFiles32\)\\Microsoft SDKs\\Windows\\v7\.1A\\lib;\$\(LibraryPath\)<\/LibraryPath>/g, (string) => {
                string = string.replace('<LibraryPath>', '<LibraryPath>' + ptGroupReplaceStr[ptGroupIdx++]);
                string += `\n<IncludePath>$(ProjectDir)facebooksdk;$(ProjectDir)facebooksdk\\cpprestsdk\\include;$(IncludePath)</IncludePath>`;
                return string;
            });
        }

        let preBuildEventReplaceStr = ['if not exist "$(OutDir)" mkdir "$(OutDir)"\n' +
        'xcopy /Y /Q "$(ProjectDir)facebooksdk\\cpprestsdk\\debug\\bin\\*.dll" "$(OutDir)"\n' +
        'xcopy /Y /Q "$(ProjectDir)dlls\\*.dll" "$(OutDir)"', 'if not exist "$(OutDir)" mkdir "$(OutDir)"\n' +
        'xcopy /Y /Q "$(ProjectDir)facebooksdk\\cpprestsdk\\bin\\*.dll" "$(OutDir)"\n' +
        'xcopy /Y /Q "$(ProjectDir)dlls\\*.dll" "$(OutDir)"'];
        let preBuildIdx = 0;

        if (content.indexOf('xcopy /Y /Q $(ProjectDir)facebooksdk\\cpprestsdk') === -1) {
            content = content.replace(/<PreLinkEvent>\s*[^\\]+<\/PreLinkEvent>/g, (string) => {
                return string.replace(/<Command>\s*[^\\]+<\/Command>/, '<Command>' + preBuildEventReplaceStr[preBuildIdx++] + '</Command>');
            });
        }

        let addiDependenceReplaceStr = ['cpprest_2_10d.lib;', 'cpprest_2_10.lib;'];
        let addiIdx = 0;
//
        if (content.indexOf(addiDependenceReplaceStr[0]) === -1) {
            content = content.replace(/<AdditionalDependencies>\s*[^\\]+<\/AdditionalDependencies>/g, (str) => {
                let subStr = str.substring(24, str.length - 25);//获取<AdditionalDependencies>里面的内容
                return '<AdditionalDependencies>' + addiDependenceReplaceStr[addiIdx++] + subStr + '</AdditionalDependencies>';
            });
        }

        let clCompileStr = '<ClCompile Include="..\\Classes\\jsb_facebook_games_sdk.cpp" />';

        if (content.indexOf(clCompileStr) === -1) {
            content = content.replace('<ClCompile Include="..\\Classes\\jsb_module_register.cpp" />', clCompileStr + '\n<ClCompile Include="..\\Classes\\jsb_module_register.cpp" />')
        }

        let clIncludeStr = '<ClInclude Include="..\Classes\jsb_facebook_games_sdk.hpp" />';
        if (content.indexOf(clIncludeStr) === -1) {
            content = content.replace('<ClInclude Include="main.h" />', clIncludeStr + '\n<ClInclude Include="main.h" />')
        }
        Fs.writeFileSync(detVcxProjPath, content);
    } while (false);
    return error;
}

/**
 * 给 .vcxproj.filters 添加 pc sdk 的依赖
 * @param packer
 * @returns {Promise<*>}
 */
async function addConfigToProjFilters(packer) {
    let error = null;
    let filterPath = Path.join(packer.options.dest, 'frameworks/runtime-src/proj.win32/', `${packer.options.title}.vcxproj.filters`);

    do {
        let content = await packer.readXML(filterPath);
        if (!content) {
            error = 'parse .vcxproj.filters fail';
            break;
        }

        let itemGroup = content.Project.ItemGroup;
        if (!itemGroup) {
            break;
        }

        let clCompile = itemGroup.find(item => {
            return item.ClCompile;
        });

        if (!clCompile) {
            error = 'ClCompile not found';
            break;
        }

        let gameSDKPath = '..\\Classes\\jsb_facebook_games_sdk.cpp';
        let gameSDK = clCompile.ClCompile.find(item => {
            return item.$.Include.indexOf(gameSDKPath) !== -1;
        });

        if (!gameSDK) {
            clCompile.ClCompile.push({
                $: {Include: gameSDKPath},
                Filter: "Classes"
            })
        }

        let clInclude = itemGroup.find(item => {
            return item.ClInclude;
        });

        if (!clInclude) {
            error = 'ClCompile not found';
            break;
        }

        let gameSDKHeaderPath = '..\\Classes\\jsb_facebook_games_sdk.hpp';
        let gameSDKHeader = clInclude.ClInclude.find(item => {
            return item.$.Include.indexOf(gameSDKHeaderPath) !== -1;
        });

        if (!gameSDKHeader) {
            clInclude.ClInclude.push({
                $: {Include: gameSDKHeaderPath},
                Filter: "Classes"
            })
        }

        let builder = new xml2js.Builder();
        Fs.writeFileSync(filterPath, builder.buildObject(content));
    } while (false);
    return error;
}

function getCocosRoot() {
    let localProfile = Editor.Profile.load('profile://local/settings.json');
    let data = localProfile.data;
    if (localProfile.data['use-global-engine-setting'] !== false) {
        data = Editor.Profile.load('profile://global/settings.json').data;
    }
    return data['use-default-cpp-engine'] ? Editor.builtinCocosRoot : data['cpp-engine-path'];
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
