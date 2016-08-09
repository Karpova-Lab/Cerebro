#include "mainwindow.h"
#include "dropbutton.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Calibration Utilty");
    helpMenu = menuBar()->addMenu("Help");
        gotoDocs = new QAction(this);
        gotoDocs->setText("User Guide");
    helpMenu->addAction(gotoDocs);
        about = new QAction(this);
        about->setText("About...");
    helpMenu->addAction(about);
    aboutDialog = new QMessageBox();
        aboutDialog->setWindowTitle("About");
        aboutDialog->setText("Version:\t1.2.0\nUpdated:\t6/21/2016");
        aboutDialog->setStandardButtons(QMessageBox::Close);


    chooseBox = new QGroupBox();
        chooseLayout = new QGridLayout();
            wantedLevel = new QSlider(Qt::Horizontal);
            wantedLevel->setRange(0,15);
            wantedLevel->setValue(4);
            wantedLevel->setTickPosition(QSlider::TicksBelow);
        chooseLayout->addWidget(wantedLevel,1,0);
            slideLabel = new QLabel("Target Power: "+QString::number(wantedLevel->value())+ " mW");
        chooseLayout->addWidget(slideLabel,0,0,Qt::AlignCenter);
            showGraph = new QCheckBox("Show Graphs");
        chooseLayout->addWidget(showGraph,0,1,2,1);
            selectFile_btn = new DropButton;//("Select Power Meter\ndata file");
            selectFile_btn->setCheckable(true);
            selectFile_btn->setAcceptDrops(true);
            selectFile_btn->setCheckable(false);
            selectFile_btn->setMinimumHeight(75);
        chooseLayout->addWidget(selectFile_btn,0,2,2,1);
            fileLabel = new QLabel();
        chooseLayout->addWidget(fileLabel,2,0,1,3,Qt::AlignCenter);
            codeTextBox = new QPlainTextEdit();
            codeTextBox->setReadOnly(true);
            codeTextBox->setMinimumHeight(130);
            codeTextBox->setMinimumWidth(450);
        chooseLayout->addWidget(codeTextBox,3,0,1,3);
    chooseBox->setLayout(chooseLayout);

    mainLayout = new QGridLayout();
    mainLayout->addWidget(chooseBox,0,0);
    this->resize(0,0);
    this->setFixedSize(this->size());
    QWidget *window = new QWidget();
    window->setLayout(mainLayout);
    setCentralWidget(window);

    connect(selectFile_btn,SIGNAL(clicked()),this,SLOT(chooseFile()));
    connect(selectFile_btn, SIGNAL(dropped(const QMimeData*)),this, SLOT(useDropped(const QMimeData*)));
    connect(wantedLevel,SIGNAL(sliderMoved(int)),this,SLOT(slideValueUpdate(int)));
    connect(about,SIGNAL(triggered(bool)),aboutDialog,SLOT(exec()));
    connect(gotoDocs,SIGNAL(triggered()),this,SLOT(openDocs()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::slideValueUpdate(int newVal){
    wantedLevel->setValue(newVal);
    slideLabel->setText("Target Power: "+QString::number(wantedLevel->value())+ " mW");
}


void MainWindow::chooseFile(){
    fileLabel->setText("");
    codeTextBox->clear();
    QString pathFromDialog = QFileDialog::getOpenFileName(this,tr("Select Power Meter Data"),QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),tr("(*.txt)"));
    qDebug()<<pathFromDialog;
    if(!pathFromDialog.isEmpty()){
        // Run python script to summarize data from base station and cerebro logs
        getCalVals(pathFromDialog);
    }
    else{
        selectFile_btn->clearFocus();
    }
}

void MainWindow::openDocs(){
//    opens readthedocs.com documentation
    QUrl site = QUrl::fromEncoded( "http://cerebro.readthedocs.io/en/latest/Hardware/implant.html#calibration-instructions");
    QDesktopServices::openUrl(site);
}

void MainWindow::useDropped(const QMimeData *mimeData)
{
    fileLabel->setText("");
    codeTextBox->clear();
    QString dataPath =  mimeData->text().simplified();
    dataPath.remove("file:///");
    getCalVals(dataPath);
}

void MainWindow::getCalVals(QString calibrateDataPath ){
    QProcess *process = new QProcess(this);
    QStringList pythonArgs;
    pythonArgs<<qApp->applicationDirPath()+"/python scripts/getCalibrationVec.py"<<"\""+calibrateDataPath+"\""<<QString::number(wantedLevel->value())<<QString::number(showGraph->isChecked()); //pass the calibration data into python script
    process->start("python",pythonArgs);
    process->waitForFinished(-1);
    QString errorString = process->readAllStandardError();
    QString resultString = process->readAll();
    codeTextBox->insertPlainText(resultString);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(resultString);
    QMessageBox alert;
    alert.setWindowTitle("Results are in!");
    alert.setText("The calibration vector has\nbeen copied to your clipboard");
    alert.exec();
    fileLabel->setText("<h3>"+calibrateDataPath+"</h3>");
    if (!errorString.isEmpty()){
        alert.setWindowTitle("Error Message from Python");
        alert.setText(errorString);
        alert.exec();
    }
}
