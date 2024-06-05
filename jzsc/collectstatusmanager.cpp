#include "collectstatusmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include "Utility/ImPath.h"

CollectStatusManager::CollectStatusManager()
{
    load();
}

CollectStatusManager* CollectStatusManager::getInstance()
{
    static CollectStatusManager* instance = new CollectStatusManager();
    return instance;
}

void CollectStatusManager::save()
{
    QJsonObject root;
    root["code_prefix"] = m_codePrefix;
    root["current_date"] = m_currentDate.toString("yyyy-MM-dd");
    root["next_index"] = m_nextIndex;
    root["finish"] = m_finish;

    QJsonArray datasJson;
    for (const auto& data : m_collectDatas)
    {
        QJsonObject dataJson;
        dataJson["id"] = data.m_id;
        dataJson["name"] = data.m_name;
        dataJson["type"] = data.m_type;
        dataJson["data_level"] = data.m_dataLevel;
        dataJson["build_license_num"] = data.m_buildLicenseNum;
        dataJson["fact_cost"] = data.m_factCost;
        dataJson["fact_size"] = data.m_factSize;
        dataJson["begin_date"] = data.m_beginDate;
        dataJson["end_date"] = data.m_endDate;
        dataJson["data_source"] = data.m_dataSource;
        dataJson["remark"] = data.m_remark;
        datasJson.append(dataJson);
    }
    root["datas"] = datasJson;

    QJsonDocument jsonDocument(root);
    QByteArray jsonData = jsonDocument.toJson(QJsonDocument::Indented);
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"collect_status.json";
    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical("failed to save the collecting status");
        return;
    }
    file.write(jsonData);
    file.close();
}

void CollectStatusManager::startNewTasks(QString codePrefix, QDate beginDate)
{
    reset();
    m_codePrefix = codePrefix;
    m_currentDate = beginDate;
    save();
}

QString CollectStatusManager::getNextTask()
{
    if (m_codePrefix.isEmpty())
    {
        return "";
    }

    QString code = m_codePrefix + m_currentDate.toString("yy-MM-dd") + QString("%04d").arg(m_nextIndex);
    return code;
}

void CollectStatusManager::finishCurrentTask(const QVector<DataModel>& dataModels)
{
    for (const auto& dataModel : dataModels )
    {
        m_collectDatas.push_back(dataModel);
    }

    m_nextIndex++;
    save();
}

void CollectStatusManager::switchToNextDay()
{
    m_nextIndex = 1;
    int year = m_currentDate.year();
    m_currentDate = m_currentDate.addDays(1);
    if (m_currentDate.year() != year) // 新的一年就认为结束
    {
        m_finish = true;
    }
    save();
}

void CollectStatusManager::reset()
{
    m_codePrefix = "";
    m_currentDate = QDate();
    m_nextIndex = 1;
    m_collectDatas.clear();
    m_finish = false;
    save();
}

void CollectStatusManager::load()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"collect_status.json";
    if (!QFileInfo(QString::fromStdWString(strConfFilePath)).exists())
    {
        return;
    }

    QFile file(QString::fromStdWString(strConfFilePath));
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical("failed to load the collecting status configure file");
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonObject root = jsonDocument.object();
    m_codePrefix = root["code_prefix"].toString();
    m_currentDate = QDate::fromString(root["current_date"].toString(), "yyyy-MM-dd");
    m_nextIndex = root["next_index"].toInt();
    m_finish = root["finish"].toBool();

    m_collectDatas.clear();
    QJsonArray datasJson = root["datas"].toArray();
    for (int i=0; i < datasJson.size(); i++)
    {
        auto dataJson = datasJson.at(i);
        DataModel data;
        data.m_id = dataJson["id"].toString();
        data.m_name = dataJson["name"].toString();
        data.m_type = dataJson["type"].toString();
        data.m_dataLevel = dataJson["data_level"].toString();
        data.m_buildLicenseNum = dataJson["build_license_num"].toString();
        data.m_factCost = dataJson["fact_cost"].toString();
        data.m_factSize = dataJson["fact_size"].toString();
        data.m_beginDate = dataJson["begin_date"].toString();
        data.m_endDate = dataJson["end_date"].toString();
        data.m_dataSource = dataJson["data_source"].toString();
        data.m_remark = dataJson["remark"].toString();
        m_collectDatas.append(data);
    }
}
