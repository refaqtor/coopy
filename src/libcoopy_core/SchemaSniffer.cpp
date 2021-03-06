#include <coopy/SchemaSniffer.h>
#include <coopy/NameSniffer.h>
#include <coopy/IndexSniffer.h>
#include <coopy/Dbg.h>
#include <coopy/CompareFlags.h>
#include <coopy/Stringer.h>

#include <stdlib.h>

using namespace coopy::store;
using namespace coopy::cmp;

using namespace std;

void SchemaSniffer::sniff(bool force) {
  dbg_printf("Sniff schema\n");
  fallback = SimpleSheetSchema();
  COOPY_ASSERT(sheet!=NULL);
  if (!force) {
    if (sheet->getSchema()!=NULL) {
      if (sheet->getSchema()->getColumnCount()!=0) {
	schema = sheet->getSchema();
	dbg_printf("  - Schema built in\n");
	return;
      }
    }
  }
  CompareFlags flags;
  if (!ns) {
    ns = new NameSniffer(*sheet,flags);
    COOPY_ASSERT(ns);
  }
  NameSniffer& nameSniffer = *ns;
  vector<string> names = nameSniffer.suggestNames();
  vector<ColumnType> ct = nameSniffer.suggestTypes();
  IndexSniffer indexSniffer(*sheet,flags,nameSniffer);
  vector<int> indexes = indexSniffer.suggestIndexes();
  bool haveKey = false;
  bool shouldSetKey = true;
  while (ct.size()<indexes.size()) {
    ct.push_back(ColumnType());
  }
  for (int i=0; i<(int)indexes.size(); i++) {
    if (indexes[i]>0) {
      haveKey = true;
    }
  }
  for (int i=0; i<(int)ct.size(); i++) {
    if (ct[i].primaryKeySet) {
      shouldSetKey = false;
      break;
    }
  }
  if (haveKey&&shouldSetKey) {
    for (int i=0; i<(int)indexes.size(); i++) {
      ct[i].primaryKeySet = true;
      ct[i].primaryKey = (indexes[i]>0);
    }
  }
  bool force_guess = true;
  for (int i=0; i<(int)names.size(); i++) {
    ColumnType base = ct[i];
    fallback.addColumn(names[i].c_str(),ct[i]);
    force_guess = false;
  }
  int at = (int)names.size();
  string col = Stringer::getSpreadsheetColumnName(at);
  //char col[3] = "A2";
  while ((int)fallback.getColumnCount()<sheet->width()) {
    fallback.addColumn(col.c_str(),ct[at]);
    col = Stringer::nextSpreadsheetColumnName(col);
    at++;
  }
  fallback.setSheetName(name.c_str());
  fallback.setHasSheetName(sheet->hasSheetName());
  fallback.setHeaderHeight(nameSniffer.getHeaderHeight());
  schema = &fallback;
  fallback.setGuess(nameSniffer.isFake()||force_guess);
}

SheetSchema *SchemaSniffer::suggestSchema() {
  return schema;
}

void SchemaSniffer::resniff(coopy::store::SchemaSniffer& alt) {
  if (ns && alt.ns) {
    if (ns->resniff(*alt.ns)) {
      sniff();
    }
  }
}
