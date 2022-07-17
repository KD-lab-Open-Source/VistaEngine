#ifndef __PARAMETER_TREE_H_INCLUDED__
#define __PARAMETER_TREE_H_INCLUDED__

#include "Handle.h"
#include "Parameters.h"

class UnitAttribute;
class AttributeBase;
class WeaponPrm;
class SourceBase;
class ParameterType;

namespace ParameterTree{

	class Node;
	class Exporter{
	public:
		Exporter();
		void exportExcel(const char* filename);
		void importExcel(const char* filename);

		void clearExported();
		template<class T>
		void setWorksheetPosition(T* addr, const Vect2i& pos){
			setWorksheetPosition(reinterpret_cast<void*>(addr), pos);
		}
		void setWorksheetPosition(void* addr, const Vect2i& pos);
		template<class T>
		bool alreadyExported(T* addr){
			return alreadyExported(reinterpret_cast<void*>(addr));
		}
		bool alreadyExported(void* addr);

		int currentSection() const { return currentSection_; }

		typedef std::vector<bool> UsedTypes;
	protected:
		void exportExcelNode(ExcelExporter& excel, Vect2i& pos, Node* root, int maxLevel, const char* prefix = "");
		void removeUnusedTypeColumns(ExcelExporter& excel, int startColumn);

		void importExcelNode(ExcelImporter& excel, Vect2i& pos, ParameterTree::Node* root, int maxLevel, const char* prefix = "");
		void readParametersRow(ExcelImporter& excel, Vect2i& pos, ParameterTree::Node* node);
		void readParametersHeader(ExcelImporter& excel, const Vect2i& pos);

		// export
		typedef StaticMap<void*, Vect2i> ExportedObjects;
		ExportedObjects exportedObjects_;
		int currentSection_;
		UsedTypes usedTypes_;

		// import
		typedef std::vector<ParameterType*> Types;
		Types types_;
	};

	class Node : public ShareHandleBase{
	public:
		Node(const char* name, void* objectAddress, ParameterCustom* values, float multiplier = 1.0f);
		typedef std::vector< ShareHandle<Node> > Children;
		Children& children(){ return children_; };
		const char* name() const{ return name_.c_str(); }
		const ParameterCustom* values() const{ return values_; }
		ParameterCustom* values(){ return values_; }
		Node* parent() { return parent_; }
		const Node* parent() const{ return parent_; }
		void setParent(Node* node);
		void push_back(Node* node) {
			children_.push_back(node);
			node->setParent(this);
		}
		void* objectAddress() const{ return objectAddress_; }
		bool empty() const;

		void setReadOnly(bool readOnly);
		bool readOnly() const;
		const char* path() const;
		void makeSingleUser(bool recurse);
		bool setValue(const ParameterType* type, float value);
		float multiplier() const{ return multiplier_; }
	protected:
		std::string name_;
		std::string path_;
		bool readOnly_;
		float multiplier_;
		Children children_;
		Node* parent_;
		ParameterCustom* values_;
		void* objectAddress_;
	};

	class ParameterArchive{
	public:
		ParameterArchive(Exporter* exporter);
		// virtuals:
		void openObject(const char* name, void* objectAddress);
		void closeObject();
		void operator()(const ParameterCustom& set, const char* name, float multiplier = 1.0f);
		void operator()(const WeaponDamage& set, const char* name){
			operator()(static_cast<const ParameterCustom&>(set), name);
		}
		template<class T>
		void operator()(const T& const_object, const char* name){
			T& object = const_cast<T&>(const_object);
			openObject(name, reinterpret_cast<void*>(&object));
			obtainParameters(*this, object);
			closeObject();
		}

		void setReadOnly(bool readOnly);

		std::string path();
		std::string subPath(const char* name);
		// ^^^

		Node* rootNode() { return rootNode_; }
		Exporter* exporter() { return exporter_; }
	protected:
		Exporter* exporter_;
		ShareHandle<Node> rootNode_;
		Node* currentNode_;
		int offset_;
	};


};
#endif
