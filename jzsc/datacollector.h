#ifndef DATACOLLECTOR_H
#define DATACOLLECTOR_H

#include <QObject>
#include "datamodel.h"

// 采集失败原因
#define COLLECT_SUCCESS                 0
#define COLLECT_ERROR                   1
#define COLLECT_ERROR_INVALID_CODE      2  // 无效编号
#define COLLECT_ERROR_NOT_LOGIN         3  // 未登录
#define COLLECT_ERROR_CONNECTION_FAILED 4  // 连接失败

class DataCollector : public QObject
{
    Q_OBJECT
public:
    explicit DataCollector(QObject *parent = nullptr);

public:
    DataModel& getDataModel() { return m_dataModel; }

    void run();

signals:
    // 运行结束
    void runFinish(int errorCode);

private:
    DataModel m_dataModel;
};

#endif // DATACOLLECTOR_H
