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
#include <QHBoxLayout>
#include <QComboBox>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QDateTime>
#include "appconfig.h"

TodoWidget::TodoWidget(QWidget *parent)
    : ComponentBase(parent), m_filterMode(FilterAll)
{
    setupTransparentBackground();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // 标题 + 筛选下拉栏
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel(QStringLiteral("📝 待办清单"));
    titleLabel->setStyleSheet("color: white; font-size: 12px; font-weight: bold;");
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    headerLayout->addWidget(titleLabel);

    m_filterCombo = new QComboBox();
    m_filterCombo->addItem(QStringLiteral("全部"), FilterAll);
    m_filterCombo->addItem(QStringLiteral("未完成"), FilterActive);
    m_filterCombo->addItem(QStringLiteral("已完成"), FilterDone);
    m_filterCombo->setStyleSheet(
        "QComboBox {"
        "  background: rgba(255,255,255,180); border: none;"
        "  border-radius: 8px; padding: 3px 8px; font-size: 10px;"
        "  color: #0f172a; min-width: 60px;"
        "}"
        "QComboBox::drop-down { border: none; width: 14px; }"
        "QComboBox QAbstractItemView {"
        "  background: rgba(255,255,255,240); border: none;"
        "  border-radius: 8px; selection-background-color: #fbbf24;"
        "  color: #0f172a; padding: 4px; font-size: 10px;"
        "}"
    );
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TodoWidget::onFilterChanged);
    headerLayout->addWidget(m_filterCombo);
    layout->addLayout(headerLayout);

    // 列表
    m_listWidget = new QListWidget();
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setTextElideMode(Qt::ElideRight);
    m_listWidget->setMouseTracking(true);
    m_listWidget->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item {"
        "  color: white;"
        "  padding: 4px 8px 4px 2px;"
        "  font-size: 11px;"
        "}"
        "QListWidget::item:selected { background: rgba(255,255,255,30); }"
        // 美化勾选框 - 圆形，左对齐，右侧留间距给文字
        "QListWidget::indicator {"
        "  width: 16px; height: 16px; border-radius: 8px;"
        "  margin: 0px 12px 0px 2px;"
        "  border: 2px solid rgba(255,255,255,180);"
        "  background: rgba(255,255,255,40);"
        "}"
        "QListWidget::indicator:hover {"
        "  border: 2px solid white;"
        "  background: rgba(255,255,255,80);"
        "}"
        "QListWidget::indicator:checked {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 #fbbf24, stop:1 #f59e0b);"
        "  border: 2px solid #fbbf24;"
        "}"
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
    m_addBtn->setFixedSize(32, 32);
    m_addBtn->setCursor(Qt::PointingHandCursor);
    m_addBtn->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 #fbbf24, stop:1 #f59e0b);"
        "  border: none; border-radius: 16px;"
        "  font-size: 18px; font-weight: bold; color: white;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 #f59e0b, stop:1 #d97706);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 #d97706, stop:1 #b45309);"
        "}"
    );

    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_addBtn);
    layout->addLayout(inputLayout);

    // 连接信号
    connect(m_addBtn, &QPushButton::clicked, this, &TodoWidget::onAdd);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &TodoWidget::onAdd);
    connect(m_listWidget, &QListWidget::itemChanged, this, &TodoWidget::onItemChanged);
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_listWidget, &QListWidget::customContextMenuRequested,
            this, &TodoWidget::onItemContextMenu);

    loadTodos();
}

void TodoWidget::loadTodos()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(path);
    QFile file(path + "/backpet_todos.json");

    if (!file.open(QFile::ReadOnly)) {
        m_allTodos = QJsonArray();
        applyFilter();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    m_allTodos = doc.isArray() ? doc.array() : QJsonArray();

    // 为缺少 id 的旧数据补上唯一 id（向后兼容）
    bool needSave = false;
    qint64 baseId = QDateTime::currentMSecsSinceEpoch();
    for (int i = 0; i < m_allTodos.size(); ++i) {
        QJsonObject obj = m_allTodos[i].toObject();
        if (obj["id"].toString().isEmpty()) {
            obj["id"] = QString::number(baseId + i);
            m_allTodos[i] = obj;
            needSave = true;
        }
    }
    if (needSave) saveTodos();

    applyFilter();
}

