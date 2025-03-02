#  竣工验收
class JungongYanshou:
    def __init__(self):
        # ID
        self.id = ''

        # 项目编号
        self.project_id = ''

        # 数据等级
        self.data_level = ''

        # 施工许可证编号
        self.shigong_license_id = ''

        # 实际造价（万元）
        self.actual_use_money = ''

        # 实际开工日期
        self.begin_date = ''

        # 竣工验收备案日期
        self.end_date = ''

        # 数据来源
        self.data_source = ''

        # 实际建设规模
        self.construct_scale = ''

        # 备注
        self.remark = ''


# 人员
class Person:
    def __init__(self):
        # 姓名
        self.name = ''

        # 身份证ID
        self.shenfenzheng_id = ''

        # 在项目所起的作用
        self.role = ''


# 业绩技术指标
class YejiJiShuZhibiao:
    def __init__(self):
        # ID
        self.id = ''

        # 项目编号
        self.project_id = ''

        # 业绩对应资质标准及等级
        self.zizibiaozhun = ''

        # 工作开始时间
        self.begin_date = ''

        # 工作结束时间
        self.end_date = ''

        # 工程项目规模等级及详细技术指标
        self.guimo_dengji = ''

        # 技术指标内数据等级
        self.data_level = ''

        # 业绩记录编号
        self.yejijilubianhao = ''

        # 相关人员
        self.people = []
