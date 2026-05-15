#ifndef GROUPBOXAREAWIDGET_H
#define GROUPBOXAREAWIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>

class groupBoxAreaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit groupBoxAreaWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , nextRow(0)
        , nextColumn(0)
        , group(nullptr)
    {
        setupUi();
        this->group = nullptr;
    }

    ~groupBoxAreaWidget()
    {
        for (auto& item : groupBoxVector) {
            delete item.first;
        }
        groupBoxVector.clear();

        delete verticalLayout;
        delete scrollArea;
    }

    bool removeGroup(int row, int column)
    {
        QLayoutItem* item = gridLayoutGroupBoxes->itemAtPosition(row, column);
        if (!item) return false;

        QGroupBox* group = qobject_cast<QGroupBox*>(item->widget());
        if (!groupBox) return false;

        removeItem(group);
        return true;
    }
    QGroupBox* groupAtPosition(int row, int column){
        QLayoutItem* item = gridLayoutGroupBoxes->itemAtPosition(row, column);
        if (!item) return nullptr;

        QGroupBox* group = qobject_cast<QGroupBox*>(item->widget());
        if (!group) return nullptr;
        return group;
    }

    QGroupBox* groupAtPosition(int index){
        int row = index/2;
        int column = index%2;
        QLayoutItem* item = gridLayoutGroupBoxes->itemAtPosition(row, column);
        if (!item) return nullptr;

        QGroupBox* group = qobject_cast<QGroupBox*>(item->widget());
        if (!group) return nullptr;
        return group;
    }

    int count(){
        return nextColumn + nextRow*2;
    }

    int addGroup(QGroupBox* group)
    {
        if (group == nullptr) return -1;
        int row = nextRow;
        int column = nextColumn;
        gridLayoutGroupBoxes->removeWidget(groupBox);
        groupBoxVector.append(qMakePair(group, qMakePair(row, column)));
        gridLayoutGroupBoxes->addWidget(group, row, column);
        bool newRow = false;
        if(column == 1){
            nextRow++;
            newRow = true;
            nextColumn = 0;
        }
        else{
            nextColumn++;
        }
        gridLayoutGroupBoxes->addWidget(groupBox, nextRow, nextColumn);

        if (newRow){
            gridLayoutGroupBoxes->setRowStretch(nextRow, 1);
        }
        this->group = nullptr;
        return column + row*2;
    }

    bool setGroup(QGroupBox* group)
    {
        if (group == nullptr) return false;
        this->group = group;
        return true;
    }

public slots:
    void addGroup()
    {
        emit requestGroup();
    }

signals:
    void requestGroup();

private:
    void setupUi()
    {
        verticalLayout = new QVBoxLayout(this);
        verticalLayout->setContentsMargins(0, 0, 0, 0);

        scrollArea = new QScrollArea(this);
        scrollArea->setMinimumSize(400, 0);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setWidgetResizable(true);

        scrollAreaWidgetContents = new QWidget();
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        scrollAreaWidgetContents->setSizePolicy(sizePolicy);

        gridLayout = new QGridLayout(scrollAreaWidgetContents);

        gridLayoutGroupBoxes = new QGridLayout();

        groupBox = new QGroupBox(scrollAreaWidgetContents);
        groupBox->setTitle("Add group");
        groupBox->setAlignment(Qt::AlignCenter);
        groupBox->setFlat(false);

        gridLayout_2 = new QGridLayout(groupBox);

        horizontalSpacer_2 = new QSpacerItem(1, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        gridLayout_2->addItem(horizontalSpacer_2, 0, 0, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        gridLayout_2->addItem(horizontalSpacer_3, 0, 2, 1, 1);

        pushButton = new QPushButton(groupBox);
        pushButton->setText("+");
        gridLayout_2->addWidget(pushButton, 0, 1, 1, 1);

        gridLayoutGroupBoxes->addWidget(groupBox, 0, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(422, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        gridLayoutGroupBoxes->addItem(horizontalSpacer, 1, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        gridLayoutGroupBoxes->addItem(verticalSpacer, 1, 0, 1, 1);

        gridLayoutGroupBoxes->setRowStretch(0, 1);
        gridLayoutGroupBoxes->setRowStretch(1, 1);
        gridLayoutGroupBoxes->setColumnStretch(0, 1);
        gridLayoutGroupBoxes->setColumnStretch(1, 1);

        gridLayout->addLayout(gridLayoutGroupBoxes, 0, 0, 1, 1);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);

        connect(pushButton, SIGNAL(clicked(bool)), this, SLOT(addGroup()));
    }

    void rearrangeGrid()
    {
        gridLayoutGroupBoxes->removeWidget(groupBox);

        for (auto& item : groupBoxVector) {
            gridLayoutGroupBoxes->removeWidget(item.first);
        }

        nextRow = 0;
        nextColumn = 0;

        for (int i = 0; i < groupBoxVector.size(); ++i) {
            gridLayoutGroupBoxes->addWidget(groupBoxVector[i].first, nextRow, nextColumn);
            groupBoxVector[i].second = qMakePair(nextRow, nextColumn);

            if(nextColumn == 1){
                nextRow++;
                nextColumn = 0;
            }
            else{
                nextColumn++;
            }
        }

        gridLayoutGroupBoxes->addWidget(groupBox, nextRow, nextColumn);
    }

    void removeItem(QGroupBox* itemToRemove)
    {
        int removeIndex = -1;
        for (int i = 0; i < groupBoxVector.size(); ++i) {
            if (groupBoxVector[i].first == itemToRemove) {
                removeIndex = i;
                break;
            }
        }

        if (removeIndex == -1) return;

        gridLayoutGroupBoxes->removeWidget(itemToRemove);
        delete itemToRemove;

        groupBoxVector.remove(removeIndex);
        rearrangeGrid();
    }

    // UI элементы
    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QGridLayout *gridLayout;
    QGridLayout *gridLayoutGroupBoxes;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *pushButton;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *verticalSpacer;

    // Данные
    QVector<QPair<QGroupBox*, QPair<int, int>>> groupBoxVector;
    int nextRow;
    int nextColumn;
    QGroupBox* group;
};

#endif // GROUPBOXAREAWIDGET_H
