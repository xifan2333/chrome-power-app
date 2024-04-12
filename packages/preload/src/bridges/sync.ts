import { ipcRenderer} from 'electron';

/**
 * 同步桥接器对象，用于与主进程进行通信和控制。
 */
export const SyncBridge = {
  /**
   * 平铺窗口的异步方法。
   */
  async tileWindows() {
    const result = await ipcRenderer.invoke('tile-windows');
    return result;
  },
  async startGroupControl() {
    const result = await ipcRenderer.invoke('start-group-control');
    return result;
  },
};
