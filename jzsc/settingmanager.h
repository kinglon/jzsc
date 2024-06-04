#pragma once

#include <QString>
#include <QVector>

class CSettingManager
{
protected:
	CSettingManager();

public:
	static CSettingManager* GetInstance();

private:
	void Load();

public:
    int m_nLogLevel = 2;  // debug

    // 是否启用webview日志输出
    bool m_enableWebviewLog = false;

    // 浏览器宽高
    int m_browserWidth = 1920;
    int m_browserHeight = 1080;
};
