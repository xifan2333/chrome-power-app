// import {ipcRenderer} from 'electron';
import type {SafeAny} from '../../../shared/types/db';

// 预定义一个变量，用于存储原生插件的引用
let windowAddon: unknown;
import * as path from 'path';
// 根据开发环境加载不同的原生插件
if (process.env.MODE === 'development') {
  // 如果为开发环境，则加载编译后的插件
  windowAddon = require(path.join(
    __dirname,
    '../src/native-addon/build/Release/window-addon.node',
  ));
} else {
  // 如果为生产环境，则加载 asar 包内的插件
  windowAddon = require(path.join(
    process.resourcesPath,
    'app.asar.unpacked',
    'node_modules',
    'window-addon',
    'window-addon.node',
  ));
}
export const tileWindows = async () => {
  try {
    const result = (windowAddon as unknown as SafeAny)!.tileChromeWindows();
    return result;
  } catch (error) {
    console.error(error);
  }
};
export const startGroupControl = async () => {
  try {
    const result = (windowAddon as unknown as SafeAny)!.startGroupControl();
    return result;
  } catch (error) {
    console.error(error);
  }
};




