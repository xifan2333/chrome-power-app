import {Button, Card, Col, Row, Space, Table, Tabs, Input, Radio,message} from 'antd';
import {SyncBridge, WindowBridge} from '#preload';
import {useTranslation} from 'react-i18next';
import {useEffect, useState} from 'react';
import {MESSAGE_CONFIG} from '/@/constants';




const {TextArea} = Input;
const Sync = () => {
  const {t} = useTranslation();
  const [windows, setWindows] = useState<{ id: string }[]>([]);
  const [messageApi, contextHolder] = message.useMessage(MESSAGE_CONFIG);
  const fetchOpenedWindows = async () => {
    const windows = await WindowBridge.getOpenedWindows();
    if (windows) {
      setWindows(windows);
    }
  };

  useEffect(() => {
    fetchOpenedWindows();
  }, []);

  const handleTileWindows = async () => {
    const result = await SyncBridge.tileWindows();
    if (result?.success) {
      messageApi.success(t('sync_msg_tile_windows_success'));
    }
  };

  // type FieldType = SettingOptions;

  const handleGroupControl = async() => {
    await SyncBridge.startGroupControl();
  };

  const columns = [
    {
      title: t('sync_column_id'),
      dataIndex: 'id',

    },
    {
      title: t('sync_column_name'),
      dataIndex: 'name',

    },
    {
      title: t('sync_column_group'),
      dataIndex: 'group_name',

    },
    {
      title: t('sync_column_tags'),
      dataIndex: 'tags',

    },
  ];

  const tabItems = [
    {
      key: '1',
      label: t('sync_text_manager'),
      children: <div className="wrapper">
        <label>{t('sync_label_text_same')}</label>
        <Input type="text" className="mt-2 w-full h-12" />
        <Button className="mt-2 w-full h-12"  type="primary">{t('sync_btn_text_simulate')}</Button>
        <label className="mr-2">{t('sync_label_text_specified')}</label>
        <Radio className="mt-2">{t('sync_radio_text_order')}</Radio>
        <Radio className="mt-2">{t('sync_radio_text_random')}</Radio>
        <TextArea className="mt-2 w-full" rows={10} />
        <Button className="mt-2 w-full h-12"  type="primary">{t('sync_btn_text_simulate')}</Button>

      </div>,
    },
    {
      key: '2',
      label: t('sync_tab_manager'),
      children: (
        <div className="wrapper">
          <label>{t('sync_label_open_url')}</label>
          <TextArea
            className="mt-2"
            rows={4}
            placeholder={t('sync_placeholder_open_url')}
          />
          <Radio className='mt-2'>{t('sync_radio_open_url')}</Radio>
          <Button className="mt-2 w-full h-12" type="primary">
            {t('sync_btn_open_url')}
          </Button>
        </div>
      ),
    },
  ];

  return (

    <>
    {contextHolder}
      <Card
        className="content-card p-6"
        bordered={false}
      >
        <Row>
          <Col span={17}>
            <Space>
              <Button
                type="primary"
                onClick={handleTileWindows}
              >
                {t('tile_windows')}
              </Button>
              <Button
                type="primary"
                onClick={handleGroupControl}
              >
                {t('sync_btn_control')}
              </Button>
            </Space>
            <Table
              className="mt-4"
              dataSource={windows}
              columns={columns}
              rowKey="id"
            ></Table>
          </Col>
          <Col span={7}>
            <div className="wrapper ml-4">
              <h2 className="title bg-blue-100 text-gray-700 text-center text-lg rounded">
                {t('sync_operation_section')}
              </h2>
              <Tabs
                className="mt-4"
                defaultActiveKey="1"
                items={tabItems}

              >
              </Tabs>
            </div>
          </Col>
        </Row>
      </Card>
    </>
  );
};
export default Sync;
