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
    bool isCollecting() { return m_endIndex > 0; }

    // 启动新任务采集
    void startNewTasks(int beginIndex, int endIndex);

    // 获取下一个采集任务ID
    QString getNextTask() { return QString::number(m_nextIndex); }

    QVector<DataModel>& getCollectDatas() { return m_collectDatas; }

    void finishCurrentTask(const QVector<DataModel>& dataModel);   

    bool isFinish() { return m_finish;}

    void reset();

private:
    void load();

private:
    // 下一个采集索引
    int m_nextIndex = 0;

    // 结束索引
    int m_endIndex = 0;

    // 采集的结果
    QVector<DataModel> m_collectDatas;

    // 是否采集完成
    bool m_finish = false;
};

#endif // COLLECTSTATUSMANAGER_H
