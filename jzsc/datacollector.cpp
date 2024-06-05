#include "datacollector.h"
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include "qaesencryption.h"
#include "browserwindow.h"

DataCollector::DataCollector(QObject *parent)
    : QObject{parent}
{
    int key[] = {1148467306, 964118391, 624314466, 2019968622};
    m_key = intArrayToByteArray(key, 4);

    int iv[] = {808530483, 875902519, 943276354, 1128547654};
    m_iv = intArrayToByteArray(iv, 4);
}

bool DataCollector::run()
{
    if (m_code.isEmpty())
    {
        return false;
    }

    // 获取accessToken
    connect(BrowserWindow::getInstance(), &BrowserWindow::runJsCodeFinished, this, &DataCollector::runJsCodeFinished);
    BrowserWindow::getInstance()->runJsCode("localStorage");
    m_timer = new QTimer(this);
    m_timer->setInterval(3000);
    connect(m_timer, &QTimer::timeout, [this]() {
        killTimer();
        qCritical("failed to get the access token");
        emit runFinish(COLLECT_ERROR_NOT_LOGIN);
    });
    m_timer->start();

    return true;
}

QByteArray DataCollector::intArrayToByteArray(int datas[], int size)
{
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    for (int i = 0; i < size; i++)
    {
        stream << datas[i];
    }
    return byteArray;
}

void DataCollector::httpGetData()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(3000);
    connect(manager, &QNetworkAccessManager::finished, this, &DataCollector::onHttpFinished);

    QNetworkRequest request;
    QUrl url(QString("https://jzsc.mohurd.gov.cn/APi/webApi/dataservice/query/project/projectFinishManage?jsxmCode=%1&pg=0&pgsz=15").arg(m_code));
    request.setUrl(url);

    // 设置头部
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Accept-Encoding", "gzip, deflate, br, zstd");
    request.setRawHeader("Origin", "https://jzsc.mohurd.gov.cn");
    request.setRawHeader("Referer", "https://jzsc.mohurd.gov.cn");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36");
    request.setRawHeader("Accesstoken", m_accessToken.toUtf8());
    request.setRawHeader("Timeout", "30000");
    request.setRawHeader("V", "231012");
    manager->get(request);
}

QByteArray DataCollector::decode(const QByteArray& data)
{
    QByteArray dataAfterHexDecode = QByteArray::fromHex(data);
    QString base64Data = dataAfterHexDecode.toBase64();
    QVector<int> r;
    int o = 0;
    int s = 0;
    int pos = base64Data.indexOf('=');
    int t = pos == -1? base64Data.length() : pos;
    while (s < t)
    {
        if (s % 4 != 0)
        {
            int value = base64Data[s - 1].unicode();
            if (value >= m_reverseMapData.size())
            {
                qCritical("the value(%d) is greater than the length of the reverse map data", value);
                return QByteArray();
            }
            int a = m_reverseMapData[value] << (s % 4) * 2;

            value = base64Data[s].unicode();
            if (value >= m_reverseMapData.size())
            {
                qCritical("the value(%d) is greater than the length of the reverse map data", value);
                return QByteArray();
            }
            int l = m_reverseMapData[value] >> (6 - (s % 4) * 2);

            int i = o / 4;
            int v = ((a | l) << (24 - o % 4 * 8)) & 0xFFFFFFFF;
            if (i >= r.size())
            {
                r.append(v);
            }
            else
            {
                r[i] |= v;
            }
            o += 1;
        }
        s += 1;
    }

    QByteArray dataAfterReverse;
    foreach (int val, r)
    {
        dataAfterReverse.append(static_cast<char>(val));
    }

    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CBC, QAESEncryption::PKCS7);
    QByteArray decodeText = encryption.decode(dataAfterReverse, m_key, m_iv);
    decodeText = encryption.removePadding(decodeText);
    return decodeText;
}

void DataCollector::killTimer()
{
    if (m_timer)
    {
        m_timer->stop();
        m_timer->deleteLater();
    }
}

