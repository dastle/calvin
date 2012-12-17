/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Implementation of the payment transaction in TPC-C

#include "stored_procedures/tpcc/tpcc.h"

namespace calvin {

// The payment function is executed when the application receives a
// payment transaction.  This follows the TPC-C standard.
TxnStatus TPCC::PaymentTxn(TxnProto* txn,
                           TransactionalStorageManager* storage) const {
    // First, we parse out the transaction args from the TPCC proto
    TPCCArgs* tpcc_args = new TPCCArgs();
    tpcc_args->ParseFromString(txn->arg());
    int amount = tpcc_args->amount();

    // We create a string to hold up the customer object we look up
    Key customer_key;

    // If there's a last name we do secondary keying
    if (tpcc_args->has_last_name()) {
      customer_key = tpcc_args->last_name();

      // If the RW set is not at least of size 3, then no customer key was
      // given to this transaction.  Otherwise, we perform a check to ensure
      // the secondary key we just looked up agrees with our previous lookup
      if (txn->read_write_set_size() < 3 ||
          customer_key != txn->read_write_set(2)) {
        // Append the newly read key to write set
        if (txn->read_write_set_size() < 3)
          txn->add_read_write_set(customer_key);

        // Or the old one was incorrect so we overwrite it
        else
          txn->set_read_write_set(2, customer_key);

        return REDO;
      }

    // Otherwise we use the final argument
    } else {
      customer_key = txn->read_write_set(2);
    }

    // Grab customer from DB
    Customer* customer = storage->GetMutable<Customer>(customer_key);

    // Deserialize the warehouse object and update YTD
    Key warehouse_key = txn->read_write_set(0);
    Warehouse* warehouse = storage->GetMutable<Warehouse>(warehouse_key);
    warehouse->year_to_date += amount;

    // Deserialize the district object and update YTD
    Key district_key = txn->read_write_set(1);
    District* district = storage->GetMutable<District>(district_key);
    district->year_to_date += amount;

    // Next, we update the customer's balance, payment and payment count
    customer->balance -= amount;
    customer->year_to_date_payment += amount;
    customer->payment_count++;

    // If the customer has bad credit, we update the data information attached
    // to her
    if (!strcmp(customer->credit, "BC")) {
      // Print the new_information into the buffer
      char new_information[500];
      snprintf(new_information, sizeof(new_information), "%s%s%s%s%s%d%s",
               customer->id, customer->warehouse_id, customer->district_id,
               district->id, warehouse->id, amount, customer->data);
    }

    // Finally, we create a history object and update the data
    History* history = new History();
    memcpy(history->customer_id, customer_key.c_str(),
           customer_key.length() + 1);
    memcpy(history->customer_warehouse_id, customer->warehouse_id, 20);
    memcpy(history->customer_district_id, customer->district_id, 20);
    memcpy(history->warehouse_id, warehouse_key.c_str(),
           warehouse_key.length() + 1);
    memcpy(history->district_id, district_key.c_str(),
           district_key.length() + 1);

    // Create the data for the history object
    char history_data[100];
    snprintf(history_data, sizeof(history_data), "%s    %s",
             warehouse->name, district->name);
    memcpy(history->data, history_data, sizeof(history_data));

    // Write the history object to disk
    storage->Put<History>(txn->write_set(0), history);

    // Successfully completed transaction
    delete tpcc_args;
    return SUCCESS;
}

}  // namespace calvin
