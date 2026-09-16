#include "prolist.h"

#include <QLabel>
#include <QVBoxLayout>

ProList::ProList(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 16); // not-last:mb-4
    outer->setSpacing(8);                   // gap="small"

    m_title = new QLabel(title, this);
    m_title->setObjectName(QStringLiteral("proListTitle"));
    outer->addWidget(m_title);

    m_itemsLayout = new QVBoxLayout;
    m_itemsLayout->setContentsMargins(0, 0, 0, 0);
    m_itemsLayout->setSpacing(8); // gap="middle"
    outer->addLayout(m_itemsLayout);
}

void ProList::addItem(QWidget *item)
{
    m_itemsLayout->addWidget(item);
}

void ProList::setTitle(const QString &title)
{
    m_title->setText(title);
}
