import json
from datetime import datetime

import requests
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

from datamodel import *


class JzscClient:
    def __init__(self):
        self.host = 'https://jzsc.mohurd.gov.cn/'
        self.proxies = {"http": "", "https": ""}
        self.timeout = 20
        self.iv = self.int_array_to_bytes([808530483, 875902519, 943276354, 1128547654])
        self.key = self.int_array_to_bytes([1148467306, 964118391, 624314466, 2019968622])

    @staticmethod
    def int_array_to_bytes(intarray):
        bytes_obj = bytearray()
        for x in intarray:
            bytes_obj.extend(x.to_bytes(4, 'big'))
        return bytes_obj

    def get_common_request_header(self):
        headers = {
            'Content-Type': 'application/json',
            'Accept': 'application/json, text/plain, */*',
            'Accept-Encoding': 'gzip, deflate, br, zstd',
            'Origin': self.host,
            'Referer': self.host,
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36',
        }
        return headers

    def decode_response(self, data):
        data = bytes.fromhex(data)
        cipher = Cipher(algorithms.AES(self.key), modes.CBC(self.iv), backend=default_backend())
        decryptor = cipher.decryptor()
        plaintext_padded = decryptor.update(data) + decryptor.finalize()
        unpadder = padding.PKCS7(128).unpadder()
        plaintext = unpadder.update(plaintext_padded) + unpadder.finalize()
        plaintext = plaintext.decode('utf-8')
        return plaintext

    # 查询竣工验收
    # jungongyanshou_id 竣工验收ID
    # 返回 (request success, None|JungongYanshou, error_message)
    def get_jungongyanshou(self, jungongyanshou_id):
        error_result = (False, None, '')

        try:
            uri = '/APi/webApi/dataservice/query/project/projectFinishManageDetail?id={}'.format(jungongyanshou_id)
            url = self.host + uri
            headers = self.get_common_request_header()
            headers['Timeout'] = '30000'
            headers['V'] = '231012'
            response = requests.get(url, headers=headers, proxies=self.proxies, timeout=5)
            if not response.ok:
                print("查询竣工验收失败，错误是：{}".format(response))
                return error_result
            else:
                data = response.content.decode('utf-8')
                data = self.decode_response(data)
                root = json.loads(data)

                if root['code'] != 200:
                    print("查询竣工验收失败，错误是：code={}".format(root['code']))
                    return error_result

                if 'data' not in root:
                    print("查询竣工验收失败，错误是：缺失data字段")
                    return error_result

                data_object = root['data']
                if 'PRJNUM' not in data_object:
                    return True, None, ''

                jungongyanshou = JungongYanshou()
                jungongyanshou.id = str(jungongyanshou_id)
                jungongyanshou.project_id = str(data_object['PRJNUM'])
                jungongyanshou.data_level = str(data_object['DATALEVEL'])
                jungongyanshou.shigong_license_id = data_object['BUILDERLICENCENUM']
                jungongyanshou.actual_use_money = str(data_object['FACTCOST'])
                if 'BDATE' in data_object and data_object['BDATE']:
                    jungongyanshou.begin_date = datetime.fromtimestamp(float(data_object['BDATE'])/1000).strftime('%Y-%m-%d')
                if 'EDATE' in data_object and data_object['EDATE']:
                    jungongyanshou.end_date = datetime.fromtimestamp(float(data_object['EDATE'])/1000).strftime('%Y-%m-%d')
                jungongyanshou.data_source = JzscClient.get_jungongyanshou_data_source(data_object)
                jungongyanshou.construct_scale = data_object['FACTSIZE']
                jungongyanshou.remark = data_object['MARK']
                if jungongyanshou.remark is None:
                    jungongyanshou.remark = ''

                return True, jungongyanshou, ''
        except Exception as e:
            print("查询竣工验收失败，错误是：{}".format(e))
            return error_result

    @staticmethod
    def get_jungongyanshou_data_source(data_object):
        if 'DATASOURCE' not in data_object or data_object['DATASOURCE'] is None:
            return ''

        value = data_object['DATASOURCE']
        if value == 1:
            return '业务办理'
        elif value == 2:
            return '信息登记'
        elif value == 3:
            return '历史业绩补录'
        else:
            return str(value)

    # 查询业绩技术指标
    # jishuzhibiao_id 技术指标ID
    # 返回 (request success, None|YejiJiShuZhibiao, error_message)
    def get_jishuzhibiao(self, jishuzhibiao_id):
        error_result = (False, None, '')

        try:
            uri = '/APi/webApi/dataservice/query/project/proBizIndicatorDetail?id={}'.format(jishuzhibiao_id)
            url = self.host + uri
            headers = self.get_common_request_header()
            headers['Timeout'] = '30000'
            headers['V'] = '231012'
            response = requests.get(url, headers=headers, proxies=self.proxies, timeout=5)
            if not response.ok:
                print("查询业绩技术指标失败，错误是：{}".format(response))
                return error_result
            else:
                data = response.content.decode('utf-8')
                data = self.decode_response(data)
                root = json.loads(data)

                if root['code'] == 2000:
                    return True, None, ''

                if root['code'] != 200:
                    print("查询业绩技术指标失败，错误是：code={}".format(root['code']))
                    return error_result

                if 'data' not in root:
                    print("查询业绩技术指标失败，错误是：缺失data字段")
                    return error_result

                data_object = root['data']
                if 'PRJNUM' not in data_object:
                    return True, None, ''

                jishuzhibiao = YejiJiShuZhibiao()
                jishuzhibiao.id = str(jishuzhibiao_id)
                jishuzhibiao.project_id = str(data_object['PRJNUM'])
                jishuzhibiao.zizibiaozhun = data_object['APTITUDECONTENT']
                if 'BDATE' in data_object and data_object['BDATE']:
                    jishuzhibiao.begin_date = datetime.fromtimestamp(float(data_object['BDATE']) / 1000).strftime(
                        '%Y-%m-%d')
                if 'EDATE' in data_object and data_object['EDATE']:
                    jishuzhibiao.end_date = datetime.fromtimestamp(float(data_object['EDATE']) / 1000).strftime(
                        '%Y-%m-%d')
                jishuzhibiao.guimo_dengji = data_object['TECHPARAMINFO']
                jishuzhibiao.data_level = data_object['DATALEVEL']
                jishuzhibiao.yejijilubianhao = data_object['PERFNUM']

                return True, jishuzhibiao, ''
        except Exception as e:
            print("查询业绩技术指标失败，错误是：{}".format(e))
            return error_result

    # 获取相关人员在项目中所起的作用
    @staticmethod
    def get_role(person_object):
        if 'PRJDUTY' not in person_object:
            return ''

        value = person_object['PRJDUTY']
        if value == 2:
            return '技术负责人'
        elif value == 6:
            return '项目经理'
        else:
            return str(value)

    # 查询业绩技术指标的相关人员
    # yejijilubianhao 业绩记录编号
    # 返回 (request success, None|[Person], error_message)
    def get_xiangguanrenyuan(self, yejijilubianhao):
        error_result = (False, None, '')

        try:
            uri = '/APi/webApi/dataservice/query/project/proStaffIndicatorList?perfnum={}&pg=0&pgsz=15'.format(yejijilubianhao)
            url = self.host + uri
            headers = self.get_common_request_header()
            headers['Timeout'] = '30000'
            headers['V'] = '231012'
            response = requests.get(url, headers=headers, proxies=self.proxies, timeout=5)
            if not response.ok:
                print("查询业绩技术指标的相关人员失败，错误是：{}".format(response))
                return error_result
            else:
                data = response.content.decode('utf-8')
                data = self.decode_response(data)
                root = json.loads(data)

                if root['code'] != 200:
                    print("查询业绩技术指标的相关人员失败，错误是：code={}".format(root['code']))
                    return error_result

                if 'data' not in root:
                    print("查询业绩技术指标的相关人员失败，错误是：缺失data字段")
                    return error_result

                if 'list' not in root['data']:
                    return True, [], ''

                list_object = root['data']['list']
                people = []
                for item in list_object:
                    person = Person()
                    person.name = item['PERSONNAME']
                    person.shenfenzheng_id = item['PERSONIDCARD']
                    person.role = JzscClient.get_role(item)
                    people.append(person)

                return True, people, ''
        except Exception as e:
            print("查询业绩技术指标的相关人员失败，错误是：{}".format(e))
            return error_result


