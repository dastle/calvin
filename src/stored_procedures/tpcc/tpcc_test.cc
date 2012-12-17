/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// TPC-C unit test suite

#include "stored_procedures/tpcc/tpcc.h"
#include <gtest/gtest.h>
#include "common/black_box_factory.h"

namespace calvin {

class TPCCTest : public testing::Test {
 protected:
  TPCCTest() {
    BlackBoxFactory::ConfigurationBlackBox();
    TPCC::InitializeApplication();
    tpcc_         =
      reinterpret_cast<TPCC*>(Application::GetApplicationInstance());
    storage_      = BlackBoxFactory::StorageBlackBox();
    multiplexer_  = BlackBoxFactory::ConnectionMultiplexerBlackBox();
    connection_   = BlackBoxFactory::ConnectionBlackBox();
  }

  virtual ~TPCCTest() {
    delete multiplexer_;
    delete connection_;
  }

  TPCC* tpcc_;
  Storage* storage_;
  ConnectionMultiplexer* multiplexer_;
  Connection* connection_;
};

/// @test Test for creation of a warehouse, ensure the attributes are correct
TEST_F(TPCCTest, WarehouseTest) {
  Warehouse* warehouse = tpcc_->CreateWarehouse("w1");

  EXPECT_EQ(strcmp(warehouse->id, "w1"), 0);
  EXPECT_GT(static_cast<int>(strlen(warehouse->name)), 0);
  EXPECT_GT(static_cast<int>(strlen(warehouse->street_1)), 0);
  EXPECT_GT(static_cast<int>(strlen(warehouse->street_2)), 0);
  EXPECT_GT(static_cast<int>(strlen(warehouse->city)), 0);
  EXPECT_GT(static_cast<int>(strlen(warehouse->state)), 0);
  EXPECT_GT(static_cast<int>(strlen(warehouse->zip)), 0);
  EXPECT_EQ(warehouse->tax, 0.05);
  EXPECT_EQ(warehouse->year_to_date, 0.0);

  // Finish
  delete warehouse;
}

/// @test Test for creation of a district, ensure the attributes are correct
TEST_F(TPCCTest, DistrictTest) {
  District* district = tpcc_->CreateDistrict("d1", "w1");

  EXPECT_EQ(strcmp(district->id, "d1"), 0);
  EXPECT_EQ(strcmp(district->warehouse_id, "w1"), 0);
  EXPECT_GT(static_cast<int>(strlen(district->name)), 0);
  EXPECT_GT(static_cast<int>(strlen(district->street_1)), 0);
  EXPECT_GT(static_cast<int>(strlen(district->street_2)), 0);
  EXPECT_GT(static_cast<int>(strlen(district->city)), 0);
  EXPECT_GT(static_cast<int>(strlen(district->state)), 0);
  EXPECT_GT(static_cast<int>(strlen(district->zip)), 0);
  EXPECT_EQ(district->tax, 0.05);
  EXPECT_EQ(district->year_to_date, 0.0);
  EXPECT_EQ(district->next_order_id, 1);

  // Finish
  delete district;
}

/// @test Test for creation of a customer, ensure the attributes are correct
TEST_F(TPCCTest, CustomerTest) {
  // Create a transaction so the customer creation can do secondary insertion
  TxnProto* secondary_keying = new TxnProto();
  secondary_keying->set_txn_id(1);
  Customer* customer = tpcc_->CreateCustomer("c1", "d1", "w1");

  EXPECT_EQ(strcmp(customer->id, "c1"), 0);
  EXPECT_EQ(strcmp(customer->district_id, "d1"), 0);
  EXPECT_EQ(strcmp(customer->warehouse_id, "w1"), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->first)), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->middle)), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->last)), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->street_1)), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->street_2)), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->city)), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->state)), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->zip)), 0);
  EXPECT_GT(static_cast<int>(strlen(customer->data)), 0);
  EXPECT_EQ(strcmp(customer->credit, "GC"), 0);
  EXPECT_EQ(customer->since, 0);
  EXPECT_EQ(customer->credit_limit, 0.01);
  EXPECT_EQ(customer->discount, 0.5);
  EXPECT_EQ(customer->balance, 0);
  EXPECT_EQ(customer->year_to_date_payment, 0);
  EXPECT_EQ(customer->payment_count, 0);
  EXPECT_EQ(customer->delivery_count, 0);

  // Finish
  delete secondary_keying;
  delete customer;
}

