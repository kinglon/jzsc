#ifndef DATACOLLECTOR_H
#define DATACOLLECTOR_H

#include <QObject>
#include <QVector>
#include <QNetworkReply>
#include <QTimer>
#include <QNetworkAccessManager>
#include "datamodel.h"

// 采集失败原因
#define COLLECT_SUCCESS                 0
#define COLLECT_ERROR                   1
#define COLLECT_ERROR_NOT_LOGIN         2  // 未登录
#define COLLECT_ERROR_CONNECTION_FAILED 3  // 连接失败

// 采集步骤
#define COLLECT_STEP_INIT                  0
#define COLLECT_STEP_GET_TOKEN             1
#define COLLECT_STEP_GET_PROJECT_DATA_1    2
#define COLLECT_STEP_GET_PROJECT_DATA_2    3

class DataCollector : public QObject
{
    Q_OBJECT
public:
    explicit DataCollector(QObject *parent = nullptr);

public:
    void setCode(QString code) { m_code = code; }

    bool run();

    QVector<DataModel>& getDataModel() { return m_dataModel; }

    static QByteArray intArrayToByteArray(int datas[], int size);

private:
    void httpGetData1();

    void httpGetData2();

    void addCommonHeader(QNetworkRequest& request);

    void processHttpReply1(QNetworkReply *reply);

    void processHttpReply2(QNetworkReply *reply);

    QByteArray decode(const QByteArray& data);

    void parseData2(const QJsonArray& datas);

    void killTimer();

private slots:
    void onHttpFinished(QNetworkReply *reply);

    void runJsCodeFinished(const QVariant& result);

signals:
    // 运行结束
    void runFinish(int errorCode);

private:
    QString m_code;

    // 项目类别
    QString m_projectType;

    // 项目编号
    QString m_projectNum;

    // 项目名称
    QString m_projectName;

    QVector<DataModel> m_dataModel;

    int m_retryCount = 0;

    QTimer* m_timer = nullptr;

    QString m_accessToken;

    int m_currentStep = COLLECT_STEP_INIT;

    QByteArray m_key;

    QByteArray m_iv;

    static QNetworkAccessManager *m_networkAccessManager;
};

#endif // DATACOLLECTOR_H
