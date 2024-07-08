#include "datacollector.h"
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QNetworkProxy>
#include "qaesencryption.h"
#include "browserwindow.h"
#include "qcompressor.h"

#define MAX_RETRY_COUNT 4

QNetworkAccessManager *DataCollector::m_networkAccessManager = nullptr;

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
    m_currentStep = COLLECT_STEP_GET_TOKEN;
    connect(BrowserWindow::getInstance(), &BrowserWindow::runJsCodeFinished, this, &DataCollector::runJsCodeFinished);
    QString jsCode = "var jsResult = {};jsResult['accessToken'] = '';if (localStorage.accessToken){jsResult['accessToken'] = localStorage.accessToken;} jsResult;";
    BrowserWindow::getInstance()->runJsCode(jsCode);
    m_timer = new QTimer(this);
    m_timer->setInterval(3000);
    connect(m_timer, &QTimer::timeout, [this]() {
        killTimer();
        qCritical("failed to get the access token");
        emit runFinish(COLLECT_ERROR_NOT_LOGIN);
    });
    m_timer->start();

    // 初始化网络请求
    if (m_networkAccessManager == nullptr)
    {
        m_networkAccessManager = new QNetworkAccessManager();
        m_networkAccessManager->setProxy(QNetworkProxy());        
    }
    m_networkAccessManager->setTransferTimeout(m_networkTimeout*1000);
    connect(m_networkAccessManager, &QNetworkAccessManager::finished, this, &DataCollector::onHttpFinished);

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

void DataCollector::httpGetData1()
{
    if (m_networkAccessManager == nullptr)
    {
        return;
    }

    QNetworkRequest request;
    QUrl url(QString("https://jzsc.mohurd.gov.cn/APi/webApi/dataservice/query/project/projectDetail?id=%1").arg(m_code));
    request.setUrl(url);
    addCommonHeader(request);
    m_networkAccessManager->get(request);
}

void DataCollector::httpGetData2()
{
    if (m_networkAccessManager == nullptr)
    {
        return;
    }

    QNetworkRequest request;
    QUrl url(QString("https://jzsc.mohurd.gov.cn/APi/webApi/dataservice/query/project/projectFinishManageDetail?id=%1").arg(m_code));
    request.setUrl(url);
    addCommonHeader(request);
    m_networkAccessManager->get(request);
}

void DataCollector::addCommonHeader(QNetworkRequest& request)
{
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Accept-Encoding", "gzip, deflate, br, zstd");
    request.setRawHeader("Origin", "https://jzsc.mohurd.gov.cn/");
    request.setRawHeader("Referer", "https://jzsc.mohurd.gov.cn/");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36");
    request.setRawHeader("Accesstoken", m_accessToken.toUtf8());
    request.setRawHeader("Timeout", "30000");
    request.setRawHeader("V", "231012");
}

QByteArray DataCollector::decode(const QByteArray& data)
{
    QByteArray dataAfterHexDecode = QByteArray::fromHex(data);
    QAESEncryption encryption(QAESEncryption::AES_128, QAESEncryption::CBC, QAESEncryption::PKCS7);
    QByteArray decodeText = encryption.decode(dataAfterHexDecode, m_key, m_iv);
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

void DataCollector::processHttpReply1(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the http request for data1, error: %d", reply->error());
        if (m_retryCount >= MAX_RETRY_COUNT)
        {
            emit runFinish(COLLECT_ERROR_CONNECTION_FAILED);
        }
        else
        {
            QTimer::singleShot(1000, [this]() {
                m_retryCount++;
                httpGetData1();
            });
        }
    }
    else
    {
        QByteArray rawData = reply->readAll();
        if (reply->rawHeader("Content-Encoding").toStdString() == "gzip")
        {
            QByteArray decompressData;
            if (!QCompressor::gzipDecompress(rawData, decompressData))
            {
                qCritical("failed to decompress the gzip response data");
                emit runFinish(COLLECT_ERROR);
                return;
            }
            rawData = decompressData;
        }

        QByteArray decodeData = decode(rawData);
        if (decodeData.size() == 0)
        {
            emit runFinish(COLLECT_ERROR);
            return;
        }

        QJsonDocument jsonDocument = QJsonDocument::fromJson(decodeData);
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
            if (root.contains("data"))
            {
//                QJsonObject data = root["data"].toObject();
//                if (!data.contains("PRJNUM"))
//                {
//                    qInfo("id=%s not exist", m_code.toStdString().c_str());
//                    emit runFinish(COLLECT_ERROR_NOT_EXIST);
//                    return;
//                }
//                qulonglong projectNum = (qulonglong)(data["PRJNUM"].toDouble());
//                m_projectNum = QString::number(projectNum);

//                if (data.contains("PRJTYPENUM"))
//                {
//                    m_projectType = data["PRJTYPENUM"].toString();
//                }

//                if (data.contains("PRJNAME"))
//                {
//                    m_projectName = data["PRJNAME"].toString();
//                }

                m_currentStep = COLLECT_STEP_GET_PROJECT_DATA_2;
                m_retryCount = 0;
                httpGetData2();
                return;
            }
            else
            {
                qCritical("the response data not have data memeber");
                emit runFinish(COLLECT_ERROR);
                return;
            }
        }
    }
}

