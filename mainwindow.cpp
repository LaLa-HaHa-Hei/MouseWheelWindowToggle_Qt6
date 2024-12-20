#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QDir>
#include <QProcess>
#include "aboutdialog.h"

MainWindow* g_mainWindowInstance = nullptr;
HWND g_mainWindowHandle = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    g_mainWindowInstance  = this;
    g_mainWindowHandle = (HWND)this->winId();
    ui->setupUi(this);
    ui->tableWidgetWindowInfo->setColumnWidth(0, 55);
    ui->tableWidgetWindowInfo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableWidgetWindowInfo->setColumnWidth(1, 75);
    ui->tableWidgetWindowInfo->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->tableWidgetWindowInfo->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableWidgetWindowInfo->setColumnWidth(3, 15);
    ui->tableWidgetWindowInfo->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    RefreshTableWidgetWindowInfo();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::RefreshTableWidgetWindowInfo()
{
    ui->tableWidgetWindowInfo->setSortingEnabled(false);
    _targetWindowHandleList.clear();
    ui->statusbar->showMessage(QString("当前目标窗口数：0"));
    // // 遍历所有行和列，查找并断开复选框连接
    // int rowCount = ui->tableWidgetWindowInfo->rowCount();
    // for (int row = 0; row < rowCount; ++row)
    // {
    //     // 获取单元格中的小部件
    //     QWidget *widget = ui->tableWidgetWindowInfo->cellWidget(row, 3);
    //     QCheckBox *checkBox = qobject_cast<QCheckBox*>(widget);
    //     // 断开复选框的toggled信号连接
    //     QObject::disconnect(checkBox, &QCheckBox::toggled, nullptr, nullptr);
    // }
    ui->tableWidgetWindowInfo->clearContents();
    ui->tableWidgetWindowInfo->setRowCount(0);
    EnumWindows(EnumWindowsProc, 0); //reinterpret_cast<LPARAM>(this)
    ui->tableWidgetWindowInfo->setSortingEnabled(true);
    ui->tableWidgetWindowInfo->sortByColumn(0, Qt::DescendingOrder);
}

BOOL CALLBACK MainWindow::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindow(hwnd) || !IsWindowEnabled(hwnd))
        return TRUE;

    // 获取窗口标题
    WCHAR windowTitle[256];
    GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(WCHAR));
    if (wcslen(windowTitle) == 0)
        return TRUE;
    // 在 QTableWidget 中插入一行
    int row = g_mainWindowInstance->ui->tableWidgetWindowInfo->rowCount();
    g_mainWindowInstance->ui->tableWidgetWindowInfo->insertRow(row);
    // 添加可视情况
    QString visible = IsWindowVisible(hwnd) == TRUE ? "是" : "否";
    QTableWidgetItem *visibleItem = new QTableWidgetItem(visible);
    // 添加窗口标题
    QTableWidgetItem *titleItem = new QTableWidgetItem(QString::fromWCharArray(windowTitle));
    g_mainWindowInstance->ui->tableWidgetWindowInfo->setItem(row, 2, titleItem);
    // 获取窗口句柄
    QString hwndStr = QString::number(reinterpret_cast<quintptr>(hwnd), 16).toUpper();
    QTableWidgetItem *handleItem = new QTableWidgetItem(hwndStr);
    g_mainWindowInstance->ui->tableWidgetWindowInfo->setItem(row, 1, handleItem);
    visibleItem->setTextAlignment(Qt::AlignCenter);
    g_mainWindowInstance->ui->tableWidgetWindowInfo->setItem(row, 0, visibleItem);
    // 添加复选框
    /*
    QTableWidgetItem *checkBox = new QTableWidgetItem();
    checkBox->setCheckState(Qt::Checked);
    g_mainWindowInstance->ui->tableWidgetWindowInfo ->setItem(row, 3, checkBox);
    */
    QCheckBox *checkBox = new QCheckBox();
    g_mainWindowInstance->ui->tableWidgetWindowInfo->setCellWidget(row, 3, checkBox);
    // 连接复选框的toggled信号到槽函数
    QObject::connect(checkBox, &QCheckBox::toggled, [=](bool checked) {
        g_mainWindowInstance->onCheckboxToggled(checked, hwnd);
    });

    return TRUE;
}

