#include "EquipmentConfigWidget.h"
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>
#include <QApplication>
#include <QPalette>
#include <QTextStream>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupConnections();
        
        // 尝试加载默认配置文件
        openConfigFile("equipment_config.json");
    }

private slots:
    void openConfigFile() {
        QString fileName = QFileDialog::getOpenFileName(this, u8"打开配置文件", "", u8"JSON文件 (*.json)");
        if (!fileName.isEmpty()) {
            openConfigFile(fileName);
        }
    }
    
    void openConfigFile(const QString& fileName) {
        if (m_configWidget->loadFromJson(fileName)) {
            statusBar()->showMessage(QString(u8"已加载配置文件: %1").arg(fileName), 3000);
            updateWindowTitle();
        }
        m_configWidget->setCurrentFilePath(fileName);
    }
    
    void saveConfigFile() {
        if (m_configWidget->hasCurrentFile()) {
            // 如果有当前文件，直接保存
            if (m_configWidget->autoSave()) {
                statusBar()->showMessage(QString(u8"已保存配置文件: %1").arg(m_configWidget->getCurrentFilePath()), 3000);
            }
        } else {
            // 如果没有当前文件，弹出另存为对话框
            saveAsConfigFile();
        }
    }
    
    void saveAsConfigFile() {
        QString fileName = QFileDialog::getSaveFileName(this, u8"保存配置文件", "", u8"JSON文件 (*.json)");
        if (!fileName.isEmpty()) {
            if (m_configWidget->saveToJson(fileName)) {
                m_configWidget->setCurrentFilePath(fileName);
                statusBar()->showMessage(QString(u8"已保存配置文件: %1").arg(fileName), 3000);
                updateWindowTitle();
            }
        }
    }
    
    void onConfigChanged() {
        statusBar()->showMessage(u8"配置已修改", 2000);
        updateWindowTitle();
    }
    
    void onValidationError(const QString& message) {
        QMessageBox::warning(this, u8"配置错误", message);
        statusBar()->showMessage(QString(u8"错误: %1").arg(message), 5000);
    }
    
    void onFilePathChanged(const QString& filePath) {
        updateWindowTitle();
    }
    
    void showAbout() {
        QMessageBox::about(this, u8"关于", 
            u8"装备参数配置工具 v1.0\n\n"
            u8"支持多设备多状态的参数配置\n"
            u8"基于Qt框架开发");
    }

private:
    void setupUI() {
        setMinimumSize(1200, 800);
        
        // 创建主配置widget
        m_configWidget = new EquipmentConfigWidget(this);
        setCentralWidget(m_configWidget);
        
        // 创建菜单
        QMenuBar* menuBar = this->menuBar();
        
        QMenu* fileMenu = menuBar->addMenu(u8"文件(&F)");
        QAction* openAction = fileMenu->addAction(u8"打开配置文件(&O)");
        openAction->setShortcut(QKeySequence::Open);
        connect(openAction, &QAction::triggered, this,
                static_cast<void (MainWindow::*)()>(&MainWindow::openConfigFile));
        
        QAction* saveAction = fileMenu->addAction(u8"保存配置文件(&S)");
        saveAction->setShortcut(QKeySequence::Save);
        connect(saveAction, &QAction::triggered, this, &MainWindow::saveConfigFile);
        
        QAction* saveAsAction = fileMenu->addAction(u8"另存为(&A)...");
        saveAsAction->setShortcut(QKeySequence::SaveAs);
        connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAsConfigFile);
        
        fileMenu->addSeparator();
        QAction* exitAction = fileMenu->addAction(u8"退出(&X)");
        exitAction->setShortcut(QKeySequence::Quit);
        connect(exitAction, &QAction::triggered, this, &QWidget::close);
        
        QMenu* helpMenu = menuBar->addMenu(u8"帮助(&H)");
        QAction* aboutAction = helpMenu->addAction(u8"关于(&A)");
        connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
        
        // 创建状态栏
        statusBar()->showMessage(u8"就绪");
        
        updateWindowTitle();
    }
    
    void setupConnections() {
        connect(m_configWidget, &EquipmentConfigWidget::configChanged,
                this, &MainWindow::onConfigChanged);
        connect(m_configWidget, &EquipmentConfigWidget::validationError,
                this, &MainWindow::onValidationError);
        connect(m_configWidget, &EquipmentConfigWidget::filePathChanged,
                this, &MainWindow::onFilePathChanged);
    }
    
    void updateWindowTitle() {
        QString title = u8"装备参数配置工具";
        if (m_configWidget->hasCurrentFile()) {
            QString fileName = QFileInfo(m_configWidget->getCurrentFilePath()).fileName();
            title += QString(" - %1").arg(fileName);
        }
        setWindowTitle(title);
    }

