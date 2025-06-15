//#pragma execution_character_set("utf-8")

#include "actionedit.h"
#include "ui_actionedit.h"

ActionEdit::ActionEdit(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ActionEdit)
{
    ui->setupUi(this);
    this->setWindowTitle("添加动作");
}

ActionEdit::~ActionEdit()
{
    delete ui;
}

void ActionEdit::on_btn_AddAction_clicked()
{
    this->hide();
}


void ActionEdit::on_btn_Cancel_clicked()
{
    this->hide();
}

