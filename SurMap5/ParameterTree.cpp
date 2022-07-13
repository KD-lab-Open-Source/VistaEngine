#include "StdAfx.h"
#include "ExcelExport\ExcelExporter.h"

#include "Environment\SourceZone.h"

#include "AttributeReference.h"
#include "UnitAttribute.h"
#include "WeaponPrms.h"
#include "UnitAttribute.h"

#include "ParameterTree.h"

#include "Console.h"
#include "UserInterface\UI_Render.h"
#include "Serialization\StringTable.h"

#include "UnicodeConverter.h"
#include "WBuffer.h"

namespace ParameterTree{

static std::string unitNameFromInterfaceName(const wchar_t* interfaceName)
{
	return UI_Render::instance().extractFirstLineText(interfaceName);
}

template<class Reference>
const char* makeReferenceCopy(const Reference& const_reference, const char* newName)/*{{{*/
{
	Reference& reference = const_cast<Reference&>(const_reference);
	typedef Reference::StringTableType Library;
	Library& library = Library::instance();
	xassert(!library.exists(newName));

	BinaryOArchive oa;
	oa.serialize(*reference, "element", "element");

	BinaryIArchive ia(oa);

	library.add(newName);
	ia.serialize(*Reference(newName), "element", "element");
	return newName;
}/*}}}*/

const char* makeParameterCopy(const ParameterValueReference& const_reference, const char* newName)/*{{{*/
{
	ParameterValueReference& reference = const_cast<ParameterValueReference&>(const_reference);
	ParameterValue value = *reference;
	float val = value.value();
	value.setRawValue(val);
	value.setFormula(ParameterFormulaReference());
	value.setName(newName);
	value.setGroup(ParameterGroupReference());
	value.setType(reference->type());
	ParameterValueReference result;
	if(ParameterValueTable::instance().exists(newName)){
		reference = ParameterValueReference(newName);
		ParameterValue& currentValue = const_cast<ParameterValue&>(*reference);
		int stringIndex = currentValue.stringIndex();
		currentValue = value;
		currentValue.setStringIndex(stringIndex);
	}
	else{
		ParameterValueTable::instance().add(value);
		reference = ParameterValueReference(newName); 
	}
	return newName;
}/*}}}*/

void makeParameterCopy(const ParameterCustom& params, const char* prefix)/*{{{*/
{
	ParameterCustom::Vector::iterator it;
	ParameterCustom::Vector& vec = const_cast<ParameterCustom::Vector&>(params.customVector());
	FOR_EACH(vec, it){
		ParameterValueReference& ref = *it;
		std::string name = prefix;
		name += " / ";
		name += ref->type().c_str();
		makeParameterCopy(ref, name.c_str());
	}
}/*}}}*/


void Exporter::clearExported()
{
	exportedObjects_.clear();
	usedTypes_.clear();
	usedTypes_.resize(ParameterTypeTable::instance().size());
}

void Exporter::removeUnusedTypeColumns(ExcelExporter& excel, int startColumn)
{
	UsedTypes::reverse_iterator it;
	int index = usedTypes_.size();
	for(it = usedTypes_.rbegin(); it != usedTypes_.rend(); ++it){
		--index;
		if(!*it){
			excel.setBackColor(Recti(startColumn + index, 0, 1, 100), 3);
			excel.removeColumn(startColumn + index - 1);
		}
	}
}

bool Exporter::alreadyExported(void* addr)
{
	bool alreadyExported = exportedObjects_.exists(addr);
	if(!alreadyExported)
		exportedObjects_[addr] = Vect2i::ZERO;
	return alreadyExported;
}

void Exporter::setWorksheetPosition(void* addr, const Vect2i& pos)
{
	exportedObjects_[addr] = pos;
}

typedef std::vector<WeaponPrmReference> Weapons;

void collectUpgrades(const WeaponPrm* constWeapon, Weapons& upgrades)
{
	WeaponPrm* weapon = const_cast<WeaponPrm*>(constWeapon);
	const WeaponPrm::Upgrades& currentUpgrades = weapon->upgrades();
	WeaponPrm::Upgrades::const_iterator it;
	FOR_EACH(currentUpgrades, it){
		WeaponPrmReference ref = it->reference();
		if(&*ref){
			if(std::find(upgrades.begin(), upgrades.end(), &*ref) == upgrades.end()){
				upgrades.push_back(ref);
				collectUpgrades(&*ref, upgrades);
			}
		}
	}
}

void obtainParameters(ParameterArchive& ar, UnitAttribute& attribute)/*{{{*/
{
	int section = ar.exporter()->currentSection();
	if(AttributeBase* attr = const_cast<AttributeBase*>(attribute.get())){
		switch(section){
		case 0:
			ar(attr->parametersInitial, "Initial Parameters");
			ar(attr->installValue, "Install Value");
			break;
		case 1:
		{
			ar(attr->productivity, "Productivity (per second)");
			ar(attr->resourceCapacity, "Resource Capacity");
			break;
		}
		case 2:
		{
			WeaponSlotAttributes::iterator it;
			FOR_EACH(attr->weaponAttributes, it){
				WeaponPrmReference ref = const_cast<WeaponPrmReference&>(it->second.weaponPrmReference());
				std::string name = ref.c_str();
				ar(*ref, name.c_str());

				Weapons upgrades;
				collectUpgrades(&*ref, upgrades);

				Weapons::iterator wit;
				FOR_EACH(upgrades, wit){
					WeaponPrm* weapon = const_cast<WeaponPrm*>(&**wit);
					std::string name = wit->c_str();
					ar(*weapon, name.c_str());
				}
			}
			break;
		}
		case 3:
		{
			AttributeBase::Upgrades::iterator it;
			FOR_EACH(attr->upgrades, it){
				AttributeBase::Upgrade& upgrade = it->second;
				ar(upgrade.upgradeValue, unitNameFromInterfaceName(upgrade.upgrade->interfaceName(0)).c_str());		
			}
			break;
		}
		case 4:
		{
			AttributeBase::ProducedParametersList& producedList = attr->producedParameters;
			AttributeBase::ProducedParametersList::iterator it;
			FOR_EACH(producedList, it){
				XBuffer buf;
				buf <= it->first;
				ProducedParameters& params = it->second;
				ar(params.cost, buf);
			}

		}
		}
	}
}/*}}}*/

void obtainParameters(ParameterArchive& ar, SourceBase& source)/*{{{*/
{
	if(&source){
		if(SourceDamage* sourceDamage = dynamic_cast<SourceDamage*>(&source)){
			ar(sourceDamage->damage(), "Damage");
			ar(sourceDamage->abnormalState().damage(), "Abnormal State Damage");
		}
		else if(SourceZone* sourceZone = dynamic_cast<SourceZone*>(&source)){
			ar(sourceZone->damage(), "Damage");
			ar(sourceZone->abnormalState().damage(), "Abnormal State Damage");
		}
		else if(SourceImpulse* sourceImpulse = dynamic_cast<SourceImpulse*>(&source)){
			ar(sourceImpulse->abnormalState().damage(), "Abnormal State Damage");
		}

		SourceBase::ChildSources::const_iterator it;
		FOR_EACH(source.childSources(), it){
			const SourceBase::ChildSource& childSource = *it;
			SourceAttribute& source = const_cast<SourceAttribute&>(childSource.source_);
			ar(*source.source(), source.attr().c_str());
		}
	}
}/*}}}*/

void obtainParameters(ParameterArchive& ar, SourceWeaponAttributes& sources)/*{{{*/
{
	xassert(&sources);
	SourceWeaponAttributes::iterator it;
	FOR_EACH(sources, it){
		SourceWeaponAttribute& source = *it;
		ar(*source.source(), source.attr().c_str());
	}
}/*}}}*/

void obtainParameters(ParameterArchive& ar, AttributeProjectile& projectile)
{
	xassert(&projectile);
	ar(projectile.damage(), "Projectile Damage");
	ar(projectile.explosionState().damage(), "Projectile Abnormal State Damage");
}

void obtainParameters(ParameterArchive& ar, WeaponPrm& weapon)/*{{{*/
{
	WeaponPrm* attr = &weapon;
	if(attr){
		ar(const_cast<ParameterCustom&>(attr->parameters()), "Initial Parameters");
		ar(const_cast<ParameterCustom&>(attr->fireCost()), "Fire Cost");

		switch(attr->weaponClass()){
		case WeaponPrm::WEAPON_AREA_EFFECT:
			{
				WeaponAreaEffectPrm* prm = safe_cast<WeaponAreaEffectPrm*>(attr);
				obtainParameters(ar, const_cast<SourceWeaponAttributes&>(prm->sources()));
			}
			break;
		case WeaponPrm::WEAPON_WAITING_SOURCE:
			{
				WeaponWaitingSourcePrm* prm = safe_cast<WeaponWaitingSourcePrm*>(attr);
				ar(const_cast<SourceBase&>(*prm->source().source()), prm->source().attr().c_str());
			}
			break;
		case WeaponPrm::WEAPON_BEAM:
			{
				ar(const_cast<WeaponDamage&>(attr->damage()), "Damage");
				ar(const_cast<WeaponDamage&>(attr->abnormalState().damage()), "Abnormal State Damage");
				WeaponBeamPrm* prm = safe_cast<WeaponBeamPrm*>(attr);
				obtainParameters(ar, const_cast<SourceWeaponAttributes&>(prm->sourceAttr()));
			}
			break;
		case WeaponPrm::WEAPON_PROJECTILE:
			{
				WeaponProjectilePrm* prm = safe_cast<WeaponProjectilePrm*>(attr);
				ar(const_cast<AttributeProjectile&>(*prm->missileID()), prm->missileID()->c_str());
			}
			break;
		case WeaponPrm::WEAPON_GRIP:
			ar(const_cast<WeaponDamage&>(attr->damage()), "Damage");
			ar(const_cast<WeaponDamage&>(attr->abnormalState().damage()), "Abnormal State Damage");
			break;
		}
	}
}/*}}}*/

const int READ_ONLY_COLOR = 7;

void writeNameRow(ExcelExporter& excel, const Vect2i& pos, int length, const char* name, bool readOnly, const Vect2i& linkPos)
{
	excel.setCellText(pos, a2w(name).c_str());
	if(readOnly){
		excel.setBackColor(Recti(pos, Vect2i(2, 1)), READ_ONLY_COLOR);
		if(linkPos != Vect2i::ZERO){
			std::string addr = excel.cellName(linkPos);
			excel.addHyperlink(pos + Vect2i(1, 0), "", addr.c_str(), L"Original", L"Navigate to original location");
		}
	}
}

void writeParametersRow(ExcelExporter& excel, const Vect2i& pos, ParameterCustom& params, bool readOnly, Exporter::UsedTypes& usedTypes, float multiplier = 1.0f)
{
	ParameterTypeTable::Strings& strings = const_cast<ParameterTypeTable::Strings&>(ParameterTypeTable::instance().strings());

	const ParameterCustom::Vector& vec = params.customVector();
	for(int i = 0; i < vec.size(); ++i){
		const ParameterValue& value = *vec[i];
		WBuffer buf;
		buf <= value.value() * multiplier;
		int typeIndex = -1;
		const ParameterType* type = &*value.type();
		for(int j = 0; j < strings.size(); ++j){
			if(strcmp(type->c_str(),  strings[j].c_str()) == 0){
				typeIndex = j;
				break;
			}
		}
		xassert(typeIndex >= 0);
		if(typeIndex >= 0){
			usedTypes[typeIndex] = true;
			Vect2i position(pos.x + typeIndex, pos.y);
			excel.setCellText(position, buf.c_str());
		}
	}
	if(readOnly)
		excel.setBackColor(Recti(pos, Vect2i(strings.size(), 1)), READ_ONLY_COLOR);
}

void writeParametersHeader(ExcelExporter& excel, const Vect2i& pos)/*{{{*/
{
	ParameterTypeTable::Strings& strings = const_cast<ParameterTypeTable::Strings&>(ParameterTypeTable::instance().strings());
	for(int index = 0; index < strings.size(); ++index){
		std::string name = strings[index].c_str();
		Vect2i position(pos.x + index, pos.y);
		excel.setCellText(position, a2w(name).c_str());
		excel.setCellTextOrientation(position, 90);
	}
}/*}}}*/

void Exporter::readParametersRow(ExcelImporter& excel, Vect2i& pos, ParameterTree::Node* node)/*{{{*/
{
	if(node->values()){
		int len = node->values()->customVector().size();
		for(int i = 0; i < len; ++i){
			const ParameterValueReference& ref = node->values()->customVector()[i];
			const ParameterType* type = &*ref->type();

			Types::iterator it = std::find(types_.begin(), types_.end(), type);
			if(it != types_.end()){
				int index = std::distance(types_.begin(), it);

				float v = excel.getCellFloat(Vect2i(pos.x + index, pos.y));
				node->setValue(type, v);
			}
		}
		node->setReadOnly(true);
	}
}/*}}}*/

Node* findNodeByPath(Node* root, const char* path)
{
	const char* end = path + strlen(path);
	const char* p = std::find(path, end, '/');
	const char* next = p;
	if(p != end){
		--p;
		next += 2;
	}
	std::string current(path, p);
	for(Node::Children::iterator it = root->children().begin(); it != root->children().end(); ++it){
		Node* node = *it;
		const char* name = node->name();
		if(strcmp(name, current.c_str()) == 0 && !node->readOnly()){
			if(next == end)
				return node;
			else{
				if(Node* result = findNodeByPath(node, next))
					return result;
			}
		}
	}
	return 0;
}

void Exporter::importExcelNode(ExcelImporter& excel, Vect2i& pos, ParameterTree::Node* root, int maxLevel, const char* prefix)
{
	while(true){
		wstring text = excel.getCellText(pos);
		if(text.empty())
			break;

		Node* node = findNodeByPath(root, w2a(text).c_str());
		if(node/* && !node->readOnly()*/)
			readParametersRow(excel, Vect2i(pos.x + maxLevel, pos.y), node);

		++pos.y;
	}
}

void Exporter::exportExcelNode(ExcelExporter& excel, Vect2i& pos, Node* root, int maxLevel, const char* prefix)/*{{{*/
{
	xassert(prefix && root);
	Node::Children::iterator it;
	std::string text = prefix;
	if(prefix[0] != '\0')
		text += " / ";
	text += root->name();

	if(!root->empty()){
		Vect2i linkPos = Vect2i::ZERO;
		ExportedObjects::iterator it = exportedObjects_.find(root->objectAddress());
		bool alreadyExported = (it != exportedObjects_.end());
		if(alreadyExported)
			linkPos = it->second;
		writeNameRow(excel, pos, maxLevel, text.c_str(), root->readOnly(), linkPos);
		Vect2i paramsRowStart(pos.x + maxLevel, pos.y);
		writeParametersRow(excel, paramsRowStart, *root->values(), root->readOnly(), usedTypes_, root->multiplier());
		if(!alreadyExported)
			setWorksheetPosition(root->objectAddress(), pos);
		++pos.y;
	}

	FOR_EACH(root->children(), it){
		ParameterTree::Node* node = *it;
		exportExcelNode(excel, pos, node, maxLevel, text.c_str());
	}
}/*}}}*/

void obtainAllParameters(ParameterArchive& ar)/*{{{*/
{
	{
		typedef AttributeLibrary Library;
		Library::Strings& strings = const_cast<Library::Strings&>(Library::instance().strings());
		Library::Strings::iterator it;
		FOR_EACH(strings, it){
			std::string name = unitNameFromInterfaceName(it->get()->interfaceName(0));
			if(it->get()->isBuilding())
				name += "(B)";
			ar(*it, name.c_str());
		}
	}
}/*}}}*/

static const int MAX_LEVEL = 2;
static const Vect2i TREE_POSITION(1, 1);

void Exporter::exportExcel(const char* filename)
{
	ExcelExporter* exporter = ExcelExporter::create(filename);

	xassert(exporter);

	const char* sheetNames[] = { "Initial", "Production", "Weapons", "Upgrades", "Parameter Production" };
	int numSections = sizeof(sheetNames) / sizeof(sheetNames[0]);

	bool firstSection = true;
	for(int section = numSections - 1; section >= 0; --section){
		const char* sheetName = sheetNames[section];
		if(firstSection){
			exporter->beginSheet();
			exporter->setSheetName(a2w(sheetName).c_str());
			firstSection = false;
		}
		else
			exporter->addSheet(a2w(sheetName).c_str());

		currentSection_ = section;

		ParameterArchive oa(this);
		clearExported();
		obtainAllParameters(oa);
		Vect2i pos = TREE_POSITION;
		writeParametersHeader(*exporter, Vect2i(pos.x + MAX_LEVEL, 0));
		exportExcelNode(*exporter, pos, oa.rootNode(), MAX_LEVEL);
		removeUnusedTypeColumns(*exporter, pos.x + MAX_LEVEL);
		exporter->setColumnWidthAuto(1);
		exporter->setColumnWidthAuto(2);
	}

	exporter->free();
}

void Exporter::readParametersHeader(ExcelImporter& excel, const Vect2i& pos)
{
	ParameterTypeTable::Strings::iterator it;
	ParameterTypeTable::Strings& strings = const_cast<ParameterTypeTable::Strings&>(ParameterTypeTable::instance().strings());

	types_.clear();
	Vect2i current = pos;
	while(true){
		string name = w2a(excel.getCellText(current));
		if(name.empty())
			break;
		
		ParameterType* type = 0;		
		FOR_EACH(strings, it){
			if(strcmp(it->c_str(), name.c_str()) == 0){
				type = const_cast<ParameterType*>(&*it);
				break;
			}
		}
		types_.push_back(type);

		current.x += 1;
	}
}

Exporter::Exporter()
: currentSection_(2)
{

}

void Exporter::importExcel(const char* filename)
{
	ExcelImporter* excel = ExcelImporter::create(filename);
	xassert(excel);

	excel->openSheet();

	for(int i = 0; i < 5; ++i){
		clearExported();
		excel->setCurrentSheet(i);
		currentSection_ = i;
		ParameterArchive oa(this);
		obtainAllParameters(oa);
		oa.rootNode()->makeSingleUser(true);
		ParameterValueTable::instance().buildComboList();
		
		Vect2i pos(1, 1);
		readParametersHeader(*excel, Vect2i(pos.x + MAX_LEVEL, pos.y - 1));
		importExcelNode(*excel, pos, oa.rootNode(), MAX_LEVEL);
	}

	excel->free();
}


// ---------------------------------------------------------------------------

Node::Node(const char* name, void* objectAddress, ParameterCustom* values, float multiplier)
: objectAddress_(objectAddress)
, values_(values)
, parent_(0)
, readOnly_(false)
, multiplier_(multiplier)
, name_(name)
{
	name_ = transliterate(name_.c_str());
	path_ = name_;
}

void Node::setParent(Node* node)
{
	parent_ = node;
	if(parent_ && strlen(parent_->name())){
		path_ = std::string(parent_->path()) + " / " + name_;
	}
	else
		path_ = name_;
}

bool Node::readOnly() const
{
	const Node* current = this;
	while(current){
		if(current->readOnly_)
			return true;
		current = current->parent();
	}
	return false;
}

bool Node::empty() const
{
	return !values_ || values_->empty();
}

void Node::setReadOnly(bool readOnly)
{
	readOnly_ = readOnly;
}

void Node::makeSingleUser(bool recurse)
{
	if(!readOnly_){
		if(values_)
			makeParameterCopy(*values_, path());
		if(recurse){
			Children::iterator it;
			FOR_EACH(children_, it){
				(*it)->makeSingleUser(recurse);
			}
		}
	}
}

bool Node::setValue(const ParameterType* type, float value)
{
	xassert(values_);
	xassert(!readOnly_);
	if(values_){
		for(int i = 0; i < values_->customVector().size(); ++i){
			const ParameterValueReference& ref = values_->customVector()[i];
			ParameterValue& val = const_cast<ParameterValue&>(*ref);
			const ParameterType* val_type = &*val.type();
			if(val_type == type){
				float newValue = value / multiplier_;
				if(fabs(val.rawValue() - value / multiplier_) > FLT_COMPARE_TOLERANCE){
					XBuffer buf;
					buf < path() < " / " < type->c_str() <  ": " <= val.rawValue() < " => " <= newValue;
					kdMessage("Import", buf);
				}
				val.setRawValue(newValue);
				return true;
			}
		}
	}
	return false;
}

const char* Node::path() const
{
	return path_.c_str();
	/*
	std::string result;
	const Node* node = this;
	while(node->parent()){
		if(result.empty())
			result = node->name();
		else
			result = std::string(node->name()) + " / " + result;
		node = node->parent();
	}
	return result;*/
}


// ---------------------------------------------------------------------------

ParameterArchive::ParameterArchive(Exporter* exporter)
: rootNode_(new Node("", 0, 0))
, exporter_(exporter)
, offset_(0)
{
	currentNode_ = rootNode_;
}


void ParameterArchive::openObject(const char* name, void* objectAddress)
{
	Node* node = new Node(name, objectAddress, 0);
	node->setReadOnly(exporter_->alreadyExported(objectAddress));
	currentNode_->push_back(node);
	currentNode_ = node;
	++offset_;
}

void ParameterArchive::setReadOnly(bool readOnly)
{
	currentNode_->setReadOnly(readOnly);
}

std::string ParameterArchive::path()
{
	return currentNode_->path();
}

std::string ParameterArchive::subPath(const char* name)
{
	std::string parentPath = path();
	if(!parentPath.empty())
		parentPath += " / ";
	parentPath += name;
	return parentPath;
}

void ParameterArchive::closeObject()
{
	xassert(currentNode_->parent());
	currentNode_ = currentNode_->parent();
	--offset_;
}


void ParameterArchive::operator()(const ParameterCustom& set, const char* name, float multiplier)
{
	ParameterCustom& custom = const_cast<ParameterCustom&>(set);
	Node* node = new Node(name, reinterpret_cast<void*>(&custom), &custom, multiplier);
	currentNode_->push_back(node);
}

};
