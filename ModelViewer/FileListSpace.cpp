#include "StdAfx.h"
#include "FileListSpace.h"
#include "kdw/Serialization.h"
#include "kdw/HBox.h"
#include "kdw/CommandManager.h"
#include "kdw/Label.h"
#include "kdw/Button.h"

#include "FileTree.h"
#include "MainView.h"

FileListSpace::FileListSpace()
{
	kdw::VBox* box = new kdw::VBox();
	add(box);
	{
		commands().add("fileList.go.up");
		commands().add("fileList.go.back");
		commands().add("fileList.go.workingDirectory");

		setMenu("fileList");

		FileTreeModel* model = new FileTreeModel;
		fileTree_ = new kdw::Tree(model);
		fileTree_->setColumn(0, new kdw::StringTreeColumnDrawer(fileTree_), 1.0f);

		model->signalLocationChanged().connect(this, &FileListSpace::onLocationChanged);
		model->signalFileSelected().connect(this, &FileListSpace::onFileSelected);

		kdw::HBox* hbox = new kdw::HBox(0, 4);
		box->pack(hbox);
		{
			hbox->pack(locationLabel_ = new kdw::Label(".", false, 0), false, true, true);
			locationLabel_->setExpandByContent(false);
			
			kdw::Button* button = new kdw::Button(TRANSLATE("Вверх"));
			button->signalPressed().connect(model, &FileTreeModel::goUp);
			hbox->pack(button);
		}

		box->pack(fileTree_, false, true, true);
	}

}

void FileListSpace::serialize(Archive& ar)
{
	Space::serialize(ar);
	FileTreeModel* model = safe_cast<FileTreeModel*>(fileTree_->model());
	std::string location = model->currentDirectory();
	if(ar.serialize(location, "location", "Путь") && ar.isInput())
		model->setCurrentDirectory(location.c_str());
	fileTree_->serializeState(ar, "treeState", 0);
}

void FileListSpace::onLocationChanged()
{
	if(FileTreeModel* model = dynamic_cast<FileTreeModel*>(fileTree_->model()))
		locationLabel_->setText(model->currentDirectory());
}

void FileListSpace::onFileSelected(const char* fileName)
{
	xassert(globalDocument);
	globalDocument->loadModel(fileName);
}

KDW_REGISTER_CLASS(Space, FileListSpace, "Список файлов")
