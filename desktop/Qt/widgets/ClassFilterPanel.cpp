#include "widgets/ClassFilterPanel.hpp"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace odf::desktop {

ClassFilterPanel::ClassFilterPanel(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel(QStringLiteral("Class filter"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    root->addWidget(title);
    search_ = new QLineEdit(this);
    search_->setPlaceholderText(QStringLiteral("Search classes…"));
    root->addWidget(search_);

    auto* actions = new QHBoxLayout();
    auto* selectAll = new QPushButton(QStringLiteral("Select all"), this);
    auto* clear = new QPushButton(QStringLiteral("Clear"), this);
    actions->addWidget(selectAll);
    actions->addWidget(clear);
    selectedCount_ = new QLabel(QStringLiteral("0 selected"), this);
    actions->addWidget(selectedCount_);
    root->addLayout(actions);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* contents = new QWidget(scroll);
    classesLayout_ = new QVBoxLayout(contents);
    classesLayout_->setContentsMargins(2, 2, 2, 2);
    classesLayout_->addStretch();
    scroll->setWidget(contents);
    root->addWidget(scroll, 1);

    connect(search_, &QLineEdit::textChanged, this, &ClassFilterPanel::applySearch);
    connect(selectAll, &QPushButton::clicked, this, [this] { setAll(true); });
    connect(clear, &QPushButton::clicked, this, [this] { setAll(false); });
}

void ClassFilterPanel::setClasses(const QStringList& labels, bool selectAll) {
    for (auto* checkbox : checkboxes_) {
        classesLayout_->removeWidget(checkbox);
        checkbox->deleteLater();
    }
    checkboxes_.clear();
    for (int index = 0; index < labels.size(); ++index) {
        auto* checkbox = new QCheckBox(
            QStringLiteral("%1  %2").arg(index).arg(labels[index]), this);
        checkbox->setProperty("classId", index);
        checkbox->setChecked(selectAll);
        classesLayout_->insertWidget(classesLayout_->count() - 1, checkbox);
        checkboxes_.push_back(checkbox);
        connect(checkbox, &QCheckBox::toggled, this, [this] { notifySelection(); });
    }
    applySearch(search_->text());
    notifySelection();
}

void ClassFilterPanel::setSelectedClasses(const QSet<int>& selectedClasses) {
    for (auto* checkbox : checkboxes_) {
        const QSignalBlocker blocker(checkbox);
        checkbox->setChecked(selectedClasses.contains(checkbox->property("classId").toInt()));
    }
    notifySelection();
}

QSet<int> ClassFilterPanel::selectedClasses() const {
    QSet<int> selected;
    for (const auto* checkbox : checkboxes_) {
        if (checkbox->isChecked()) selected.insert(checkbox->property("classId").toInt());
    }
    return selected;
}

void ClassFilterPanel::applySearch(const QString& text) {
    for (auto* checkbox : checkboxes_) {
        checkbox->setVisible(checkbox->text().contains(text, Qt::CaseInsensitive));
    }
}

void ClassFilterPanel::setAll(bool selected) {
    for (auto* checkbox : checkboxes_) {
        const QSignalBlocker blocker(checkbox);
        checkbox->setChecked(selected);
    }
    notifySelection();
}

void ClassFilterPanel::notifySelection() {
    const auto selected = selectedClasses();
    selectedCount_->setText(QStringLiteral("%1 selected").arg(selected.size()));
    emit selectionChanged(selected);
}

}  // namespace odf::desktop