void DataCollector::processHttpReply2(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCritical("failed to send the http request for data2, error: %d", reply->error());
        if (m_retryCount >= 4)
        {
            emit runFinish(COLLECT_ERROR_CONNECTION_FAILED);
        }
        else
        {
            QTimer::singleShot(1000, [this]() {
                m_retryCount++;
                httpGetData2();
            });
        }
    }
    else
    {
        QByteArray rawData = reply->readAll();
        if (reply->rawHeader("Content-Encoding").toStdString() == "gzip")
        {
            QByteArray decompressData;
            if (!QCompressor::gzipDecompress(rawData, decompressData))
            {
                qCritical("failed to decompress the gzip response data");
                emit runFinish(COLLECT_ERROR);
                return;
            }
            rawData = decompressData;
        }

        QByteArray decodeData = decode(rawData);
        if (decodeData.size() == 0)
        {
            emit runFinish(COLLECT_ERROR);
            return;
        }

        QJsonDocument jsonDocument = QJsonDocument::fromJson(decodeData);
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
            if (root.contains("data"))
            {
                parseData2(root["data"].toObject());
                if (m_dataModel.size() > 0)
                {
                    emit runFinish(COLLECT_SUCCESS);
                }
                else
                {
                    emit runFinish(COLLECT_ERROR_NOT_EXIST);
                }
                return;
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

void DataCollector::onHttpFinished(QNetworkReply *reply)
{
    if (m_currentStep == COLLECT_STEP_GET_PROJECT_DATA_1)
    {
        processHttpReply1(reply);
    }
    else if (m_currentStep == COLLECT_STEP_GET_PROJECT_DATA_2)
    {
        processHttpReply2(reply);
    }
    reply->deleteLater();
}

void DataCollector::runJsCodeFinished(const QVariant& result)
{
    if (result.type() != QVariant::Map)
    {
        return;
    }

    QVariantMap storageItems = result.toMap();
    if (storageItems.contains("accessToken"))
    {
        killTimer();
        disconnect(BrowserWindow::getInstance(), &BrowserWindow::runJsCodeFinished, this, &DataCollector::runJsCodeFinished);
        m_accessToken = storageItems["accessToken"].toString();
        m_currentStep = COLLECT_STEP_GET_PROJECT_DATA_1;
        m_retryCount = 0;
        httpGetData1();
    }
}

void DataCollector::parseData2(const QJsonObject& itemJson)
{
    DataModel dataModel;
    dataModel.m_queryId = m_code;

    if (itemJson.contains("PRJNUM"))
    {
        qulonglong projectNum = (qulonglong)(itemJson["PRJNUM"].toDouble());
        dataModel.m_id = QString::number(projectNum);
    }
    if (dataModel.m_id.isEmpty())
    {
        return;
    }

    if (itemJson.contains("FINPRJNAME"))
    {
        dataModel.m_name = itemJson["FINPRJNAME"].toString();
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
        if (itemJson["DATASOURCE"].isNull())
        {
            dataModel.m_dataSource = "--";
        }
        else
        {
            int dataSource = itemJson["DATASOURCE"].toInt();
            if (dataSource == 1)
            {
                dataModel.m_dataSource = QString::fromWCharArray(L"业务办理");
            }
            else if (dataSource == 2)
            {
                dataModel.m_dataSource = QString::fromWCharArray(L"信息登记");
            }
            else if (dataSource == 3)
            {
                dataModel.m_dataSource = QString::fromWCharArray(L"历史业绩补录");
            }
            else
            {
                dataModel.m_dataSource = QString::number(dataSource);
            }
        }
    }

    if (itemJson.contains("DATALEVEL"))
    {
        dataModel.m_dataLevel = itemJson["DATALEVEL"].toString();
    }

    m_dataModel.append(dataModel);
}
