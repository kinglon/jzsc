#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QString>

class DataModel
{
public:
    // 项目编号
    QString m_id;

    // 项目名称
    QString m_name;

    // 项目类别
    QString m_type;

    // 数据等级
    QString m_dataLevel;

    // 实际建设规模
    QString m_factSize;

    // 实际开工日期
    QString m_beginDate;

    // 竣工验收备案日期
    QString m_endDate;

    // 数据来源
    QString m_dataSource;

    // 备注
    QString m_remark;
};

#endif // DATAMODEL_H
