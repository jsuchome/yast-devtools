#include <QPushButton>
#include <QMessageBox>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QStringList>
#include <QScrollBar>
#include <QString>
#include <QTextStream>
#include <QDialog>
#include <vector>
#include <QFileDialog>
#include <QLCDNumber>
#include <QLineEdit>
#include <QMenuBar>
#include <QCursor>
#include <QDomDocument>
#include <QColorDialog>
#include <QPixmap>
#include <QLabel>
#include <QCheckBox>
#include "cgui.h"

#include <QtDebug>

using namespace std;

MyWidget::MyWidget(QWidget *parent) : QWidget(parent){
	
	configPath = QDir::home().path();
	configPath += "/.y2log_ana";

	loadConfigs();
	setWindowTitle("y2log - Analyzer");
	setMinimumSize(1024,800);
	setContextMenuPolicy(Qt::CustomContextMenu);

	//Menu-Bar

	connect(&hMenu, SIGNAL(aboutToShow()), SLOT(buildHMenu()));
	QMenuBar *menu = new QMenuBar(this);

	hMenu.setTitle("H&idden Types");
	hMenu.setEnabled(false);
	
	fMenu = new QMenu(this);
	fMenu->setTitle("&File");
	fMenu->addAction("Open", this, SLOT(open()), QKeySequence(tr("Ctrl+O", "File|Open")));
	fMenu->addAction("Save", this, SLOT(saveEditor()), QKeySequence(tr("Ctrl+S", "File|Save")));
	fMenu->addSeparator();
	fMenu->addAction("Exit", qApp, SLOT(quit()));

	menu->addMenu(fMenu);
	menu->addMenu(&hMenu);
	menu->addAction("&Config", this, SLOT(saveConfigs()));
	menu->addAction("&Help", this, SLOT(help()));

	QPushButton *quit = new QPushButton("Quit", this);
	QPushButton *save = new QPushButton("Save", this);
	upDown = new QPushButton("Editor", this);
	upDown->setMaximumHeight(20);
	
	connect(quit, SIGNAL(clicked()), qApp, SLOT(quit()));
	connect(save, SIGNAL(clicked()), SLOT(saveEditor()));
	connect(upDown, SIGNAL(clicked()), SLOT(upDownEditor()));

	//TreeWidget for Entries
	
	listview = new QTreeWidget(this);
	QStringList headers;

	connect(listview, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(highOne(QTreeWidgetItem*, int)));
	connect(listview, SIGNAL(itemExpanded(QTreeWidgetItem*)), SLOT(resizeColumns()));
	connect(listview, SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(searchAhead(QTreeWidgetItem*, int)));

	listview->setColumnCount(7);
	listview->setSelectionMode(QAbstractItemView::ExtendedSelection);
	//listview->setAlternatingRowColors(true);
	listview->setMinimumHeight(400);

	headers << tr("host") << tr("date") << tr("time") << tr("loglevel");
	headers << tr("type") << tr("file") << tr("message");	

	listview->setHeaderLabels(headers);

	//TextEdit

	editor = new QTextEdit(this);
	editor->setLineWrapMode(QTextEdit::NoWrap);
	editor->hide();

	searchLine = new QLineEdit(this);
	QPushButton *searchButton = new QPushButton("Search", this);
	QPushButton *clearButton = new QPushButton("Clear", this);

	connect(searchButton, SIGNAL(clicked()), SLOT(search()));
	connect(clearButton, SIGNAL(clicked()), SLOT(clear()));
	connect(searchLine, SIGNAL(returnPressed()), SLOT(search()));

	//ComboBoxes for Search-Options

	cTypes = new QComboBox(this);
	cTypes->addItem("ALL");

	connect(cTypes, SIGNAL(activated(const QString&)), SLOT(cTypeChange(const QString&)));

	cLevel = new QComboBox(this);
	cLevel->addItem("ALL");
	cLevel->addItem("<0>");
	cLevel->addItem("<1>");
	cLevel->addItem("<2>");
	cLevel->addItem("<3>");
	cLevel->addItem("<4>");
	cLevel->addItem("<5>");
	cLevel->addItem("<6>");
	cLevel->addItem("<7>");

	cFunct = new QComboBox(this);
	cFunct->addItem("Hide");
	cFunct->addItem("Highlight");

	cColor = new QComboBox(this);
	QPixmap redpix(50, 25);
	redpix.fill(Qt::red);
	cColor->addItem(QIcon(redpix), "red");
	redpix.fill(Qt::yellow);
	cColor->addItem(QIcon(redpix), "yellow");
	redpix.fill(Qt::green);
	cColor->addItem(QIcon(redpix), "green");

	QPushButton *OKButton = new QPushButton("OK", this);
	
	connect(OKButton, SIGNAL(clicked()), SLOT(selOptions()));
	
	from = new QLCDNumber(5, this);
	till = new QLCDNumber(5, this);
	
	from->display(0);
	till->display(0);

	scroll = new QScrollBar(Qt::Horizontal, this);
	scroll->setRange(0, 0);
	scroll->setMinimumWidth(500);

	connect(scroll, SIGNAL(valueChanged(int)), SLOT(scrollView(int)));

	//Defintion of Widget-Layout
	
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QVBoxLayout *layout = new QVBoxLayout();
	QHBoxLayout *hLayout = new QHBoxLayout();
	QHBoxLayout *hOptionLayout = new QHBoxLayout();
	QHBoxLayout *hScrollLayout = new QHBoxLayout();
	QHBoxLayout *hEndLayout = new QHBoxLayout();
	QVBoxLayout *vEditorLayout = new QVBoxLayout();

	hLayout->addWidget(searchLine);
	hLayout->addWidget(searchButton);
	hLayout->addWidget(clearButton);

	hOptionLayout->addWidget(cTypes);
	hOptionLayout->addWidget(cLevel);
	hOptionLayout->addWidget(cFunct);
	hOptionLayout->addWidget(cColor);
	hOptionLayout->addWidget(OKButton);

	hScrollLayout->addWidget(from);
	hScrollLayout->addWidget(scroll);
	hScrollLayout->addWidget(till);

	layout->addLayout(hLayout);
	layout->addLayout(hOptionLayout);
	layout->addWidget(listview);
	layout->addLayout(hScrollLayout);

	hEndLayout->addWidget(save);
	hEndLayout->addWidget(quit);

	vEditorLayout->addWidget(editor);

	mainLayout->addLayout(layout);
	mainLayout->addWidget(upDown);
	mainLayout->addLayout(vEditorLayout);
	mainLayout->addLayout(hEndLayout);
	mainLayout->setMenuBar(menu);

	setLayout(mainLayout);

	//loading initial log-File per command-line
	if(qApp->arguments().size() > 1){
		init(qApp->arguments().value(1));
	}
}

