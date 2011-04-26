#include <coopy/FoldTool.h>
#include <coopy/FoldedSheet.h>
#include <coopy/CsvSheet.h>
#include <coopy/SchemaSniffer.h>
#include <coopy/ShortTextBook.h>

#include <vector>
#include <map>

using namespace std;
using namespace coopy::fold;
using namespace coopy::store;

typedef vector<int> ints;

class SheetAccess {
public:
  PolySheet sheet;
  SchemaSniffer schema;

  SheetAccess() {
  }

  void setSheet(const PolySheet& sheet) {
    this->sheet = sheet;
    schema.setSheet(sheet);
  }

  bool isValid() const {
    return sheet.isValid();
  }

  std::string getName() {
    SheetSchema *ss = schema.suggestSchema();
    if (!ss) {
      printf("No schema!\n");
      return "";
    }
    return ss->getSheetName();
  }

  std::string getName(int id) {
    SheetSchema *ss = schema.suggestSchema();
    if (!ss) {
      printf("No schema!\n");
      return "";
    }
    ColumnInfo info = ss->getColumnInfo(id);
    return info.getName();
  }

  int getId(const char *name) {
    SheetSchema *ss = schema.suggestSchema();
    if (!ss) {
      printf("No schema!\n");
      return -1;
    }
    int result = ss->getColumnIndexByName(name);
    if (result<0) {
      printf("Could not find column %s\n", name);
    }
    return result;
  }
};

class FoldLayout {
public:
  SheetAccess *src;
  SheetAccess *dest;
  string srcIdName;
  string destIdName;
  int offset;

  PolySheet srcTable;
  PolySheet destTable;
  int srcId;
  int destId;
  typedef multimap<int,int> mapper;
  mapper indexedSrc;
  mapper indexedDest;

  FoldLayout() {
    src = dest = NULL;
    srcId = -1;
    destId = -1;
    offset = 0;
  }

  bool updateBase() {
    if (src!=NULL) {
      srcTable = src->sheet;
    }
    if (dest!=NULL) {
      destTable = dest->sheet;
    }
  }

  bool updateIds() {
    if (srcId!=-1 && destId!=-1) return true;
    COOPY_ASSERT(src);
    COOPY_ASSERT(dest);
    srcId = src->getId(srcIdName.c_str());
    destId = dest->getId(destIdName.c_str());
    printf("IDs %s %s %d %d\n", srcIdName.c_str(), destIdName.c_str(), 
	   srcId, destId);
    return (srcId!=-1 && destId!=-1);
  }

  bool updateMap() {
    if (indexedSrc.size()!=0) return true;
    if (indexedDest.size()!=0) return true;
    if (!updateIds()) {
      printf("Failed to get IDs\n");
      return false;
    }
    COOPY_ASSERT(src);
    COOPY_ASSERT(dest);
    for (int y=0; y<srcTable.height(); y++) {
      int id = srcTable.cellSummary(srcId,y).asInt();
      indexedSrc.insert(std::make_pair(y,id));
      //printf("src %d -> %d\n", y, id);
    }
    for (int y=0; y<destTable.height(); y++) {
      int id = destTable.cellSummary(destId,y).asInt();
      indexedDest.insert(std::make_pair(id,y));
      //printf("dest %d -> %d\n", id, y);
    }
    return true;
  }

  int transformForward(int y) {
    mapper::iterator it1 = indexedSrc.find(y);
    if (it1==indexedSrc.end()) {
      printf("src %d went nowhere\n", y);
      return -1;
    }
    mapper::iterator it2 = indexedDest.find(it1->second);
    if (it2==indexedDest.end()) {
      printf("dest %d from src %d went nowhere\n", it1->second, y);
      return -1;
    }
    return it2->second;
  }

  /*
  ints transform(ints y) {
    ints result;
    for (int i=0; i<(int)y.size(); i++) {
      for (mapper::iterator it1 = indexedSrc.lower_bound(y);
	   it1 != indexedSrc.upper_bound(y); it1++) {	
	for (mapper::iterator it2 = indexedDest.lower_bound(it1->second);
	     it2 != indexedDest.upper_bound(it1->second); it2++) {	
	  results.push_back(it2->second);
	}
      }
    }
    return results;
  }
  */
};
  
  class FoldCache {
public:
  SheetAccess dud;
  PolyBook book;

  void setBook(const PolyBook& book) {
    this->book = book;
  }

  map<string,SheetAccess> sheets;

  SheetAccess& getSheet(const char *name) {
    map<string,SheetAccess>::iterator it = sheets.find(name);
    if (it!=sheets.end()) {
      return it->second;
    }
    PolySheet sheet = book.readSheet(name);
    if (!sheet.isValid()) {
      fprintf(stderr,"Could not find table %s\n", name);
      return dud;
    }
    sheets[name] = SheetAccess();
    sheets[name].setSheet(sheet);
    return sheets[name];
  }
};

