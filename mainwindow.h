#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 刷新窗口信息表格
    void on_pushButtonRefreshTableWidgetWindowInfo_clicked();
    // 表格中选项框点击
    void onCheckboxToggled(bool checked, HWND hwnd);
    // 显示所有选中的窗口
    void on_pushButtonShowCheckedWindow_clicked();
    // 开始监听鼠标事件
    void on_pushButtonStart_clicked();
    // 停止监听鼠标事件
    void on_pushButtonStop_clicked();
    // 打开关于窗口
    void on_actionOpenAboutDialog_triggered();

private:
    Ui::MainWindow *ui;
    // 目标窗口信息
    struct TargetWindowInfo
    {
        HWND hwnd;
        bool visible;
    };

    // 要隐藏显示的窗口句柄列表
    QList<TargetWindowInfo> _targetWindowHandleList;
    // 鼠标钩子id
    HHOOK _hookId = nullptr;
    // 刷新窗口信息表格
    void RefreshTableWidgetWindowInfo();
    // 切换窗口显示隐藏状态
    void ToggleWindowsVisibility();
    // 遍历所有窗口，显示到表格中
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    // 鼠标钩子回调函数
    static LRESULT CALLBACK  LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    // 隐藏目标窗口
    static void HideTargetWindow(HWND hwnd, bool byTransparency);
    // 显示目标窗口
    static void ShowTargetWindow(HWND hwnd, bool byTransparency);
};
#endif // MAINWINDOW_H
