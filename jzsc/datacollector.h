#ifndef DATACOLLECTOR_H
#define DATACOLLECTOR_H

#include <QObject>
#include <QVector>
#include <QNetworkReply>
#include <QTimer>
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
    void SetCode(QString code) { m_code = code; }

    bool run();

    QVector<DataModel>& getDataModel() { return m_dataModel; }

    static QByteArray intArrayToByteArray(int datas[], int size);

private:
    void httpGetData();

    QByteArray decode(const QByteArray& data);

    void parseDatas(const QJsonArray& datas);

    void killTimer();

private slots:
    void onHttpFinished(QNetworkReply *reply);

    void runJsCodeFinished(const QVariant& result);

signals:
    // 运行结束
    void runFinish(int errorCode);

private:
    QString m_code;

    QVector<DataModel> m_dataModel;

    int m_retryCount = 0;

    QTimer* m_timer = nullptr;

    QString m_accessToken;

    QVector<int> m_reverseMapData;

    QByteArray m_key;

    QByteArray m_iv;
};

#endif // DATACOLLECTOR_H
