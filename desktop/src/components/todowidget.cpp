#include "todowidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QStandardPaths>
#include <QDir>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QCheckBox>
#include "appconfig.h"

TodoWidget::TodoWidget(QWidget *parent)
    : ComponentBase(parent)
{
    setupTransparentBackground();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // 标题
    auto *titleLabel = new QLabel(QStringLiteral("📝 待办清单"));
    titleLabel->setStyleSheet("color: white; font-size: 12px; font-weight: bold;");
    layout->addWidget(titleLabel);

    // 列表
    m_listWidget = new QListWidget();
    m_listWidget->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { color: white; padding: 4px; font-size: 11px; }"
        "QListWidget::item:selected { background: rgba(255,255,255,30); }"
    );
    layout->addWidget(m_listWidget);

    // 输入框
    auto *inputLayout = new QHBoxLayout();
    m_inputEdit = new QLineEdit();
    m_inputEdit->setPlaceholderText(QStringLiteral("添加任务..."));
    m_inputEdit->setStyleSheet(
        "QLineEdit { background: rgba(255,255,255,200); border: none; "
        "border-radius: 10px; padding: 6px 10px; font-size: 11px; }"
    );

    m_addBtn = new QPushButton("+");
    m_addBtn->setFixedSize(30, 30);
    m_addBtn->setStyleSheet(
        "QPushButton { background: rgba(255,255,255,200); border: none; "
        "border-radius: 15px; font-size: 16px; font-weight: bold; color: #0f172a; }"
    );

    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_addBtn);
    layout->addLayout(inputLayout);

    // 连接信号
    connect(m_addBtn, &QPushButton::clicked, this, &TodoWidget::onAdd);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &TodoWidget::onAdd);
    connect(m_listWidget, &QListWidget::itemChanged, this, &TodoWidget::onItemChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &TodoWidget::onItemDoubleClicked);

    loadTodos();
}

void TodoWidget::loadTodos()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(path);
    QFile file(path + "/backpet_todos.json");

    if (!file.open(QFile::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return;

    m_listWidget->clear();
    QJsonArray todos = doc.array();
    for (const QJsonValue &val : todos) {
        QJsonObject obj = val.toObject();
        auto *item = new QListWidgetItem(obj["text"].toString(), m_listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(obj["done"].toBool() ? Qt::Checked : Qt::Unchecked);

        if (obj["done"].toBool()) {
            QFont f = item->font();
            f.setStrikeOut(true);
            item->setFont(f);
        }
    }
}

void TodoWidget::saveTodos()
{
    QJsonArray todos;
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto *item = m_listWidget->item(i);
        QJsonObject obj;
        obj["text"] = item->text();
        obj["done"] = (item->checkState() == Qt::Checked);
        todos.append(obj);
    }

    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QFile file(path + "/backpet_todos.json");
    if (file.open(QFile::WriteOnly)) {
        file.write(QJsonDocument(todos).toJson());
        file.close();
    }
}

void TodoWidget::onAdd()
{
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty()) return;

    auto *item = new QListWidgetItem(text, m_listWidget);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);

    m_inputEdit->clear();
    saveTodos();
}

void TodoWidget::onItemChanged(QListWidgetItem *item)
{
    QFont f = item->font();
    f.setStrikeOut(item->checkState() == Qt::Checked);
    item->setFont(f);
    saveTodos();
}

void TodoWidget::onItemDoubleClicked(QListWidgetItem *item)
{
    // 双击删除
    delete m_listWidget->takeItem(m_listWidget->row(item));
    saveTodos();
}

void TodoWidget::refresh()
{
    loadTodos();
}

void TodoWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto &cfg = AppConfig::instance();
    QColor bg = cfg.secondaryColor();
    bg.setAlpha(200);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(4, 4, -4, -4), 15, 15);
    p.fillPath(path, bg);
}
