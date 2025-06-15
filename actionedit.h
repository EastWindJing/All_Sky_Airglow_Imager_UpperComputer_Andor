#pragma execution_character_set("utf-8")

#ifndef ACTIONEDIT_H
#define ACTIONEDIT_H

#include <QWidget>

namespace Ui {
class ActionEdit;
}

class ActionEdit : public QWidget
{
    Q_OBJECT

public:
    explicit ActionEdit(QWidget *parent = nullptr);
    ~ActionEdit();

private slots:
    void on_btn_AddAction_clicked();

    void on_btn_Cancel_clicked();

private:
    Ui::ActionEdit *ui;
};

#endif // ACTIONEDIT_H
