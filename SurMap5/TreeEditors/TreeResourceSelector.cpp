#include "StdAfx.h"
#include <typeinfo>

#include "TreeEditor.h"
#include "Dictionary.h"
#include "ResourceSelector.h"
#include "EditArchive.h"
#include "FileUtils.h"

bool GetAllTextureNames(const char* filename, vector<string>& names);

namespace {

bool testExistingFile(const char* fName){
	DWORD fa = GENERIC_READ;
	DWORD fs = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD fc = OPEN_EXISTING;
	DWORD ff = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	HANDLE hFile=CreateFile(fName, fa, fs, NULL, fc, ff, 0);
	if(hFile == INVALID_HANDLE_VALUE) return 0;
	else { CloseHandle(hFile); return 1; }
}

AttribEditorInterface& attribEditorInterface();

};


string selectAndCopyResource(const char* resourceHomePath, const char* filter, const char* defaultName, const char* title)
{
	static StaticMap<std::string, std::string> initialDirs_;

    string result;
    string filterstr= string("(") + filter + ")|" + filter + "||";
    CFileDialog fileDlg(TRUE, filter, defaultName, OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR, filterstr.c_str());
    fileDlg.m_ofn.lpstrTitle= title;
    std::string initial_dir = resourceHomePath;
	if(initialDirs_.find(resourceHomePath) != initialDirs_.end())
		initial_dir = initialDirs_[resourceHomePath];
	fileDlg.m_ofn.lpstrInitialDir = initial_dir.c_str ();//NULL;// (curDir???)

    if((fileDlg.DoModal()==IDOK) && testExistingFile(fileDlg.GetPathName()) ){
        string tmp=fileDlg.GetPathName();
        string path2file;
        string fileName;
        string::size_type i=tmp.find_last_of("\\");
        if(i!=string::npos){
            path2file=tmp.substr(0, i);
            fileName=&(tmp.c_str()[i+1]);
        }
        else{
            path2file="";
            fileName=tmp;
        }
        char fileFullPath[_MAX_PATH];
        char homeFullPath[_MAX_PATH];

        if(_fullpath(fileFullPath, path2file.c_str(), _MAX_PATH) == 0)
            return result; // ""
        if(_fullpath(homeFullPath, resourceHomePath, _MAX_PATH) == 0)
            return result; // ""

        result = resourceHomePath;
        if(!result.empty())
            result += "\\";
        result+=fileName;
        
        int filePathLen = strlen(fileFullPath);
        int homePathLen = strlen(homeFullPath);

        if(homePathLen <= filePathLen && strnicmp(fileFullPath, homeFullPath, homePathLen) == 0){
			initialDirs_[resourceHomePath] = std::string(fileFullPath + homePathLen, fileFullPath + filePathLen);
            result = resourceHomePath;
            result += std::string(fileFullPath + homePathLen, fileFullPath + filePathLen);
            result += "\\";
            result += fileName;
            return result; //ok(файл уже во внутренних ресурсах)
		}
        //копирование ресурса
        if(!CopyFile(fileDlg.GetPathName(), result.c_str(), FALSE)){
            result.clear();//err ошибка копирования во внутренние ресурсы
        }
    }
    return result;
}


string selectAndCopyModel(const char* resourceHomePath, const char* filter, const char* defaultName, const char* title)
{
    string result;
    string filterstr= string("(") + filter + ")|" + filter + "||";
    CFileDialog fileDlg(TRUE, filter, defaultName, OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR, filterstr.c_str());
    fileDlg.m_ofn.lpstrTitle = title;
    fileDlg.m_ofn.lpstrInitialDir = resourceHomePath;
    if((fileDlg.DoModal()==IDOK) && testExistingFile(fileDlg.GetPathName()) ){
        string tmp=fileDlg.GetPathName();
        string path2file;
        string fileName;

        string::size_type i = tmp.find_last_of("\\");
        if(i != string::npos){
            path2file = tmp.substr(0, i);
            fileName = &(tmp.c_str()[i+1]);
        }
        else{
            path2file = "";
            fileName = tmp;
        }
        char fileFullPath[_MAX_PATH];
        char homeFullPath[_MAX_PATH];

        if(_fullpath(fileFullPath, path2file.c_str(), _MAX_PATH) == 0)
            return result;//error //"";
        if(_fullpath(homeFullPath, resourceHomePath, _MAX_PATH) == 0)
            return result;//error //"";

        result = resourceHomePath;
        if(!result.empty())
            result += "\\";
        result += fileName;
        
        int filePathLen = strlen(fileFullPath);
        int homePathLen = strlen(homeFullPath);

        if(homePathLen <= filePathLen && strnicmp(fileFullPath, homeFullPath, homePathLen) == 0){
            result = resourceHomePath;
            result += std::string(fileFullPath + homePathLen, fileFullPath + filePathLen);
            result += "\\";
            result += fileName;
            return result; //ok(файл уже во внутренних ресурсах)
        }
        //копирование моделей
        if(!CopyFile(fileDlg.GetPathName(), result.c_str(), FALSE)){
            result.clear();//err ошибка копирования во внутренние ресурсы
            CString errorMessage;
            errorMessage = TRANSLATE("Ошибка копирования во внутренние ресурсы");
            AfxMessageBox (errorMessage, MB_OK | MB_ICONERROR);
        }

        //извлечение названий текстур из модели и копирование
        vector<string> textureNames;
        GetAllTextureNames(fileDlg.GetPathName(), textureNames);
        vector<string>::iterator p;
        for(p=textureNames.begin(); p!=textureNames.end(); p++){
            tmp=resourceHomePath;
            if(!tmp.empty()) tmp+="\\";
            tmp+="Textures\\";

            string::size_type i=p->find_last_of("\\");
            if(i!=string::npos)
                tmp+=&(p->c_str()[i+1]);
            else
                tmp+=*p;

            if(!CopyFile(p->c_str(), tmp.c_str(), FALSE)){
                result.clear();//err ошибка копирования во внутренние ресурсы
                CString errorMessage;
                errorMessage.Format (TRANSLATE("Не могу скопировать текстуру\nиз: %s\nв: %s"),
                                     p->c_str(), tmp.c_str());
                AfxMessageBox (errorMessage, MB_OK | MB_ICONERROR);
                break;
            }
        }
    }
    return result;
}