FoldTool::FoldTool() {
}

class FoldEdge {
public:
  string table;
  string key;

  string toString() const {
    return table + ":" + key;
  }
};

class FoldEdgeCmp {
public:
  bool operator() (const FoldEdge& e1, const FoldEdge& e2) {
    if (e1.table<e2.table) return true;
    return e1.key<e2.key;
  }
};

class Folds {
public:
  typedef map<FoldEdge,FoldEdge,FoldEdgeCmp> EdgeMap;

  EdgeMap fwd;
  EdgeMap rev;

  void add(const char *fromTable, const char *toTable, 
	   const char *fromField, const char *toField) {
    FoldEdge e1, e2;
    e1.table = fromTable;
    e1.key = fromField;
    e2.table = toTable;
    e2.key = toField;
    fwd[e1] = e2;
    rev[e2] = e1;
    printf("Added %s -> %s\n", e1.toString().c_str(), e2.toString().c_str());
  }
};

class FoldSelector {
public:
  string tableName;
  string keyName;
  SheetCell val;
};

static bool fold_expander(Folds& folds, FoldCache& cache, FoldedSheet& sheet,
			  FoldSelector& sel) {
  SheetAccess& base = cache.getSheet(sel.tableName.c_str());
  if (!base.isValid()) {
    return false;
  }
  PolySheet& src = base.sheet;
  SchemaSniffer& schema = base.schema;
  int selId = -1;
  if (sel.keyName!="") {
    selId = base.getId(sel.keyName.c_str());
  }
  printf("Working on sheet %dx%d\n", src.width(), src.height());
  FoldedCell fcell;
  vector<int> selected;
  for (int y=0; y<src.height(); y++) {
    if (selId!=-1) {
      if (src.cellSummary(selId,y)==sel.val) {
	selected.push_back(y);
      }
    } else {
      selected.push_back(y);
    }
  }
  vector<int> expanded;
  for (int x=0; x<src.width(); x++) {
    FoldEdge e;
    e.table = sel.tableName;
    e.key = base.getName(x);
    printf("Checking %s\n", e.toString().c_str());
    Folds::EdgeMap::iterator it = folds.fwd.find(e);
    if (it!=folds.fwd.end()) {
      printf("Should expand out %s (to %s)\n", e.key.c_str(),
	     it->second.toString().c_str());
    }
    Folds::EdgeMap::iterator it2 = folds.rev.find(e);
    if (it2!=folds.rev.end()) {
      printf("Should expand in %s (from %s)\n", e.key.c_str(),
	     it2->second.toString().c_str());
    }
  }

  printf("Producting sheet %dx%d\n", src.width(), selected.size());

  sheet.resize(src.width(),selected.size(),fcell);
  for (vector<int>::iterator yit=selected.begin(); yit!=selected.end(); yit++) {
    int y = *yit;
    int y0 = yit-selected.begin();
    for (int x=0; x<sheet.width(); x++) {
      FoldedCell& cell = sheet.cell(x,y0);
      cell.datum = src.cellSummary(x,y);
    }
  }
  return true;
}

