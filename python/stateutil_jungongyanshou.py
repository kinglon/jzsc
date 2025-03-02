import json
import os


# 竣工验收状态工具
class JgysStateUtil:
    __instance = None

    @staticmethod
    def get():
        if JgysStateUtil.__instance is None:
            JgysStateUtil.__instance = JgysStateUtil()
        return JgysStateUtil.__instance

    def __init__(self):
        # 下一个采集ID
        self.next_collect_id = 1

        self.load()

    def load(self):
        current_file_path = os.path.dirname(os.path.abspath(__file__))
        config_file_path = os.path.join(current_file_path, r'configs\states_jungongyanshou.json')
        if not os.path.exists(config_file_path):
            return
        with open(config_file_path, 'r', encoding='utf-8') as file:
            json_data = file.read()
            root = json.loads(json_data)
            self.next_collect_id = root['next_collect_id']

    def _save(self):
        current_file_path = os.path.dirname(os.path.abspath(__file__))
        config_file_path = os.path.join(current_file_path, r'configs\states_jungongyanshou.json')
        root = {'next_collect_id': self.next_collect_id}

        with open(config_file_path, "w", encoding='utf-8') as file:
            json.dump(root, file, indent=4)

    # 更新下一个采集ID
    def update_next_collect_id(self, next_id):
        self.next_collect_id = next_id
        self._save()
