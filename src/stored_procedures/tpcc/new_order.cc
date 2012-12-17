/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Implementation of the new order transaction in TPC-C

#include "stored_procedures/tpcc/tpcc.h"

namespace calvin {

// The new order function is executed when the application receives a new order
// transaction.  This follows the TPC-C standard.
TxnStatus TPCC::NewOrderTxn(TxnProto* txn,
                            TransactionalStorageManager* storage) const {
  // First, we retrieve the warehouse from storage
  const Warehouse* warehouse = storage->Get<Warehouse>(txn->read_set(0));

  // Next, we retrieve the district and increment next order ID
  District* district = storage->GetMutable<District>(txn->read_write_set(0));
  district->next_order_id++;

  // Retrieve the customer we are looking for
  const Customer* customer = storage->Get<Customer>(txn->read_set(1));

  // Next, we get the order line count, system time, and other args from the
  // transaction proto
  TPCCArgs* tpcc_args = new TPCCArgs();
  tpcc_args->ParseFromString(txn->arg());
  int order_line_count = tpcc_args->order_line_count();
  double system_time = tpcc_args->system_time();

  // Next we create an Order object
  Key order_key = txn->write_set(order_line_count + 1);
  Order* order = new Order();
  memcpy(order->id, order_key.c_str(), order_key.length() + 1);
  memcpy(order->warehouse_id, warehouse->id, 21);
  memcpy(order->district_id, district->id, 21);
  memcpy(order->customer_id, customer->id, 21);

  // Set some of the auxiliary data
  order->entry_date = system_time;
  order->carrier_id = -1;
  order->order_line_count = order_line_count;
  order->all_items_local = txn->multipartition();

  // We initialize the order line amount total to 0
  int order_line_amount_total = 0;

  for (int i = 0; i < order_line_count; i++) {
    // For each order line we parse out the three args
    string stock_key = txn->read_write_set(i + 1);
    string supply_warehouse_key = stock_key.substr(0, stock_key.find("s"));
    int quantity = tpcc_args->quantities(i);

    // Find the item key within the stock key
    string item_key = stock_key.substr(stock_key.find("i"), string::npos);

    // First, we check if the item number is valid
    if (item_key == "i-1") {
      delete order;
      delete tpcc_args;
      return FAILURE;
    }
    Item* item = reinterpret_cast<Item*>(GetItem(item_key)->data());

    // Next, we create a new order line object with std attributes
    OrderLine order_line;
    Key order_line_key = txn->write_set(i);
    memcpy(order_line.order_id, order_line_key.c_str(),
           order_line_key.length() + 1);

    // Set the attributes for this order line
    memcpy(order_line.district_id, district->id, 21);
    memcpy(order_line.warehouse_id, warehouse->id, 21);
    memcpy(order_line.item_id, item_key.c_str(), item_key.length() + 1);
    memcpy(order_line.supply_warehouse_id, supply_warehouse_key.c_str(),
           supply_warehouse_key.length() + 1);
    order_line.number = i;
    order_line.quantity = quantity;
    order_line.delivery_date = system_time;

    // Next, we get the correct stock from the data store and update
    Stock* stock  = storage->GetMutable<Stock>(stock_key);
    stock->year_to_date += quantity;
    stock->order_count--;
    if (txn->multipartition())
      stock->remote_count++;

    // And we decrease the stock's supply appropriately and rewrite to storage
    if (stock->quantity >= quantity + 10)
      stock->quantity -= quantity;
    else
      stock->quantity -= quantity - 91;

    // Next, we update the order line's amount and add it to the running sum
    order_line.amount = quantity * item->price;
    order_line_amount_total += (quantity * item->price);

    // Shove the order into the OrderLine
    order->order_lines[i] = order_line;
  }

  // We create a new NewOrder object
  Key new_order_key = txn->write_set(order_line_count);
  NewOrder* new_order = new NewOrder();
  memcpy(new_order->id, new_order_key.c_str(), new_order_key.length() + 1);
  memcpy(new_order->warehouse_id, warehouse->id, 21);
  memcpy(new_order->district_id, district->id, 21);

  // Put order and new_order in database
  storage->Put<NewOrder>(new_order_key, new_order);
  storage->Put<Order>(order_key, order);

  // Successfully completed transaction
  delete tpcc_args;
  return SUCCESS;
}

}  // namespace calvin
