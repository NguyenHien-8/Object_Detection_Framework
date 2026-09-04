#pragma once

#include <QSet>
#include <QWidget>

class QCheckBox;
class QLineEdit;
class QLabel;
class QVBoxLayout;

namespace odf::desktop {

class ClassFilterPanel final : public QWidget {
    Q_OBJECT

public:
    explicit ClassFilterPanel(QWidget* parent = nullptr);

    void setClasses(const QStringList& labels, bool selectAll);
    void setSelectedClasses(const QSet<int>& selectedClasses);
    [[nodiscard]] QSet<int> selectedClasses() const;

signals:
    void selectionChanged(QSet<int> selectedClasses);

private:
    void applySearch(const QString& text);
    void setAll(bool selected);
    void notifySelection();

    QLineEdit* search_{nullptr};
    QLabel* selectedCount_{nullptr};
    QVBoxLayout* classesLayout_{nullptr};
    QList<QCheckBox*> checkboxes_;
};

}  // namespace odf::desktop