/// @test Test for creation of an item, ensure the attributes are correct
TEST_F(TPCCTest, ItemTest) {
  Item* item = tpcc_->CreateItem("i1");

  EXPECT_EQ(strcmp(item->id, "i1"), 0);
  EXPECT_GT(static_cast<int>(strlen(item->name)), 0);
  EXPECT_TRUE(item->price >= 0 && item->price < 100);
  EXPECT_GT(static_cast<int>(strlen(item->data)), 0);

  // Finish
  delete item;
}

/// @test Test for creation of a stock, ensure the attributes are correct
TEST_F(TPCCTest, StockTest) {
  Stock* stock = tpcc_->CreateStock("i1", "w1");

  EXPECT_EQ(strcmp(stock->id, "w1si1"), 0);
  EXPECT_EQ(strcmp(stock->warehouse_id, "w1"), 0);
  EXPECT_EQ(strcmp(stock->item_id, "i1"), 0);
  EXPECT_TRUE(stock->quantity >= 100 && stock->quantity < 200);
  EXPECT_GT(static_cast<int>(strlen(stock->data)), 0);
  EXPECT_EQ(stock->year_to_date, 0);
  EXPECT_EQ(stock->order_count, 0);
  EXPECT_EQ(stock->remote_count, 0);

  // Finish
  delete stock;
}

/// @test This initializes a new transaction and ensures it has the desired
///       properties
TEST_F(TPCCTest, NewTxnTest) {
  TPCCArgs* tpcc_args = new TPCCArgs();
  tpcc_args->set_system_time(GetTime());

  // Txn args w/number of warehouses
  TPCCArgs* txn_args = new TPCCArgs();
  txn_args->set_multipartition(false);
  txn_args->set_system_time(GetTime());
  string txn_args_value;
  assert(txn_args->SerializeToString(&txn_args_value));

  // New Order Transaction Generation
  TxnProto* txn = tpcc_->NewTxn(1, TPCC::NEW_ORDER, txn_args_value);
  EXPECT_EQ(txn->txn_id(), 1);
  EXPECT_EQ(txn->txn_type(), TPCC::NEW_ORDER);
  EXPECT_EQ(txn->isolation_level(), TxnProto::SERIALIZABLE);
  EXPECT_EQ(txn->status(), TxnProto::NEW);

  EXPECT_TRUE(tpcc_args->ParseFromString(txn->arg()));
  EXPECT_TRUE(tpcc_args->order_line_count() >= 5 &&
              tpcc_args->order_line_count() <= 15);
  EXPECT_TRUE(txn->write_set_size() == tpcc_args->order_line_count() + 2);
  for (int i = 0; i < tpcc_args->order_line_count(); i++)
    EXPECT_TRUE(tpcc_args->quantities(i) <= 10 && tpcc_args->quantities(i) > 0);

  // Payment Transaction Generation
  delete txn;
  txn = tpcc_->NewTxn(2, TPCC::PAYMENT, txn_args_value);
  EXPECT_EQ(txn->txn_id(), 2);
  EXPECT_EQ(txn->txn_type(), TPCC::PAYMENT);
  EXPECT_EQ(txn->isolation_level(), TxnProto::SERIALIZABLE);
  EXPECT_EQ(txn->status(), TxnProto::NEW);

  EXPECT_TRUE(tpcc_args->ParseFromString(txn->arg()));
  EXPECT_TRUE(tpcc_args->amount() >= 1 && tpcc_args->amount() <= 5000);
  EXPECT_EQ(txn->write_set_size(), 1);
  EXPECT_TRUE(txn->read_set_size() == 0 || txn->read_set_size() == 1);

  // Finish
  delete txn;
  delete txn_args;
  delete tpcc_args;
}

