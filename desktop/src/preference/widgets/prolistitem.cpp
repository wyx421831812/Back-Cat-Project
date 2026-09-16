#include "prolistitem.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

ProListItem::ProListItem(const QString &title, const QString &description,
                         QWidget *control, bool vertical, QWidget *parent)
    : QFrame(parent)
    , m_control(control)
    , m_vertical(vertical)
{
    setObjectName(QStringLiteral("proListItem"));
    buildUi(vertical);
    setTitle(title);
    setDescription(description);
}

void ProListItem::buildUi(bool vertical)
{
    // p-4
    constexpr int pad = 16;

    if (vertical) {
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(pad, pad, pad, pad);
        root->setSpacing(12); // gap="middle"

        m_titleLabel = new QLabel(this);
        m_titleLabel->setObjectName(QStringLiteral("proItemTitle"));
        root->addWidget(m_titleLabel);

        m_descLabel = new QLabel(this);
        m_descLabel->setObjectName(QStringLiteral("proItemDesc"));
        m_descLabel->setWordWrap(true);
        m_descLabel->hide();
        root->addWidget(m_descLabel);

        if (m_control) {
            root->addWidget(m_control);
        }
        return;
    }

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(pad, pad - 2, pad, pad - 2);
    root->setSpacing(16); // gap="large"

    auto *textBox = new QVBoxLayout;
    textBox->setContentsMargins(0, 0, 0, 0);
    textBox->setSpacing(6);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("proItemTitle"));
    textBox->addWidget(m_titleLabel);

    m_descLabel = new QLabel(this);
    m_descLabel->setObjectName(QStringLiteral("proItemDesc"));
    m_descLabel->setWordWrap(true);
    m_descLabel->hide();
    textBox->addWidget(m_descLabel);
    textBox->addStretch();

    root->addLayout(textBox, 1); // flex-1

    if (m_control) {
        root->addWidget(m_control, 0, Qt::AlignRight | Qt::AlignVCenter);
    }
}

void ProListItem::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void ProListItem::setDescription(const QString &description)
{
    const bool has = !description.isEmpty();
    m_descLabel->setText(description);
    m_descLabel->setVisible(has);
}

void ProListItem::setControl(QWidget *control)
{
    if (m_control == control) {
        return;
    }
    if (m_control) {
        m_control->deleteLater();
    }
    m_control = control;
    if (m_vertical) {
        qobject_cast<QVBoxLayout *>(layout())->addWidget(control);
    } else {
        qobject_cast<QHBoxLayout *>(layout())->addWidget(control, 0,
                                                        Qt::AlignRight | Qt::AlignVCenter);
    }
}

void ProListItem::setDescriptionWidget(QWidget *widget)
{
    if (!widget) {
        return;
    }
    m_descLabel->hide();
    auto *box = qobject_cast<QBoxLayout *>(layout());
    if (m_vertical) {
        box->insertWidget(1, widget);
    } else {
        auto *textBox = qobject_cast<QVBoxLayout *>(layout()->itemAt(0)->layout());
        textBox->insertWidget(1, widget);
    }
}
