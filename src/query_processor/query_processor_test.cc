/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// QueryProcessor unit test suite

#include <gtest/gtest.h>
#include <glog/logging.h>
#include "query_processor/query_processor.h"
#include "query_processor/catalog_manager.h"
#include "common/black_box_factory.h"

namespace calvin {

class QueryProcessorTest : public testing::Test {
 protected:
  QueryProcessorTest() {}

  virtual ~QueryProcessorTest() {}
};

/// @TODO (Anton and Michael): Implement separate test units for each function
///                            or concept in the QueryProcessor you are testing
TEST_F(QueryProcessorTest, ProcessesQuery) {
  Storage* storage = Storage::GetStorageInstance();
  CatalogManager* catalog = CatalogManager::GetCatalogManagerInstance();

  // Until full CRUD support exists in QueryProcessor, add data manually

  // Fake Atom table inserts
  catalog->at().AddAtom("index",     1, AtomTable::NONE);
  catalog->at().AddAtom("warehouse", 2, AtomTable::UINT64);

  // Add table
  const char* tbl1 = "warehouse";
  const char* sch1 = "id:int:4;city:string:10;zip:string:5;";
  catalog->AddTable(tbl1, sch1);

  // Insert into warehouse
  Key key0 = Key("warehouse(0)");
  Key key1 = Key("warehouse(2)");
  Key key2 = Key("warehouse(3)");
  Key key3 = Key("warehouse(4)");
  Key key4 = Key("warehouse(6)");
  Key key5 = Key("warehouse(8)");

  // Get size of warehouse table
  size_t size1 = catalog->TupleSize(tbl1);

  size_t size_city = 10;
  size_t size_zip = 5;

  Tuple t1(size1);
  t1.add_value(1);
  t1.add_value("Richmond", size_city);
  t1.add_value("23059", size_zip);

  Tuple t2(size1);
  t2.add_value(20);
  t2.add_value("New Haven", size_city);
  t2.add_value("06520", size_zip);

  Tuple t3(size1);
  t3.add_value(0);
  t3.add_value("New York", size_city);
  t3.add_value("10001", size_zip);

  Tuple t4(size1);
  t4.add_value(5);
  t4.add_value("Hartford", size_city);
  t4.add_value("06101", size_zip);

  Tuple t5(size1);
  t5.add_value(3);
  t5.add_value("Los Angeles", size_city);
  t5.add_value("", size_zip);

  storage->Put(Key(catalog->at().HRToCoded(key0)),
               NULL, 0);

  char* value1 = new char[size1];
  char* value2 = new char[size1];
  char* value3 = new char[size1];
  char* value4 = new char[size1];
  char* value5 = new char[size1];

  memcpy(value1, t1.value(), size1);
  storage->Put(Key(catalog->at().HRToCoded(key1)),
               new Value(value1, size1), 0);
  memcpy(value2, t2.value(), size1);
  storage->Put(Key(catalog->at().HRToCoded(key2)),
               new Value(value2, size1), 0);
  memcpy(value3, t3.value(), size1);
  storage->Put(Key(catalog->at().HRToCoded(key3)),
               new Value(value3, size1), 0);
  memcpy(value4, t4.value(), size1);
  storage->Put(Key(catalog->at().HRToCoded(key4)),
               new Value(value4, size1), 0);
  memcpy(value5, t5.value(), size1);
  storage->Put(Key(catalog->at().HRToCoded(key5)),
               new Value(value5, size1), 0);

  // Create transaction to test QueryProcessor
  TxnProto* txn = new TxnProto();
  txn->set_txn_id(1);
  txn->set_sql_code("SELECT * "
                    "FROM warehouse "
                    "WHERE !(-3 < -id);");
  // Parsing and execution shouldn't fail
  EXPECT_TRUE(QueryProcessor::ProcessQuery(txn));
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