/// @test    Initialize the database and ensure that there are the correct
///          objects actually in the database
TEST_F(TPCCTest, InitializeTest) {
  // Run initialization method.
  tpcc_->InitializeStorage();

  // Expect all the warehouses to be there
  for (int i = 0; i < WAREHOUSES_PER_NODE; i++) {
    char warehouse_key[MAX_KEY_LENGTH];
    snprintf(warehouse_key, sizeof(warehouse_key), "w%d", i);

    Warehouse* warehouse =
      reinterpret_cast<Warehouse*>(storage_->Get(warehouse_key, 0)->data());
    EXPECT_EQ(strcmp(warehouse->id, warehouse_key), 0);

    // Expect all the districts to be there
    for (int j = 0; j < DISTRICTS_PER_WAREHOUSE; j++) {
      char district_key[MAX_KEY_LENGTH];
      snprintf(district_key, sizeof(district_key), "w%dd%d",
               i, j);

      District* district =
        reinterpret_cast<District*>(storage_->Get(district_key, 0)->data());
      EXPECT_EQ(strcmp(district->id, district_key), 0);

      // Expect all the customers to be there
      for (int k = 0; k < CUSTOMERS_PER_DISTRICT; k++) {
        char customer_key[MAX_KEY_LENGTH];
        snprintf(customer_key, sizeof(customer_key),
                 "w%dd%dc%d", i, j, k);

        Customer* customer =
          reinterpret_cast<Customer*>(storage_->Get(customer_key, 0)->data());
        EXPECT_EQ(strcmp(customer->id, customer_key), 0);
      }
    }

    // Expect all stock to be there
    for (int j = 0; j < NUMBER_OF_ITEMS; j++) {
      char item_key[MAX_KEY_LENGTH], stock_key[MAX_KEY_LENGTH];
      snprintf(item_key, sizeof(item_key), "i%d", j);
      snprintf(stock_key, sizeof(stock_key), "%ss%s",
               warehouse_key, item_key);

      Stock* stock =
        reinterpret_cast<Stock*>(storage_->Get(stock_key, 0)->data());
      EXPECT_EQ(strcmp(stock->id, stock_key), 0);
    }
  }

  // Expect all items to be there
  for (int i = 0; i < NUMBER_OF_ITEMS; i++) {
      char item_key[MAX_KEY_LENGTH];
      snprintf(item_key, sizeof(item_key), "i%d", i);
      Item* item =
        reinterpret_cast<Item*>(tpcc_->GetItem(Key(item_key))->data());
      EXPECT_EQ(strcmp(item->id, item_key), 0);
  }
}