void TodoWidget::applyFilter()
{
    QSignalBlocker blocker(m_listWidget);
    m_listWidget->clear();

    for (const QJsonValue &val : m_allTodos) {
        QJsonObject obj = val.toObject();
        bool done = obj["done"].toBool();
        bool include = false;
        switch (m_filterMode) {
            case FilterAll:    include = true; break;
            case FilterActive: include = !done; break;
            case FilterDone:   include = done; break;
        }
        if (!include) continue;

        QString text = obj["text"].toString();
        QString id = obj["id"].toString();
        auto *item = new QListWidgetItem(text, m_listWidget);
        item->setData(Qt::UserRole, id);
        item->setToolTip(text);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(done ? Qt::Checked : Qt::Unchecked);

        if (done) {
            QFont f = item->font();
            f.setStrikeOut(true);
            item->setFont(f);
        }
    }
}

void TodoWidget::saveTodos()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QFile file(path + "/backpet_todos.json");
    if (file.open(QFile::WriteOnly)) {
        file.write(QJsonDocument(m_allTodos).toJson());
        file.close();
    }
}

void TodoWidget::onAdd()
{
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty()) return;

    QJsonObject obj;
    obj["id"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    obj["text"] = text;
    obj["done"] = false;
    m_allTodos.append(obj);

    m_inputEdit->clear();
    saveTodos();
    applyFilter();
}

void TodoWidget::onItemChanged(QListWidgetItem *item)
{
    QFont f = item->font();
    bool done = (item->checkState() == Qt::Checked);
    f.setStrikeOut(done);
    item->setFont(f);

    QString targetId = item->data(Qt::UserRole).toString();
    // 通过唯一 id 精确匹配对应项并更新
    for (int i = 0; i < m_allTodos.size(); ++i) {
        QJsonObject obj = m_allTodos[i].toObject();
        if (obj["id"].toString() == targetId) {
            obj["done"] = done;
            m_allTodos[i] = obj;
            break;
        }
    }
    saveTodos();
    applyFilter();
}

void TodoWidget::onItemContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_listWidget->itemAt(pos);
    if (!item) return;

    m_listWidget->setCurrentItem(item);

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: rgba(30,30,50,240); color: white; border: none; border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
        "QMenu::item:selected { background: rgba(251,191,36,180); }"
    );

    QAction *editAction = menu.addAction(QStringLiteral("✏️ 编辑"));
    QAction *deleteAction = menu.addAction(QStringLiteral("🗑️ 删除"));

    QAction *selected = menu.exec(m_listWidget->mapToGlobal(pos));

    if (selected == editAction) {
        onEditItem();
    } else if (selected == deleteAction) {
        onDeleteItem();
    }
}

void TodoWidget::onEditItem()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) return;

    bool ok = false;
    QString newText = QInputDialog::getText(
        this,
        QStringLiteral("编辑任务"),
        QStringLiteral("任务内容:"),
        QLineEdit::Normal,
        item->text(),
        &ok
    );

    if (!ok || newText.trimmed().isEmpty()) return;
    newText = newText.trimmed();

    QString targetId = item->data(Qt::UserRole).toString();

    // 更新视图项
    {
        QSignalBlocker blocker(m_listWidget);
        item->setText(newText);
        item->setToolTip(newText);
    }

    // 更新数据
    for (int i = 0; i < m_allTodos.size(); ++i) {
        QJsonObject obj = m_allTodos[i].toObject();
        if (obj["id"].toString() == targetId) {
            obj["text"] = newText;
            m_allTodos[i] = obj;
            break;
        }
    }
    saveTodos();
}

void TodoWidget::onDeleteItem()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) return;

    QString targetId = item->data(Qt::UserRole).toString();

    // 从 m_allTodos 中删除对应项
    for (int i = 0; i < m_allTodos.size(); ++i) {
        QJsonObject obj = m_allTodos[i].toObject();
        if (obj["id"].toString() == targetId) {
            m_allTodos.removeAt(i);
            break;
        }
    }
    saveTodos();
    applyFilter();
}

void TodoWidget::onFilterChanged(int index)
{
    m_filterMode = static_cast<FilterMode>(m_filterCombo->itemData(index).toInt());
    applyFilter();
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