void DataCollector::onHttpFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the http request, error: %d", reply->error());
        if (m_retryCount >= 2)
        {
            emit runFinish(COLLECT_ERROR_CONNECTION_FAILED);
        }
        else
        {
            QTimer::singleShot(1000, [this]() {
                m_retryCount++;
                httpGetData();
            });
        }
    }
    else
    {
        QByteArray data = decode(reply->readAll());
        if (data.size() == 0)
        {
            emit runFinish(COLLECT_ERROR);
            return;
        }

        QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
        if (jsonDocument.isNull() || jsonDocument.isEmpty())
        {
            qCritical("failed to parse the json data");
            emit runFinish(COLLECT_ERROR);
            return;
        }

        QJsonObject root = jsonDocument.object();
        if (!root.contains("code"))
        {
            qCritical("the response data not have code memeber");
            emit runFinish(COLLECT_ERROR);
            return;
        }

        int errorCode = root["code"].toInt();
        if (errorCode != 200)
        {
            qCritical("the response data have error: %d", errorCode);
            emit runFinish(COLLECT_ERROR_NOT_LOGIN);
            return;
        }
        else
        {
            if (root.contains("data") && root["data"].toObject().contains("list"))
            {
                QJsonArray datas = root["data"].toObject()["list"].toArray();
                if (datas.size() == 0)
                {
                    emit runFinish(COLLECT_ERROR_INVALID_CODE);
                    return;
                }
                else
                {
                    parseDatas(datas);
                    emit runFinish(COLLECT_SUCCESS);
                    return;
                }
            }
            else
            {
                qCritical("the response data not have list memeber");
                emit runFinish(COLLECT_ERROR);
                return;
            }
        }
    }
}

void DataCollector::runJsCodeFinished(const QVariant& result)
{
    QVariantMap storageItems = result.toMap();
    if (storageItems.contains("accessToken"))
    {
        killTimer();
        disconnect(BrowserWindow::getInstance(), &BrowserWindow::runJsCodeFinished, this, &DataCollector::runJsCodeFinished);
        m_accessToken = storageItems["accessToken"].toString();
        httpGetData();
    }
}

void DataCollector::parseDatas(const QJsonArray& datas)
{
    m_dataModel.clear();
    for (const auto& dataJson : datas)
    {
        QJsonObject itemJson = dataJson.toObject();
        DataModel dataModel;
        dataModel.m_id = m_code;

        if (itemJson.contains("FINPRJNAME"))
        {
            dataModel.m_name = itemJson["FINPRJNAME"].toString();
        }

        if (itemJson.contains("RN"))
        {
            int rn = itemJson["RN"].toInt();
            if (rn == 1)
            {
                dataModel.m_type = QString::fromWCharArray(L"房屋建筑工程");
            }
            else if (rn == 2)
            {
                dataModel.m_type = QString::fromWCharArray(L"市政工程");
            }
            else
            {
                dataModel.m_type = QString::fromWCharArray(L"其它");
            }
        }

        if (itemJson.contains("BUILDERLICENCENUM"))
        {
            dataModel.m_buildLicenseNum = itemJson["BUILDERLICENCENUM"].toString();
        }

        if (itemJson.contains("FACTCOST"))
        {
            dataModel.m_factCost = QString::number(itemJson["FACTCOST"].toDouble(), 'f', 2);
        }

        if (itemJson.contains("BDATE"))
        {
            int unixTimeSec = (int)(itemJson["BDATE"].toDouble()/1000);
            QDateTime dt = QDateTime::fromSecsSinceEpoch(unixTimeSec);
            QDate date = dt.date();
            QString dateString = date.toString("yyyy-MM-dd");
            dataModel.m_beginDate = dateString;
        }

        if (itemJson.contains("EDATE"))
        {
            int unixTimeSec = (int)(itemJson["EDATE"].toDouble()/1000);
            QDateTime dt = QDateTime::fromSecsSinceEpoch(unixTimeSec);
            QDate date = dt.date();
            QString dateString = date.toString("yyyy-MM-dd");
            dataModel.m_endDate = dateString;
        }

        if (itemJson.contains("MARK"))
        {
            dataModel.m_remark = itemJson["MARK"].toString();
        }

        if (itemJson.contains("FACTSIZE"))
        {
            dataModel.m_factSize = itemJson["FACTSIZE"].toString();
        }

        if (itemJson.contains("DATASOURCE"))
        {
            int dataSource = itemJson["DATASOURCE"].toInt();
            if (dataSource == 2)
            {
                dataModel.m_dataSource = QString::fromWCharArray(L"信息登记");
            }
            else
            {
                dataModel.m_dataSource = QString::number(dataSource);
            }
        }

        if (itemJson.contains("DATALEVEL"))
        {
            dataModel.m_dataLevel = itemJson["DATALEVEL"].toString();
        }
    }
}