/// @test   This tests whether or not the New-Order transaction works
TEST_F(TPCCTest, NewOrderTest) {
  // Txn args w/number of warehouses
  TPCCArgs* txn_args = new TPCCArgs();
  txn_args->set_multipartition(false);
  txn_args->set_system_time(GetTime());
  string txn_args_value;
  assert(txn_args->SerializeToString(&txn_args_value));

  // Populate the database because it's destroyed every test
  tpcc_->InitializeStorage();

  // Do work here to confirm new orders are satisfying TPC-C standards
  TxnProto* txn;
  bool invalid;
  do {
    txn = tpcc_->NewTxn(2, TPCC::NEW_ORDER, txn_args_value);
    assert(txn_args->ParseFromString(txn->arg()));
    invalid = false;
    for (int i = 0; i < txn_args->order_line_count(); i++) {
      if (txn->read_write_set(i + 1).find("i-1") != string::npos)
        invalid = true;
    }
  } while (invalid);
  txn->add_readers(0);
  txn->add_writers(0);
  TransactionalStorageManager* storage
    = BlackBoxFactory::TransactionalStorageManagerBlackBox(txn, false);

  // Prefetch some values in order to ensure our ACIDity after
  District *district = storage->GetMutable<District>(txn->read_write_set(0));

  // Prefetch the stocks
  Stock* old_stocks[txn_args->order_line_count()];
  for (int i = 0; i < txn_args->order_line_count(); i++) {
    Stock* stock = new Stock(*storage->Get<Stock>(txn->read_write_set(i + 1)));
    old_stocks[i] = stock;
  }

  // Prefetch the actual values
  int old_next_order_id = district->next_order_id;

  // Execute the transaction
  tpcc_->Execute(txn, storage);

  // Let's prefetch the keys we need for the post-check
  Key district_key = txn->read_write_set(0);
  Key new_order_key = txn->write_set(txn_args->order_line_count());
  Key order_key = txn->write_set(txn_args->order_line_count() + 1);

  // Add in all the keys and re-initialize the storage manager/storage
  txn->add_read_set(new_order_key);
  txn->add_read_set(order_key);
  for (int i = 0; i < txn_args->order_line_count(); i++) {
    txn->add_read_set(txn->write_set(i));
  }
  delete storage;
  txn->set_txn_id(3);
  storage = BlackBoxFactory::TransactionalStorageManagerBlackBox(txn, false);

  // Ensure that D_NEXT_O_ID is incremented for district
  district = storage->GetMutable<District>(district_key);
  EXPECT_EQ(old_next_order_id + 1, district->next_order_id);

  // TPCC::NEW_ORDER row was inserted with appropriate fields
  NewOrder* new_order = storage->GetMutable<NewOrder>(new_order_key);
  EXPECT_EQ(strcmp(new_order->id, new_order_key.c_str()), 0);

  // ORDER row was inserted with appropriate fields (used w/deterministic seed)
  Order* order = storage->GetMutable<Order>(order_key);
  EXPECT_EQ(strcmp(order->id, order_key.c_str()), 0);

  // For each item in O_OL_CNT
  for (int i = 0; i < txn_args->order_line_count(); i++) {
    Stock* stock = storage->GetMutable<Stock>(txn->read_write_set(i + 1));
    EXPECT_TRUE(stock != NULL);

    // Check YTD, order_count, and remote_count
    int corrected_year_to_date = old_stocks[i]->year_to_date;
    for (int j = 0; j < txn_args->order_line_count(); j++) {
      if (txn->read_write_set(j + 1) == txn->read_write_set(i + 1))
        corrected_year_to_date += txn_args->quantities(j);
    }
    EXPECT_EQ(stock->year_to_date, corrected_year_to_date);

    // Check order_count
    int corrected_order_count = old_stocks[i]->order_count;
    for (int j = 0; j < txn_args->order_line_count(); j++) {
      if (txn->read_write_set(j + 1) == txn->read_write_set(i + 1))
        corrected_order_count--;
    }
    EXPECT_EQ(stock->order_count, corrected_order_count);

    // Check remote_count
    if (txn->multipartition()) {
      int corrected_remote_count = old_stocks[i]->remote_count;
      for (int j = 0; j < txn_args->order_line_count(); j++) {
        if (txn->read_write_set(j + 1) == txn->read_write_set(i + 1))
          corrected_remote_count++;
      }
      EXPECT_EQ(stock->remote_count, corrected_remote_count);
    }

    // Check stock supply decrease
    int corrected_quantity = old_stocks[i]->quantity;
    for (int j = 0; j < txn_args->order_line_count(); j++) {
      if (txn->read_write_set(j + 1) == txn->read_write_set(i + 1)) {
        if (old_stocks[i]->quantity >= txn_args->quantities(i) + 10)
          corrected_quantity -= txn_args->quantities(j);
        else
          corrected_quantity -= txn_args->quantities(j) - 91;
      }
    }
    EXPECT_EQ(stock->quantity, corrected_quantity);

    // First, we check if the item is valid
    size_t item_idx = txn->read_write_set(i + 1).find("i");
    Key item_key = txn->read_write_set(i + 1).substr(item_idx, string::npos);
    Item* item =
      reinterpret_cast<Item*>(tpcc_->GetItem(Key(item_key))->data());
    EXPECT_EQ(strcmp(item->id, item_key.c_str()), 0);

    // Check the order line itself
    OrderLine order_line = order->order_lines[i];
    EXPECT_EQ(order_line.amount, item->price * txn_args->quantities(i));
    EXPECT_EQ(order_line.number, i);

    // Free memory
    delete item;
  }

  // Free memory
  delete storage;
  delete txn_args;
  delete txn;
}

