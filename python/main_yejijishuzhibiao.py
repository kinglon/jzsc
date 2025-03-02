import os
import shutil
import time

import openpyxl

from jzscclient import JzscClient
from setting import Setting
from stateutil_yejijishuzhibiao import YjjszbStateUtil
from logutil import LogUtil


# 目录
g_config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'configs')
g_yjjszb_data_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data', '业绩技术指标')
os.makedirs(g_yjjszb_data_path, exist_ok=True)


def save_datas(datas):
    if len(datas) == 0:
        return

    # 根据最后一个数据获取表格文件名
    data_id = int(datas[-1].id)
    index = (data_id - 1) // Setting.get().yjjszb_count_per_file + 1
    excel_file_path = os.path.join(g_yjjszb_data_path, '业绩技术指标{}.xlsx'.format(index))
    if not os.path.exists(excel_file_path):
        src_excel_file = os.path.join(g_config_path, '业绩技术指标模板.xlsx')
        shutil.copyfile(src_excel_file, excel_file_path)

    while True:
        try:
            workbook = openpyxl.load_workbook(excel_file_path)
            sheet = workbook.worksheets[0]
            current_row = sheet.max_row + 1
            for row in range(len(datas)):
                data = datas[row]
                sheet.cell(current_row, 1, data.id)
                sheet.cell(current_row, 2, data.project_id)
                sheet.cell(current_row, 3, data.zizibiaozhun)
                sheet.cell(current_row, 4, data.begin_date)
                sheet.cell(current_row, 5, data.end_date)
                sheet.cell(current_row, 6, data.guimo_dengji)
                sheet.cell(current_row, 7, data.data_level)
                for person in data.people:
                    sheet.cell(current_row, 8, person.name)
                    sheet.cell(current_row, 9, person.shenfenzheng_id)
                    sheet.cell(current_row, 10, person.role)
                    current_row += 1
            workbook.save(excel_file_path)
            break
        except Exception as e:
            print('保存数据遇到问题：{}'.format(e))
            print('如果表格文件被打开，请先关闭，过30秒程序将继续尝试保存')
            time.sleep(30)


def main():
    print('启动业绩技术指标信息采集工具')
    jzsc_client = JzscClient()
    datas = []
    not_has_data_count = 0  # 连续没有数据的个数
    begin_collect_id = max(Setting.get().yjjszb_begin_id, YjjszbStateUtil.get().next_collect_id)
    for collect_id in range(begin_collect_id, Setting.get().yjjszb_end_id):
        data = None
        for _ in range(100000000):
            time.sleep(Setting.get().yjjszb_collect_interval)
            print('开始采集第{}个业绩技术指标'.format(collect_id))
            request_success, data, _ = jzsc_client.get_jishuzhibiao(collect_id)
            if not request_success:
                continue
            if data is None:
                not_has_data_count += 1
            else:
                not_has_data_count = 0
                datas.append(data)
            break

        if data is None:
            continue

        for _ in range(100000000):
            time.sleep(Setting.get().yjjszb_collect_interval)
            print('开始采集第{}个业绩技术指标的相关人员'.format(collect_id))
            request_success, people, _ = jzsc_client.get_xiangguanrenyuan(datas[-1].yejijilubianhao)
            if not request_success:
                continue
            datas[-1].people = people
            break

        # 采集结束或每采集到200条数据或需要换新文件的时候就保存
        is_finish = not_has_data_count >= 100
        if is_finish or len(datas) >= 200 or collect_id % Setting.get().yjjszb_count_per_file == 0:
            print('保存数据')
            save_datas(datas)
            print('保存数据完成')

            if len(datas) > 0:
                YjjszbStateUtil.get().update_next_collect_id(int(datas[-1].id)+1)
            datas = []

        if is_finish:
            print('采集完成')
            break


if __name__ == '__main__':
    LogUtil.file_name_prefix = 'yjjszb'
    LogUtil.enable()
    main()