bool FoldTool::fold(PolyBook& src, PolyBook& dest, FoldOptions& options) {
  printf("Starting fold/unfold\n");

  if (options.tableName=="") {
    fprintf(stderr,"Please supply a root table name\n");
    return false;
  }

  FoldCache cache;
  cache.setBook(src);

  SheetAccess& base = cache.getSheet(options.tableName.c_str());
  if (!base.isValid()) {
    return false;
  }

  vector<FoldLayout> layout;
  FoldLayout baseLayout;
  baseLayout.src = &base;
  baseLayout.updateBase();
  layout.push_back(baseLayout);

  PolySheet recipe = options.recipe.readSheetByIndex(0);
  recipe.hideHeaders();
  Folds folds;
  for (int i=0; i<recipe.height(); i++) {
    string fromTable = recipe.cellString(0,i);
    string fromField = recipe.cellString(1,i);
    string toTable = recipe.cellString(2,i);
    string toField = recipe.cellString(3,i);
    printf("Recipe line %d: %s %s %s %s\n", i,
	   fromTable.c_str(), toTable.c_str(), fromField.c_str(), toField.c_str());
    folds.add(fromTable.c_str(), toTable.c_str(), fromField.c_str(), toField.c_str());
    if (fromTable==options.tableName) {
      /*
      SheetAccess& alt = cache.getSheet(toTable.c_str());
      if (!alt.isValid()) {
	return false;
      }
      layout.push_back(FoldLayout());
      FoldLayout& l = layout.back();
      l.src = &base;
      l.dest = &alt;
      l.srcIdName = fromField;
      l.destIdName = toField;
      l.updateBase();
      l.updateMap();
      */
    }
  }

  FoldedSheet fsheet;
  FoldSelector sel;
  sel.tableName = options.tableName;
  fold_expander(folds, cache, fsheet, sel);

  printf("Generated sheet %dx%d\n", fsheet.width(), fsheet.height());

  printf("Incomplete...\n");
  exit(1);

  return true;
}



/*
bool FoldTool::fold(PolyBook& src, PolyBook& dest, FoldOptions& options) {
  printf("Starting fold/unfold\n");

  if (options.tableName=="") {
    fprintf(stderr,"Please supply a root table name\n");
    return false;
  }

  FoldCache cache;
  cache.setBook(src);

  SheetAccess& base = cache.getSheet(options.tableName.c_str());
  if (!base.isValid()) {
    return false;
  }

  vector<FoldLayout> layout;
  FoldLayout baseLayout;
  baseLayout.src = &base;
  baseLayout.updateBase();
  layout.push_back(baseLayout);

  PolySheet recipe = options.recipe.readSheetByIndex(0);
  recipe.hideHeaders();
  for (int i=0; i<recipe.height(); i++) {
    string fromTable = recipe.cellString(0,i);
    string fromField = recipe.cellString(1,i);
    string toTable = recipe.cellString(2,i);
    string toField = recipe.cellString(3,i);
    printf("Recipe line %d: %s %s %s %s\n", i,
	   fromTable.c_str(), toTable.c_str(), fromField.c_str(), toField.c_str());
    if (fromTable==options.tableName) {
      SheetAccess& alt = cache.getSheet(toTable.c_str());
      if (!alt.isValid()) {
	return false;
      }
      layout.push_back(FoldLayout());
      FoldLayout& l = layout.back();
      l.src = &base;
      l.dest = &alt;
      l.srcIdName = fromField;
      l.destIdName = toField;
      l.updateBase();
      l.updateMap();
    }
  }

  ShortTextBook *book = new ShortTextBook();
  if (book==NULL) {
    fprintf(stderr,"Failed to allocate output\n");
    return 1;
  }
  printf("Starting to generate output\n");
  SimpleSheetSchema schema;
  schema.setSheetName("flat");
  for (int i=0; i<(int)layout.size(); i++) {
    FoldLayout& unit = layout[i];
    if (i==0) {
      for (int x=0; x<unit.srcTable.width(); x++) {
	schema.addColumn(unit.src->getName(x).c_str());
      }
    } else {
      for (int x=0; x<unit.destTable.width(); x++) {
	string remoteField = unit.dest->getName(x);
	string localField = unit.srcIdName;
	string remoteTable = unit.dest->getName();
	string name = remoteField + ":" + remoteTable + "[" + localField + "]";
	schema.addColumn(name.c_str());
      }
    }
  }
  book->provideSheet(schema);
  CsvSheet& accum = book->sheet;

  for (int y=0; y<base.sheet.height(); y++) {
    for (int i=0; i<(int)layout.size(); i++) {
      FoldLayout& unit = layout[i];
      if (i==0) {
	for (int x=0; x<unit.srcTable.width(); x++) {
	  accum.addField(unit.srcTable.cellSummary(x,y));
	}
      } else {
	int yy = unit.transformForward(y);
	for (int x=0; x<unit.destTable.width(); x++) {
	  if (yy>=0) {
	    accum.addField(unit.destTable.cellSummary(x,yy));
	  } else {
	    accum.addField(SheetCell());
	  }
	}
      }
    }
    accum.addRecord();
  }

  dest.take(book);

  return true;
}


*/