def test():
    jzsc_client = JzscClient()
    # response_data = '5588a9e126c91a28cc2f6813e379336923bf42c9814280cca7955d2725a88b285eb120e6bf5a55fa882a3f10fbd8391f0f26e0bb44e47c4ad14843048ed09f57086b3df88552bd6f68c5e3f3c838a52f7ed0d9a4d78d57a64ef553783388409eb7424f40220b277e79e99b6c07828ed108d4c895a6a004f700562a3e5c25def7c546de639daaba3b1c6cc181a694c8b5680546c3d238f4962b9be4b0ea03704ea7537e1c4f83b588bdaf6ff3eebd7873daffb15736f7d399002013abe94840621368aa08f18f496a2bd7a6e4627d851e8aa0a6a5268dd74e1301ea055ff3b268a9d1f549de49c34bfdfc9cd5d9a7e7e481372b5b0798b26fbb47db8d5fe867d06d0a12d03626bc48d147c38981b3de15ff733c92487e847d467f22721d48a90f2682667a358f7cde22e19e9a7128d2a99fdcd7573e958fe6827ca527eb4d85f476d2eca377928e84b2928a92b104440ff63216622bd4b04096af8502c68f5b39f7e2c6f27fcf5e211f3428d2aec8a53a505bee8733cb525bb0d46bb99153383e70df9e30702b7188c5f247931cf3bc88ec0d41600261e042bb8a00064b97962a47724de74821aaacab59b0a491faa84ca30ecb8c1c98570c0d25e93f26df38d2e719a1a6dcabff5ee448d9d9e52142583e59520c00efbdd84866b91af5a13d36d106d3cfff91d7e2acacc017b9ca351e55dc7cd64f44b4a8c74f5e5aa302809b64fedaad13a28edc96b3de165ea296403f343ce2eefbfac198f5a42de860bfc556fa759e5cf82e3d8df13c00fd130cd39f21ebcd6973cbf123cb613246152c594313be5a158167a3353ba03dc6492ef1676961ffa55985b7533de8692f5197b1b5d785ba79918083147429c1f746770b'
    # response_data = jzsc_client.decode_response(response_data)
    # print(response_data)
    jzsc_client.get_jungongyanshou(1)
    # jzsc_client.get_jishuzhibiao(17892)
    # jzsc_client.get_xiangguanrenyuan('YJ-4501082411210001-007')


if __name__ == '__main__':
    test()
