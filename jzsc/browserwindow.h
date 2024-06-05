#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QCloseEvent>
#include <QWebEnginePage>

namespace Ui {
class BrowserWindow;
}

class WebEnginePage: public QWebEnginePage
{
    Q_OBJECT

public:
    using QWebEnginePage::QWebEnginePage;

protected:
    virtual QWebEnginePage *createWindow(WebWindowType) override;

private Q_SLOTS:
    void onUrlChanged(const QUrl & url);
};

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

private:
    explicit BrowserWindow(QWidget *parent = nullptr);
    ~BrowserWindow();

public:
    static BrowserWindow* getInstance();

public:
    // 设置可以关闭退出
    void setCanClose() { m_canClose = true; }

    // 设置webview大小
    void setWebViewSize(QSize size);

    // 加载网页
    void load(const QUrl& url);

    // 获取当前页面url
    QString getUrl();

    // 截图
    bool captrueImage(const QString& savePath);

    // 执行JS脚本
    void runJsCode(const QString& jsCode);

    // 设置关闭窗口是否变成隐藏窗口
    void setHideWhenClose(bool hideWhenClose) { m_hideWhenClose = hideWhenClose;}

signals:
    // 加载网页完成
    void loadFinished(bool ok);

    // 执行JS脚本的结果
    void runJsCodeFinished(const QVariant& result);

private slots:
    void onLoadFinished(bool ok);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QWebEngineView* m_webView = nullptr;

    bool m_canClose = false;

    // 关闭窗口是否变成隐藏窗口
    bool m_hideWhenClose = false;
};

#endif // BROWSERWINDOW_H