//Load LogFile in TreeWidget

void MyWidget::init(QString logName){
	int fail;

	setCursor(Qt::WaitCursor);
	listview->hide();
	log y2log;
	char *tlog = qstrdup(qPrintable(logName));

	logFile y2File(tlog, &y2log);
	fail = y2File.readout();
	if(fail == 1){
		QMessageBox::critical(this, "ERROR", "Log-File could not be opened!", QMessageBox::Ok, QMessageBox::NoButton);
	}

	vector<string> keys;
	int temp;

	keys = y2log.getKeys();
	temp = keys.size();

	for (int i = 0; i < temp; i++){
		QTreeWidgetItem *tItem = new QTreeWidgetItem(listview);
		tItem->setText(0, (keys[i].c_str()));
		
		vector<entry> entries;
		entries = y2log.getEntries(keys[i]);

		//Show first entry as Parent-Item

		tItem->setText(1, (entries[0].getDate()).c_str());
		tItem->setText(2, (entries[0].getTime()).c_str());
		tItem->setText(3, (entries[0].getI()).c_str());
		tItem->setText(4, (entries[0].getType()).c_str());
		tItem->setText(5, (entries[0].getFile()).c_str());
		tItem->setText(6, (entries[0].getMsg()).c_str());

		int temp2 = entries.size();

		for (int y = 1; y < temp2; y++){
			QTreeWidgetItem *cItem = new QTreeWidgetItem(tItem);
			cItem->setText(0, (entries[y].getHost()).c_str());
			cItem->setText(1, (entries[y].getDate()).c_str());
			cItem->setText(2, (entries[y].getTime()).c_str());
			cItem->setText(3, (entries[y].getI()).c_str());
			cItem->setText(4, (entries[y].getType()).c_str());
			cItem->setText(5, (entries[y].getFile()).c_str());
			cItem->setText(6, (entries[y].getMsg()).c_str());

			highLine(cItem);

			if (types.contains((entries[y].getType()).c_str()) == false){
				types << (entries[y].getType()).c_str();
			}
		}
	}
	types.sort();
	cTypes->addItems(types);
	resizeColumns();
	listview->show();
	setCursor(Qt::ArrowCursor);
}

