#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "browserwindow.h"
#include "collectstatusmanager.h"
#include <QMessageBox>
#include <QDesktopServices>
#include "datacollector.h"
#include "Utility/ImPath.h"
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

using namespace QXlsx;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    // 异步调用
    connect(this, &MainWindow::collectNextTask, this, &MainWindow::onCollectNextTask, Qt::QueuedConnection);

    initCtrls();

    BrowserWindow::getInstance()->setHideWhenClose(true);
    BrowserWindow::getInstance()->load(QUrl("https://jzsc.mohurd.gov.cn/data/project/detail?id=3175497"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initCtrls()
{
    updateButtonStatus();

    connect(ui->startButton, &QPushButton::clicked, [this]() {
            startCollect();
        });
    connect(ui->continueButton, &QPushButton::clicked, [this]() {
            continueCollect();
        });
    connect(ui->stopButton, &QPushButton::clicked, [this]() {
            stopCollect();
        });
    connect(ui->manualVerifyButton, &QPushButton::clicked, []() {
        BrowserWindow::getInstance()->showMaximized();
        BrowserWindow::getInstance()->load(QUrl("https://jzsc.mohurd.gov.cn/data/project/detail?id=3175497"));
    });
}

void MainWindow::updateButtonStatus()
{
    ui->startButton->setEnabled(!m_isCollecting);
    ui->continueButton->setEnabled(!m_isCollecting && CollectStatusManager::getInstance()->isCollecting());
    ui->stopButton->setEnabled(ui->continueButton->isEnabled());
}

void MainWindow::showTip(QString tip)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(QString::fromWCharArray(L"提示"));
    msgBox.setText(tip);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::addCollectLog(const QString& log)
{
    qInfo(log.toStdString().c_str());
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentTimeString = currentDateTime.toString("[MM-dd hh:mm:ss] ");
    QString line = currentTimeString + log;
    ui->logEdit->append(line);
}

void MainWindow::startCollect()
{
    int beginId = ui->beginIdEdit->text().toInt();
    int endId = ui->endIdEdit->text().toInt();
    if (beginId >= endId)
    {
        showTip(QString::fromWCharArray(L"结束ID必须大于开始ID"));
        return;
    }

    CollectStatusManager::getInstance()->startNewTasks(beginId, endId);
    updateButtonStatus();
    continueCollect();
}

void MainWindow::continueCollect()
{
    if (!CollectStatusManager::getInstance()->isCollecting())
    {
        showTip(QString::fromWCharArray(L"上次采集已经结束"));
        return;
    }

    // 如果采集已经完成，就结束采集
    if (CollectStatusManager::getInstance()->isFinish())
    {
        stopCollect();
        return;
    }

    m_isCollecting = true;
    updateButtonStatus();
    ui->logEdit->setText("");

    onCollectNextTask();
}

void MainWindow::stopCollect()
{
    m_isCollecting = false;
    updateButtonStatus();

    // 保存采集结果并打开保存目录    
    if (!saveCollectResult())
    {
        showTip(QString::fromWCharArray(L"保存采集结果到表格失败"));
        return;
    }

    CollectStatusManager::getInstance()->reset();
    updateButtonStatus();
}

void MainWindow::onCollectNextTask()
{
    QString taskCode = CollectStatusManager::getInstance()->getNextTask();
    if (taskCode.isEmpty())
    {
        qCritical("failed to get the next task");
        return;
    }

    addCollectLog(QString::fromWCharArray(L"编号%1开始采集").arg(taskCode));
    DataCollector* collector = new DataCollector(this);
    collector->setCode(taskCode);
    connect(collector, &DataCollector::runFinish, [collector, taskCode, this](int errorCode) {
        if (errorCode == COLLECT_SUCCESS)
        {
            addCollectLog(QString::fromWCharArray(L"编号%1完成采集").arg(taskCode));
            finishCurrentTask(collector->getDataModel());
        }        
        else if (errorCode == COLLECT_ERROR_NOT_LOGIN)
        {
            addCollectLog(QString::fromWCharArray(L"编号%1采集失败，请人工验证").arg(taskCode));
            m_isCollecting = false;
            updateButtonStatus();
        }
        else if (errorCode == COLLECT_ERROR_CONNECTION_FAILED)
        {
            addCollectLog(QString::fromWCharArray(L"编号%1采集失败，无法连接服务器").arg(taskCode));
            m_isCollecting = false;
            updateButtonStatus();
        }
        else
        {
            addCollectLog(QString::fromWCharArray(L"编号%1采集失败，未知原因").arg(taskCode));
            m_isCollecting = false;
            updateButtonStatus();
        }
        collector->deleteLater();
    });
    collector->run();
}

void MainWindow::finishCurrentTask(const QVector<DataModel>& dataModel)
{
    CollectStatusManager::getInstance()->finishCurrentTask(dataModel);
    if (CollectStatusManager::getInstance()->isFinish())
    {
        // 结束采集计划
        stopCollect();
    }
    else
    {
        int interval = ui->intervalEdit->text().toInt();
        if (interval == 0)
        {
            interval = 3;
        }
        QTimer::singleShot(interval*1000, [this]() {
            emit collectNextTask();
        });
    }
}

bool MainWindow::saveCollectResult()
{
    // 拷贝默认采集结果输出表格到保存目录
    QString excelFileName = QString::fromWCharArray(L"采集结果.xlsx");
    QString srcExcelFilePath = QString::fromStdWString(CImPath::GetConfPath()) + excelFileName;
    QDateTime dateTime = QDateTime::currentDateTime();
    QString destFileName = dateTime.toString(QString::fromWCharArray(L"yyyyMMdd_hhmm_")) + excelFileName;
    QString destExcelFilePath = QString::fromStdWString(CImPath::GetDataPath()) + destFileName;
    ::DeleteFile(destExcelFilePath.toStdWString().c_str());
    if (!::CopyFile(srcExcelFilePath.toStdWString().c_str(), destExcelFilePath.toStdWString().c_str(), TRUE))
    {
        qCritical("failed to copy the result excel file");
        return false;
    }

    // 从第2行开始写
    Document xlsx(destExcelFilePath);
    if (!xlsx.load())
    {
        qCritical("failed to load the result excel file");
        return false;
    }

    auto& datas = CollectStatusManager::getInstance()->getCollectDatas();
    int row = 2;    
    for (const auto& data : datas)
    {
        int column = 1;
        xlsx.write(row, column, data.m_id);
        xlsx.write(row, ++column, data.m_name);
        xlsx.write(row, ++column, data.m_type);
        xlsx.write(row, ++column, data.m_dataLevel);
        xlsx.write(row, ++column, data.m_buildLicenseNum);
        xlsx.write(row, ++column, data.m_factCost);
        xlsx.write(row, ++column, data.m_beginDate);
        xlsx.write(row, ++column, data.m_endDate);
        xlsx.write(row, ++column, data.m_dataSource);
        xlsx.write(row, ++column, data.m_factSize);
        xlsx.write(row, ++column, data.m_remark);
        row++;
    }

    if (!xlsx.save())
    {
        qCritical("failed to save the result excel file");
        return false;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdWString(CImPath::GetDataPath())));
    return true;
}