private:
    EquipmentConfigWidget* m_configWidget;
};

// 全局样式，支持通过环境变量 DISABLE_CUSTOM_STYLE 一键关闭
static void applyGlobalStyle()
{
    if (!qEnvironmentVariableIsEmpty("DISABLE_CUSTOM_STYLE")) {
        return;
    }

    QString style = R"(
        QWidget {
            font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
            font-size: 12px;
            color: #1f2d3d;
        }
        QMainWindow, QDialog, QTabWidget::pane, QScrollArea {
            background: #f7f9fc;
        }
        QGroupBox {
            border: 1px solid #d0d7e2;
            border-radius: 6px;
            margin-top: 10px;
            padding: 8px 10px 10px 10px;
            background: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
            color: #2563eb;
            font-weight: 600;
        }
        QLabel {
            color: #334155;
        }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            border: 1px solid #c3cfe2;
            border-radius: 5px;
            padding: 4px 6px;
            background: #ffffff;
            selection-background-color: #2563eb;
            selection-color: #ffffff;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid #2563eb;
        }
        QComboBox::drop-down {
            border: none;
            width: 18px;
        }
        QComboBox::down-arrow {
            image: none;
            border: none;
        }
        QPushButton {
            background: #2563eb;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 6px 12px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #1d4ed8;
        }
        QPushButton:pressed {
            background: #1e40af;
        }
        QPushButton:disabled {
            background: #a5b4fc;
            color: #f1f5f9;
        }
        QTabBar::tab {
            background: #e5ecf6;
            padding: 6px 14px;
            margin: 2px;
            border-radius: 6px 6px 0 0;
            color: #1f2d3d;
        }
        QTabBar::tab:selected {
            background: #ffffff;
            color: #2563eb;
            font-weight: 600;
            border: 1px solid #d0d7e2;
            border-bottom: 1px solid #ffffff;
        }
        QTabBar::tab:hover {
            background: #f1f5fd;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 4px;
        }
        QScrollBar::handle:vertical {
            background: #c3d0e8;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #9fb6e1;
        }
        QStatusBar {
            background: #e5ecf6;
        }
    )";

    qApp->setStyleSheet(style);
}

// 简易日志过滤：默认屏蔽 qDebug（避免控制台乱码和刷屏），
// 设置环境变量 ENABLE_DEBUG_LOG 可重新启用
static void installMessageHandler()
{
    static bool verbose = !qEnvironmentVariableIsEmpty("ENABLE_DEBUG_LOG");
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
        if (type == QtDebugMsg && !verbose) {
            return;
        }
        QByteArray utf8 = msg.toUtf8();
        FILE* out = (type == QtDebugMsg || type == QtInfoMsg) ? stdout : stderr;
        fprintf(out, "%s\n", utf8.constData());
        fflush(out);
        Q_UNUSED(ctx);
    });
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    installMessageHandler();
    applyGlobalStyle();
    
    app.setApplicationName("EquipmentConfig");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("MyCompany");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

#include "main.moc" 
