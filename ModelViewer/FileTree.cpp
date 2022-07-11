#include "StdAfx.h"
#include "FileTree.h"

#include "kdw/TreeModel.h"
#include "kdw/Serialization.h"

#include "Serialization/Serialization.h"
#include "FileUtils/FileUtils.h"

KDW_REGISTER_CLASS(TreeModel, FileTreeModel, "ModelViewer\\Модель дерева файлов")

FileTreeModel::FileTreeModel()
{
	setRoot(new kdw::StringTreeRow(""));
}

void FileTreeModel::goUp()
{
	setCurrentDirectory((currentDirectory_ + "\\..").c_str());
	rebuild();
	//tree()->setFocus();
}

bool FileTreeModel::activateRow(kdw::TreeRow* treeRow, int column)
{
	kdw::StringTreeRow* row = safe_cast<kdw::StringTreeRow*>(treeRow);
	const char* str = row->value();
	if(strlen(str) > 4 && stricmp(str + strlen(str) - 4, ".3dx") == 0){
		signalFileSelected_.emit((currentDirectory_ + "\\" + str).c_str());
	}
	else{
		currentDirectory_ += "\\";
		currentDirectory_ += str;
		setCurrentDirectory(currentDirectory_.c_str());
	}
	return false;
}

void FileTreeModel::expandRow(kdw::TreeRow* treeRow)
{
	TreeModel::expandRow(treeRow);
	/*
	kdw::StringTreeRow* row = safe_cast<kdw::StringTreeRow*>(treeRow);
	kdw::StringTreeRow* n = row;
	n->clear();
	std::string path = currentDirectory_;
	while(n){
		path += "\\";
		path += n->value();
		n = n->parent();
	}
	fillNode(row, path.c_str());
	*/
}

void FileTreeModel::setCurrentDirectory(const char* path)
{
	char buffer[_MAX_PATH + 1];
	if(_fullpath(buffer, path, _MAX_PATH) == 0)
		return;
	currentDirectory_ = buffer;
	rebuild();
	signalLocationChanged_.emit();
}

void FileTreeModel::fillNode(kdw::StringTreeRow* root, const char* path)
{
	DirIterator it((std::string(path) + "\\*.*").c_str());

	while(it){
		if(it.isDirectory()){
			const char* text = *it;
			if(strcmp(text, ".") == 0 || strcmp(text, "..") == 0){
				++it;
				continue;
			}
			root->add(new kdw::StringTreeRow(text));
		}
		++it;
	}

	{
		DirIterator it((std::string(path) + "\\*.3dx").c_str());
		DirIterator end;
		while(it != end){
			if(it.isFile()){
				const char* text = *it;
				root->add(new kdw::StringTreeRow(text));
			}
			++it;
		}
	}
}

void FileTreeModel::rebuild()
{
	root()->clear();
	fillNode(root(), currentDirectory_.c_str());
	signalUpdated_.emit();
	//tree()->update();
}

void FileTreeModel::serialize(Archive& ar)
{
	std::string oldCurrentDirectory = currentDirectory_;
	ar.serialize(currentDirectory_, "currentDirectory", "Текущий каталог");
	if(oldCurrentDirectory != currentDirectory())
		setCurrentDirectory(currentDirectory_.c_str());

	//kdw::StringTreeModel::serialize(ar);
}
