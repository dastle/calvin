/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 5 Apr 2012
///
/// @section DESCRIPTION
///
/// CatalogManager unit test suite

#include <gtest/gtest.h>
#include <glog/logging.h>
#include "query_processor/catalog_manager.h"
#include "common/black_box_factory.h"

namespace calvin {

class CatalogManagerTest : public testing::Test {
 protected:
  CatalogManagerTest() {}

  virtual ~CatalogManagerTest() {}
};

TEST_F(CatalogManagerTest, AddTable) {
  CatalogManager* catalog;

  const string table = "customer";
  const string schema1[] = {"name:string:32;id:int:2;",
                            "name:string:32;id:int:2"};
  const string schema2[] = {"name:string:32,index;id:int:2;",
                            "name:string:32,index;id:int:2"};
  const string schema3[] = {"name:string:32;id:int:2,index;",
                            "name:string:32;id:int:2,index"};
  const string schema4[] = {"name:string:32,index;id:int:2,index;",
                            "name:string:32,index;id:int:2,index"};
  const string schemaInvalid[] = {"name:string:32;index;id:int:2",
                                  "name:string:index;id:int:2",
                                  "name:string:id:int:2;",
                                  "name:string:0;id:int:2",
                                  "name:string:2;id:int:-2"};

  catalog = CatalogManager::GetCatalogManagerInstance();
  EXPECT_FALSE(catalog == NULL);

  // Test with and without trailing semicolon
  for (int i = 0; i < 2; i++) {
    EXPECT_TRUE(catalog->AddTable(table, schema1[i]));
    EXPECT_TRUE(catalog->AttributeExists(table, "name"));
    EXPECT_TRUE(catalog->AttributeExists(table, "id"));
    EXPECT_FALSE(catalog->IndexExists(table, "name"));
    EXPECT_FALSE(catalog->IndexExists(table, "id"));
    EXPECT_TRUE(catalog->DeleteTable(table));
  }

  for (int i = 0; i < 2; i++) {
    EXPECT_TRUE(catalog->AddTable(table, schema2[i]));
    EXPECT_TRUE(catalog->AttributeExists(table, "name"));
    EXPECT_TRUE(catalog->AttributeExists(table, "id"));
    EXPECT_TRUE(catalog->IndexExists(table, "name"));
    EXPECT_FALSE(catalog->IndexExists(table, "id"));
    EXPECT_TRUE(catalog->DeleteTable(table));
  }

  for (int i = 0; i < 2; i++) {
    EXPECT_TRUE(catalog->AddTable(table, schema3[i]));
    EXPECT_TRUE(catalog->AttributeExists(table, "name"));
    EXPECT_TRUE(catalog->AttributeExists(table, "id"));
    EXPECT_FALSE(catalog->IndexExists(table, "name"));
    EXPECT_TRUE(catalog->IndexExists(table, "id"));
    EXPECT_TRUE(catalog->DeleteTable(table));
  }

  for (int i = 0; i < 2; i++) {
    EXPECT_TRUE(catalog->AddTable(table, schema4[i]));
    EXPECT_TRUE(catalog->AttributeExists(table, "name"));
    EXPECT_TRUE(catalog->AttributeExists(table, "id"));
    EXPECT_TRUE(catalog->IndexExists(table, "name"));
    EXPECT_TRUE(catalog->IndexExists(table, "id"));
    EXPECT_TRUE(catalog->DeleteTable(table));
  }

  for (int i = 0; i < 5; i++) {
    EXPECT_FALSE(catalog->AddTable("t5", schemaInvalid[i]));
  }
}

TEST_F(CatalogManagerTest, AddTables) {
  CatalogManager* catalog = CatalogManager::GetCatalogManagerInstance();

  EXPECT_TRUE(catalog->AddTable("customer", "id:int:2;name:string:4,index"));
  EXPECT_TRUE(catalog->AddTable("item", "name:string:8,index;price:int:5"));
  EXPECT_TRUE(catalog->AddTable("warehouse", "city:string:40;"
                                "state:string:2,index;zip:string:5;"));

  EXPECT_TRUE(catalog->DeleteTable("item"));

  EXPECT_TRUE(catalog->TableExists("customer"));
  EXPECT_FALSE(catalog->TableExists("item"));
  EXPECT_TRUE(catalog->TableExists("warehouse"));

  EXPECT_TRUE(catalog->AttributeExists("customer", "id"));
  EXPECT_FALSE(catalog->IndexExists("customer", "id"));
  EXPECT_EQ((size_t)2, catalog->AttributeSize("customer", "id"));
  EXPECT_EQ((size_t)2, catalog->AttributeOffset("customer", "name"));
  EXPECT_EQ((size_t)7, catalog->TupleSize("customer"));
  EXPECT_EQ(CatalogManager::STRING, catalog->AttributeType("customer", "name"));
  EXPECT_EQ(CatalogManager::INT, catalog->AttributeType("customer", "id"));
  EXPECT_FALSE(catalog->AttributeExists("customer", "city"));

  EXPECT_TRUE(catalog->AttributeExists("warehouse", "state"));
  EXPECT_TRUE(catalog->IndexExists("warehouse", "state"));
  EXPECT_EQ((size_t)5, catalog->AttributeSize("warehouse", "zip"));
  EXPECT_EQ((size_t)44, catalog->AttributeOffset("warehouse", "zip"));
  EXPECT_FALSE(catalog->AttributeExists("warehouse", "name"));

  EXPECT_TRUE(catalog->DeleteTable("customer"));
  EXPECT_TRUE(catalog->DeleteTable("warehouse"));
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
