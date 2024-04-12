import {BrowserWindow, app, globalShortcut} from 'electron';
import './security-restrictions';
import {restoreOrCreateWindow} from '/@/mainWindow';
import {platform} from 'node:process';
import {db, initializeDatabase} from './db';
import {initServices} from './services';
import {createLogger} from '../../shared/utils/logger';
import {MAIN_LOGGER_LABEL} from './constants';
import {extractChromeBin} from './utils/extract';
import './server/index';

const logger = createLogger(MAIN_LOGGER_LABEL);

/**
 * Prevent electron from running multiple instances.
 */
const isSingleInstance = app.requestSingleInstanceLock();
if (!isSingleInstance) {
  app.quit();
  process.exit(0);
}
app.on('second-instance', restoreOrCreateWindow);

/**
 * Disable Hardware Acceleration to save more system resources.
 */
app.disableHardwareAcceleration();

/**
 * Shout down background process if all windows was closed
 */
app.on('window-all-closed', () => {
  if (platform !== 'darwin') {
    app.quit();
  }
});

/**
 * @see https://www.electronjs.org/docs/latest/api/app#event-activate-macos Event: 'activate'.
 */
app.on('activate', restoreOrCreateWindow);

/**
 * Create the application window when the background process is ready.
 */
app
  .whenReady()
  .then(async () => {
    globalShortcut.register('CommandOrControl+Shift+I', () => {
      const win = BrowserWindow.getFocusedWindow();
      if (win) {
        win.webContents.toggleDevTools();
      }
    });
    try {
      await initializeDatabase();
    } catch (error) {
      const errorString = error && typeof error === 'string' ? error : JSON.stringify(error);
      logger.error(`Failed initialize database: ${errorString}`);
    }
    await initServices();
    await restoreOrCreateWindow();
    if (!import.meta.env.DEV) {
      const {result, error, exist} = await extractChromeBin();
      if (result) {
        if (!exist) {
          logger.info('Extracted Chrome-bin.zip');
        }
      } else {
        logger.error('Failed extract Chrome-bin.zip, try to manually extract it', error);
      }
    }
  })
  .catch(e => logger.error('Failed create window:', e));


if (import.meta.env.PROD) {
  app
    .whenReady()
    .then(() =>
      require('electron-updater').autoUpdater.checkForUpdatesAndNotify(),
    )
    .catch(e => console.error('Failed check and install updates:', e));
}

app.on('before-quit', async () => {
  await db.destroy();
});