/// @test   This tests whether or not the payment transaction is operational
TEST_F(TPCCTest, PaymentTest) {
  // Txn args w/number of warehouses
  TPCCArgs* txn_args = new TPCCArgs();
  txn_args->set_multipartition(false);
  txn_args->set_system_time(GetTime());
  string txn_args_value;
  assert(txn_args->SerializeToString(&txn_args_value));

  // Populate the database because it's destroyed every test
  tpcc_->InitializeStorage();

  // Do work here to confirm payment transactions are satisfying standards
  TxnProto* txn = new TxnProto();
  do {
    delete txn;
    txn = tpcc_->NewTxn(4, TPCC::PAYMENT, txn_args_value);
    ASSERT_TRUE(txn_args->ParseFromString(txn->arg()));
  } while (txn->read_write_set_size() < 3);
  txn->add_read_set(txn->write_set(0));
  txn->add_readers(0);
  txn->add_writers(0);
  TransactionalStorageManager* storage
    = BlackBoxFactory::TransactionalStorageManagerBlackBox(txn, false);

  // Prefetch some values in order to ensure our ACIDity after
  Warehouse *warehouse = storage->GetMutable<Warehouse>(txn->read_write_set(0));
  int old_warehouse_year_to_date = warehouse->year_to_date;

  // Prefetch district
  District *district = storage->GetMutable<District>(txn->read_write_set(1));
  int old_district_year_to_date = district->year_to_date;

  // Preetch customer
  Customer *customer = storage->GetMutable<Customer>(txn->read_write_set(2));
  int old_customer_year_to_date_payment = customer->year_to_date_payment;
  int old_customer_balance = customer->balance;
  int old_customer_payment_count = customer->payment_count;

  // Execute the transaction
  tpcc_->Execute(txn, storage);

  // Get the data back from the database
  delete storage;
  txn->set_txn_id(5);
  storage = BlackBoxFactory::TransactionalStorageManagerBlackBox(txn, false);

  // Check the old values against the new
  EXPECT_EQ(warehouse->year_to_date, old_warehouse_year_to_date +
            txn_args->amount());
  EXPECT_EQ(district->year_to_date, old_district_year_to_date +
            txn_args->amount());
  EXPECT_EQ(customer->year_to_date_payment,
            old_customer_year_to_date_payment + txn_args->amount());
  EXPECT_EQ(customer->balance, old_customer_balance - txn_args->amount());
  EXPECT_EQ(customer->payment_count, old_customer_payment_count + 1);

  // Ensure the history record is valid
  History* history = storage->GetMutable<History>(txn->read_set(0));
  EXPECT_EQ(strcmp(history->warehouse_id, warehouse->id), 'y');
  EXPECT_EQ(strcmp(history->district_id, district->id), 'y');
  EXPECT_EQ(strcmp(history->customer_id, customer->id), 0);
  EXPECT_EQ(strcmp(history->customer_warehouse_id, customer->warehouse_id), 0);
  EXPECT_EQ(strcmp(history->customer_district_id, customer->district_id), 0);

  // Free memory
  delete storage;
  delete txn_args;
  delete txn;
}

}  // namespace calvin

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
