/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MIT License

Copyright (c) 2015-2017 Andy S. Lustig

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <qtimer.h>
#include <settingsDialog.h>
#include "dropbutton.h"


class DropButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event);

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:

    QString                 usbTag,usbDescription;
    QString                 onTimeString,offTimeString;
    bool                    baseConnected,downloadConnected;
    bool                    inTestloop;
    int                     testCount;
    bool                    startingUp;
    bool                    debugOn;
    QSerialPort*            serial,*serial2;
    QTimer*                 timer;
    settingsDialog          *settingsDlog;
    bool                    pythonEnabled,mcubeEnabled,showHistogram;
    bool                    errorThrown;
    int                     baseFilter;
    QString                 startTime;
    QString                 saveName1;
    bool                    repeatOn;
    int                     titleLeftPower, titleRightPower;
    QString                 ratNumber;
    QString                 xavierVersion;
    bool                    sessionHasBegun;

    //Menus
    QAction*                gotoSettings,*toggleDebug,*openDir,*gotoApplocation,*gotoDocs,*about,*graphResults;
    QMenu*                  fileMenu,*viewMenu,*toolMenu,*goMenu,*helpMenu;
    QMessageBox*            aboutDialog;

    //Experimental Setup
    QGroupBox*              equipmentBox;
    QGridLayout*            equipmentLayout;
    QLabel*                 rig_lbl,*rat_lbl;
    QListWidget*            rigSelect,*ratSelect;
    QComboBox*              serialPortList;
    QLabel*                 connectBS_label;
    QPushButton*            refresh_btn,*connect_btn;
    QCheckBox*              debugCheckbox;
    QStringList             aliasStringList,rigList,ratList;

    //Cerebro Status
    QGroupBox*              cerStatusBox;
    QGridLayout*            cerStatusLayout;
    QLabel*                 serialNumber_lbl,*cerFirmware_lbl,*Lset_lbl,*Rset_lbl,*cerDelay_lbl,*cerOn_lbl,*cerOff_lbl,*cerTrain_lbl,*cerRamp_lbl;
    QPushButton*            getInfo_btn;
    QProgressBar*           batteryIndicator;
    QLabel*                 battery_lbl;

    //Base Station Monitor
    QPushButton*            clearBase_btn;
    QLabel*                 baseFilter_label;
    QGridLayout*            serialMonitorLayout;
    QGroupBox*              baseBox;
    QPlainTextEdit*         baseMonitor;
    QPushButton*            saveMonitor_btn;
    QString                 baseBuffer;

    //Waveform Adjustment
    QGroupBox*              adjustBox;
    QLabel*                 onTime_lbl,*offTime_lbl,*trainDescription_lbl,*trainDuration_lbl,*startDelay_lbl,*fade_label,*power_lbl,*last_settings;
    QRadioButton*           singleShot,*pulseTrain;
    QSpinBox*               onTime_spn,*offTime_spn,*trainDuration_spn,*startDelay_spn,*fade_spn,*power_spn;
    QPushButton*            newPower_btn,*sendSettings_btn;
    QGridLayout*            adjustmentLayout;
    QCheckBox*              fade_checkbox;

    //Characterization Commands
    QGroupBox*              charBox;
    QGridLayout*            charLayout;
    QSpinBox*               leftDiode_spn,*rightDiode_spn;
    QPushButton*            leftTest_btn,*rightTest_btn,*setDiode_btn,*isolationTest_btn,*combinedTest_btn,*startDiode_btn,*initialize_btn;

    //Calibration Dialogs
    QDialog*                sendFadeDialog,*createFadeDialog;
    QGroupBox*              chooseBox;
    QPushButton*            createVecBtn,*sendCal_btn;
    QGridLayout*            sendFadeLayout,*createVecLayout;
    QPlainTextEdit*         codeTextBox;
    QLabel*                 slideLabel,*dropLabel,*cerebroNum_lbl,*ldNum_lbl,*orLabel;
    QLineEdit*              cerebroNum_edit,*ldNum_edit;
    QCheckBox*              showGraph;
    QLineEdit*              wantedLevel;
    DropButton*             selectFile_btn;
    bool                    isFirstTime;

    //Triggering & Debugging
    QGroupBox*              bugBox;
    QGridLayout*            triggerLayout;
    QCheckBox*              trigger_checkbox;
    QSpinBox*               trials_spn;
    QPushButton*            trigger_btn,*stop_btn,*abort_btn,*batteryStatus_btn,*macro_btn;
    QProgressBar*           testProgress;
    QLineEdit*              macroText;

    //Download Monitor
    QDialog*                downloaderDialog;
    QPushButton*            refresh2_btn,*connect2_btn,*clearDownload_btn;
    QGridLayout*            connectionLayout2;
    QComboBox*              serialPortList2;
    QLabel*                 connectLU_label,*download_title;
    QPlainTextEdit*         downloadMonitor;

    //Session Start
    QDialog*                sessionStartDialog;
    QGridLayout*            sessionStartLayout;
    QLabel*                 baseConnected_lbl,*cerebroConnected_lbl,*implantSettingsMatch_lbl;
    QPlainTextEdit*         sessionStartMonitor;
    QPushButton*            startSession_btn,*retry_btn;
    QGridLayout*            mainLayout;

private slots:

    //Menu functions
    void getGraphs();
    void openDocs();
    void gotoDir();
    void gotoAppLocation();
    void showDebug();
    void setDebug();

    //Connect to ports
    void applySettings();
    void fillBasestationPorts();
    void fillDownloaderPorts();
    void connectBasePort();
    void connectDownloadPort();
    void sendTime();
    void matchPowers();
    void startSession();

    //Monitors
    void errorMsg();
    void clearMonitor();
    void clearMonitor2();
    void readSerial();
    void readLog();
    void updateFilter();
    void saveFile();

    //Debug Commands
    void sendTrigger();
    void abort();
    void macro();
    void getInfo();
    void getBatteryStatus();
    void checkForBase();

    //Cerebro Parmeters
    void trainChecked();
    void trainDur();
    void fadeChecked();
    void sendNewPower();
    void set();
    void powerSending();

    //Calibration
    void chooseFile();
    void getCalVals(QString calibrateDataPath);
    void useDropped(const QMimeData *mimeData);
    void sendToDiode();
    void setDiodePower();
    void testCombined();
    void sendCalVector();
    void sendCalGroups();
    void sendHardwareVals();

    //    void getJSON();
};


#endif // MAINWINDOW_H
