/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// A concrete implementation of TPC-C

#ifndef _DB_STORED_PROCEDURES_TPCC_TPCC_H_
#define _DB_STORED_PROCEDURES_TPCC_TPCC_H_

#include <gtest/gtest.h>
#include <string>
#include <set>

#include "stored_procedures/application.h"
#include "common/configuration.h"
#include "common/utils.h"
#include "proto/tpcc_structs.h"
#include "proto/tpcc_args.pb.h"
#include "proto/txn.pb.h"
#include "common/atomic.h"

namespace calvin {

using std::string;

/// @class TPCC
/// @brief The standard TPC-C application running on Calvin
///
/// We provide our own implementation of the TPC-C standard so that when we're
/// publishing we have something to compare our database on.
class TPCC : public Application {
 public:
  /// A la Singleton DP, we allow a manner in which the user can initialize
  /// the application as a TPCC application
  static void InitializeApplication();

  virtual const char* Name() const;
  virtual TxnProto* NewTxn(int64 txn_id, int txn_type, string args) const;
  virtual TxnStatus Execute(TxnProto* txn,
                            TransactionalStorageManager* storage) const;
  virtual void InitializeStorage() const;
  virtual int KeyToInt(const Key& key) const;
  virtual int DBSize() const;

  /// @enum   TxnType
  /// @brief  Types of stored_procedures that TPC-C application can execute
  ///
  /// This enum describes the set of transaction types available specifically
  /// to the TPC-C application.
  enum TxnType {
    NEW_ORDER = 0,
    PAYMENT,
    ORDER_STATUS,
    DELIVERY,
    STOCK_LEVEL,
  };

  /// @todo Get rid of these hacks!
  /// This is a hack that needs to be replaced... If you're reading this to
  /// know what it does, rewrite it so that Items aren't special cases.
  Value* GetItem(Key key) const;
  void SetItem(Key key, Value* value) const;

 private:
  /// The constructor for TPCC MUST NOT do anything, an application is stateless
  TPCC() : Application() {}

  /// The destructor for TPCC MUST NOT do anything, an application is stateless
  virtual ~TPCC() {}

  /// Create a random warehouse object to put in the database
  ///
  /// @param    id      Key of the warehouse to be created
  ///
  /// @returns  Warehouse (ProtoBuf) object pointer to add to database
  Warehouse* CreateWarehouse(Key id) const;
  FRIEND_TEST(TPCCTest, WarehouseTest);

  /// Create a random district object to put in the database
  ///
  /// @param    id            Key of the district to be created
  /// @param    warehouse_id  Key of the district's parent warehouse
  ///
  /// @returns  District (ProtoBuf) object pointer to add to database
  District* CreateDistrict(Key id, Key warehouse_id) const;
  FRIEND_TEST(TPCCTest, DistrictTest);

  /// Create a random customer object to put in the database
  ///
  /// @param    id            Key of the customer to be created
  /// @param    district_id   Key of the customer's parent district
  /// @param    warehouse_id  Key of the customer's parent warehouse
  ///
  /// @returns  Customer (ProtoBuf) object pointer to add to database
  Customer* CreateCustomer(Key id, Key district_id, Key warehouse_id) const;
  FRIEND_TEST(TPCCTest, CustomerTest);

  /// Create a random item object to put in the database
  ///
  /// @param    id      Key of the item to be created
  ///
  /// @returns  Item (ProtoBuf) object pointer to add to database
  Item* CreateItem(Key id) const;
  FRIEND_TEST(TPCCTest, ItemTest);

  /// Create a random stock object to put in the database
  ///
  /// @param    id      Key of the stock to be created
  ///
  /// @returns  Stock (ProtoBuf) object pointer to add to database
  Stock* CreateStock(Key id, Key warehouse_id) const;
  FRIEND_TEST(TPCCTest, StockTest);

  /// Performs standard new-order transaction as specified in TPC-C
  ///
  /// @param    txn     The transaction object to be executed
  /// @param    storage The TSM interface to use throughout the txn
  ///
  /// @returns Status code of app execution as specified in @ref TxnStatus
  TxnStatus NewOrderTxn(TxnProto* txn,
                        TransactionalStorageManager* storage) const;

  /// Performs standard payment transaction as specified in TPC-C
  ///
  /// @param    txn     The transaction object to be executed
  /// @param    storage The TSM interface to use throughout the txn
  ///
  /// @returns Status code of app execution as specified in @ref TxnStatus
  TxnStatus PaymentTxn(TxnProto* txn,
                       TransactionalStorageManager* storage) const;
};

}  // namespace calvin

#endif  // _DB_STORED_PROCEDURES_TPCC_TPCC_H_
