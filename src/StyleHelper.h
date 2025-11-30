#pragma once

#include <QString>

namespace StyleHelper {
// 现代深色面板风格，便于多处复用，避免 main.cpp 硬编码。
inline QString darkPanelStyle()
{
    return QStringLiteral(R"(
        QWidget {
            background: #0b1220;
            color: #e2e8f0;
            font-family: "Inter", "SF Pro Text", "Microsoft YaHei", sans-serif;
            font-size: 12px;
        }
        QLabel {
            color: #f8fafc;
            font-weight: 600;
        }
        QListWidget, QSpinBox, QTextEdit, QLineEdit, QComboBox, QPlainTextEdit {
            background: #0f172a;
            border: 1px solid #1f2937;
            color: #e2e8f0;
            selection-background-color: #2563eb;
            selection-color: #f8fafc;
        }
        QAbstractItemView {
            background: #111827;
            color: #f8fafc;
            selection-background-color: #2563eb;
            selection-color: #f8fafc;
            outline: 0;
        }
        QListWidget::item:selected {
            background: #1e293b;
        }
        QGroupBox {
            border: 1px solid #1f2937;
            border-radius: 8px;
            margin-top: 10px;
            padding: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 2px 6px;
            color: #e0f2fe;
            font-weight: 700;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #2563eb, stop:1 #1d4ed8);
            border: none;
            border-radius: 8px;
            padding: 6px 12px;
            color: #f8fafc;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #1d4ed8;
        }
        QPushButton:pressed {
            background: #1e40af;
        }
        QPushButton:disabled {
            background: #334155;
            color: #94a3b8;
        }
        QTabBar::tab {
            background: #0f172a;
            color: #cbd5e1;
            padding: 10px 14px;
            border: 1px solid #1f2937;
            border-bottom: none;
            border-radius: 10px 10px 0 0;
            min-width: 120px;
            qproperty-expanding: false;
        }
        QTabBar::tab:selected {
            background: #1e293b;
            color: #f8fafc;
            font-weight: 700;
        }
        QTabBar::scroller {
            width: 24px;
            background: #111827;
            border-left: 1px solid #1f2937;
        }
        QTabBar QToolButton {
            background: transparent;
            border: none;
            color: #e2e8f0;
            padding: 0 4px;
        }
        QTabBar QToolButton:hover {
            background: #2563eb;
            color: #f8fafc;
        }
        QTabWidget::pane {
            border: 1px solid #1f2937;
            top: -1px;
            background: #0b1220;
        }
        QMenu {
            background: #0f172a;
            color: #e2e8f0;
            border: 1px solid #1f2937;
        }
        QMenu::item:selected {
            background: #2563eb;
            color: #f8fafc;
        }
        QMenu::separator {
            height: 1px;
            background: #1f2937;
            margin: 4px 0;
        }
        QScrollBar:vertical {
            background: #0f172a;
            width: 10px;
            margin: 4px;
        }
        QScrollBar::handle:vertical {
            background: #2563eb;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #1d4ed8;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            background: #1f2937;
            border: 1px solid #334155;
            height: 12px;
        }
        QScrollBar::add-line:vertical:hover, QScrollBar::sub-line:vertical:hover {
            background: #2563eb;
            border: 1px solid #2563eb;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
    )");
}
} // namespace StyleHelper
