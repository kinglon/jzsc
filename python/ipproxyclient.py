import json
from datetime import datetime
import requests


def get_common_request_header():
    headers = {
        'Content-Type': 'application/json',
        'Accept': 'application/json, text/plain, */*',
        'Accept-Encoding': 'gzip, deflate, br, zstd',
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36',
    }
    return headers


class IpProxyClient:
    def __init__(self):
        self.proxies = {"http": "", "https": ""}
        self.timeout = 20
        self.link = ''

    # 提取IP
    # 返回 (request success, None|(IP地址, port), error_message)
    def extract_ip(self):
        error_result = (False, None, '')

        try:
            url = self.link
            headers = get_common_request_header()
            response = requests.get(url, headers=headers, proxies=self.proxies, timeout=10)
            if not response.ok:
                print("提取IP失败，错误是：{}".format(response))
                try:
                    data = response.content.decode('utf-8')
                    root = json.loads(data)
                    print("提取IP失败，错误是：{}".format(root['msg']))
                except Exception:
                    pass
                return error_result
            else:
                data = response.content.decode('utf-8')
                root = json.loads(data)

                if root['code'] != 1000:
                    print("提取IP失败，错误是：{}".format(root['msg']))
                    return error_result

                ip = root['data'][0]['ip']
                port = root['data'][0]['port']
                end_time_string = root['data'][0]['expire']
                end_time_ts = int(datetime.strptime(end_time_string, '%Y-%m-%d %H:%M:%S').timestamp())
                print('提取IP成功，IP是{}，端口是{}，到期时间是{}'.format(ip, port, end_time_ts))
                return True, (ip, port, end_time_ts), ''
        except Exception as e:
            print("提取IP失败，错误是：{}".format(e))
            return error_result


def test():
    client = IpProxyClient()
    client.link = 'http://api.tianqiip.com/getip?secret=du1uati3axzjy7bf&num=1&type=json&port=1&time=3&ts=1&mr=1&sign=4ed903f61ca021aa4a8ec9f2bc54f8d6'
    client.extract_ip()


if __name__ == '__main__':
    test()