//Highlight double-clicked Item and its parent

void MyWidget::highOne(QTreeWidgetItem *item, int column){
	QString temp;

	item->setBackgroundColor(column, Qt::red);
	cItems.append(item);
	if (item->childCount() == 0){
		cItems.append(item->parent());
		item->parent()->setTextColor(0, Qt::darkRed);
	
		for (int i = 0; i < listview->columnCount(); i++){
			temp.append(item->text(i));
			temp.append(" ");
		}
		editor->append(temp);
	}
}

//Search through all columns (searchLine)

void MyWidget::search(){
	if (sItems.isEmpty() != true)
		sItems.clear();
	if(searchLine->text() == "")
		return;

	for(int i = 0; i < listview->columnCount(); i++){
		highlight(listview->findItems(searchLine->text(), Qt::MatchContains | Qt::MatchRecursive, i));
	}
}

// Highlight Item

void MyWidget::highlight(QList<QTreeWidgetItem*> items, QString col){
	cItems += items;
	//sItems += items;
	QColor color(col);
	for(int i = 0; i < items.count(); i++){
		items[i]->setBackgroundColor(0, color);
		if (items[i]->childCount() == 0){
			items[i]->parent()->setTextColor(0, color);
			cItems.append(items[i]->parent());
			if (sItems.contains(items[i]) == false)
				sItems.append(items[i]);
		}
	}
	till->display(sItems.count());
	scroll->setMaximum(sItems.count());
}

// Highlight Line

void MyWidget::highLine(QTreeWidgetItem *item){
	
	QColor color;

	color = levelBg[item->text(3)];
	for(int i = 0; i < listview->columnCount(); i++){
		item->setBackgroundColor(i, color);
	}
}

// Clear all colored Items

void MyWidget::clear(){
	for(int i = 0; i < cItems.count(); i++){
		cItems[i]->setTextColor(0, Qt::black);
		for(int y = 0; y < listview->columnCount(); y++)
			cItems[i]->setBackgroundColor(y, levelBg[cItems[i]->text(3)]);
	}
	cItems.clear();
	if(sItems.isEmpty() != true){
		sItems.clear();
	}
	till->display(0);
}

// Selection-Options

void MyWidget::selOptions(){
	if (sItems.isEmpty() != true)
		sItems.clear();
	if(cFunct->currentText() == "Highlight"){
		hideItems(cTypes->currentText(), cColor->currentText(), cLevel->currentText(), 0);
	}else if(cFunct->currentText() == "Hide"){
		hideItems(cTypes->currentText(), cColor->currentText(), cLevel->currentText(), 1);
	}else if(cFunct->currentText() == "Show"){
		hideItems(cTypes->currentText(), cColor->currentText(), cLevel->currentText(), 2);
	}
}

// Select all matching items

void MyWidget::hideItems(QString type, QString color, QString level, int opt){
	setCursor(Qt::WaitCursor);
	listview->hide();
	QList<QTreeWidgetItem*> items;

	if (type != "ALL"){
		items = listview->findItems(type, Qt::MatchExactly | Qt::MatchRecursive, 4); 
		
		if (level != "ALL"){
			for(int i = 0; i < items.count(); i++){
				if(items[i]->text(3) != level)
					items.takeAt(i);
			}
		}
	}else{
		if (level != "ALL")
			items = listview->findItems(level, Qt::MatchExactly | Qt::MatchRecursive, 3);
	}
	
	if (opt == 0){
		highlight(items, color);
	}else{
		for(int y = 0; y < items.count(); y++){
			if(opt == 1){
				listview->setItemHidden(items[y], true);
			}else{
				listview->setItemHidden(items[y], false);
			}
		}
	}
	if (opt == 1){
		hTypes.append(type);
	}
	if (opt == 2){
		hTypes.removeAll(type);
	}

	// enable/disable HiddenTypes Menu
	if(hTypes.size() > 0){
		hMenu.setEnabled(true);
	}else{
		hMenu.setEnabled(false);
	}

	cTypeChange(cTypes->currentText());
	listview->show();
	setCursor(Qt::ArrowCursor);
}

