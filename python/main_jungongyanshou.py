import os
import shutil
import time

import openpyxl

from jzscclient import JzscClient
from setting import Setting
from stateutil_jungongyanshou import JgysStateUtil
from logutil import LogUtil


# 目录
g_config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'configs')
g_jgys_data_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data', '竣工验收')
os.makedirs(g_jgys_data_path, exist_ok=True)


def save_datas(datas):
    if len(datas) == 0:
        return

    # 根据最后一个数据获取表格文件名
    data_id = int(datas[-1].id)
    index = (data_id - 1) // Setting.get().jgys_count_per_file + 1
    excel_file_path = os.path.join(g_jgys_data_path, '竣工验收{}.xlsx'.format(index))
    if not os.path.exists(excel_file_path):
        src_excel_file = os.path.join(g_config_path, '竣工验收模板.xlsx')
        shutil.copyfile(src_excel_file, excel_file_path)

    while True:
        try:
            workbook = openpyxl.load_workbook(excel_file_path)
            sheet = workbook.worksheets[0]
            begin_row_index = sheet.max_row + 1
            for row in range(len(datas)):
                data = datas[row]
                current_row = begin_row_index + row
                sheet.cell(current_row, 1, data.id)
                sheet.cell(current_row, 2, data.project_id)
                sheet.cell(current_row, 3, data.data_level)
                sheet.cell(current_row, 4, data.shigong_license_id)
                sheet.cell(current_row, 5, data.actual_use_money)
                sheet.cell(current_row, 6, data.begin_date)
                sheet.cell(current_row, 7, data.end_date)
                sheet.cell(current_row, 8, data.data_source)
                sheet.cell(current_row, 9, data.construct_scale)
                sheet.cell(current_row, 10, data.remark)
            workbook.save(excel_file_path)
            break
        except Exception as e:
            print('保存数据遇到问题：{}'.format(e))
            print('如果表格文件被打开，请先关闭，过30秒程序将继续尝试保存')
            time.sleep(30)


def main():
    print('启动竣工验收信息采集工具')
    jzsc_client = JzscClient()
    datas = []
    not_has_data_count = 0  # 连续没有数据的个数
    begin_collect_id = max(Setting.get().jgys_begin_id, JgysStateUtil.get().next_collect_id)
    for collect_id in range(begin_collect_id, Setting.get().jgys_end_id):
        for _ in range(100000000):
            time.sleep(Setting.get().jgys_collect_interval)
            print('开始采集第{}个'.format(collect_id))
            request_success, data, _ = jzsc_client.get_jungongyanshou(collect_id)
            if not request_success:
                continue
            if data is None:
                not_has_data_count += 1
            else:
                not_has_data_count = 0
                datas.append(data)
            break

        # 采集结束或每采集到200条数据或需要换新文件的时候就保存
        is_finish = not_has_data_count >= 100
        if is_finish or len(datas) >= 200 or collect_id % Setting.get().jgys_count_per_file == 0:
            print('保存数据')
            save_datas(datas)
            print('保存数据完成')

            if len(datas) > 0:
                JgysStateUtil.get().update_next_collect_id(int(datas[-1].id)+1)
            datas = []

        if is_finish:
            print('采集完成')
            break


if __name__ == '__main__':
    LogUtil.file_name_prefix = 'jgys'
    LogUtil.enable()
    main()