class TreeResourceSelector : public TreeEditorImpl<TreeResourceSelector, ResourceSelector> {
public:
	static StaticMap<std::string, std::string> initialDirsMap_;

    bool invokeEditor (ResourceSelector& selector, HWND parent) {
        if(selector.options().copy){
            string request = selectAndCopyResource(selector.options().initialDir.c_str(),
												   selector.options().filter.c_str(), ::extractFileName(selector).c_str(),
                                                   selector.options().title.c_str());
            if(!request.empty()){
                selector.setFileName(request.c_str());
                return true;
            }
		}
		else{
			string filter = std::string("(") + selector.options().filter + ")|" + selector.options().filter + "||";
			CFileDialog fileDlg (TRUE, filter.c_str(), ::extractFileName(selector).c_str(), OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR,
							     filter.c_str(), CWnd::FromHandle(parent));
			fileDlg.m_ofn.lpstrTitle = selector.options().title.c_str();
			fileDlg.m_ofn.lpstrInitialDir = selector.options().initialDir.c_str();
			//fileDlg.m_ofn.lpstrFile = selector.fileName();

			if(fileDlg.DoModal () == IDOK){
				char full_path[MAX_PATH];
				char cw_path[MAX_PATH];
				_fullpath (full_path, fileDlg.GetPathName (), sizeof(full_path) - 1);
				_fullpath (cw_path, ".", sizeof(cw_path) - 1);
				std::size_t cw_len = strlen (cw_path);
				if (strnicmp (full_path, cw_path, cw_len) == 0) {
					std::string relative_path = std::string(".") + std::string(full_path + cw_len, full_path + strlen (full_path));
						selector.setFileName (relative_path.c_str());
				} else {
					int result = AfxMessageBox ("Selected file lies outside project directory.\nIt would be stored as absolute path...\nProceed?", MB_OK | MB_YESNO, 0);
					if (result == IDYES) {
						selector.setFileName (fileDlg.GetPathName ());
					} else {

					}
				}

				return true;
			} else {
				return false;
			}
		}
        return false;
    }
    std::string nodeValue () const {
		return static_cast<std::string>(getData ());
    }
	bool canBeCleared() const{ return true; }
    void onClear(ResourceSelector& selector) {
		selector.setFileName("");
	}
	Icon buttonIcon() const{
		return TreeEditor::ICON_FILE;
	}
	bool hideContent () const {
        return true;
    }
};
REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid (ResourceSelector).name (), TreeResourceSelector);
//////////////////////////////////////////////////////////////////////////////
class TreeModelSelector : public TreeEditorImpl<TreeModelSelector, ModelSelector> {
public:
    bool invokeEditor (ModelSelector& selector, HWND parent) {
        if (selector.options().copy) {
            string request = selectAndCopyModel(selector.options().initialDir.c_str (),
                                                selector.options().filter.c_str (), ::extractFileName(selector).c_str(),
                                                selector.options().title.c_str ());
            if (!request.empty()) {
                selector.setFileName (request.c_str());
                return true;
			} else {
				//AfxMessageBox (attribEditorInterface().dictionary().translate("ОШИБКА: Модель или одна из её текстур не может быть скопированна"), MB_OK | MB_ICONERROR);
				return false;
			}
		} else {
			string filter = std::string("(") + selector.options().filter + ")|" + selector.options().filter + "||";
			CFileDialog fileDlg (TRUE, filter.c_str(), ::extractFileName(selector).c_str(), OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR,
								 filter.c_str(), CWnd::FromHandle(parent));
			fileDlg.m_ofn.lpstrTitle = selector.options().title.c_str();
			fileDlg.m_ofn.lpstrInitialDir = selector.options().initialDir.c_str();

			if (fileDlg.DoModal () == IDOK) {
				char full_path[MAX_PATH];
				char cw_path[MAX_PATH];
				_fullpath (full_path, fileDlg.GetPathName (), sizeof(full_path) - 1);
				_fullpath (cw_path, ".", sizeof(cw_path) - 1);
				std::size_t cw_len = strlen (cw_path);
				if (strnicmp (full_path, cw_path, cw_len) == 0) {
					std::string relative_path = std::string(".") + std::string(full_path + cw_len, full_path + strlen (full_path));
						selector.setFileName (relative_path.c_str());
				} else {
					int result = AfxMessageBox ("Selected file lies outside project directory.\nPath will be stored as absolute path...\nProceed?", MB_OK | MB_YESNO, 0);
					if (result == IDYES) {
						selector.setFileName (fileDlg.GetPathName ());
					} else {

					}
				}

				return true;
			} else {
				return false;
			}
		}
        return false;
    }
    std::string nodeValue () const {
		return static_cast<std::string>(getData ());
    }
	bool canBeCleared() const{ return true; }
    void onClear(ResourceSelector& selector){
		selector.setFileName("");
	}
	bool hideContent () const {
        return true;
    }
	Icon buttonIcon() const{
		return TreeEditor::ICON_FILE;
	}
};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid (ModelSelector).name (), TreeModelSelector);
