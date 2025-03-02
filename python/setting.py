import os


class Setting:
    __instance = None

    @staticmethod
    def get():
        if Setting.__instance is None:
            Setting.__instance = Setting()
        return Setting.__instance

    def __init__(self):
        # 竣工验收采集间隔秒数
        self.jgys_collect_interval = 1

        # 竣工验收采集ID范围
        self.jgys_begin_id = 1
        self.jgys_end_id = 100000000

        # 竣工验收每个表格个数
        self.jgys_count_per_file = 5000

        # 业绩技术指标采集间隔秒数
        self.yjjszb_collect_interval = 1

        # 业绩技术指标采集ID范围
        self.yjjszb_begin_id = 1
        self.yjjszb_end_id = 100000000

        # 业绩技术指标每个表格个数
        self.yjjszb_count_per_file = 5000

        self.load()

    def load(self):
        current_file_path = os.path.dirname(os.path.abspath(__file__))

        # 加载设置
        config_file_path = os.path.join(current_file_path, r'configs\configs.txt')
        with open(config_file_path, 'r', encoding='utf-8') as file:
            for line in file:
                # Remove newline characters and process each line
                processed_line = line.strip()
                if processed_line.find('#') == 0:
                    # 注释行忽略
                    continue
                parts = processed_line.split('=')
                if len(parts) != 2:
                    continue
                key = parts[0].strip()
                value = parts[1].strip()
                if key == '竣工验收采集间隔':
                    self.jgys_collect_interval = int(value) / 1000
                elif key == '竣工验收采集开始ID':
                    self.jgys_begin_id = int(value)
                elif key == '竣工验收采集结束ID':
                    self.jgys_end_id = int(value)
                elif key == '竣工验收每个表格个数':
                    self.jgys_count_per_file = int(value)
                elif key == '业绩技术指标采集间隔':
                    self.yjjszb_collect_interval = int(value) / 1000
                elif key == '业绩技术指标采集开始ID':
                    self.yjjszb_begin_id = int(value)
                elif key == '业绩技术指标采集结束ID':
                    self.yjjszb_end_id = int(value)
                elif key == '业绩技术指标每个表格个数':
                    self.yjjszb_count_per_file = int(value)
