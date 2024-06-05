#ifndef COLLECTSTATUSMANAGER_H
#define COLLECTSTATUSMANAGER_H

#include <QString>
#include <QVector>
#include <QDate>
#include "datamodel.h"

class CollectStatusManager
{
public:
    CollectStatusManager();

public:
    static CollectStatusManager* getInstance();

public:
    void save();

    // 查询是否正在采集
    bool isCollecting() { return !m_codePrefix.isEmpty(); }

    // 启动新任务采集
    void startNewTasks(QString codePrefix, QDate beginDate);

    // 获取下一个采集任务编号
    QString getNextTask();

    QVector<DataModel>& getCollectDatas() { return m_collectDatas; }

    void finishCurrentTask(const QVector<DataModel>& dataModel);

    // 切到下一天继续采集
    void switchToNextDay();

    bool isFinish() { return m_finish;}

    void reset();

private:
    void load();

private:
    // 编号前6位
    QString m_codePrefix;

    // 日期
    QDate m_currentDate;

    // 下一个采集索引
    int m_nextIndex = 1;

    // 采集的结果
    QVector<DataModel> m_collectDatas;

    // 是否采集完成
    bool m_finish = false;
};

#endif // COLLECTSTATUSMANAGER_H
