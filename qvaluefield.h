#ifndef QVALUEFIELD_H
#define QVALUEFIELD_H



#include <QDoubleValidator>
#include <QLineEdit>
/// Нужно:
/// функция настройки крайних значений
/// fixup при конце редактирования
/// функция возвращающая число нужном формате
enum currValidator{
    INTVAL,
    DOUBLEVAL
};

class QValueField : public QLineEdit
{
    Q_OBJECT

signals:

private:
    QIntValidator* intValidator;
    QDoubleValidator* doubleValidator;
    int currVal;

public:
    QValueField(QWidget* _parent = 0) : QLineEdit(_parent)
    {
        intValidator = new QIntValidator();
        intValidator->setLocale(QLocale::C);
        doubleValidator = new QDoubleValidator();
        doubleValidator->setNotation(QDoubleValidator::StandardNotation);
        doubleValidator->setLocale(QLocale::C);
        connect(this, SIGNAL(editingFinished()), this, SLOT(fixUpValue()));
    }

    ~QValueField(){
        delete intValidator;
        delete doubleValidator;
    }

    void setBounds(double bottom, double top, int decimals){
        doubleValidator->setRange(bottom, top, decimals);
        this->setValidator(doubleValidator);
        currVal = DOUBLEVAL;
    }

    void setBounds(double bottom, double top){
        intValidator->setRange(bottom, top);
        this->setValidator(intValidator);
        currVal = INTVAL;
    }


protected:


private:


private slots:
    void fixUpValue(){
        QString value = this->text();
        if(currVal == INTVAL){
            intValidator->fixup(value);
        }
        else doubleValidator->fixup(value);
        this->setText(value);
    }

public:

};


#endif // QVALUEFIELD_H
