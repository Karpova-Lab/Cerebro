#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QWidget>
#include <QtWidgets>
#include <QDialog>
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>


class settingsDialog : public QDialog
{
Q_OBJECT

public:
    settingsDialog(QWidget *parent = 0);

//    QDialog*                settingsDialog;
    QGridLayout*            settingsLayout;

    QGroupBox*              directoryBox;
    QGridLayout*            directoryLayout;
    QLabel*                 directoryLabel;
    QPushButton*            changeDir_btn;

    QGroupBox*              sessionListsBox;
    QGridLayout*            sessionListsLayout;
    QLabel*                 rigLabel, *ratLabel, *cerebroLabel;
    QListWidget*            rigVals,*ratVals, *cerebroVals;
    QPushButton*            add1_btn,*add2_btn,*add3_btn;
    QLineEdit*              newItem1,*newItem2,*newItem3;
    QPushButton*            rmv1_btn,*rmv2_btn,*rmv3_btn;

    QGroupBox*              portEditBox;
    QGridLayout*            portEditLayout;
    QLabel*                 portnameLabel, *aliasLabel,*currentPorts, *newDescription;
    QListWidget*            aliasWidget;
    QStringList             aliasStringList;
    QComboBox*              portDropdown;
    QPushButton*            addAlias_btn,*rmvAlias_btn;
    QLineEdit*              newAlias;

    QDialog*                editLabelDialog;
    QGroupBox*              buttonBox;
    QGridLayout*            editLayout,*buttonLayout;
    QLineEdit*              editText;
    QLabel*                 newLabel;
    QPushButton*            changeLabel_btn,*cancelChange_btn;

    QGroupBox*              featuresBox;
    QGridLayout*            featuresLayout;
    QCheckBox*              pythonCheckbox,*mcubeCheckbox;
    bool                    pythonEnabled;
    bool                    mcubeEnabled;
    QStringList             rigList,ratList,cerebroList;


private:
    QString                 usbTag,usbDescription;

public slots:
    void myexec();
    void setPath();
    void removeItem();
    void addListItem();
    void openSettings();
    void addAlias();
    void removeAlias();
    void editLabel();
    void saveChanges();

signals:
    void dialogClosed();
};
#endif // TESTDIALOG_H