void MainWindow::onCheckboxToggled(bool checked, HWND hwnd)
{
    if (checked)
    {
        TargetWindowInfo newWindowInfo;
        newWindowInfo.hwnd = hwnd;  // 设置窗口句柄
        newWindowInfo.visible = true;
        _targetWindowHandleList.append(newWindowInfo);
    }
    else
    {
        for (int i = 0; i < _targetWindowHandleList.size(); i++)
        {
            if (_targetWindowHandleList[i].hwnd == hwnd)
            {
                _targetWindowHandleList.removeAt(i);
                break;
            }
        }
    }
    ui->statusbar->showMessage(QString("当前目标窗口数：%1").arg(_targetWindowHandleList.size()));
}

void MainWindow::on_pushButtonRefreshTableWidgetWindowInfo_clicked()
{
    RefreshTableWidgetWindowInfo();
}

void MainWindow::on_pushButtonShowCheckedWindow_clicked()
{
    for (auto windowInfo : _targetWindowHandleList)
    {
        ShowTargetWindow(windowInfo.hwnd, g_mainWindowInstance->ui->checkBoxHideByTransparency->isChecked());
    }
}

void MainWindow::on_pushButtonStart_clicked()
{
    if (_hookId != nullptr)
    {
        QMessageBox::about(this, "错误", "鼠标钩子已经安装");
        return;
    }

    _hookId = SetWindowsHookEx(WH_MOUSE_LL,  LowLevelMouseProc, nullptr, 0); //(HWND)winId()?
    if (_hookId == nullptr)
    {
        QMessageBox::critical(this, "错误", QString("安装钩子失败！错误码: %1").arg(GetLastError()));
    }
}

void MainWindow::on_pushButtonStop_clicked()
{
    if (_hookId == nullptr)
    {
        QMessageBox::about(this, "错误", "鼠标钩子尚未安装");
    }
    else
    {
        if(UnhookWindowsHookEx(_hookId))
        {
            _hookId = nullptr;
        }
        else
        {
            QMessageBox::critical(this, "错误", QString("卸载钩子失败！错误码: %1").arg(GetLastError()));
        }
    }
}

LRESULT CALLBACK MainWindow::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_MBUTTONDOWN && g_mainWindowInstance->_targetWindowHandleList.size() != 0)
    {
        g_mainWindowInstance->ToggleWindowsVisibility();
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void MainWindow::on_actionOpenAboutDialog_triggered()
{
    AboutDialog *w = new AboutDialog();
    w->show();
}

void MainWindow::HideTargetWindow(HWND hwnd, bool byTransparency)
{
    if (byTransparency)
    {
        LONG currentStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        SetWindowLong(hwnd, GWL_EXSTYLE, currentStyle | WS_EX_TOOLWINDOW | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    }
    else
    {
        ShowWindow(hwnd, SW_HIDE);
    }
}

void MainWindow::ShowTargetWindow(HWND hwnd, bool byTransparency)
{
    if (byTransparency)
    {
        SetLayeredWindowAttributes(hwnd, 0, 255, 0);
        LONG  currentStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        SetWindowLong(hwnd, GWL_EXSTYLE, currentStyle & ~WS_EX_TOOLWINDOW);
    }
    else
    {
        ShowWindow(hwnd, SW_SHOWNORMAL);
    }
}

void MainWindow::ToggleWindowsVisibility()
{
    for (int i = 0; i < _targetWindowHandleList.size(); i++)
    {
        if (_targetWindowHandleList[i].visible == true)
        {
            HideTargetWindow(_targetWindowHandleList[i].hwnd, ui->checkBoxHideByTransparency->isChecked());
            _targetWindowHandleList[i].visible = false;
        }
        else
        {
            ShowTargetWindow(_targetWindowHandleList[i].hwnd, ui->checkBoxHideByTransparency->isChecked());
            _targetWindowHandleList[i].visible = true;
        }
    }
}