// Show Qt-Open Dialog for log-File selection

void MyWidget::open(){
	QString fileName = QFileDialog::getOpenFileName(this, "Open y2log-File...", QDir::currentPath(), "y2log*");
	if (fileName != "")
		init(fileName);
}

// Scrolling through matching entries

void MyWidget::scrollView(int val){
	if (sItems.isEmpty() != true && val != 0){
		if (listview->isItemExpanded(sItems.at(val - 1)->parent())){
			listview->setCurrentItem(sItems.at(val - 1));
			from->display(val);
		
		//If Item is not expanded
		}else{
			if (from->intValue() != scroll->value()){	
				if (from->intValue() > scroll->value()){
					//BACKWARD
					if(listview->currentItem() == sItems.at(scroll->value() -1)->parent()){
						int indx = from->intValue();
						while(sItems.at(from->intValue())->parent() == sItems.at(indx)->parent()){
							indx--;
							if(indx == 0){
								indx = sItems.size();
								break;
							}
						}
						scroll->setValue(indx);	
						listview->setCurrentItem(sItems.at(indx)->parent());
					}else{
						listview->setCurrentItem(sItems.at(from->intValue() - 2)->parent());	
					}
				}else{
					//FORWARD
					if(listview->currentItem() == sItems.at(scroll->value() - 1)->parent()){
						int indx = from->intValue();
						while(sItems.at(from->intValue())->parent() == sItems.at(indx)->parent()){
							indx++;
							if(indx == sItems.size()){
								indx = 0;
								break;
							}
						}
						scroll->setValue(indx);	
						listview->setCurrentItem(sItems.at(indx)->parent());
					}else{
						listview->setCurrentItem(sItems.at(scroll->value() - 1)->parent());
					}
				}

				from->display(scroll->value());
			}
		}	
	}
}

MyWidget::~MyWidget(){
}

// Resize columns to contents

void MyWidget::resizeColumns(){
	for(int i = 0; i < listview->columnCount(); i++){
		listview->resizeColumnToContents(i);
	}
}

// Changing the Functions-ComboBox (cFunct)

void MyWidget::cTypeChange(const QString& type){
	if (hTypes.contains(type) == true){
		if (cFunct->findText("Show", Qt::MatchExactly) == -1)
			cFunct->addItem("Show");
		if (cFunct->findText("Hide", Qt::MatchExactly) != -1)
			cFunct->removeItem(cFunct->findText("Hide", Qt::MatchExactly));
	}else{
		if (cFunct->findText("Hide", Qt::MatchExactly) == -1){
			cFunct->addItem("Hide");
		}
		if (cFunct->findText("Show", Qt::MatchExactly) != -1)
			cFunct->removeItem(cFunct->findText("Show", Qt::MatchExactly));
	}
}

// Build MenuBar for hidden Types

void MyWidget::buildHMenu(){
	hMenu.clear();
	QAction *temp;
	for (int i = 0; i < hTypes.size(); i++){
		temp = hMenu.addAction(hTypes.at(i));
		connect(temp, SIGNAL(triggered()), SLOT(hMenuSel()));
	}
}

// Sets choosen Type to "unhidden"

void MyWidget::hMenuSel(){
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		hideItems(action->text(), cColor->currentText(), cLevel->currentText(), 2);
}

// Opens Qt-FileDialog for saving Comment-File (Editor)

