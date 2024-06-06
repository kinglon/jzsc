#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "datamodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initCtrls();

    void updateButtonStatus();

    void showTip(QString tip);

    void addCollectLog(const QString& log);

private:
    void startCollect();

    void continueCollect();

    void stopCollect();

    void finishCurrentTask(const QVector<DataModel>& dataModel);

    bool saveCollectResult();

private slots:
    void onCollectNextTask();

signals:
    void collectNextTask();

private:
    bool m_isCollecting = false;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
