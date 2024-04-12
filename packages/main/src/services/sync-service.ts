import { ipcMain } from 'electron';
import { createLogger } from '../../../shared/utils/logger';
import { SERVICE_LOGGER_LABEL } from '../constants';
import { startGroupControl, tileWindows } from '../sync';

const logger = createLogger(SERVICE_LOGGER_LABEL);

export const initSyncService = () => {

  ipcMain.handle('tile-windows', async () => {
    const result = await tileWindows();
    logger.info('tile-windows', result);
    return result;
  });
  ipcMain.handle('start-group-control', async()=>{
    const result = await startGroupControl();
    logger.info('start-group-control', result);
    return result;

  });

};