void MyWidget::saveEditor(){
	if(editor->toPlainText() == ""){
		QMessageBox::information(this, "Nothing to save!", "Comment-Box ist empty...", QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, "Save Comment-File", QDir::currentPath());
	if(fileName != ""){
		QFile sFile(fileName);
		
		sFile.open(QIODevice::WriteOnly);
		QTextStream out(&sFile);
		out << editor->toPlainText();
		sFile.close();
		if(sFile.exists() == false)
			QMessageBox::critical(this, "ERROR", "File could not be saved", QMessageBox::Ok, QMessageBox::NoButton);
	}
}

// Loads HelpFile in Help-Dialog

void MyWidget::help(){
	QDialog *helpDialog = new QDialog(this);
	helpDialog->setMinimumSize(450,100);
	QVBoxLayout *vLayout = new QVBoxLayout(helpDialog);

	QTextEdit *helpText = new QTextEdit(helpDialog);
	QPushButton *close = new QPushButton("Close", helpDialog);

	helpDialog->connect(close, SIGNAL(clicked()), SLOT(close()));

	vLayout->addWidget(helpText);
	vLayout->addWidget(close);
	helpDialog->setLayout(vLayout);

	QFile hFile("help");
	helpText->setReadOnly(true);
	if (hFile.open(QIODevice::ReadOnly) == false){
		QMessageBox::critical(this, "ERROR", "Help-File could not be found", QMessageBox::Ok, QMessageBox::NoButton);
	}else{
		QTextStream in(&hFile);
		helpText->append(in.readAll());
		helpDialog->show();
		helpDialog->raise();
		helpDialog->activateWindow();
	}
}

// Show and hide Editor-Widget

void MyWidget::upDownEditor(){
	if(editor->isHidden() == true){
		editor->show();
	}else{
		editor->hide();
	}
}

void MyWidget::loadConfigs(){

	// load Default-Settings
	levelBg["<0>"] = QColor(135, 206, 255);
	levelBg["<1>"] = QColor(242, 242, 242);
	levelBg["<2>"] = QColor(255, 255, 224);
	levelBg["<3>"] = QColor(255, 193, 192);
	levelBg["<4>"] = QColor(221, 160, 221);
	levelBg["<5>"] = QColor(84, 255, 159);
	levelBg["<6>"] = QColor(255, 255, 0);
	levelBg["<7>"] = QColor(173,255, 47);
	levelBg[""] = QColor(255, 255, 255);

	QDomDocument config("y2log_ana-settings");

	QFile configFile(configPath);	
	
	if(!configFile.open(QIODevice::ReadOnly)){
		return;
	}

	if(!config.setContent(&configFile)){
		configFile.close();
		return;
	}

	configFile.close();

	QDomElement conElem = config.documentElement();
	QDomNode conNod = conElem.firstChild();

	while(!conNod.isNull()){
		QDomElement e = conNod.toElement();
		if(!e.isNull()){
			// Load BG-Colors
			if(e.tagName() == "levelBg"){
				int r, g, b;
				
				r = e.attribute("r", "").toInt();
				g = e.attribute("g", "").toInt();
				b = e.attribute("b", "").toInt();

				if(!(r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255))
					levelBg[e.attribute("id", "unknown")] = QColor(r, g, b);
			}
		}
		conNod = conNod.nextSibling();
	}

}

// Widget for Configuration

void MyWidget::saveConfigs(){
	QDialog *confDialog = new QDialog(this);
	confDialog->setMinimumSize(450,100);
	
	QWidget *bgColWidget = new QWidget(confDialog);
	QGridLayout *bgColLay = new QGridLayout(bgColWidget);

	level = new QComboBox(bgColWidget);
	level->setEditable(false);

	map<QString, QColor>::iterator pIterMap;

	for(pIterMap = levelBg.begin(); pIterMap != levelBg.end(); pIterMap++){
		level->addItem(pIterMap->first);		
	}

	hexCol = new QLineEdit(bgColWidget);
	hexCol->setReadOnly(true);
	hexCol->setText(levelBg[level->currentText()].name());
	connect(level, SIGNAL(currentIndexChanged(const QString&)), SLOT(showColHex(const QString&)));

	colApply = new QCheckBox("apply after save", bgColWidget);
	colLabel = new QLabel(confDialog);
	QPixmap colPix(30, 30);
	colPix.fill(levelBg[level->currentText()]);
	colLabel->setPixmap(colPix);

	QPushButton *colChooser = new QPushButton("Choose Color", bgColWidget);
	connect(colChooser, SIGNAL(clicked()), SLOT(colorDialog()));

	bgColLay->addWidget(level, 0, 0);
	bgColLay->addWidget(hexCol, 0, 1);
	bgColLay->addWidget(colChooser, 0, 3);
	bgColLay->addWidget(colLabel, 0, 2);
	bgColLay->addWidget(colApply, 1, 0);
	bgColWidget->setLayout(bgColLay);
	
	QVBoxLayout *vLayout = new QVBoxLayout(confDialog);
	QHBoxLayout *hLayout = new QHBoxLayout(confDialog);

	QTabWidget *confTab = new QTabWidget(confDialog);
	confTab->addTab(bgColWidget, "BG-Color");

	colSave = new QPushButton("Save", confDialog);
	colSave->setEnabled(false);
	QPushButton *close = new QPushButton("Close", confDialog);

	confDialog->connect(close, SIGNAL(clicked()), SLOT(close()));
	connect(colSave, SIGNAL(clicked()), SLOT(writeConf()));

	hLayout->addWidget(colSave);
	hLayout->addWidget(close);
	vLayout->addWidget(confTab);
	vLayout->addLayout(hLayout);
	confDialog->setLayout(vLayout);

	confDialog->show();
	confDialog->raise();
	confDialog->activateWindow();
}

// Save config to file
void MyWidget::writeConf(){
QDomDocument config("y2log_ana-settings");

	QFile configFile(configPath);	
	
	// Look for existing config-File otherwise build one
	if(!configFile.exists())
		buildConfig();

	if(!configFile.open(QIODevice::ReadOnly)){
		qDebug() << "Datei nicht da";
		return;
	}

	QString errorStr;
	int errorLine;
	int errorColumn;
	
	if(!config.setContent(&configFile, true, &errorStr, &errorLine, &errorColumn)){
		configFile.close();
		qDebug() << "ErrorString" << errorStr;
		qDebug() << "ErrorLine" << errorLine;
		qDebug() << "ErrorColumn" << errorColumn;
		return;
	}
	
	configFile.close();	

	QDomElement conElem = config.documentElement();
	QDomNode conNod = conElem.firstChild();

	while(!conNod.isNull()){
		QDomElement e = conNod.toElement();
		if(!e.isNull()){
			// Save BG-Colors
			if(e.tagName() == "levelBg"){
				
				e.setAttribute("r", levelBg[e.attribute("id", "")].red());
				e.setAttribute("g", levelBg[e.attribute("id", "")].green());
				e.setAttribute("b", levelBg[e.attribute("id", "")].blue());

			}
		}
		conNod = conNod.nextSibling();
	}

	if(!configFile.open(QIODevice::WriteOnly)){
		QMessageBox::critical(this, "ERROR", "Config-File could no be written", QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	QTextStream out(&configFile);
	out << config.toString();
	configFile.close();
	colSave->setEnabled(false);

	//apply if colApply is checked
	if(colApply->checkState() == 2){
		QList<QTreeWidgetItem*> allItems = listview->findItems("*", Qt::MatchWildcard);
		for(int i = 0; i < allItems.size(); i++){
			for(int y = 0; y < allItems.at(i)->childCount(); y++){
				highLine(allItems.at(i)->child(y));	
			}
		}
	}

}

// Show Hex-Code of the Color
void MyWidget::showColHex(const QString &id){
	hexCol->setText(levelBg[id].name());
	QPixmap colPix(30, 30);
	colPix.fill(levelBg[id]);
	colLabel->setPixmap(colPix);
}

void MyWidget::colorDialog(){
	hexCol->setText(QColorDialog::getColor(QColor(hexCol->text()), this).name());
	levelBg[level->currentText()] = QColor(hexCol->text());
	colSave->setEnabled(true);
	QPixmap colPix(30, 30);
	colPix.fill(QColor(hexCol->text()));
	colLabel->setPixmap(colPix);
}

// build initial config-file

void MyWidget::buildConfig(){
	QDomDocument configTemp("y2log_ana-settings");
	QDomElement root = configTemp.createElement("y2log_ana-settings");
	configTemp.appendChild(root);

	// insert Section for levelBG
	map<QString, QColor>::iterator pIterMap;

	for(pIterMap = levelBg.begin(); pIterMap != levelBg.end(); pIterMap++){
		QDomElement child = configTemp.createElement("levelBg");
		child.setAttribute("id", pIterMap->first);
		child.setAttribute("r", pIterMap->second.red());
		child.setAttribute("g", pIterMap->second.green());
		child.setAttribute("b", pIterMap->second.blue());	
		root.appendChild(child);
	}
	
	QFile configFile(configPath);
	
	if(!configFile.open(QIODevice::WriteOnly))
		return;

	QTextStream out(&configFile);
	out << configTemp.toString();
	configFile.close();

}

// set clicked item to start point for search

void MyWidget::searchAhead(QTreeWidgetItem *item, int column){
	if(sItems.isEmpty() == true)
		return;

	if(sItems.contains(item)){
		from->display(sItems.indexOf(item));
		scroll->setSliderPosition(sItems.indexOf(item) + 1);
	}
}
