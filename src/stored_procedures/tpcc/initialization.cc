/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// A concrete implementation of TPC-C

#include "stored_procedures/tpcc/tpcc.h"

namespace calvin {

void TPCC::InitializeApplication() {
  if (application_instance_ == NULL)
    application_instance_ = new TPCC();
}

// ---- THIS IS A HACK TO MAKE ITEMS WORK ON LOCAL MACHINE ---- //
AtomicMap<Key, Value*> ItemList;
Value* TPCC::GetItem(Key key) const {
  return ItemList.Get(key);
}
void TPCC::SetItem(Key key, Value* value) const {
  ItemList.InsertOrSet(key, value);
}
// -------------------------- END HACK ------------------------ //

// The initialize function is executed when an initialize transaction comes
// through, indicating we should populate the database with fake data
void TPCC::InitializeStorage() const {
  Storage* storage = Storage::GetStorageInstance();
  Config* conf = Config::GetConfigInstance();

  // We create and write out all of the warehouses
  for (int i = 0; i < (WAREHOUSES_PER_NODE * conf->all_nodes.size()); i++) {
    // First, we create a key for the warehouse
    char w_key[MAX_KEY_LENGTH], w_key_ytd[MAX_KEY_LENGTH];
    snprintf(w_key, sizeof(w_key), "w%d", i);
    snprintf(w_key_ytd, sizeof(w_key_ytd), "w%dy", i);

    // Next we initialize the object and write to disk
    Warehouse* warehouse = CreateWarehouse(Key(w_key));
    if (conf->LookupPartition(w_key) == conf->this_node_id) {
      storage->Put(Key(w_key), new Value(warehouse, sizeof(Warehouse)), -1);
      storage->Put(Key(w_key_ytd), new Value(warehouse, sizeof(Warehouse)), -1);
    }

    // Next, we create and write out all of the districts
    for (int j = 0; j < DISTRICTS_PER_WAREHOUSE; j++) {
      // First, we create a key for the district
      char d_key[MAX_KEY_LENGTH], d_key_ytd[MAX_KEY_LENGTH];
      snprintf(d_key, sizeof(d_key), "w%dd%d",
               i, j);
      snprintf(d_key_ytd, sizeof(d_key_ytd), "w%dd%dy",
               i, j);

      // Next we initialize the object and write to disk
      District* district = CreateDistrict(d_key, w_key);
      if (conf->LookupPartition(d_key) == conf->this_node_id) {
        storage->Put(Key(d_key), new Value(district, sizeof(District)), -1);
        storage->Put(Key(d_key_ytd), new Value(district, sizeof(District)), -1);
      }

      // Next, we create and write out all of the customers
      for (int k = 0; k < CUSTOMERS_PER_DISTRICT; k++) {
        // First, we create a key for the customer
        char c_key[MAX_KEY_LENGTH];
        snprintf(c_key, sizeof(c_key), "w%dd%dc%d", i, j, k);

        // Next we initialize the object and write to disk
        Customer* customer = CreateCustomer(c_key, d_key, w_key);
        if (conf->LookupPartition(c_key) == conf->this_node_id)
          storage->Put(Key(c_key), new Value(customer, sizeof(Customer)), -1);
      }
    }

    // Next, we create and write out all of the stock
    srand(1000);
    for (int j = 0; j < NUMBER_OF_ITEMS; j++) {
      // First, we create a key for the stock
      char item_key[MAX_KEY_LENGTH];
      snprintf(item_key, sizeof(item_key), "i%d", j);

      // Next we initialize a stock, and an item (only once)
      Stock* stock = CreateStock(item_key, w_key);
      if (i == 0) {
        Item* item = CreateItem(item_key);
        SetItem(Key(item_key), new Value(item, sizeof(Item)));
      }

      // Finally, we pass it off to the storage manager to write to disk
      if (conf->LookupPartition(Key(stock->id)) == conf->this_node_id)
        storage->Put(Key(stock->id), new Value(stock, sizeof(Stock)), -1);
    }
  }
}

// The following method is a dumb constructor for the warehouse protobuffer
Warehouse* TPCC::CreateWarehouse(Key w_key) const {
  Warehouse* warehouse = new Warehouse();

  // We initialize the id and the name fields
  memcpy(warehouse->id, w_key.c_str(), w_key.length() + 1);
  memcpy(warehouse->name, RandomString(10).c_str(), 11);

  // Provide some information to make TPC-C happy
  memcpy(warehouse->street_1, RandomString(20).c_str(), 21);
  memcpy(warehouse->street_2, RandomString(20).c_str(), 21);
  memcpy(warehouse->city, RandomString(20).c_str(), 21);
  memcpy(warehouse->state, RandomString(2).c_str(), 3);
  memcpy(warehouse->zip, RandomString(9).c_str(), 10);

  // Set default financial information
  warehouse->tax = 0.05;
  warehouse->year_to_date = 0.0;

  return warehouse;
}

District* TPCC::CreateDistrict(Key d_key, Key w_key) const {
  District* district = new District();

  // We initialize the id and the name fields
  memcpy(district->id, d_key.c_str(), d_key.length() + 1);
  memcpy(district->warehouse_id, w_key.c_str(), w_key.length() + 1);
  memcpy(district->name, RandomString(10).c_str(), 10);

  // Provide some information to make TPC-C happy
  memcpy(district->street_1, RandomString(20).c_str(), 21);
  memcpy(district->street_2, RandomString(20).c_str(), 21);
  memcpy(district->city, RandomString(20).c_str(), 21);
  memcpy(district->state, RandomString(2).c_str(), 3);
  memcpy(district->zip, RandomString(9).c_str(), 10);

  // Set default financial information
  district->tax = 0.05;
  district->year_to_date = 0.0;
  district->next_order_id = 1;

  return district;
}

Customer* TPCC::CreateCustomer(Key c_key, Key d_key, Key w_key) const {
  Customer* customer = new Customer();

  // We initialize the various keys
  memcpy(customer->id, c_key.c_str(), c_key.length() + 1);
  memcpy(customer->district_id, d_key.c_str(), d_key.length() + 1);
  memcpy(customer->warehouse_id, w_key.c_str(), w_key.length() + 1);

  // Next, we create a first and middle name
  memcpy(customer->first, RandomString(20).c_str(), 21);
  memcpy(customer->middle, RandomString(20).c_str(), 21);
  memcpy(customer->last, c_key.c_str(), c_key.length() + 1);

  // Provide some information to make TPC-C happy
  memcpy(customer->street_1, RandomString(20).c_str(), 21);
  memcpy(customer->street_2, RandomString(20).c_str(), 21);
  memcpy(customer->city, RandomString(20).c_str(), 21);
  memcpy(customer->state, RandomString(2).c_str(), 3);
  memcpy(customer->zip, RandomString(9).c_str(), 10);

  // Set default financial information
  customer->since = 0;
  memcpy(customer->credit, "GC", 3);
  customer->credit_limit = 0.01;
  customer->discount = 0.5;
  customer->balance = 0;
  customer->year_to_date_payment = 0;
  customer->payment_count = 0;
  customer->delivery_count = 0;

  // Set some miscellaneous data
  memcpy(customer->data, RandomString(50).c_str(), 51);

  return customer;
}

Stock* TPCC::CreateStock(Key item_key, Key w_key) const {
  Stock* stock = new Stock();

  // We initialize the various keys
  char stock_key[MAX_KEY_LENGTH];
  snprintf(stock_key, sizeof(stock_key), "%ss%s",
           w_key.c_str(), item_key.c_str());

  memcpy(stock->id, stock_key, MAX_KEY_LENGTH);
  memcpy(stock->warehouse_id, w_key.c_str(), w_key.length() + 1);
  memcpy(stock->item_id, item_key.c_str(), item_key.length() + 1);

  // Next, we create a first and middle name
  stock->quantity = rand() % 100 + 100;

  // Set default financial information
  stock->year_to_date = 0;
  stock->order_count = 0;
  stock->remote_count = 0;

  // Set some miscellaneous data
  memcpy(stock->data, RandomString(50).c_str(), 51);

  return stock;
}

Item* TPCC::CreateItem(Key item_key) const {
  Item* item = new Item();

  // We initialize the item's key and fake data for the name, price and data
  memcpy(item->id, item_key.c_str(), item_key.length() + 1);
  memcpy(item->name, RandomString(24).c_str(), 25);
  item->price = rand() % 100;
  memcpy(item->data, RandomString(50).c_str(), 51);

  return item;
}

}  // namespace calvin
