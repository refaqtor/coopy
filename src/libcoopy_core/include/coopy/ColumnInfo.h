#ifndef COOPY_COLUMNINFO
#define COOPY_COLUMNINFO

#include <string>
#include <coopy/Property.h>

namespace coopy {
  namespace store {
    class ColumnType;
    class ColumnInfo;
  }
}

/**
 * General column type (text, integer, etc).
 */
class coopy::store::ColumnType {
public:
  enum {
    COLUMN_FAMILY_NONE,
    COLUMN_FAMILY_INTEGER,
    COLUMN_FAMILY_REAL,
    COLUMN_FAMILY_TEXT,
    COLUMN_FAMILY_DATETIME,
    COLUMN_FAMILY_BOOLEAN,
    COLUMN_FAMILY_CURRENCY,
    COLUMN_FAMILY_BLOB,

    COLUMN_SOURCE_NONE,
    COLUMN_SOURCE_SQLITE,
    COLUMN_SOURCE_MYSQL,
    COLUMN_SOURCE_EXCEL,
    COLUMN_SOURCE_CSV,
    COLUMN_SOURCE_ACCESS,
  };
    
  std::string src_name;
  std::string src_lang;
  bool allowNull;
  int family;
  int src;
  int size;
  int size2;
  bool primaryKey;
  bool primaryKeySet;
  bool autoIncrement;
  bool autoIncrementSet;

  std::string foreignTable;
  std::string foreignKey;
  bool foreignKeySet;

  ColumnType() {
    reset();
  }

  ColumnType(const std::string& name) {
    setType(name);
  }

  void reset() {
    allowNull = true;
    family = COLUMN_FAMILY_NONE;
    src = COLUMN_SOURCE_NONE;
    size = 0;
    size2 = 0;
    primaryKey = false;
    primaryKeySet = false;
    src_lang = "";
    src_name = "";
    autoIncrement = false;
    autoIncrementSet = false;
    foreignKeySet = false;
  }

  // do guess-work
  bool setType(const std::string& name, const std::string& lang = "unknown");

  std::string asSqlite(bool addPrimaryKey) const;
};


/**
 * Name and type of column, when available.
 */
class coopy::store::ColumnInfo {
private:
  std::string name;
  bool nameSet;
  ColumnType columnType;
  bool typeSet;
public:
  ColumnInfo() : nameSet(false), typeSet(false) {}
  ColumnInfo(std::string name) : name(name), nameSet(true), typeSet(false) {}
  ColumnInfo(std::string name, const ColumnType& t) : name(name), nameSet(true), typeSet(true), columnType(t) {}
  
  virtual ~ColumnInfo() {}
  
  virtual bool hasName() const { return nameSet; }
  
  virtual std::string getName() const { return name; }

  virtual bool hasPrimaryKey() const { return columnType.primaryKeySet; }
  
  virtual bool isPrimaryKey() const { return columnType.primaryKey; }
  
  virtual bool hasForeignKey() const { return columnType.foreignKeySet; }

  virtual bool isForeignKey() const { return columnType.foreignKey!=""; }
    
  const ColumnType& getColumnType() const { return columnType; }

  virtual bool hasType() const { return typeSet; }

  void setName(std::string name) {
    this->name = name;
    nameSet = true;
  }

  void setPk(bool pk) {
    columnType.primaryKeySet = true;
    columnType.primaryKey = pk;
  }

  void setAutoIncrement(bool ai) {
    columnType.autoIncrementSet = true;
    columnType.autoIncrement = ai;
  }

  void setReference(std::string t, std::string c) {
    columnType.foreignKeySet = true;
    columnType.foreignTable = t;
    columnType.foreignKey = c;    
  }

  void setType(const std::string name, const std::string lang = "unknown") {
    columnType.setType(name,lang);
    typeSet = true;
  }
};

#endif
