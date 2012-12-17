/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Handling of transactions in TPC-C including loadgen and executions

#include "stored_procedures/tpcc/tpcc.h"

namespace calvin {

const char* TPCC::Name() const {
  return "TPCC";
}

int TPCC::DBSize() const {
  return TOTAL_RECORDS;
}

int TPCC::KeyToInt(const Key& key) const {
  // Initial dissection of the key
  size_t id_idx;

  // Switch based on key type
  size_t bad = string::npos;

  // Stock
  if ((id_idx = key.find("s")) != bad) {
    size_t ware = key.find("w");
    return WAREHOUSES_PER_NODE + DISTRICTS_PER_NODE + CUSTOMERS_PER_NODE
           + NUMBER_OF_ITEMS +
           + NUMBER_OF_ITEMS * atoi(&key[ware + 1]) + atoi(&key[id_idx + 2]);

  // Customer
  } else if ((id_idx = key.find("c")) != bad) {
    return WAREHOUSES_PER_NODE + DISTRICTS_PER_NODE
           + atoi(&key[id_idx + 1]);

  // Item
  } else if ((id_idx = key.find("i")) != bad) {
    return WAREHOUSES_PER_NODE + DISTRICTS_PER_NODE + CUSTOMERS_PER_NODE
           + atoi(&key[id_idx + 1]);

  // Order-Line
  } else if ((id_idx = key.find("ol")) != bad) {
    return WAREHOUSES_PER_NODE + DISTRICTS_PER_NODE + CUSTOMERS_PER_NODE
           + NUMBER_OF_ITEMS + (WAREHOUSES_PER_NODE * NUMBER_OF_ITEMS)
           + (3 * NUMBER_OF_TXNS * WAREHOUSES_PER_NODE)
           + atoi(&key[id_idx + 2]);

  // New Order
  } else if ((id_idx = key.find("no")) != bad) {
    return WAREHOUSES_PER_NODE + DISTRICTS_PER_NODE + CUSTOMERS_PER_NODE
           + NUMBER_OF_ITEMS + (WAREHOUSES_PER_NODE * NUMBER_OF_ITEMS)
           + atoi(&key[id_idx + 2]);

  // Order
  } else if ((id_idx = key.find("o")) != bad) {
    return WAREHOUSES_PER_NODE + DISTRICTS_PER_NODE + CUSTOMERS_PER_NODE
           + NUMBER_OF_ITEMS + (WAREHOUSES_PER_NODE * NUMBER_OF_ITEMS)
           + (NUMBER_OF_TXNS * WAREHOUSES_PER_NODE)
           + atoi(&key[id_idx + 1]);

  // History
  } else if ((id_idx = key.find("h")) != bad) {
    return WAREHOUSES_PER_NODE + DISTRICTS_PER_NODE + CUSTOMERS_PER_NODE
           + NUMBER_OF_ITEMS + (WAREHOUSES_PER_NODE * NUMBER_OF_ITEMS)
           + (2 * NUMBER_OF_TXNS * WAREHOUSES_PER_NODE)
           + atoi(&key[id_idx + 1]);

  // District
  } else if ((id_idx = key.find("d")) != bad)   {
    return WAREHOUSES_PER_NODE
           + atoi(&key[id_idx + 1]);

  // Warehouse
  } else if ((id_idx = key.find("w")) != bad)  {
    return atoi(&key[id_idx + 1]);
  }

  // Invalid key
  return -1;
}

// The load generator can be called externally to return a
// transaction proto containing a new type of transaction.
TxnProto* TPCC::NewTxn(int64 txn_id, int txn_type, string args) const {
  // Create the new transaction object
  Config* config = Config::GetConfigInstance();
  TxnProto* txn = new TxnProto();

  // Set the transaction's standard attributes
  txn->set_txn_id(txn_id);
  txn->set_txn_type(txn_type);
  txn->set_isolation_level(TxnProto::SERIALIZABLE);
  txn->set_status(TxnProto::NEW);
  txn->set_multipartition(false);

  // Parse out the arguments to the transaction
  // Hacked out for SIGMOD 2013 Checkpointing paper
  //TPCCArgs* txn_args = new TPCCArgs();
  //assert(txn_args->ParseFromString(args));
  bool mp = false; //txn_args->multipartition();
  int remote_node;
  if (mp) {
    do {
      remote_node = rand() % config->all_nodes.size();
    } while (config->all_nodes.size() > 1 &&
             remote_node == config->this_node_id);
  }

  // Create an arg list
  TPCCArgs* tpcc_args = new TPCCArgs();
  tpcc_args->set_system_time(GetTime());

  // Because a switch is not scoped we declare our variables outside of it
  int warehouse_id, district_id, customer_id;
  char warehouse_key[MAX_KEY_LENGTH], district_key[MAX_KEY_LENGTH],
       customer_key[MAX_KEY_LENGTH];
  int order_line_count;
  bool invalid;
  string customer_value;
  std::set<int> items_used;

  // First, we pick a local warehouse for all transactions
  do {
    warehouse_id = rand() % (WAREHOUSES_PER_NODE * config->all_nodes.size());
    snprintf(warehouse_key, sizeof(warehouse_key), "w%d", warehouse_id);
  } while (config->LookupPartition(warehouse_key) != config->this_node_id);

  // Next, we pick a random district in the warehouse to save space later
  district_id = rand() % DISTRICTS_PER_WAREHOUSE;
  snprintf(district_key, sizeof(district_key), "w%dd%d",
           warehouse_id, district_id);

  switch (txn_type) {
    // <--- New-order transaction --->
    case NEW_ORDER:
      // Warehouse is only read, but district is written
      txn->add_read_set(Key(warehouse_key));
      txn->add_read_write_set(Key(district_key));

      // Finally, we pick a random customer
      customer_id = rand() % CUSTOMERS_PER_DISTRICT;
      snprintf(customer_key, sizeof(customer_key),
               "w%dd%dc%d",
               warehouse_id, district_id, customer_id);
      txn->add_read_set(Key(customer_key));

      // We set the length of the read and write set uniformly between 5 and 15
      order_line_count = (rand() % 11) + 5;

      // Let's choose a bad transaction 1% of the time
      invalid = false;
      if (rand() / (static_cast<double>(RAND_MAX + 1.0)) <= 0.01)
        invalid = true;

      // Iterate through each order line
      for (int i = 0; i < order_line_count; i++) {
        // Set the item id (Invalid orders have the last item be -1)
        int item;
        do {
          item = rand() % NUMBER_OF_ITEMS;
        } while (items_used.count(item) > 0);
        items_used.insert(item);

        if (invalid && i == order_line_count - 1)
          item = -1;

        // Print the item key into a buffer
        char item_key[MAX_KEY_LENGTH];
        snprintf(item_key, sizeof(item_key), "i%d", item);

        // Create an order line warehouse key (default is local)
        char remote_warehouse_key[MAX_KEY_LENGTH];
        snprintf(remote_warehouse_key, sizeof(remote_warehouse_key),
                 "%s", warehouse_key);

        // We only do ~1% remote transactions
        if (mp) {
          txn->set_multipartition(true);

          // We loop until we actually get a remote one
          int remote_warehouse_id;
          do {
            remote_warehouse_id = rand() % (WAREHOUSES_PER_NODE *
                                            config->all_nodes.size());
            snprintf(remote_warehouse_key, sizeof(remote_warehouse_key),
                     "w%d", remote_warehouse_id);
          } while (config->all_nodes.size() > 1 &&
                   config->LookupPartition(remote_warehouse_key) !=
                     remote_node);
        }

        // Determine if we should add it to read set to avoid duplicates
        bool needed = true;
        for (int j = 0; j < txn->read_set_size(); j++) {
          if (txn->read_set(j) == remote_warehouse_key)
            needed = false;
        }
        if (needed)
          txn->add_read_set(Key(remote_warehouse_key));

        // Finally, we set the stock key to the read and write set
        Key stock_key = string(remote_warehouse_key) + "s" + item_key;
        txn->add_read_write_set(stock_key);

        // Set the quantity randomly within [1..10]
        tpcc_args->add_quantities(rand() % 10 + 1);

        // Finally, we add the order line key to the write set
        char order_line_key[MAX_KEY_LENGTH];
        snprintf(order_line_key, sizeof(order_line_key), "%so%ldol%d",
                 warehouse_key, txn->txn_id(), i);
        txn->add_write_set(Key(order_line_key));
      }

      // Create a new order key to add to write set
      char new_order_key[MAX_KEY_LENGTH];
      snprintf(new_order_key, sizeof(new_order_key),
               "%sno%ld", district_key, txn->txn_id());
      txn->add_write_set(Key(new_order_key));

      // Create an order key to add to write set
      char order_key[MAX_KEY_LENGTH];
      snprintf(order_key, sizeof(order_key), "%so%ld",
               customer_key, txn->txn_id());
      txn->add_write_set(Key(order_key));

      // Set the order line count in the args
      tpcc_args->set_order_line_count(order_line_count);
      break;

    // <--- Payment transaction --->
    case PAYMENT:
      /// @TODO     (Thad):  I really want to get rid of "y"s because this is
      ///                    a super big fudge...
      // Specify an amount for the payment; add district, warehouse to RW
      tpcc_args->set_amount(rand() / (static_cast<double>(RAND_MAX + 1.0)) *
                            4999.0 + 1);
      txn->add_read_write_set(Key(warehouse_key) + "y");
      txn->add_read_write_set(Key(district_key) + "y");

      // Add history key to write set
      char history_key[MAX_KEY_LENGTH];
      snprintf(history_key, sizeof(history_key), "w%dh%ld",
               warehouse_id, txn->txn_id());
      txn->add_write_set(Key(history_key));

      // Next, we find the customer as a local one
      if (WAREHOUSES_PER_NODE * config->all_nodes.size() == 1 || !mp) {
        customer_id = rand() % CUSTOMERS_PER_DISTRICT;
        snprintf(customer_key, sizeof(customer_key),
                 "w%dd%dc%d",
                 warehouse_id, district_id, customer_id);

      // If the probability is 15%, we make it a remote customer
      } else {
        int remote_warehouse_id;
        int remote_district_id;
        int remote_customer_id;
        char remote_warehouse_key[MAX_KEY_LENGTH];
        do {
          remote_warehouse_id = rand() % (WAREHOUSES_PER_NODE *
                                          config->all_nodes.size());
          snprintf(remote_warehouse_key, sizeof(remote_warehouse_key), "w%d",
                   remote_warehouse_id);

          remote_district_id = rand() % DISTRICTS_PER_WAREHOUSE;

          remote_customer_id = rand() % CUSTOMERS_PER_DISTRICT;
          snprintf(customer_key, sizeof(customer_key), "w%dd%dc%d",
                   remote_warehouse_id, remote_district_id, remote_customer_id);
        } while (config->all_nodes.size() > 1 &&
                 config->LookupPartition(remote_warehouse_key) != remote_node);
      }

      // We only do secondary keying ~60% of the time
      if (rand() / (static_cast<double>(RAND_MAX + 1.0)) < 0.00) {
        // Now that we have the object, let's create the txn arg
        tpcc_args->set_last_name(customer_key);
        txn->add_read_set(Key(customer_key));

      // Otherwise just give a customer key
      } else {
        txn->add_read_write_set(Key(customer_key));
      }

      break;

    // <--- Order-status transaction --->
    case ORDER_STATUS:
      // Secondary key on last name 60%, o/w just standard customer key
      customer_id = rand() % CUSTOMERS_PER_DISTRICT;
      snprintf(customer_key, sizeof(customer_key), "w%dd%dc%d",
               warehouse_id, district_id, customer_id);
      if (rand() / (static_cast<double>(RAND_MAX + 1.0)) < 0.00)
        tpcc_args->set_last_name(Key(customer_key));
      else
        txn->add_read_write_set(Key(customer_key));

      /// @TODO    (Thad): Here we need to get only the most recent order with
      ///                  order-key "wW_IDdD_IDcC_IDo#"

      break;

    // <--- Delivery transaction --->
    case DELIVERY:
      /// @TODO    (Thad): We need to get all 10 districts and the new_orders,
      ///                  orders, nd matching customers for each

      // Set O_CARRIER_ID and OL_DELIVERY_D
      srand(time(NULL));
      tpcc_args->set_carrier_id(rand() % 10);
      tpcc_args->set_delivery_date(time(NULL));
      break;

    // <--- Stock-level transaction --->
    case STOCK_LEVEL:
      // Set the threshold [10..20] and add the district key to the read set
      tpcc_args->set_threshold(rand() % 11 + 10);
      txn->add_read_set(district_key);

      /// @TODO     (Thad): Need to get last 20 orders in that district and run
      ///                   a query on Stock for those w/quantity < threshold

      break;

    // Invalid transaction
    default:
      return NULL;
      break;
  }

  // Set the transaction's args field to a serialized version
  string args_string;
  assert(tpcc_args->SerializeToString(&args_string));
  txn->set_arg(args_string);

  // Free memory
  delete tpcc_args;
  //delete txn_args;

  return txn;
}

// The execute function takes a single transaction proto and executes it based
// on what the type of the transaction is.
TxnStatus TPCC::Execute(TxnProto* txn,
                        TransactionalStorageManager* storage) const {
  switch (txn->txn_type()) {
    // New Order
    case NEW_ORDER:
      return NewOrderTxn(txn, storage);
      break;

    // Payment
    case PAYMENT:
      return PaymentTxn(txn, storage);
      break;

    // Invalid transaction
    default:
      return FAILURE;
      break;
  }

  return FAILURE;
}

}  // namespace calvin
